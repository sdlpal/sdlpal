/* -*- mode: c; tab-width: 4; c-basic-offset: 3; c-file-style: "linux" -*- */
//
// Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
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

#ifndef RIX_PLAY_H
#define RIX_PLAY_H

#include "common.h"

#ifdef __cplusplus

extern "C"
{
#endif

typedef struct tagMUSICPLAYER
{
#define MUSICPLAYER_FUNCTIONS \
	VOID (*Shutdown)(VOID*); \
	BOOL (*Play)(VOID*, INT, BOOL, FLOAT); \
	VOID (*FillBuffer)(VOID*, LPBYTE, INT)

	MUSICPLAYER_FUNCTIONS;
} MUSICPLAYER, *LPMUSICPLAYER;

/* RIX */

LPMUSICPLAYER
RIX_Init(
   LPCSTR     szFileName
);

/* OGG */
#if PAL_HAS_OGG

LPMUSICPLAYER
OGG_Init(
	LPCSTR    szFileName
);

#endif

/* MP3 */
#if PAL_HAS_MP3

LPMUSICPLAYER
MP3_Init(
	LPCSTR    szFileName
);

#endif

#ifdef __cplusplus
}
#endif

#endif
