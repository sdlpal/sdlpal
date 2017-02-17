/* -*- mode: c; tab-width: 4; c-basic-offset: 3; c-file-style: "linux" -*- */
//
// Copyright (c) 2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
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

#include "main.h"

#if !defined (CYGWIN) && !defined (DINGOO) &&  !defined (GEKKO) && !defined (GPH) && !defined(__N3DS__)

static INT iMidCurrent = -1;
static BOOL fMidLoop = FALSE;

static NativeMidiSong *g_pMid = NULL;

VOID
MIDI_Play(
   INT       iNumRIX,
   BOOL      fLoop
)
/*++
  Purpose:

    Start playing the specified music in MIDI format.

  Parameters:

    [IN]  iNumRIX - number of the music. 0 to stop playing current music.

    [IN]  fLoop - Whether the music should be looped or not.

  Return value:

    None.

--*/
{
   FILE            *fp;
   unsigned char   *buf;
   int              size;
   SDL_RWops       *rw;
#ifdef PAL_WIN95
   char             filename[1024];
#endif

   if (g_pMid != NULL && iNumRIX == iMidCurrent && native_midi_active())
   {
      return;
   }

   SOUND_PlayCDA(-1);
   native_midi_freesong(g_pMid);
   g_pMid = NULL;
   iMidCurrent = -1;

   if (g_fNoMusic || iNumRIX <= 0)
   {
      return;
   }

#ifdef PAL_WIN95
   sprintf(filename, "%s/musics/%.3d.mid", PAL_PREFIX, iNumRIX);

   g_pMid = native_midi_loadsong(filename);
   if (g_pMid != NULL)
   {
      native_midi_start(g_pMid);

      iMidCurrent = iNumRIX;
      fMidLoop = fLoop;
   }
#else
   fp = UTIL_OpenFile("midi.mkf");
   if (fp == NULL)
   {
      return;
   }

   if (iNumRIX > PAL_MKFGetChunkCount(fp))
   {
      fclose(fp);
      return;
   }

   size = PAL_MKFGetChunkSize(iNumRIX, fp);
   if (size <= 0)
   {
      fclose(fp);
      return;
   }

   buf = (unsigned char *)UTIL_malloc(size);

   PAL_MKFReadChunk((LPBYTE)buf, size, iNumRIX, fp);
   fclose(fp);

   rw = SDL_RWFromConstMem((const void *)buf, size);

   g_pMid = native_midi_loadsong_RW(rw);
   if (g_pMid != NULL)
   {
      native_midi_start(g_pMid);

      iMidCurrent = iNumRIX;
      fMidLoop = fLoop;
   }

   SDL_RWclose(rw);
   free(buf);
#endif
}

VOID
MIDI_CheckLoop(
   VOID
)
{
   if (fMidLoop && g_pMid != NULL && !native_midi_active())
   {
      MIDI_Play(iMidCurrent, TRUE);
   }
}

#endif
