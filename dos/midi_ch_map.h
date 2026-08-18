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
// midi_ch_map.h: MIDI track to hardware channel mapping interface.
//                 @Author: palxex, 2026
//

#ifndef MIDI_CH_MAP_H
#define MIDI_CH_MAP_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Get the target hardware channel for a given MIDI track index.
 *
 * @param track_index  MIDI track number (as used in the original game's MIDI resources)
 * @return             Target hardware channel (0-8) if the track is explicitly mapped,
 *                     or -1 if the track is not present in the mapping table.
 */
int MIDI_GetMappedChannel(int track_index);

#ifdef __cplusplus
}
#endif

#endif /* MIDI_CH_MAP_H */