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

#include "palcommon.h"
#include "global.h"
#include "palcfg.h"
#include "audio.h"
#include "players.h"
#include "util.h"
#include "resampler.h"
#include "midi.h"
#include <math.h>

#if PAL_HAS_OGG
#include <vorbis/codec.h>
#endif

#define PAL_CDTRACK_BASE    10000

typedef void(*ResampleMixFunction)(void *, const void *, int, void *, int, int, uint8_t);

typedef struct tagAUDIODEVICE
{
   SDL_AudioSpec             spec;		/* Actual-used sound specification */
   SDL_AudioCVT              cvt;		/* Audio format conversion parameter */
   SDL_mutex                *mtx;		/* Mutex for preventing using destroyed objects */
   AUDIOPLAYER              *pMusPlayer;
   AUDIOPLAYER              *pCDPlayer;
#if PAL_HAS_SDLCD
   SDL_CD                   *pCD;
#endif
   AUDIOPLAYER              *pSoundPlayer;
   void                     *pSoundBuffer;	/* The output buffer for sound */
   INT                       iMusicVolume;	/* The BGM volume ranged in [0, 128] for better performance */
   INT                       iSoundVolume;	/* The sound effect volume ranged in [0, 128] for better performance */
   BOOL                      fMusicEnabled; /* Is BGM enabled? */
   BOOL                      fSoundEnabled; /* Is sound effect enabled? */
   BOOL                      fOpened;       /* Is the audio device opened? */
} AUDIODEVICE;

static AUDIODEVICE gAudioDevice;

PAL_FORCE_INLINE
void
AUDIO_MixNative(
	short     *dst,
	short     *src,
	int        samples
)
{
	while (samples > 0)
	{
		int val = *src++ + *dst;
		if (val > SHRT_MAX)
			*dst++ = SHRT_MAX;
		else if (val < SHRT_MIN)
			*dst++ = SHRT_MIN;
		else
			*dst++ = (short)val;
		samples--;
	}
}

PAL_FORCE_INLINE
void
AUDIO_AdjustVolume(
	short     *srcdst,
	int        iVolume,
	int        samples
)
{
	if (iVolume == SDL_MIX_MAXVOLUME) return;
	if (iVolume == 0) { memset(srcdst, 0, samples << 1); return; }
	while (samples > 0)
	{
		*srcdst = *srcdst * iVolume / SDL_MIX_MAXVOLUME;
		samples--; srcdst++;
	}
}

static VOID SDLCALL
AUDIO_FillBuffer(
   LPVOID          udata,
   LPBYTE          stream,
   INT             len
)
/*++
  Purpose:

    SDL sound callback function.

  Parameters:

    [IN]  udata - pointer to user-defined parameters (Not used).

    [OUT] stream - pointer to the stream buffer.

    [IN]  len - Length of the buffer.

  Return value:

    None.

--*/
{
#if SDL_VERSION_ATLEAST(2,0,0)
   memset(stream, 0, len);
#endif

   SDL_mutexP(gAudioDevice.mtx);

   gAudioDevice.cvt.buf = stream;
   gAudioDevice.cvt.len = len;

   //
   // Play music
   //
   if (gAudioDevice.fMusicEnabled && gAudioDevice.iMusicVolume > 0)
   {
      if (gAudioDevice.pMusPlayer)
      {
         gAudioDevice.pMusPlayer->FillBuffer(gAudioDevice.pMusPlayer, stream, len);
      }

      if (gAudioDevice.pCDPlayer)
      {
         gAudioDevice.pCDPlayer->FillBuffer(gAudioDevice.pCDPlayer, stream, len);
      }

      //
      // Adjust volume for music
      //
      AUDIO_AdjustVolume((short *)stream, gAudioDevice.iMusicVolume, len >> 1);
   }

   //
   // Play sound
   //
   if (gAudioDevice.fSoundEnabled && gAudioDevice.pSoundPlayer && gAudioDevice.iSoundVolume > 0)
   {
	   memset(gAudioDevice.pSoundBuffer, 0, len);

	   gAudioDevice.pSoundPlayer->FillBuffer(gAudioDevice.pSoundPlayer, gAudioDevice.pSoundBuffer, len);

	   //
	   // Adjust volume for sound
	   //
	   AUDIO_AdjustVolume((short *)gAudioDevice.pSoundBuffer, gAudioDevice.iSoundVolume, len >> 1);

	   //
	   // Mix sound & music
	   //
	   AUDIO_MixNative((short *)stream, gAudioDevice.pSoundBuffer, len >> 1);
   }

   //
   // Convert audio from native byte-order to actual byte-order
   //
   SDL_ConvertAudio(&gAudioDevice.cvt);

   SDL_mutexV(gAudioDevice.mtx);
}

