/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2020, SDLPAL development team.
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
// players.h: Common definition of sound/music players.
//   @Author: Lou Yihua <louyihua@21cn.com>, 2015-07-28.
//

#ifndef PLAYERS_H
#define PLAYERS_H

#include "common.h"

typedef struct tagAUDIOPLAYER
{
#define AUDIOPLAYER_COMMONS \
    INT                        iMusic;  \
    BOOL                       fLoop; \
	VOID (*Shutdown)(VOID*); \
	BOOL (*Play)(VOID*, INT, BOOL, FLOAT); \
	VOID (*FillBuffer)(VOID*, LPBYTE, INT)

	AUDIOPLAYER_COMMONS;
} AUDIOPLAYER, *LPAUDIOPLAYER;

PAL_C_LINKAGE_BEGIN

/* RIX */

LPAUDIOPLAYER
RIX_Init(
   LPCSTR     szFileName
);

/* OGG */

LPAUDIOPLAYER
OGG_Init(
	VOID
);

/* OPUS */

LPAUDIOPLAYER
OPUS_Init(
	VOID
);

/* MP3 */

LPAUDIOPLAYER
MP3_Init(
	VOID
);

LPAUDIOPLAYER
SOUND_Init(
	VOID
);

PAL_C_LINKAGE_END

#endif
