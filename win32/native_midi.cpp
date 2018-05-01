/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2018, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// native_midi.cpp: Native Windows Desktop MIDI player for SDLPal.
//         @Author: Lou Yihua, 2017
//

#include "SDL.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <stdlib.h>
#include <memory>
#include <future>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <vector>

#if (defined(__MINGW32__) || defined(__MINGW64__)) && !defined(_GLIBCXX_HAS_GTHREADS)
#include "mingw.condition_variable.h"
#include "mingw.mutex.h"
#include "mingw.thread.h"
#endif

#include "native_midi/native_midi.h"
#include "native_midi/native_midi_common.h"

static int native_midi_available = -1;

enum class MidiSystemMessage {
	Exclusive = 0,
	TimeCode = 1,
	SongPositionPointer = 2,
	SongSelect = 3,
	TuneRequest = 6,
	EndOfExclusive = 7,
	TimingClock = 8,
	Start = 10,
	Continue = 11,
	Stop = 12,
	ActiveSensing = 14,
	SystemReset = 15
};

struct MidiMessage
{
	virtual MMRESULT Send(HMIDIOUT hmo) = 0;
};

struct MidiShortMessage
	: public MidiMessage
{
	uint32_t data;

	MidiShortMessage(uint32_t data) : data(data) {}

	virtual MMRESULT Send(HMIDIOUT hmo) { return midiOutShortMsg(hmo, data); }
};

struct MidiLongMessage
	: public MidiMessage
{
	MIDIHDR hdr;

	MidiLongMessage(uint8_t* data, int length)
	{
		memset(&hdr, 0, sizeof(MIDIHDR));
		hdr.lpData = (LPSTR)malloc(length);
		hdr.dwBufferLength = hdr.dwBytesRecorded = length;
		memcpy(hdr.lpData, data, length);
	}

	virtual MMRESULT Send(HMIDIOUT hmo)
	{
		MMRESULT retval;
		if (MMSYSERR_NOERROR == (retval = midiOutPrepareHeader(hmo, &hdr, sizeof(MIDIHDR))))
		{
			retval = midiOutLongMsg(hmo, &hdr, sizeof(MIDIHDR));
			midiOutUnprepareHeader(hmo, &hdr, sizeof(MIDIHDR));
		}
		return retval;
	}
};

struct MidiResetMessage
	: public MidiMessage
{
	virtual MMRESULT Send(HMIDIOUT hmo) { return midiOutReset(hmo); }
};

struct MidiCustomMessage
	: public MidiMessage
{
	uint32_t message;
	uint32_t data1;
	uint32_t data2;

	MidiCustomMessage(uint8_t status, uint8_t data1, uint8_t data2)
		: message(status), data1(data1), data2(data2)
	{}

	virtual MMRESULT SendEvent(HMIDIOUT hmo) { return midiOutMessage(hmo, message, data1, data2); }
};

struct MidiEvent
{
	std::unique_ptr<MidiMessage> message;
	uint32_t                     deltaTime;		// time in ticks
	uint32_t                     tempo;			// microseconds per quarter note

	std::chrono::system_clock::duration DeltaTimeAsTick(uint16_t ppq)
	{
		return std::chrono::microseconds((int64_t)deltaTime * tempo / ppq);
	}

	MMRESULT Send(HMIDIOUT hmo) { return message->Send(hmo); }
};

struct _NativeMidiSong {
	std::vector<MidiEvent>  Events;
	std::thread             Thread;
	std::mutex              Mutex;
	std::condition_variable Stop;
	HMIDIOUT                Synthesizer;
	int                     Size;
	int                     Position;
	Uint16                  ppq;		// parts (ticks) per quarter note
	volatile bool           Playing;
	bool                    Loaded;
	bool                    Looping;

	_NativeMidiSong()
		: Synthesizer(nullptr), Size(0), Position(0)
		, ppq(0), Playing(false), Loaded(false), Looping(false)
	{ }
};