INT
AUDIO_OpenDevice(
   VOID
)
/*++
  Purpose:

    Initialize the audio subsystem.

  Parameters:

    None.

  Return value:

    0 if succeed, others if failed.

--*/
{
   SDL_AudioSpec spec;

   if (gAudioDevice.fOpened)
   {
      //
      // Already opened
      //
      return -1;
   }

   gAudioDevice.fOpened = FALSE;
   gAudioDevice.fMusicEnabled = TRUE;
   gAudioDevice.fSoundEnabled = TRUE;
   gAudioDevice.iMusicVolume = gConfig.iMusicVolume * SDL_MIX_MAXVOLUME / PAL_MAX_VOLUME;
   gAudioDevice.iSoundVolume = gConfig.iSoundVolume * SDL_MIX_MAXVOLUME / PAL_MAX_VOLUME;

   //
   // Initialize the resampler module
   //
   resampler_init();

   //
   // Open the audio device.
   //
   gAudioDevice.spec.freq = gConfig.iSampleRate;
   gAudioDevice.spec.format = AUDIO_S16;
   gAudioDevice.spec.channels = gConfig.iAudioChannels;
   gAudioDevice.spec.samples = gConfig.wAudioBufferSize;
   gAudioDevice.spec.callback = AUDIO_FillBuffer;

   if (SDL_OpenAudio(&gAudioDevice.spec, &spec) < 0)
   {
      //
      // Failed
      //
      return -3;
   }
   else
   {
      gAudioDevice.spec = spec;
      gAudioDevice.pSoundBuffer = malloc(spec.size);
   }

   SDL_BuildAudioCVT(&gAudioDevice.cvt, AUDIO_S16SYS, spec.channels, spec.freq, spec.format, spec.channels, spec.freq);

   gAudioDevice.mtx = SDL_CreateMutex();
   gAudioDevice.fOpened = TRUE;

   //
   // Initialize the sound subsystem.
   //
   gAudioDevice.pSoundPlayer = SOUND_Init();

   //
   // Initialize the music subsystem.
   //
   switch (gConfig.eMusicType)
   {
   case MUSIC_RIX:
	   if (!(gAudioDevice.pMusPlayer = RIX_Init(va("%s%s", gConfig.pszGamePath, "mus.mkf"))))
	   {
		   gAudioDevice.pMusPlayer = RIX_Init(va("%s%s", gConfig.pszGamePath, "MUS.MKF"));
	   }
	   break;
   case MUSIC_MP3:
#if PAL_HAS_MP3
	   gAudioDevice.pMusPlayer = MP3_Init(NULL);
#else
	   gAudioDevice.pMusPlayer = NULL;
#endif
	   break;
   case MUSIC_OGG:
#if PAL_HAS_OGG
	   gAudioDevice.pMusPlayer = OGG_Init(NULL);
#else
	   gAudioDevice.pMusPlayer = NULL;
#endif
	   break;
   case MUSIC_MIDI:
	   gAudioDevice.pMusPlayer = NULL;
	   break;
   }

   //
   // Initialize the CD audio.
   //
   switch (gConfig.eCDType)
   {
   case MUSIC_SDLCD:
   {
#if PAL_HAS_SDLCD
	   int i;
	   gAudioDevice.pCD = NULL;

	   for (i = 0; i < SDL_CDNumDrives(); i++)
	   {
		   gAudioDevice.pCD = SDL_CDOpen(i);
		   if (gAudioDevice.pCD != NULL)
		   {
			   if (!CD_INDRIVE(SDL_CDStatus(gAudioDevice.pCD)))
			   {
				   SDL_CDClose(gAudioDevice.pCD);
				   gAudioDevice.pCD = NULL;
			   }
			   else
			   {
				   break;
			   }
		   }
	   }
#endif
	   gAudioDevice.pCDPlayer = NULL;
	   break;
   }
   case MUSIC_MP3:
#if PAL_HAS_MP3
	   gAudioDevice.pCDPlayer = MP3_Init(NULL);
#else
	   gAudioDevice.pCDPlayer = NULL;
#endif
	   break;
   case MUSIC_OGG:
#if PAL_HAS_OGG
	   gAudioDevice.pCDPlayer = OGG_Init(NULL);
#else
	   gAudioDevice.pCDPlayer = NULL;
#endif
	   break;
   }

   //
   // Let the callback function run so that musics will be played.
   //
   SDL_PauseAudio(0);

   return 0;
}

