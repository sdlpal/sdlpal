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
// midi_ch_map.c: Channel mapping table and lookup implementation.
//                Original authors: palxex, PalMusicFan, 2026
//

#include "midi_ch_map.h"
#include <stddef.h>

/*===========================================================================
 * Internal mapping table
 *---------------------------------------------------------------------------
 * Maps original MIDI track indices (1-87) to hardware channels (0-8).
 * This table is private to this compilation unit.
 *===========================================================================*/
static const struct
{
    int track_index;
    int target_channel;
} s_channel_map[] =
{
    {  1, 3 },
    {  2, 0 },
    {  3, 0 },
    {  4, 1 },
    {  5, 1 },
    {  6, 0 },
    {  7, 0 },
    {  8, 0 },
    {  9, 0 },
    { 10, 0 },

    { 11, 0 },
    { 12, 0 },
    { 13, 0 },
    { 14, 0 },
    { 15, 1 },
    { 16, 0 },
    { 17, 1 },
    { 18, 0 },
    { 19, 0 },
    { 20, 3 },

    { 21, 6 },
    { 22, 1 },
    { 23, 8 },
    { 24, 0 },
    { 25, 0 },
    { 26, 5 },
    { 27, 0 },
    { 28, 1 },
    //{ 29, None },
    { 30, 0 },

    { 31, 0 },
    { 32, 4 },
    { 33, 7 },
    { 34, 0 },
    { 35, 1 },
    { 36, 2 },
    { 37, 0 },
    { 38, 0 },
    { 39, 1 },
    { 40, 2 },

    { 41, 2 },
    { 42, 1 },
    { 43, 0 },
    { 44, 7 },
    { 45, 3 },
    { 46, 3 },
    { 47, 0 },
    { 48, 0 },
    { 49, 0 },
    { 50, 0 },

    { 51, 3 },
    { 52, 0 },
    { 53, 1 },
    { 54, 0 },
    { 55, 2 },
    { 56, 1 },
    { 57, 3 },
    { 58, 0 },
    { 59, 0 },
    { 60, 1 },

    { 61, 1 },
    { 62, 4 },
    { 63, 0 },
    { 64, 0 },
    { 65, 0 },
    { 66, 3 },
    { 67, 7 },
    { 68, 3 },
    { 69, 0 },
    { 70, 1 },

    { 71, 0 },
    { 72, 0 },
    { 73, 2 },
    { 74, 0 },
    { 75, 6 },
    { 76, 6 },
    { 77, 3 },
    { 78, 7 },
    { 79, 2 },
    { 80, 2 },

    { 81, 0 },
    { 82, 0 },
    { 83, 2 },
    { 84, 3 },
    { 85, 4 },
    { 86, 0 },
    { 87, 0 }
};

/*===========================================================================
 * Public API implementation
 *===========================================================================*/
int MIDI_GetMappedChannel(int track_index)
{
    size_t i;
    const size_t map_size = sizeof(s_channel_map) / sizeof(s_channel_map[0]);

    for (i = 0; i < map_size; i++)
    {
        if (s_channel_map[i].track_index == track_index)
        {
            return s_channel_map[i].target_channel;   /* 0~8 */
        }
    }

    return -1;   /* explicitly indicates "not found" */
}