/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2018, SDLPAL development team.
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
// native_midi.h: Header of native MIDI player for SDLPal.
//       @Author: Lou Yihua, 2017
//

#ifndef _NATIVE_MIDI_H_
#define _NATIVE_MIDI_H_

#include <SDL_rwops.h>

typedef struct _NativeMidiSong NativeMidiSong;

#ifdef __cplusplus
extern "C"
{
#endif

/*++
  Purpose:

    Check whether there is an available MIDI player.

  Parameters:

    None.

  Return value:

    Returns 1 for success and 0 for failure.

--*/
int native_midi_detect();

/*++
  Purpose:

    Load the MIDI song from given file.

  Parameters:

    [IN]  midifile - the MIDI file name.

  Return value:

    A NativeMidiSong object on success, NULL on failure.

--*/
NativeMidiSong *native_midi_loadsong(const char *midifile);

/*++
  Purpose:

    Load the MIDI song from SDL_RWops stream.

  Parameters:

    [IN]  rw - pointer to the SDL_RWops stream.

  Return value:

    A NativeMidiSong object on success, NULL on failure.

--*/
NativeMidiSong *native_midi_loadsong_RW(SDL_RWops *rw);

/*++
  Purpose:

    Free a NativeMidiSong object returned from native_midi_loadsong_*.

  Parameters:

    [IN]  song - the NativeMidiSong object.

  Return value:

    None.

--*/
void native_midi_freesong(NativeMidiSong *song);

/*++
  Purpose:

    Start playing the MIDI.

  Parameters:

    [IN]  song    - the NativeMidiSong object.
	[IN]  looping - whether the play should be automatically looped.

  Return value:

    None.

--*/
void native_midi_start(NativeMidiSong *song, int looping);

/*++
  Purpose:

    Stop playing the MIDI.

  Parameters:

    [IN]  song - the NativeMidiSong object.

  Return value:

    None.

--*/
void native_midi_stop(NativeMidiSong *song);

/*++
  Purpose:

    Check whether the MIDI is now playing.

  Parameters:

    [IN]  song - the NativeMidiSong object.

  Return value:

    Returns 1 if MIDI is now playing, and 0 otherwise.

--*/
int native_midi_active(NativeMidiSong *song);

/*++
  Purpose:

    Set the volume for the MIDI playing.

  Parameters:

    [IN]  song - the NativeMidiSong object.
    [IN]  volume - midi volume in range 0-127.

  Return value:

    None.

--*/
void native_midi_setvolume(NativeMidiSong *song, int volume);

/*++
  Purpose:

    Get the last error if any.

  Parameters:

    [IN]  song - the NativeMidiSong object.

  Return value:

    None.

--*/
const char *native_midi_error(NativeMidiSong *song);

#ifdef __cplusplus
}
#endif

#endif /* _NATIVE_MIDI_H_ */
