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

#ifndef FONT_H
#define FONT_H

#include "common.h"
#include "palcommon.h"
#include "palcfg.h"

PAL_C_LINKAGE_BEGIN

/*++
  Purpose:

    Initialize the font subsystem.

  Parameters:

    [IN]  cfg - Pointer to the configuration object.

  Return value:

    0 = success, -1 = failure.
--*/
int
PAL_InitFont(
	const CONFIGURATION* cfg
);

void
PAL_FreeFont(
	void
);

/*++
  Purpose:

    Draw a Unicode character on a surface.

  Parameters:

    [IN]  wChar - the unicode character to be drawn.

    [OUT] lpSurface - the destination surface.

    [IN]  pos - the destination location of the surface.

    [IN]  bColor - the color of the character.

  Return value:

    None.

--*/
void
PAL_DrawCharOnSurface(
	uint16_t                 wChar,
	SDL_Surface             *lpSurface,
	PAL_POS                  pos,
	uint8_t                  bColor,
	BOOL                     fUse8x8Font
);

/*++
  Purpose:

    Get the text width of a character.

  Parameters:

    [IN]  wChar - the unicode character for width calculation.

  Return value:

    The width of the character in pixels, 16 for full-width char and 8 for half-width char.

--*/
int
PAL_CharWidth(
	uint16_t                 wChar
);

/*++
  Purpose:

    Get the height of the currently used font.

  Parameters:

    None.

  Return value:

    The height of the font in pixels.

--*/
int
PAL_FontHeight(
	void
);

PAL_C_LINKAGE_END

#endif
