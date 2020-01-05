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

#ifndef _PALUTILS_H
#define _PALUTILS_H

#include "common.h"

typedef LPBYTE      LPSPRITE, LPBITMAPRLE;
typedef LPCBYTE     LPCSPRITE, LPCBITMAPRLE;

#ifndef PAL_POS_DEFINED
#define PAL_POS_DEFINED
typedef DWORD           PAL_POS;
#endif

#define PAL_XY(x, y)    (PAL_POS)(((((WORD)(y)) << 16) & 0xFFFF0000) | (((WORD)(x)) & 0xFFFF))
#define PAL_X(xy)       (SHORT)((xy) & 0xFFFF)
#define PAL_Y(xy)       (SHORT)(((xy) >> 16) & 0xFFFF)
#define PAL_XY_OFFSET(xy, x, y)    (PAL_POS)(((((INT)(y) << 16) & 0xFFFF0000) + ((xy) & 0xFFFF0000)) | (((INT)(x) & 0xFFFF) + ((xy) & 0xFFFF)))

// maximum number of players in party
#define     MAX_PLAYERS_IN_PARTY         3

// total number of possible player roles
#define     MAX_PLAYER_ROLES             6

// totally number of playable player roles
#define     MAX_PLAYABLE_PLAYER_ROLES    5

// maximum entries of inventory
#define     MAX_INVENTORY                256

// maximum items in a store
#define     MAX_STORE_ITEM               9

// total number of magic attributes
#define     NUM_MAGIC_ELEMENTAL          5

// maximum number of enemies in a team
#define     MAX_ENEMIES_IN_TEAM          5

// maximum number of equipments for a player
#define     MAX_PLAYER_EQUIPMENTS        6

// maximum number of magics for a player
#define     MAX_PLAYER_MAGICS            32

// maximum number of scenes
#define     MAX_SCENES                   300

// maximum number of objects
#define     MAX_OBJECTS                  600

// maximum number of event objects (should be somewhat more than the original,
// as there are some modified versions which has more)
#define     MAX_EVENT_OBJECTS            5500

// maximum number of effective poisons to players
#define     MAX_POISONS                  16

// maximum number of level
#define     MAX_LEVELS                   99

#define     MINIMAL_WORD_COUNT           (MAX_OBJECTS + 13)

#define PAL_CDTRACK_BASE    10000

typedef enum tagPALDIRECTION
{
   kDirSouth = 0,
   kDirWest,
   kDirNorth,
   kDirEast,
   kDirUnknown
} PALDIRECTION, *LPPALDIRECTION;

typedef enum tagMUSICTYPE
{
	MUSIC_MIDI,
	MUSIC_RIX,
    MUSIC_MP3,
    MUSIC_OGG,
	MUSIC_OPUS
} MUSICTYPE, *LPMUSICTYPE;

typedef enum tagCDTYPE
{
    CD_NONE,
    CD_MP3,
	CD_OGG,
    CD_OPUS,
	CD_SDLCD
} CDTYPE, *LPCDTYPE;

typedef enum tagCODEPAGE {
	CP_MIN = 0,
	CP_BIG5 = 0,
	CP_GBK = 1,
	//CP_SHIFTJIS = 2,
	//CP_JISX0208 = 3,
	CP_MAX = CP_GBK + 1,
	CP_UTF_8 = CP_MAX + 1,
    CP_UCS = CP_UTF_8 + 1,
} CODEPAGE;

typedef enum tagPALFILE {
	PALFILE_ABC = 0x00000001,
	PALFILE_BALL = 0x00000002,
	PALFILE_DATA = 0x00000004,
	PALFILE_F = 0x00000008,
	PALFILE_FBP = 0x00000010,
	PALFILE_FIRE = 0x00000020,
	PALFILE_GOP = 0x00000040,
	PALFILE_MAP = 0x00000080,
	PALFILE_MGO = 0x00000100,
	PALFILE_PAT = 0x00000200,
	PALFILE_RGM = 0x00000400,
	PALFILE_RNG = 0x00000800,
	PALFILE_SSS = 0x00001000,
	PALFILE_MSG = 0x00002000,
	PALFILE_M = 0x00004000,
	PALFILE_WORD = 0x00008000,
	PALFILE_REQUIRED_MASK = 0x0000ffff,
	PALFILE_VOC = 0x00010000,
	PALFILE_SOUNDS = 0x00020000,
	PALFILE_SOUND_MASK = 0x00030000,
	PALFILE_MIDI = 0x00040000,
	PALFILE_MUS = 0x00080000,
	PALFILE_MUSIC_MASK = 0x000c0000,
} PALFILE;

