/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2011-2026, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
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
// ail32_midi.cpp: AIL/32 XMIDI client for DOS.
//                 @Author: palxex, 2026
//

#include "ail32_midi.h"
#include "ail32_drv.h"
#include "native_midi/native_midi_common.h"
#include "palcfg.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#define AIL_BRANCH_PT 120
#define AIL_TIMB_BNK 114
#define MAX_NOTES 32
#define MAX_TIMB 16384
#define MAX_RBRN 128

struct NoteState {
    uint8_t channel;
    uint8_t note;
    size_t duration_pos;
    uint64_t start_interval;
    bool active;
};

struct BranchState {
    uint16_t number;
    uint32_t offset;
};

struct TimbreState {
    uint8_t number;
    uint8_t bank;
};

struct XMIDBuilder {
    std::vector<uint8_t> evnt;
    NoteState notes[MAX_NOTES];
    BranchState branches[MAX_RBRN];
    TimbreState timbres[MAX_TIMB];
    unsigned branch_count;
    unsigned timbre_count;
    uint64_t dda_sum;
    uint64_t interval;
    uint64_t delta;
    uint64_t quantum;
    uint64_t tick_time;
    uint8_t timbre_bank[16];
    uint8_t rhythm_bank[16];
};

struct AIL32MidiSong {
    std::vector<uint8_t> xmid;
    void *state_table;
    HSEQUENCE sequence;
    bool playing;
    bool looping;
};

static void put_u16_le(std::vector<uint8_t> &out, uint16_t value)
{
    out.push_back((uint8_t)value);
    out.push_back((uint8_t)(value >> 8));
}

static void put_u32_be(std::vector<uint8_t> &out, uint32_t value)
{
    out.push_back((uint8_t)(value >> 24));
    out.push_back((uint8_t)(value >> 16));
    out.push_back((uint8_t)(value >> 8));
    out.push_back((uint8_t)value);
}

static void put_u32_le(std::vector<uint8_t> &out, uint32_t value)
{
    out.push_back((uint8_t)value);
    out.push_back((uint8_t)(value >> 8));
    out.push_back((uint8_t)(value >> 16));
    out.push_back((uint8_t)(value >> 24));
}

static void put_vln(std::vector<uint8_t> &out, uint64_t value)
{
    uint8_t bytes[4];
    int first = 3;
    bytes[3] = (uint8_t)(value & 0x7f);
    bytes[2] = (uint8_t)(((value >> 7) & 0x7f) | 0x80);
    bytes[1] = (uint8_t)(((value >> 14) & 0x7f) | 0x80);
    bytes[0] = (uint8_t)(((value >> 21) & 0x7f) | 0x80);
    for (int i = 0; i < 4; ++i) {
        if (bytes[i] & 0x7f) {
            first = i;
            break;
        }
    }
    for (int i = first; i < 4; ++i) out.push_back(bytes[i]);
}

static unsigned vln_size(uint64_t value)
{
    unsigned size = 1;
    while (value >>= 7) ++size;
    return size;
}

static void write_delta(XMIDBuilder &builder)
{
    while (builder.delta > 127) {
        builder.evnt.push_back(127);
        builder.delta -= 127;
    }
    if (builder.delta) {
        builder.evnt.push_back((uint8_t)builder.delta);
        builder.delta = 0;
    }
}

static void append_event(XMIDBuilder &builder, const uint8_t *data, size_t length)
{
    if (builder.delta) write_delta(builder);
    builder.evnt.insert(builder.evnt.end(), data, data + length);
}

static int event_type(const MIDIEvent *event)
{
    return (event->status >> 4) & 0x0f;
}

static bool is_note_on(const MIDIEvent *event)
{
    return event_type(event) == MIDI_STATUS_NOTE_ON && event->data[1] != 0;
}

static bool is_note_off(const MIDIEvent *event)
{
    return event_type(event) == MIDI_STATUS_NOTE_OFF ||
           (event_type(event) == MIDI_STATUS_NOTE_ON && event->data[1] == 0);
}

