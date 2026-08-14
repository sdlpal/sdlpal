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
// ail32_midi.h: AIL/32 XMIDI client interface for DOS.
//                 @Author: palxex, 2026
//

#ifndef AIL32_MIDI_H
#define AIL32_MIDI_H

#include "sdl_compat.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct AIL32MidiSong AIL32MidiSong;

int ail32_midi_detect(void);
AIL32MidiSong *ail32_midi_loadsong(const char *midifile);
AIL32MidiSong *ail32_midi_loadsong_RW(SDL_RWops *rw);
void ail32_midi_freesong(AIL32MidiSong *song);
void ail32_midi_start(AIL32MidiSong *song, int looping);
void ail32_midi_stop(AIL32MidiSong *song);
int ail32_midi_active(AIL32MidiSong *song);
void ail32_midi_setvolume(AIL32MidiSong *song, int volume);

#ifdef __cplusplus
}
#endif

#endif
