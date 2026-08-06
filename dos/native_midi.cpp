/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// native_midi.cpp: Native DOS MPU-401 MIDI player using vhook.
//                  @Author: palxex, 2026
//

#include "sdl_compat.h"
#include "native_midi/native_midi.h"
#include "native_midi/native_midi_common.h"
#include "mpu401.h"
#include "vclock.h"
#include "util.h"
#include "palcfg.h"

#include <stdlib.h>
#include <memory>
#include <vector>
#include <atomic>
#include <chrono>

/* --------------------------------
   Internal data structures
   -------------------------------- */

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

// MIDI message wrapper (keeps the parsed data)
struct MidiMessage {
    virtual ~MidiMessage() = default;
    // Returns the total number of bytes in this message
    virtual uint32_t get_length() const = 0;
    // Writes the message bytes into the provided buffer (must be large enough)
    virtual void get_bytes(uint8_t *buffer) const = 0;
};

struct ShortMessage : public MidiMessage {
    uint8_t status;
    uint8_t data1;
    uint8_t data2;
    uint8_t length;
    ShortMessage(uint8_t s, uint8_t d1, uint8_t d2, uint8_t len)
        : status(s), data1(d1), data2(d2), length(len) {}
    uint32_t get_length() const override { return length; }
    void get_bytes(uint8_t *buffer) const override {
        buffer[0] = status;
        if (length > 1) {
            buffer[1] = data1;
        }
        if (length > 2) {
            buffer[2] = data2;
        }
    }
};

struct SysExMessage : public MidiMessage {
    std::vector<uint8_t> data;
    explicit SysExMessage(const uint8_t *src, uint32_t len) : data(src, src + len) {}
    uint32_t get_length() const override { return (uint32_t)data.size(); }
    void get_bytes(uint8_t *buffer) const override {
        memcpy(buffer, data.data(), data.size());
    }
};

// Playback event: contains the message and its absolute trigger time (µs)
struct PlayEvent {
    std::unique_ptr<MidiMessage> msg;
    uint64_t                     trigger_us;   // absolute microsecond timestamp from song start
};

// Song structure (native_midi.h opaque type)
struct _NativeMidiSong {
    std::vector<PlayEvent> events;
    uint16_t               ppq;               // ticks per quarter note
    int                    current_event;     // index of event being sent
    bool                   playing;           // is playback active?
    bool                   looping;
    uint64_t               start_time_us;     // base time in microseconds (SDL_GetTicks * 1000)
    bool                   loaded;
};

// Global playback state
static NativeMidiSong *g_current_song = nullptr;
static bool            g_vhook_registered = false;

typedef struct NativeMidiHookContext {
    volatile NativeMidiSong *song;
    volatile int in_hook;
} NativeMidiHookContext;

static NativeMidiHookContext g_midi_hook_ctx = { NULL, 0 };

/* --------------------------------
   Helper functions
   -------------------------------- */

