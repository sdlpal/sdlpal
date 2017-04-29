/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// native_midi.cpp: Native UWP MIDI player for SDLPal.
// Author: Lou Yihua @ 2017
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2017 SDLPAL development team.
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

extern "C" {
#include "native_midi/native_midi.h"
#include "native_midi/native_midi_common.h"
}

struct MidiEvent
{
	Windows::Devices::Midi::IMidiMessage^ message;
	uint32_t deltaTime;		// time in ticks
	uint32_t tempo;			// microseconds per quarter note

	std::chrono::system_clock::duration DeltaTimeAsTick(uint16_t ppq)
	{
		return std::chrono::system_clock::duration((int64_t)deltaTime * tempo * 10 / ppq);
	}
};

struct _NativeMidiSong {
	Windows::Devices::Midi::MidiSynthesizer^ Synthesizer;
	MidiEvent* Events;
	int        Size;
	int        Position;
	Uint16     ppq;		// parts (ticks) per quarter note
	volatile bool       Playing;
	bool       Loaded;
	std::thread Thread;
	std::mutex  Mutex;
	std::condition_variable Stop;

	_NativeMidiSong()
		: Events(nullptr), Size(0), Position(0)
		, ppq(0), Playing(false), Loaded(false)
		, Synthesizer(nullptr)
	{ }
};

static std::atomic<NativeMidiSong*> CurrentSong = nullptr;

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
	MIDIEvent *event = eventlist;
	while (event)
	{
		eventcount++;
		event = event->next;
	}

	if (!(song->Events = new MidiEvent[eventcount]))
		return;

	for (event = eventlist, eventcount = 0; event; event = event->next)
	{
		Windows::Devices::Midi::IMidiMessage^ message = nullptr;
		int status = (event->status & 0xF0) >> 4;
		switch (status)
		{
		case MIDI_STATUS_NOTE_OFF:
			message = ref new Windows::Devices::Midi::MidiNoteOffMessage(event->status - (status << 4), event->data[0], event->data[1]);
			break;

		case MIDI_STATUS_NOTE_ON:
			message = ref new Windows::Devices::Midi::MidiNoteOnMessage(event->status - (status << 4), event->data[0], event->data[1]);
			break;

		case MIDI_STATUS_AFTERTOUCH:
			message = ref new Windows::Devices::Midi::MidiPolyphonicKeyPressureMessage(event->status - (status << 4), event->data[0], event->data[1]);
			break;

		case MIDI_STATUS_CONTROLLER:
			message = ref new Windows::Devices::Midi::MidiControlChangeMessage(event->status - (status << 4), event->data[0], event->data[1]);
			break;

		case MIDI_STATUS_PROG_CHANGE:
			message = ref new Windows::Devices::Midi::MidiProgramChangeMessage(event->status - (status << 4), event->data[0]);
			break;

		case MIDI_STATUS_PRESSURE:
			message = ref new Windows::Devices::Midi::MidiChannelPressureMessage(event->status - (status << 4), event->data[0]);
			break;

		case MIDI_STATUS_PITCH_WHEEL:
			message = ref new Windows::Devices::Midi::MidiPitchBendChangeMessage(event->status - (status << 4), event->data[0] | (event->data[1] << 7));
			break;

		case MIDI_STATUS_SYSEX:
			switch ((MidiSystemMessage)(event->status & 0xF))
			{
			case MidiSystemMessage::Exclusive:
			{
				auto buffer = NativeBuffer::GetIBuffer(event->extraData, event->extraLen);
				if (buffer)
				{
					message = ref new Windows::Devices::Midi::MidiSystemExclusiveMessage(buffer);
					delete buffer;
				}
			}
				break;

			case MidiSystemMessage::TimeCode:
				message = ref new Windows::Devices::Midi::MidiTimeCodeMessage(event->extraData[0] >> 4, event->extraData[0] & 0xF);
				break;

			case MidiSystemMessage::SongPositionPointer:
				message = ref new Windows::Devices::Midi::MidiSongPositionPointerMessage(event->extraData[0] | (event->extraData[1] << 7));
				break;

			case MidiSystemMessage::SongSelect:
				message = ref new Windows::Devices::Midi::MidiSongSelectMessage(event->extraData[0]);
				break;

			case MidiSystemMessage::TuneRequest:
				message = ref new Windows::Devices::Midi::MidiTuneRequestMessage();
				break;

			case MidiSystemMessage::TimingClock:
				message = ref new Windows::Devices::Midi::MidiTimingClockMessage();
				break;

			case MidiSystemMessage::Start:
				message = ref new Windows::Devices::Midi::MidiStartMessage();
				break;

			case MidiSystemMessage::Continue:
				message = ref new Windows::Devices::Midi::MidiContinueMessage();
				break;

			case MidiSystemMessage::Stop:
				message = ref new Windows::Devices::Midi::MidiStopMessage();
				break;

			case MidiSystemMessage::ActiveSensing:
				message = ref new Windows::Devices::Midi::MidiActiveSensingMessage();
				break;

			case MidiSystemMessage::SystemReset:
				message = ref new Windows::Devices::Midi::MidiSystemResetMessage();
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

	song->Size = eventcount;
	song->Position = 0;
	song->Loaded = 1;
}

int native_midi_detect()
{
	auto synthesizer = AWait(Windows::Devices::Midi::MidiSynthesizer::CreateAsync());
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
	std::unique_ptr<SDL_RWops> rw(SDL_RWFromFile(midifile, "rb"));
	return rw ? native_midi_loadsong_RW(rw.get()) : nullptr;
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
			return newsong.release();
		}
	}

	return nullptr;
}

