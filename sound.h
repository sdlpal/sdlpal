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

#ifndef SOUND_H
#define SOUND_H

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

INT
AUDIO_OpenDevice(
   VOID
);

VOID
AUDIO_CloseDevice(
   VOID
);

VOID
AUDIO_PlaySound(
   INT    iSoundNum
);

SDL_AudioSpec*
AUDIO_GetDeviceSpec(
   VOID
);

VOID
AUDIO_AdjustVolume(
   INT    iDirection
);

VOID
AUDIO_PlayMusic(
   INT       iNumRIX,
   BOOL      fLoop,
   FLOAT     flFadeTime
);

BOOL
AUDIO_PlayCDTrack(
   INT    iNumTrack
);

VOID
AUDIO_EnableMusic(
   BOOL   fEnable
);

BOOL
AUDIO_MusicEnabled(
   VOID
);

VOID
AUDIO_EnableSound(
   BOOL   fEnable
);

BOOL
AUDIO_SoundEnabled(
   VOID
);

#ifdef PSP
VOID
SOUND_Reload(
	VOID
);
#endif

#define AUDIO_IsIntegerConversion(a) ((((a) % gConfig.iSampleRate) | (gConfig.iSampleRate % (a))) == 0)

#ifdef __cplusplus
}
#endif

#endif
