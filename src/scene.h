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

#ifndef _SCENE_H
#define	_SCENE_H

#ifdef	__cplusplus
extern "C"
{
#endif

VOID
PAL_ApplyWave(
   SDL_Surface    *lpSurface
);

VOID
PAL_MakeScene(
   VOID
);

BOOL
PAL_CheckObstacle(
   PAL_POS         pos,
   BOOL            fCheckEventObjects,
   WORD            wSelfObject
);

VOID
PAL_UpdatePartyGestures(
   BOOL             fWalking
);

VOID
PAL_UpdateParty(
   VOID
);

VOID
PAL_NPCWalkOneStep(
   WORD          wEventObjectID,
   INT           iSpeed
);

#ifdef	__cplusplus
}
#endif

#endif