static void MIDItoStream(NativeMidiSong *song, MIDIEvent *eventlist)
{
	int eventcount = 0, prevtime = 0, tempo = 500000;

	for (MIDIEvent* event = eventlist; event; event = event->next)
	{
		if (event->status != 0xFF)
			eventcount++;
	}

	song->Events.resize(song->Size = eventcount);
	song->Position = 0;
	song->Loaded = true;

	eventcount = 0;
	for (MIDIEvent* event = eventlist; event; event = event->next)
	{
		MidiMessage* message = nullptr;
		int status = (event->status & 0xF0) >> 4;
		switch (status)
		{
		case MIDI_STATUS_NOTE_OFF:
		case MIDI_STATUS_NOTE_ON:
		case MIDI_STATUS_AFTERTOUCH:
		case MIDI_STATUS_CONTROLLER:
		case MIDI_STATUS_PROG_CHANGE:
		case MIDI_STATUS_PRESSURE:
		case MIDI_STATUS_PITCH_WHEEL:
			message = new MidiShortMessage(event->status | (event->data[0] << 8) | (event->data[1] << 16));
			break;

		case MIDI_STATUS_SYSEX:
			switch ((MidiSystemMessage)(event->status & 0xF))
			{
			case MidiSystemMessage::Exclusive:
				message = new MidiLongMessage(event->extraData, event->extraLen);
				break;

			case MidiSystemMessage::TimeCode:
			case MidiSystemMessage::SongSelect:
				message = new MidiShortMessage(event->status | (event->extraData[0] << 8));
				break;

			case MidiSystemMessage::SongPositionPointer:
				message = new MidiShortMessage(event->status | (event->extraData[0] << 8) | (event->extraData[1] << 16));
				break;

			case MidiSystemMessage::TuneRequest:
			case MidiSystemMessage::TimingClock:
			case MidiSystemMessage::Start:
			case MidiSystemMessage::Continue:
			case MidiSystemMessage::Stop:
			case MidiSystemMessage::ActiveSensing:
				message = new MidiShortMessage(event->status);
				break;

			case MidiSystemMessage::SystemReset:
				// This message is only used as meta-event in MIDI files
				if (event->data[0] == 0x51)
					tempo = (event->extraData[0] << 16) | (event->extraData[1] << 8) | event->extraData[2];
				break;
			default:
				break;
			}
			break;
		}

		if (message)
		{
			auto evt = &song->Events[eventcount++];
			evt->message.reset(message);
			evt->deltaTime = event->time - prevtime;
			evt->tempo = tempo;
			prevtime = event->time;
		}
	}
}

int native_midi_detect()
{
	if (-1 == native_midi_available)
	{
		HMIDIOUT out;

		if (MMSYSERR_NOERROR == midiOutOpen(&out, MIDI_MAPPER, 0, 0, CALLBACK_NULL))
		{
			midiOutClose(out);
			native_midi_available = 1;
		}
		else
			native_midi_available = 0;
	}
	return native_midi_available;
}

NativeMidiSong *native_midi_loadsong(const char *midifile)
{
	/* Attempt to load the midi file */
	auto rw = SDL_RWFromFile(midifile, "rb");
	if (rw)
	{
		auto song = native_midi_loadsong_RW(rw);
		SDL_RWclose(rw);
		return song;
	}
	return nullptr;
}

NativeMidiSong *native_midi_loadsong_RW(SDL_RWops *rw)
{
	std::unique_ptr<NativeMidiSong> newsong(new NativeMidiSong);

	if (newsong)
	{
		auto eventlist = CreateMIDIEventList(rw, &newsong->ppq);

		if (eventlist)
		{
			MIDItoStream(newsong.get(), eventlist);
			FreeMIDIEventList(eventlist);
			
			if (midiOutOpen(&newsong->Synthesizer, MIDI_MAPPER, NULL, 0, CALLBACK_NULL) == MMSYSERR_NOERROR)
				return newsong.release();
		}
	}

	return nullptr;
}

void native_midi_freesong(NativeMidiSong *song)
{
	if (song)
	{
		native_midi_stop(song);
		if (song->Synthesizer)
			midiOutClose(song->Synthesizer);
		delete song;
	}
}

void native_midi_start(NativeMidiSong *song, int looping)
{
	if (!song) return;

	native_midi_stop(song);

	song->Playing = true;
	song->Looping = looping ? true : false;
	song->Thread = std::move(std::thread([](NativeMidiSong *song)->void {
		auto time = std::chrono::system_clock::now();
		while (song->Playing)
		{
			do
			{
				song->Events[song->Position++].Send(song->Synthesizer);
			} while (song->Position < song->Size && song->Events[song->Position].deltaTime == 0);
			if (song->Position < song->Size)
			{
				auto mutex = std::unique_lock<std::mutex>(song->Mutex);
				time += std::chrono::system_clock::duration(song->Events[song->Position].DeltaTimeAsTick(song->ppq));
				while (song->Playing)
				{
					if (song->Stop.wait_until(mutex, time) == std::cv_status::timeout)
						break;
				}
			}
			else if (song->Playing = song->Looping)
			{
				song->Position = 0;
				midiOutReset(song->Synthesizer);
			}
		}
	}, song));
}

void native_midi_stop(NativeMidiSong *song)
{
	if (song)
	{
		song->Playing = false;
		song->Stop.notify_all();
		if (song->Thread.joinable())
			song->Thread.join();
		song->Thread = std::move(std::thread());
		if (song->Synthesizer)
			midiOutReset(song->Synthesizer);
	}
}

int native_midi_active(NativeMidiSong *song)
{
	return (song && song->Playing) ? 1 : 0;
}

void native_midi_setvolume(NativeMidiSong *song, int volume)
{
	if (song && song->Synthesizer)
	{
		uint16_t calcVolume;
		if (volume > 127)
			volume = 127;
		if (volume < 0)
			volume = 0;
		calcVolume = volume << 9;
		midiOutSetVolume(song->Synthesizer, MAKELONG(calcVolume, calcVolume));
	}
}

const char *native_midi_error(NativeMidiSong *song)
{
  return "";
}