VOID
AUDIO_CloseDevice(
   VOID
)
/*++
  Purpose:

    Close the audio subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   SDL_CloseAudio();

   SDL_mutexP(gAudioDevice.mtx);

   if (gAudioDevice.pSoundPlayer != NULL)
   {
      gAudioDevice.pSoundPlayer->Shutdown(gAudioDevice.pSoundPlayer);
      gAudioDevice.pSoundPlayer = NULL;
   }

   if (gAudioDevice.pMusPlayer)
   {
	   gAudioDevice.pMusPlayer->Shutdown(gAudioDevice.pMusPlayer);
	   gAudioDevice.pMusPlayer = NULL;
   }

   if (gAudioDevice.pCDPlayer)
   {
	   gAudioDevice.pCDPlayer->Shutdown(gAudioDevice.pCDPlayer);
	   gAudioDevice.pCDPlayer = NULL;
   }

#if PAL_HAS_SDLCD
   if (gAudioDevice.pCD != NULL)
   {
      AUDIO_PlayCDTrack(-1);
      SDL_CDClose(gAudioDevice.pCD);
   }
#endif

   if (gAudioDevice.pSoundBuffer != NULL)
   {
      free(gAudioDevice.pSoundBuffer);
	  gAudioDevice.pSoundBuffer = NULL;
   }

   if (gConfig.eMusicType == MUSIC_MIDI) MIDI_Play(0, FALSE);

   SDL_mutexV(gAudioDevice.mtx);
   SDL_DestroyMutex(gAudioDevice.mtx);
}

SDL_AudioSpec*
AUDIO_GetDeviceSpec(
	VOID
)
{
	return &gAudioDevice.spec;
}

static INT
AUDIO_ChangeVolumeByValue(
   INT   *iVolume,
   INT    iValue
)
{
   *iVolume += iValue;
   if (*iVolume > PAL_MAX_VOLUME)
      *iVolume = PAL_MAX_VOLUME;
   else if (*iVolume < 0)
      *iVolume = 0;
   return *iVolume;
}

VOID
AUDIO_IncreaseVolume(
   VOID
)
/*++
  Purpose:

    Increase global volume by 3%.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   AUDIO_ChangeVolumeByValue(&gConfig.iMusicVolume, 3);
   AUDIO_ChangeVolumeByValue(&gConfig.iSoundVolume, 3);
   gAudioDevice.iMusicVolume = gConfig.iMusicVolume * SDL_MIX_MAXVOLUME / PAL_MAX_VOLUME;
   gAudioDevice.iSoundVolume = gConfig.iSoundVolume * SDL_MIX_MAXVOLUME / PAL_MAX_VOLUME;
}

VOID
AUDIO_DecreaseVolume(
   VOID
)
/*++
  Purpose:

    Decrease global volume by 3%.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   AUDIO_ChangeVolumeByValue(&gConfig.iMusicVolume, -3);
   AUDIO_ChangeVolumeByValue(&gConfig.iSoundVolume, -3);
   gAudioDevice.iMusicVolume = gConfig.iMusicVolume * SDL_MIX_MAXVOLUME / PAL_MAX_VOLUME;
   gAudioDevice.iSoundVolume = gConfig.iSoundVolume * SDL_MIX_MAXVOLUME / PAL_MAX_VOLUME;
}

VOID
AUDIO_PlaySound(
   INT    iSoundNum
)
/*++
  Purpose:

    Play a sound in voc.mkf/sounds.mkf file.

  Parameters:

    [IN]  iSoundNum - number of the sound; the absolute value is used.

  Return value:

    None.

--*/
{
   SDL_mutexP(gAudioDevice.mtx);
   if (gAudioDevice.pSoundPlayer)
   {
      gAudioDevice.pSoundPlayer->Play(gAudioDevice.pSoundPlayer, iSoundNum, FALSE, 0.0f);
   }
   SDL_mutexV(gAudioDevice.mtx);
}