// Convert MIDI event list to PlayEvent vector with absolute timestamps (microseconds)
static bool MidiEventListToPlayEvents(MIDIEvent *eventlist, uint16_t ppq,
                                      std::vector<PlayEvent> &out_events,
                                      uint64_t &total_duration_us) {
    int tempo = 500000;              // default: 120 BPM (microseconds per quarter note)
    uint32_t last_time_ticks = 0;
    uint64_t running_time_us = 0;
    out_events.clear();

    for (MIDIEvent *ev = eventlist; ev; ev = ev->next) {
        uint32_t delta_ticks = ev->time - last_time_ticks;
        running_time_us += (uint64_t)delta_ticks * tempo / ppq;
        last_time_ticks = ev->time;

        int status = (ev->status & 0xF0) >> 4;
        std::unique_ptr<MidiMessage> msg;

        switch (status) {
        case MIDI_STATUS_NOTE_OFF:
        case MIDI_STATUS_NOTE_ON:
        case MIDI_STATUS_AFTERTOUCH:
        case MIDI_STATUS_CONTROLLER:
        case MIDI_STATUS_PITCH_WHEEL:
            msg.reset(new ShortMessage(ev->status, ev->data[0], ev->data[1], 3));
            break;

        case MIDI_STATUS_PROG_CHANGE:
        case MIDI_STATUS_PRESSURE:
            msg.reset(new ShortMessage(ev->status, ev->data[0], 0, 2));
            break;

        case MIDI_STATUS_SYSEX: {
            switch ((MidiSystemMessage)(ev->status & 0x0F)) {
            case MidiSystemMessage::Exclusive:
                msg.reset(new SysExMessage(ev->extraData, ev->extraLen));
                break;
            case MidiSystemMessage::TimeCode:
            case MidiSystemMessage::SongSelect:
                msg.reset(new ShortMessage(ev->status, ev->extraData[0], 0, 2));
                break;
            case MidiSystemMessage::SongPositionPointer:
                msg.reset(new ShortMessage(ev->status, ev->extraData[0], ev->extraData[1], 3));
                break;
            case MidiSystemMessage::TuneRequest:
            case MidiSystemMessage::TimingClock:
            case MidiSystemMessage::Start:
            case MidiSystemMessage::Continue:
            case MidiSystemMessage::Stop:
            case MidiSystemMessage::ActiveSensing:
                msg.reset(new ShortMessage(ev->status, 0, 0, 1));
                break;
            case MidiSystemMessage::SystemReset:
                if (ev->data[0] == 0x51 && ev->extraLen >= 3) {
                    tempo = (ev->extraData[0] << 16) | (ev->extraData[1] << 8) | ev->extraData[2];
                }
                break;
            default:
                break;
            }
            break;
        }
        default:
            break;
        }

        if (msg) {
            PlayEvent pe;
            pe.msg = std::move(msg);
            pe.trigger_us = running_time_us;
            out_events.push_back(std::move(pe));
        }
    }

    total_duration_us = running_time_us;
    return true;
}

/* Helper: send GM Reset and set all channels to full volume */
static void init_synthesizer(void)
{
    // GM Reset (F0 7E 7F 09 01 F7)
    uint8_t gm_reset[] = {0xF0, 0x7E, 0x7F, 0x09, 0x01, 0xF7};
    for (int i = 0; i < 6; i++) {
        mpu401_write_byte(gm_reset[i]);
    }

    // Set all channels: Volume=127, Expression=127, Pan=64
    for (int ch = 0; ch < 16; ch++) {
        uint8_t status = 0xB0 | ch;
        mpu401_write_byte(status);
        mpu401_write_byte(7);   // volume
        mpu401_write_byte(127);

        mpu401_write_byte(status);
        mpu401_write_byte(11);  // expression
        mpu401_write_byte(127);

        mpu401_write_byte(status);
        mpu401_write_byte(10);  // pan
        mpu401_write_byte(64);  // center
    }

    // Small delay to let the synth process these messages
    SDL_Delay(10);
}

static void panic_synthesizer(void)
{
    for (int ch = 0; ch < 16; ch++) {
        uint8_t status = 0xB0 | ch;

        mpu401_write_byte(status);
        mpu401_write_byte(64);   // sustain pedal off
        mpu401_write_byte(0);

        mpu401_write_byte(status);
        mpu401_write_byte(120);  // all sound off
        mpu401_write_byte(0);

        mpu401_write_byte(status);
        mpu401_write_byte(123);  // all notes off
        mpu401_write_byte(0);

        mpu401_write_byte(status);
        mpu401_write_byte(121);  // reset all controllers
        mpu401_write_byte(0);
    }
}

/* --------------------------------
   vhook callback (100 Hz, ISR context)
   -------------------------------- */
static void midi_playback_hook(void *userdata) {
    NativeMidiHookContext *ctx = (NativeMidiHookContext *)userdata;
    if (!ctx) return;

    ctx->in_hook++;

    NativeMidiSong *song = (NativeMidiSong *)ctx->song;
    if (!song || !song->playing) {
        ctx->in_hook--;
        return;
    }

    uint64_t now_us = vclock();
    int total_events = (int)song->events.size();

    // If no events, stop
    if (total_events == 0) {
        song->playing = false;
        ctx->in_hook--;
        return;
    }

    // Check if we've finished all events
    if (song->current_event >= total_events) {
        if (song->looping) {
            song->current_event = 0;
            song->start_time_us = now_us;   // reset base time for loop
        } else {
            song->playing = false;
            ctx->in_hook--;
            return;
        }
    }

    while (song->current_event < total_events) {
        PlayEvent &ev = song->events[song->current_event];
        uint64_t ev_time_us = song->start_time_us + ev.trigger_us;

        if (ev_time_us > now_us) {
            break;
        }

        uint32_t len = ev.msg->get_length();
        uint8_t buffer[256];

        if (len > sizeof(buffer)) {
            song->current_event++;
            continue;
        }

        ev.msg->get_bytes(buffer);

        static int events_sent = 0;
        events_sent++;

        for (uint32_t i = 0; i < len; i++) {
            mpu401_write_byte(buffer[i]);
        }

        song->current_event++;
    }

    if (song->current_event >= total_events && song->looping) {
        song->current_event = 0;
        song->start_time_us = now_us;
    } else if (song->current_event >= total_events) {
        song->playing = false;
    }

    ctx->in_hook--;
}