static int find_note(XMIDBuilder &builder, uint8_t channel, uint8_t note)
{
    for (int i = 0; i < MAX_NOTES; ++i) {
        if (builder.notes[i].active && builder.notes[i].channel == channel &&
            builder.notes[i].note == note)
            return i;
    }
    return -1;
}

static bool add_timbre(XMIDBuilder &builder, uint8_t bank, uint8_t number)
{
    for (unsigned i = 0; i < builder.timbre_count; ++i) {
        if (builder.timbres[i].bank == bank && builder.timbres[i].number == number)
            return true;
    }
    if (builder.timbre_count >= MAX_TIMB) return false;
    builder.timbres[builder.timbre_count].bank = bank;
    builder.timbres[builder.timbre_count].number = number;
    ++builder.timbre_count;
    return true;
}

static bool add_branch(XMIDBuilder &builder, uint8_t number)
{
    for (unsigned i = 0; i < builder.branch_count; ++i)
        if (builder.branches[i].number == number) return false;
    if (builder.branch_count >= MAX_RBRN) return false;
    builder.branches[builder.branch_count].number = number;
    builder.branches[builder.branch_count].offset = (uint32_t)builder.evnt.size();
    ++builder.branch_count;
    return true;
}

static bool add_note_on(XMIDBuilder &builder, const MIDIEvent *event)
{
    uint8_t bytes[3] = { event->status, event->data[0], event->data[1] };
    append_event(builder, bytes, 3);
    builder.evnt.push_back(0);
    for (int i = 0; i < MAX_NOTES; ++i) {
        if (!builder.notes[i].active) {
            builder.notes[i].active = true;
            builder.notes[i].channel = event->status & 0x0f;
            builder.notes[i].note = event->data[0];
            builder.notes[i].duration_pos = builder.evnt.size() - 1;
            builder.notes[i].start_interval = builder.interval;
            return true;
        }
    }
    return false;
}

static bool add_note_off(XMIDBuilder &builder, const MIDIEvent *event)
{
    int note_index = find_note(builder, event->status & 0x0f, event->data[0]);
    if (note_index < 0) return true;

    NoteState &note = builder.notes[note_index];
    uint64_t duration = builder.interval - note.start_interval;
    unsigned extra = vln_size(duration) - 1;
    size_t insert_at = note.duration_pos + 1;
    if (extra) {
        builder.evnt.insert(builder.evnt.begin() + insert_at, extra, 0);
        for (int i = 0; i < MAX_NOTES; ++i)
            if (builder.notes[i].active && builder.notes[i].duration_pos >= insert_at)
                builder.notes[i].duration_pos += extra;
        for (unsigned i = 0; i < builder.branch_count; ++i)
            if (builder.branches[i].offset >= insert_at)
                builder.branches[i].offset += extra;
    }
    std::vector<uint8_t> encoded;
    put_vln(encoded, duration);
    memcpy(&builder.evnt[note.duration_pos], encoded.data(), encoded.size());
    note.active = false;
    return true;
}

