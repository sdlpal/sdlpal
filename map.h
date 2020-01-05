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

#ifndef _MAP_H
#define _MAP_H

#include "common.h"

//
// Map format:
//
// +----------------------------------------------> x
// | * * * * * * * * * * ... * * * * * * * * * *  (y = 0, h = 0)
// |  * * * * * * * * * * ... * * * * * * * * * * (y = 0, h = 1)
// | * * * * * * * * * * ... * * * * * * * * * *  (y = 1, h = 0)
// |  * * * * * * * * * * ... * * * * * * * * * * (y = 1, h = 1)
// | * * * * * * * * * * ... * * * * * * * * * *  (y = 2, h = 0)
// |  * * * * * * * * * * ... * * * * * * * * * * (y = 2, h = 1)
// | ............................................
// v
// y
//
// Note:
//
// Tiles are in diamond shape (32x15).
//
// Each tile is represented with a DWORD value, which contains information
// about the tile bitmap, block flag, height, etc.
//
// Bottom layer sprite index:
//  (d & 0xFF) | ((d >> 4) & 0x100)
//
// Top layer sprite index:
//  d >>= 16;
//  ((d & 0xFF) | ((d >> 4) & 0x100)) - 1)
//
// Block flag (player cannot walk through this tile):
//  d & 0x2000
//

typedef struct tagPALMAP
{
   DWORD          Tiles[128][64][2];
   LPSPRITE       pTileSprite;
   INT            iMapNum;
} PALMAP, *LPPALMAP;

typedef const PALMAP *LPCPALMAP;

PAL_C_LINKAGE_BEGIN

LPPALMAP
PAL_LoadMap(
   INT               iMapNum,
   FILE             *fpMapMKF,
   FILE             *fpGopMKF
);

VOID
PAL_FreeMap(
   LPPALMAP          lpMap
);

LPCBITMAPRLE
PAL_MapGetTileBitmap(
   BYTE       x,
   BYTE       y,
   BYTE       h,
   BYTE       ucLayer,
   LPCPALMAP  lpMap
);

BOOL
PAL_MapTileIsBlocked(
   BYTE       x,
   BYTE       y,
   BYTE       h,
   LPCPALMAP  lpMap
);

BYTE
PAL_MapGetTileHeight(
   BYTE       x,
   BYTE       y,
   BYTE       h,
   BYTE       ucLayer,
   LPCPALMAP  lpMap
);

VOID
PAL_MapBlitToSurface(
   LPCPALMAP             lpMap,
   SDL_Surface          *lpSurface,
   const SDL_Rect       *lpSrcRect,
   BYTE                  ucLayer
);

PAL_C_LINKAGE_END

//
// Convert map location to the real location
//
#define PAL_XYH_TO_POS(x, y, h)                       \
   PAL_POS((x) * 32 + (h) * 16, (y) * 16 + (h) * 8)

//
// Convert real location to map location
//
#define PAL_POS_TO_XYH(pos, x, y, h)                  \
{                                                     \
   (h) = (BYTE)(((PAL_X(pos) % 32) != 0) ? 1 : 0);    \
   (x) = (BYTE)(PAL_X(pos) / 32);                     \
   (y) = (BYTE)(PAL_Y(pos) / 16);                     \
}

#endif