/* --------------------------------
   Public API implementation
   -------------------------------- */

int native_midi_detect() {
    return mpu401_init() == MPU401_OK;
}

NativeMidiSong *native_midi_loadsong(const char *midifile) {
    SDL_RWops *rw = SDL_RWFromFile(midifile, "rb");
    if (!rw){
        UTIL_LogOutput(LOGLEVEL_ERROR, "Failed to open MIDI file: %s\n", midifile);
        return nullptr;
    }
    NativeMidiSong *song = native_midi_loadsong_RW(rw);
    SDL_RWclose(rw);
    return song;
}

NativeMidiSong *native_midi_loadsong_RW(SDL_RWops *rw) {
    std::unique_ptr<NativeMidiSong> song(new NativeMidiSong());
    if (!song) return nullptr;

    MIDIEvent *eventlist = CreateMIDIEventList(rw, &song->ppq);
    if (!eventlist) return nullptr;

    uint64_t total_duration;
    if (!MidiEventListToPlayEvents(eventlist, song->ppq, song->events, total_duration)) {
        UTIL_LogOutput(LOGLEVEL_ERROR, "Failed to convert MIDI events to playback events.\n");
        FreeMIDIEventList(eventlist);
        return nullptr;
    }
    FreeMIDIEventList(eventlist);

    song->current_event = 0;
    song->playing = false;
    song->looping = false;
    song->loaded = true;
    song->start_time_us = 0;

    return song.release();
}

void native_midi_freesong(NativeMidiSong *song) {
    if (!song) return;
    native_midi_stop(song);
    delete song;
}

void native_midi_start(NativeMidiSong *song, int looping) {
    if (!song || !song->loaded) return;

    if (g_current_song && g_current_song != song) {
        native_midi_stop(g_current_song);
    } else {
        native_midi_stop(song);
    }

    // Initialize synthesizer (GM Reset + volume settings)
    init_synthesizer();

    song->playing = true;
    song->looping = (looping != 0);
    song->current_event = 0;
    song->start_time_us = vclock();

    // Register the vhook only once (fixed 100 Hz).
    if (!g_vhook_registered) {
        if (vhook_register(midi_playback_hook, 100, &g_midi_hook_ctx) == 0) {
            g_vhook_registered = true;
        } else {
            UTIL_LogOutput(LOGLEVEL_ERROR, "Failed to register vhook for MIDI playback.\n");
            song->playing = false;
            return;
        }
    }

    g_current_song = song;
    g_midi_hook_ctx.song = song;
}

void native_midi_stop(NativeMidiSong *song) {
    if (g_current_song) {
        g_current_song->playing = false;
        g_current_song->current_event = 0;
    }

    if (!song && !g_current_song) return;

    if (song) {
        song->playing = false;
    }

    if (g_midi_hook_ctx.song == g_current_song || g_midi_hook_ctx.song == song) {
        g_midi_hook_ctx.song = NULL;
    }

    panic_synthesizer();

    if (song) {
        song->current_event = 0;
    }

    // Wait for any in-flight hook to leave before caller potentially frees song.
    int wait_count = 0;
    while (g_midi_hook_ctx.in_hook > 0 && wait_count < 50) {
        SDL_Delay(1);
        wait_count++;
    }

    if (g_current_song == song) {
        g_current_song = nullptr;
    }
}

int native_midi_active(NativeMidiSong *song) {
    return (song && song->playing) ? 1 : 0;
}

void native_midi_setvolume(NativeMidiSong *song, int volume) {
    (void)song; (void)volume;
    // Volume control is handled by the MIDI file itself; we could implement
    // master volume via CC#7 or sysex, but it's not required for this port.
}

const char *native_midi_error(NativeMidiSong *song) {
    (void)song;
    return ""; // no error reporting
}