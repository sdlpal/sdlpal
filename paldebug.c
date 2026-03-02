/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
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

#include "main.h"

#if PAL_DEBUG_SHOW_SEARCH_TRIGGER_RANGE
VOID
PAL_ShowSearchTriggerRange(
   VOID
)
/*++
  Purpose:

    Display the coordinates of 13 checkpoints used for manual
    event search on the map.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   const int          iPosNum[] = { 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3 };
   int                i;
   TRIGGERRANGE       range;

   range = PAL_GetSearchTriggerRange();

   for (i = 0; i < 13; i++)
      PAL_DrawNumber(iPosNum[i], 1, range.rgPos[i] - gpGlobals->viewport + PAL_XY(-2, -6), iPosNum[i] - 1, kNumAlignLeft);
}
#endif
