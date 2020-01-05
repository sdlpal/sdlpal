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

#include "main.h"

static int  g_iMidiCurrent = -1;
static NativeMidiSong *g_pMidi = NULL;
static int  g_iMidiVolume = PAL_MAX_VOLUME;

void
MIDI_SetVolume(
	int       iVolume
)
{
#if PAL_HAS_NATIVEMIDI
	g_iMidiVolume = iVolume;
	if (g_pMidi)
	{
		native_midi_setvolume(g_pMidi, iVolume * 127 / PAL_MAX_VOLUME);
	}
#endif
}

void
MIDI_Play(
	int       iNumRIX,
	BOOL      fLoop
)
{
#if PAL_HAS_NATIVEMIDI
	if (!native_midi_detect())
		return;

	if (native_midi_active(g_pMidi) && iNumRIX == g_iMidiCurrent)
	{
		return;
	}

	native_midi_stop(g_pMidi);
	native_midi_freesong(g_pMidi);
	g_pMidi = NULL;
	g_iMidiCurrent = -1;

	if (!AUDIO_MusicEnabled() || iNumRIX <= 0)
	{
		return;
	}

	if (gConfig.fIsWIN95)
	{
		g_pMidi = native_midi_loadsong(UTIL_GetFullPathName(PAL_BUFFER_SIZE_ARGS(0), gConfig.pszGamePath, PAL_va(1, "Musics%s%.3d.mid", PAL_NATIVE_PATH_SEPARATOR, iNumRIX)));
	}

	if (!g_pMidi)
	{
		FILE    *fp  = NULL;
		uint8_t *buf = NULL;
		int      size;

		if ((fp = UTIL_OpenFile("midi.mkf")) != NULL)
		{
			if ((size = PAL_MKFGetChunkSize(iNumRIX, fp)) > 0 &&
				(buf = (uint8_t*)UTIL_malloc(size)))
			{
				PAL_MKFReadChunk(buf, size, iNumRIX, fp);
			}
			fclose(fp);
		}

		if (buf)
		{
			SDL_RWops *rw = SDL_RWFromConstMem(buf, size);
			g_pMidi = native_midi_loadsong_RW(rw);
			SDL_RWclose(rw);
			free(buf);
		}
	}

	if (g_pMidi)
	{
		MIDI_SetVolume(g_iMidiVolume);
		native_midi_start(g_pMidi, fLoop);
		g_iMidiCurrent = iNumRIX;
	}
#endif
}