static bool append_midi_event(XMIDBuilder &builder, const MIDIEvent *event)
{
    uint8_t bytes[4];
    int type = event_type(event);

    if (is_note_on(event)) {
        uint8_t channel = event->status & 0x0f;
        if (builder.rhythm_bank[channel]) {
            if (!add_timbre(builder, builder.rhythm_bank[channel], event->data[0])) return false;
        }
        return add_note_on(builder, event);
    }
    if (is_note_off(event)) return add_note_off(builder, event);

    if (type >= MIDI_STATUS_NOTE_OFF && type <= MIDI_STATUS_PITCH_WHEEL) {
        bytes[0] = event->status;
        bytes[1] = event->data[0];
        bytes[2] = event->data[1];
        if (type == MIDI_STATUS_CONTROLLER && event->data[0] == AIL_BRANCH_PT) {
            if (!add_branch(builder, event->data[1])) return false;
            append_event(builder, bytes, 3);
        } else if (type == MIDI_STATUS_PROG_CHANGE || type == MIDI_STATUS_PRESSURE) {
            append_event(builder, bytes, 2);
        } else {
            append_event(builder, bytes, 3);
        }
        if (type == MIDI_STATUS_CONTROLLER && event->data[0] == AIL_TIMB_BNK)
            builder.timbre_bank[event->status & 0x0f] = event->data[1];
        if (type == MIDI_STATUS_CONTROLLER && event->data[0] == AIL_TIMB_BNK)
            return true;
        if (type == MIDI_STATUS_PROG_CHANGE &&
            !add_timbre(builder, builder.timbre_bank[event->status & 0x0f], event->data[0]))
            return false;
        return true;
    }

    if (event->status == 0xf0 || event->status == 0xf7) {
        append_event(builder, &event->status, 1);
        put_vln(builder.evnt, event->extraLen);
        builder.evnt.insert(builder.evnt.end(), event->extraData,
                            event->extraData + event->extraLen);
        return true;
    }

    if (event->status == 0xff) {
        if (event->data[0] == 0x2f || event->data[0] == 0x03 || event->data[0] == 0x04)
            return true;
        append_event(builder, &event->status, 1);
        builder.evnt.push_back(event->data[0]);
        put_vln(builder.evnt, event->extraLen);
        builder.evnt.insert(builder.evnt.end(), event->extraData,
                            event->extraData + event->extraLen);
        return true;
    }
    return true;
}

static bool build_xmid(MIDIEvent *events, uint16_t ppq, std::vector<uint8_t> &out)
{
    XMIDBuilder builder = {};
    uint32_t last_tick = 0;
    int tempo = 500000;
    builder.quantum = 100000000UL / 120;
    if (ppq == 0) return false;
    builder.tick_time = 50000000UL / ppq;
    builder.rhythm_bank[9] = 127;

    for (MIDIEvent *event = events; event; event = event->next) {
        uint32_t delta = event->time - last_tick;
        last_tick = event->time;
        builder.dda_sum += (uint64_t)delta * builder.tick_time;
        while (builder.dda_sum >= builder.quantum) {
            builder.dda_sum -= builder.quantum;
            ++builder.interval;
            ++builder.delta;
        }
        if (event->status == 0xff && event->data[0] == 0x51 && event->extraLen >= 3) {
            tempo = ((int)event->extraData[0] << 16) |
                    ((int)event->extraData[1] << 8) | event->extraData[2];
            builder.tick_time = (uint64_t)(100 * tempo) / ppq;
        }
        if (!append_midi_event(builder, event)) return false;
    }
    for (int i = 0; i < MAX_NOTES; ++i)
        if (builder.notes[i].active) return false;
    write_delta(builder);
    builder.evnt.push_back(0xff);
    builder.evnt.push_back(0x2f);
    builder.evnt.push_back(0);

    out.insert(out.end(), {'F', 'O', 'R', 'M', 0, 0, 0, 0, 'X', 'M', 'I', 'D'});
    if (builder.timbre_count) {
        std::vector<uint8_t> block;
        put_u16_le(block, (uint16_t)builder.timbre_count);
        for (unsigned i = 0; i < builder.timbre_count; ++i) {
            block.push_back(builder.timbres[i].number);
            block.push_back(builder.timbres[i].bank);
        }
        out.insert(out.end(), {'T', 'I', 'M', 'B'});
        put_u32_be(out, (uint32_t)block.size());
        out.insert(out.end(), block.begin(), block.end());
        if (block.size() & 1) out.push_back(0);
    }
    if (builder.branch_count) {
        std::vector<uint8_t> block;
        put_u16_le(block, (uint16_t)builder.branch_count);
        for (unsigned i = 0; i < builder.branch_count; ++i) {
            put_u16_le(block, builder.branches[i].number);
            put_u32_le(block, builder.branches[i].offset);
        }
        out.insert(out.end(), {'R', 'B', 'R', 'N'});
        put_u32_be(out, (uint32_t)block.size());
        out.insert(out.end(), block.begin(), block.end());
        if (block.size() & 1) out.push_back(0);
    }
    out.insert(out.end(), {'E', 'V', 'N', 'T'});
    put_u32_be(out, (uint32_t)builder.evnt.size());
    out.insert(out.end(), builder.evnt.begin(), builder.evnt.end());
    if (builder.evnt.size() & 1) out.push_back(0);
    uint32_t form_size = (uint32_t)out.size() - 8;
    out[4] = (uint8_t)(form_size >> 24);
    out[5] = (uint8_t)(form_size >> 16);
    out[6] = (uint8_t)(form_size >> 8);
    out[7] = (uint8_t)form_size;
    (void)tempo;
    return true;
}

