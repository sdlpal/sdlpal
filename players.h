/* -*- mode: c; tab-width: 4; c-basic-offset: 3; c-file-style: "linux" -*- */
//
// Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
// All rights reserved.
// Created by Lou Yihua <louyihua@21cn.com>, 2015-07-28.
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

#ifndef PLAYERS_H
#define PLAYERS_H

#include "common.h"

#ifdef __cplusplus

extern "C"
{
#endif

	typedef struct tagAUDIOPLAYER
	{
#define AUDIOPLAYER_COMMONS \
	VOID (*Shutdown)(VOID*); \
	BOOL (*Play)(VOID*, INT, BOOL, FLOAT); \
	VOID (*FillBuffer)(VOID*, LPBYTE, INT); \
	SDL_mutex *mutex

	AUDIOPLAYER_COMMONS;
} AUDIOPLAYER, *LPAUDIOPLAYER;

/* RIX */

LPAUDIOPLAYER
RIX_Init(
   LPCSTR     szFileName,
   SDL_mutex *mutex
);

/* OGG */
#if PAL_HAS_OGG

LPAUDIOPLAYER
OGG_Init(
	SDL_mutex *mutex
);

#endif

/* MP3 */
#if PAL_HAS_MP3

LPAUDIOPLAYER
MP3_Init(
	SDL_mutex *mutex
);

#endif

LPAUDIOPLAYER
SOUND_Init(
	SDL_mutex *mutex
);

#ifdef __cplusplus
}
#endif

#endif