void native_midi_freesong(NativeMidiSong *song)
{
	if (song)
	{
		native_midi_stop();

		if (song->Events)
			delete[] song->Events;
		delete song;
	}
}

void native_midi_start(NativeMidiSong *song)
{
	if (!song) return;

	native_midi_stop();

	if (!song->Synthesizer)
	{
		song->Synthesizer = AWait(Windows::Devices::Midi::MidiSynthesizer::CreateAsync());
		if (!song->Synthesizer) return;
	}

	song->Thread = std::move(std::thread([](NativeMidiSong *song)->void {
		auto time = std::chrono::system_clock::now();
		while (song->Position < song->Size)
		{
			do
			{
				song->Synthesizer->SendMessage(song->Events[song->Position++].message);
			} while (song->Position < song->Size && song->Events[song->Position].deltaTime == 0);
			time += std::chrono::system_clock::duration(song->Events[song->Position].DeltaTimeAsTick(song->ppq));
			if (song->Stop.wait_until(std::unique_lock<std::mutex>(song->Mutex), time) == std::cv_status::no_timeout) break;
		}
		song->Playing = false;
	}, song));

	song->Playing = true;

	CurrentSong.exchange(song);
}

void native_midi_stop()
{
	NativeMidiSong* song;
	if (song = CurrentSong.exchange(nullptr))
	{
		song->Stop.notify_all();
		if (song->Thread.joinable())
			song->Thread.join();
		song->Thread = std::move(std::thread());
		song->Playing = false;
		if (song->Synthesizer)
		{
			delete song->Synthesizer;
			song->Synthesizer = nullptr;
		}
	}
}

int native_midi_active()
{
	auto song = CurrentSong.load();
	return (song && song->Playing) ? 1 : 0;
}

void native_midi_setvolume(int volume)
{
	auto song = CurrentSong.load();
	if (song && song->Synthesizer)
	{
		if (volume > 128)
			volume = 128;
		else if (volume < 0)
			volume = 0;
		song->Synthesizer->Volume = (double)volume / 128.0;
	}
}

const char *native_midi_error(void)
{
	return "";
}
