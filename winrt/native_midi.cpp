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
// native_midi.cpp: Native Windows Runtime MIDI player for SDLPal.
//         @Author: Lou Yihua, 2017
//

#include "pch.h"
#include "AsyncHelper.h"
#include "NativeBuffer.h"
#include <SDL.h>
#include <memory>
#include <future>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>

#include "native_midi/native_midi.h"
#include "native_midi/native_midi_common.h"

using namespace Windows::Devices::Midi;

struct MidiEvent
{
	IMidiMessage^ message;
	uint32_t      deltaTime;		// time in ticks
	uint32_t      tempo;			// microseconds per quarter note

	std::chrono::system_clock::duration DeltaTimeAsTick(uint16_t ppq)
	{
		return std::chrono::system_clock::duration((int64_t)deltaTime * tempo * 10 / ppq);
	}

	void Send(MidiSynthesizer^ synthesizer) { synthesizer->SendMessage(message); }
};

struct _NativeMidiSong {
	std::vector<MidiEvent>  Events;
	std::thread             Thread;
	std::mutex              Mutex;
	std::condition_variable Stop;
	MidiSynthesizer^        Synthesizer;
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

static MidiSystemResetMessage^ ResetMessage = ref new MidiSystemResetMessage();

enum class MidiSystemMessage {
	Exclusive = 0,
	TimeCode = 1,
	SongPositionPointer = 2,
	SongSelect = 3,
	TuneRequest = 6,
	TimingClock = 8,
	Start = 10,
	Continue = 11,
	Stop = 12,
	ActiveSensing = 14,
	SystemReset = 15
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
		IMidiMessage^ message = nullptr;
		int status = (event->status & 0xF0) >> 4;
		switch (status)
		{
		case MIDI_STATUS_NOTE_OFF:
			message = ref new MidiNoteOffMessage(event->status - (status << 4), event->data[0], event->data[1]);
			break;

		case MIDI_STATUS_NOTE_ON:
			message = ref new MidiNoteOnMessage(event->status - (status << 4), event->data[0], event->data[1]);
			break;

		case MIDI_STATUS_AFTERTOUCH:
			message = ref new MidiPolyphonicKeyPressureMessage(event->status - (status << 4), event->data[0], event->data[1]);
			break;

		case MIDI_STATUS_CONTROLLER:
			message = ref new MidiControlChangeMessage(event->status - (status << 4), event->data[0], event->data[1]);
			break;

		case MIDI_STATUS_PROG_CHANGE:
			message = ref new MidiProgramChangeMessage(event->status - (status << 4), event->data[0]);
			break;

		case MIDI_STATUS_PRESSURE:
			message = ref new MidiChannelPressureMessage(event->status - (status << 4), event->data[0]);
			break;

		case MIDI_STATUS_PITCH_WHEEL:
			message = ref new MidiPitchBendChangeMessage(event->status - (status << 4), event->data[0] | (event->data[1] << 7));
			break;

		case MIDI_STATUS_SYSEX:
			switch ((MidiSystemMessage)(event->status & 0xF))
			{
			case MidiSystemMessage::Exclusive:
			{
				auto buffer = NativeBuffer::GetIBuffer(event->extraData, event->extraLen);
				if (buffer)
				{
					message = ref new MidiSystemExclusiveMessage(buffer);
					delete buffer;
				}
			}
				break;

			case MidiSystemMessage::TimeCode:
				message = ref new MidiTimeCodeMessage(event->extraData[0] >> 4, event->extraData[0] & 0xF);
				break;

			case MidiSystemMessage::SongPositionPointer:
				message = ref new MidiSongPositionPointerMessage(event->extraData[0] | (event->extraData[1] << 7));
				break;

			case MidiSystemMessage::SongSelect:
				message = ref new MidiSongSelectMessage(event->extraData[0]);
				break;

			case MidiSystemMessage::TuneRequest:
				message = ref new MidiTuneRequestMessage();
				break;

			case MidiSystemMessage::TimingClock:
				message = ref new MidiTimingClockMessage();
				break;

			case MidiSystemMessage::Start:
				message = ref new MidiStartMessage();
				break;

			case MidiSystemMessage::Continue:
				message = ref new MidiContinueMessage();
				break;

			case MidiSystemMessage::Stop:
				message = ref new MidiStopMessage();
				break;

			case MidiSystemMessage::ActiveSensing:
				message = ref new MidiActiveSensingMessage();
				break;

			case MidiSystemMessage::SystemReset:
				// This message is only used as meta-event in MIDI files
				if (event->data[0] == 0x51)
					tempo = (event->extraData[0] << 16) | (event->extraData[1] << 8) | event->extraData[2];
				break;
			}
			break;
		}

		if (message)
		{
			song->Events[eventcount].message = message;
			song->Events[eventcount].deltaTime = event->time - prevtime;
			song->Events[eventcount].tempo = tempo;
			prevtime = event->time; eventcount++;
		}
	}
}

int native_midi_detect()
{
	auto synthesizer = AWait(MidiSynthesizer::CreateAsync());
	if (synthesizer)
	{
		delete synthesizer;
		return 1;
	}
	return 0;
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
		auto evntlist = CreateMIDIEventList(rw, &newsong->ppq);

		if (evntlist)
		{
			MIDItoStream(newsong.get(), evntlist);
			FreeMIDIEventList(evntlist);

			if (newsong->Synthesizer = AWait(MidiSynthesizer::CreateAsync()))
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
			delete song->Synthesizer;
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
				song->Synthesizer->SendMessage(ResetMessage);
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
		{
			song->Synthesizer->SendMessage(ResetMessage);
		}
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
		if (volume > 127)
			volume = 127;
		else if (volume < 0)
			volume = 0;
		song->Synthesizer->Volume = (double)volume / 127.0;
	}
}

const char *native_midi_error(NativeMidiSong *song)
{
	return "";
}
