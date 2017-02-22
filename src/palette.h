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

#ifndef PALETTE_H
#define PALETTE_H

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

SDL_Color *
PAL_GetPalette(
   INT         iPaletteNum,
   BOOL        fNight
);

VOID
PAL_SetPalette(
   INT         iPaletteNum,
   BOOL        fNight
);

VOID
PAL_FadeOut(
   INT         iDelay
);

VOID
PAL_FadeIn(
   INT         iPaletteNum,
   BOOL        fNight,
   INT         iDelay
);

VOID
PAL_SceneFade(
   INT         iPaletteNum,
   BOOL        fNight,
   INT         iStep
);

VOID
PAL_PaletteFade(
   INT         iPaletteNum,
   BOOL        fNight,
   INT         iDelay
);

VOID
PAL_ColorFade(
   INT        iDelay,
   BYTE       bColor,
   BOOL       fFrom
);

VOID
PAL_FadeToRed(
   VOID
);

#ifdef __cplusplus
}
#endif

#endif
