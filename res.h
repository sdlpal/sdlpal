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

#ifndef RES_H
#define RES_H

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum tagLOADRESFLAG
{
   kLoadScene          = (1 << 0),    // load a scene
   kLoadPlayerSprite   = (1 << 1),    // load player sprites
} LOADRESFLAG, *LPLOADRESFLAG;

VOID
PAL_InitResources(
   VOID
);

VOID
PAL_FreeResources(
   VOID
);

VOID
PAL_SetLoadFlags(
   BYTE       bFlags
);

VOID
PAL_LoadResources(
   VOID
);

LPPALMAP
PAL_GetCurrentMap(
   VOID
);

LPSPRITE
PAL_GetPlayerSprite(
   BYTE      bPlayerIndex
);

LPSPRITE
PAL_GetBattleSprite(
   BYTE      bPlayerIndex
);

LPSPRITE
PAL_GetEventObjectSprite(
   WORD      wEventObjectID
);

#ifdef __cplusplus
}
#endif

#endif