VOID
AUDIO_PlayMusic(
   INT       iNumRIX,
   BOOL      fLoop,
   FLOAT     flFadeTime
)
{
   SDL_mutexP(gAudioDevice.mtx);
   if (gConfig.eMusicType == MUSIC_MIDI)
   {
      MIDI_Play(iNumRIX, fLoop);
   }
   else if (gAudioDevice.pMusPlayer)
   {
      gAudioDevice.pMusPlayer->Play(gAudioDevice.pMusPlayer, iNumRIX, fLoop, flFadeTime);
   }
   SDL_mutexV(gAudioDevice.mtx);
}

BOOL
AUDIO_PlayCDTrack(
   INT    iNumTrack
)
/*++
  Purpose:

    Play a CD Audio Track.

  Parameters:

    [IN]  iNumTrack - number of the CD Audio Track.

  Return value:

    TRUE if the track can be played, FALSE if not.

--*/
{
	BOOL ret = FALSE;
#if PAL_HAS_SDLCD
   if (gAudioDevice.pCD != NULL)
   {
      if (CD_INDRIVE(SDL_CDStatus(gAudioDevice.pCD)))
      {
         SDL_CDStop(gAudioDevice.pCD);

         if (iNumTrack != -1)
         {
            AUDIO_PlayMusic(-1, FALSE, 0);

            if (SDL_CDPlayTracks(gAudioDevice.pCD, iNumTrack - 1, 0, 1, 0) == 0)
            {
               return TRUE;
            }
         }
      }
   }
#endif
   SDL_mutexP(gAudioDevice.mtx);
   if (gAudioDevice.pCDPlayer)
   {
	   if (iNumTrack != -1)
	   {
		   AUDIO_PlayMusic(-1, FALSE, 0);
		   ret = gAudioDevice.pCDPlayer->Play(gAudioDevice.pCDPlayer, PAL_CDTRACK_BASE + iNumTrack, TRUE, 0);
	   }
	   else
	   {
		   ret = gAudioDevice.pCDPlayer->Play(gAudioDevice.pCDPlayer, -1, FALSE, 0);
	   }
   }
   SDL_mutexV(gAudioDevice.mtx);

   return ret;
}

VOID
AUDIO_EnableMusic(
   BOOL   fEnable
)
{
   gAudioDevice.fMusicEnabled = fEnable;
}

BOOL
AUDIO_MusicEnabled(
   VOID
)
{
   return gAudioDevice.fMusicEnabled;
}

VOID
AUDIO_EnableSound(
   BOOL   fEnable
)
{
	gAudioDevice.fSoundEnabled = fEnable;
}

BOOL
AUDIO_SoundEnabled(
   VOID
)
{
   return gAudioDevice.fSoundEnabled;
}