#define PAL_MISSING_REQUIRED(x) (((x) & PALFILE_REQUIRED_MASK) != 0)
#define PAL_MISSING_SOUND(x) (((x) & PALFILE_SOUND_MASK) == PALFILE_SOUND_MASK)
#define PAL_MISSING_MUSIC(x) (((x) & PALFILE_MUSIC_MASK) == PALFILE_MUSIC_MASK)

PAL_C_LINKAGE_BEGIN

INT
PAL_RLEBlitToSurface(
   LPCBITMAPRLE      lpBitmapRLE,
   SDL_Surface      *lpDstSurface,
   PAL_POS           pos
);

INT
PAL_RLEBlitToSurfaceWithShadow(
   LPCBITMAPRLE      lpBitmapRLE,
   SDL_Surface      *lpDstSurface,
   PAL_POS           pos,
   BOOL              bShadow
);

INT
PAL_RLEBlitWithColorShift(
   LPCBITMAPRLE      lpBitmapRLE,
   SDL_Surface      *lpDstSurface,
   PAL_POS           pos,
   INT               iColorShift
);

INT
PAL_RLEBlitMonoColor(
   LPCBITMAPRLE      lpBitmapRLE,
   SDL_Surface      *lpDstSurface,
   PAL_POS           pos,
   BYTE              bColor,
   INT               iColorShift
);

INT
PAL_FBPBlitToSurface(
   LPBYTE            lpBitmapFBP,
   SDL_Surface      *lpDstSurface
);

INT
PAL_RLEGetWidth(
   LPCBITMAPRLE      lpBitmapRLE
);

INT
PAL_RLEGetHeight(
   LPCBITMAPRLE      lpBitmapRLE
);

WORD
PAL_SpriteGetNumFrames(
   LPCSPRITE       lpSprite
);

LPCBITMAPRLE
PAL_SpriteGetFrame(
   LPCSPRITE       lpSprite,
   INT             iFrameNum
);

INT
PAL_MKFGetChunkCount(
   FILE *fp
);

INT
PAL_MKFGetChunkSize(
   UINT    uiChunkNum,
   FILE   *fp
);

INT
PAL_MKFReadChunk(
   LPBYTE          lpBuffer,
   UINT            uiBufferSize,
   UINT            uiChunkNum,
   FILE           *fp
);

INT
PAL_MKFGetDecompressedSize(
   UINT    uiChunkNum,
   FILE   *fp
);

INT
PAL_MKFDecompressChunk(
   LPBYTE          lpBuffer,
   UINT            uiBufferSize,
   UINT            uiChunkNum,
   FILE           *fp
);

// From yj1.c:
extern INT
(*Decompress)(
   LPCVOID      Source,
   LPVOID       Destination,
   INT          DestSize
);

INT
YJ1_Decompress(
   LPCVOID      Source,
   LPVOID       Destination,
   INT          DestSize
);

INT
YJ2_Decompress(
   LPCVOID      Source,
   LPVOID       Destination,
   INT          DestSize
);

PAL_C_LINKAGE_END

#define PAL_DelayUntil(t) \
   PAL_ProcessEvent(); \
   while (!SDL_TICKS_PASSED(SDL_GetTicks(), (t))) \
   { \
      PAL_ProcessEvent(); \
      SDL_Delay(1); \
   }

#if SDL_VERSION_ATLEAST(2,0,0)
#define PAL_DelayUntilPC(t) \
   PAL_ProcessEvent(); \
   while (SDL_GetPerformanceCounter() < (t)) \
   { \
      PAL_ProcessEvent(); \
      SDL_Delay(1); \
   }
#else
#define SDL_GetPerformanceFrequency() (1000)
#define SDL_GetPerformanceCounter SDL_GetTicks
#define PAL_DelayUntilPC PAL_DelayUntil
#endif

#endif // _PALUTILS_H
