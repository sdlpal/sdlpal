/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2021, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
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

#ifndef ENDGAME_H
#define ENDGAME_H

#include "common.h"

PAL_C_LINKAGE_BEGIN

VOID
PAL_EndingSetEffectSprite(
   WORD         wSpriteNum
);

VOID
PAL_ShowFBP(
   WORD         wChunkNum,
   WORD         wFade
);

VOID
PAL_ScrollFBP(
   WORD         wChunkNum,
   WORD         wScrollSpeed,
   BOOL         fScrollDown
);

VOID
PAL_EndingAnimation(
   VOID
);

VOID
PAL_EndingScreen(
   VOID
);

PAL_C_LINKAGE_END

#endif
