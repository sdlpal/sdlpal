/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2017, SDLPAL development team.
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

static int  iMidCurrent = -1;
static BOOL fMidLoop = FALSE;

static NativeMidiSong *g_pMid = NULL;

void
MIDI_Play(
	int       iNumRIX,
	BOOL      fLoop
)
{
#if PAL_HAS_NATIVEMIDI
   if (g_pMid != NULL && iNumRIX == iMidCurrent && native_midi_active())
   {
      return;
   }

   AUDIO_PlayCDTrack(-1);
   native_midi_freesong(g_pMid);
   g_pMid = NULL;
   iMidCurrent = -1;

   if (!AUDIO_MusicEnabled() || iNumRIX <= 0)
   {
      return;
   }

   if (gConfig.fIsWIN95)
   {
      char filename[1024];
      sprintf(filename, "%s/musics/%.3d.mid", PAL_PREFIX, iNumRIX);

      g_pMid = native_midi_loadsong(filename);
      if (g_pMid != NULL)
      {
         native_midi_start(g_pMid);

         iMidCurrent = iNumRIX;
         fMidLoop = fLoop;
      }
   }

   if (!g_pMid)
   {
      unsigned char   *buf;
      int              size;
      SDL_RWops       *rw;
      FILE            *fp = UTIL_OpenFile("midi.mkf");

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
   }
#endif
}

void
MIDI_CheckLoop(
   void
)
{
#if PAL_HAS_NATIVEMIDI
   if (fMidLoop && g_pMid != NULL && !native_midi_active())
   {
      MIDI_Play(iMidCurrent, TRUE);
   }
#endif
}
