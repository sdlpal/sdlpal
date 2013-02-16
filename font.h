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

#ifndef FONT_H
#define FONT_H

#include "common.h"
#include "palcommon.h"

#ifdef __cplusplus
extern "C"
{
#endif

INT
PAL_InitFont(
   VOID
);

VOID
PAL_FreeFont(
   VOID
);

VOID
PAL_DrawCharOnSurface(
   WORD                     wChar,
   SDL_Surface             *lpSurface,
   PAL_POS                  pos,
   BYTE                     bColor
);

VOID
PAL_DrawASCIICharOnSurface(
   BYTE                     bChar,
   SDL_Surface             *lpSurface,
   PAL_POS                  pos,
   BYTE                     bColor
);

#ifdef __cplusplus
}
#endif

#endif
