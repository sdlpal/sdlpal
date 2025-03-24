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

#ifndef AUDIO_H
#define AUDIO_H

#include "common.h"
#include "players.h"

PAL_C_LINKAGE_BEGIN

typedef struct tagAUDIODEVICE
{
   SDL_AudioSpec             spec;        /* Actual-used sound specification */
   AUDIOPLAYER              *pMusPlayer;
   AUDIOPLAYER              *pCDPlayer;
#if PAL_HAS_SDLCD
   SDL_CD                   *pCD;
#endif
   AUDIOPLAYER              *pSoundPlayer;
   void                     *pSoundBuffer;    /* The output buffer for sound */
#if SDL_VERSION_ATLEAST(3,0,0)
   SDL_AudioStream             *stream;
#endif
#if SDL_VERSION_ATLEAST(2,0,0)
   SDL_AudioDeviceID         id;
#endif
   INT                       iMusicVolume;    /* The BGM volume ranged in [0, 128] for better performance */
   INT                       iSoundVolume;    /* The sound effect volume ranged in [0, 128] for better performance */
   BOOL                      fMusicEnabled; /* Is BGM enabled? */
   BOOL                      fSoundEnabled; /* Is sound effect enabled? */
   BOOL                      fOpened;       /* Is the audio device opened? */
} AUDIODEVICE;

#if SDL_VERSION_ATLEAST(3,0,0)
# define SDL_OpenAudio(desired, obtained) \
    ( \
     (gAudioDevice.id = SDL_OpenAudioDevice(iSelectedDeviceID, (desired))) && \
     (gAudioDevice.stream = SDL_CreateAudioStream(desired, desired)) && \
     SDL_SetAudioStreamGetCallback(gAudioDevice.stream, AUDIO_FillBuffer_Wrapper, NULL) && \
     SDL_BindAudioStream( gAudioDevice.id, gAudioDevice.stream ) \
    )
# define SDL_CloseAudio() SDL_DestroyAudioStream(gAudioDevice.stream)
# define SDL_LockAudio() SDL_LockAudioStream(gAudioDevice.stream)
# define SDL_UnlockAudio() SDL_UnlockAudioStream(gAudioDevice.stream)
#elif SDL_VERSION_ATLEAST(2,0,0)
# define SDL_PauseAudio(pause_on) SDL_PauseAudioDevice(gAudioDevice.id, (pause_on))
# define SDL_OpenAudio(desired, obtained) \
    ((gAudioDevice.id = SDL_OpenAudioDevice((gConfig.iAudioDevice >= 0 ? SDL_GetAudioDeviceName(gConfig.iAudioDevice, 0) : NULL), 0, (desired), (obtained), 0)) > 0 ? gAudioDevice.id : -1)
# define SDL_CloseAudio() SDL_CloseAudioDevice(gAudioDevice.id)
# define SDL_LockAudio() SDL_LockAudioDevice(gAudioDevice.id)
# define SDL_UnlockAudio() SDL_UnlockAudioDevice(gAudioDevice.id)
#endif

extern AUDIODEVICE gAudioDevice;

INT
AUDIO_OpenDevice(
   VOID
);

BOOL
AUDIO_CD_Available(
   VOID
);

VOID
AUDIO_CloseDevice(
   VOID
);

SDL_AudioSpec*
AUDIO_GetDeviceSpec(
   VOID
);

VOID
AUDIO_IncreaseVolume(
   VOID
);

VOID
AUDIO_DecreaseVolume(
   VOID
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
AUDIO_PlaySound(
   INT    iSoundNum
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

void
AUDIO_Lock(
	void
);

void
AUDIO_Unlock(
	void
);

PAL_C_LINKAGE_END

#define AUDIO_IsIntegerConversion(a) (((a) % gConfig.iSampleRate) == 0 || (gConfig.iSampleRate % (a)) == 0)

#endif