int ail32_midi_detect(void)
{
    int result = ail32_drv_init(gConfig.pszMIDIClient) == 0;
    return result;
}

AIL32MidiSong *ail32_midi_loadsong(const char *midifile)
{
    SDL_RWops *rw = SDL_RWFromFile(midifile, "rb");
    AIL32MidiSong *song;
    if (!rw) return NULL;
    song = ail32_midi_loadsong_RW(rw);
    SDL_RWclose(rw);
    return song;
}

AIL32MidiSong *ail32_midi_loadsong_RW(SDL_RWops *rw)
{
    AIL32MidiSong *song;
    MIDIEvent *events;
    uint16_t ppq;
    unsigned state_size;

    if (ail32_midi_detect() == 0) return NULL;
    events = CreateMIDIEventList(rw, &ppq);
    if (!events) {
        return NULL;
    }
    song = new AIL32MidiSong();
    if (build_xmid(events, ppq, song->xmid)) {
        state_size = AIL_state_table_size(ail32_drv_handle());
        song->state_table = calloc(1, state_size ? state_size : 1);
        if (!song->state_table) {
            delete song;
            song = NULL;
        } else {
            song->sequence = AIL_register_sequence(ail32_drv_handle(), song->xmid.data(),
                                                   0, song->state_table, NULL);
            if (song->sequence < 0) {
                free(song->state_table);
                delete song;
                song = NULL;
            } else if (ail32_drv_install_sequence_timbres(song->sequence,
                                                           gConfig.pszSoundBank) != 0) {
                AIL_release_sequence_handle(ail32_drv_handle(), song->sequence);
                free(song->state_table);
                delete song;
                song = NULL;
            }
        }
    } else {
        delete song;
        song = NULL;
    }
    FreeMIDIEventList(events);
    return song;
}

void ail32_midi_freesong(AIL32MidiSong *song)
{
    if (!song) return;
    ail32_midi_stop(song);
    if (song->sequence >= 0) AIL_release_sequence_handle(ail32_drv_handle(), song->sequence);
    free(song->state_table);
    delete song;
}

void ail32_midi_start(AIL32MidiSong *song, int looping)
{
    if (!song) return;
    song->looping = looping != 0;
    song->playing = true;
    AIL_start_sequence(ail32_drv_handle(), song->sequence);
}

void ail32_midi_stop(AIL32MidiSong *song)
{
    if (!song) return;
    AIL_stop_sequence(ail32_drv_handle(), song->sequence);
    song->playing = false;
}

int ail32_midi_active(AIL32MidiSong *song)
{
    unsigned status;
    if (!song) return 0;
    status = AIL_sequence_status(ail32_drv_handle(), song->sequence);
    if (status == SEQ_DONE && song->looping) {
        AIL_start_sequence(ail32_drv_handle(), song->sequence);
        return 1;
    }
    song->playing = status == SEQ_PLAYING;
    return song->playing ? 1 : 0;
}

void ail32_midi_setvolume(AIL32MidiSong *song, int volume)
{
    if (song) AIL_set_relative_volume(ail32_drv_handle(), song->sequence,
                                      (unsigned)volume * 100U / 127U, 0);
}
