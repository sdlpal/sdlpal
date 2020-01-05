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

#ifndef PAL_MIDI_H
#define PAL_MIDI_H

#include "common.h"
#include "native_midi/native_midi.h"

/*++
  Purpose:

    Set volume for MIDI music.

  Parameters:

    [IN]  iVolume - volume in range 0-PAL_MAX_VOLUME.

  Return value:

    None.

--*/
PAL_C_LINKAGE
void
MIDI_SetVolume(
	int       iVolume
);

/*++
  Purpose:

    Start playing the specified music in MIDI format.

  Parameters:

    [IN]  iNumRIX - number of the music. 0 to stop playing current music.

    [IN]  fLoop - Whether the music should be looped or not.

  Return value:

    None.

--*/
PAL_C_LINKAGE
void
MIDI_Play(
	int       iNumRIX,
	BOOL      fLoop
);

#endif
