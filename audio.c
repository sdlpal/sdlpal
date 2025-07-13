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

#include "palcommon.h"
#include "global.h"
#include "palcfg.h"
#include "audio.h"
#include "players.h"
#include "util.h"
#include "resampler.h"
#include "midi.h"
#include "aviplay.h"
#include <math.h>

#include <time.h>

/* WASAPI need fewer samples for less gapping */
#ifndef PAL_AUDIO_FORCE_BUFFER_SIZE_WASAPI
# define PAL_AUDIO_FORCE_BUFFER_SIZE_WASAPI   512
#endif

typedef void(*ResampleMixFunction)(void *, const void *, int, void *, int, int, uint8_t);

AUDIODEVICE gAudioDevice;

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
   memset(stream, 0, len);

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
   // Play sound for AVI
   //
   AVI_FillAudioBuffer(AVI_GetPlayState(), (LPBYTE)stream, len);
}

#if SDL_VERSION_ATLEAST(3,0,0)
static void SDLCALL AUDIO_FillBuffer_Wrapper(void* userdata, SDL_AudioStream* stream, int additional_amount, int total_amount)
{
	if (additional_amount > 0) {
		Uint8* data = SDL_stack_alloc(Uint8, additional_amount);
		if (data) {
			AUDIO_FillBuffer(userdata, data, additional_amount);
			SDL_PutAudioStreamData(stream, data, additional_amount);
			SDL_stack_free(data);
		}
	}
}
#endif

BOOL
AUDIO_CD_Available(
   VOID
)
{
   return gConfig.eCDType != CD_NONE;
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
   if(gConfig.eMIDISynth == SYNTH_NATIVE)
   MIDI_SetVolume(gConfig.iMusicVolume);

   //
   // Initialize the resampler module
   //
   resampler_init();

#if SDL_VERSION_ATLEAST(2,0,0)
    for( int i = 0; i<SDL_GetNumAudioDrivers();i++)
    {
        UTIL_LogOutput(LOGLEVEL_VERBOSE, "Available audio driver %d:%s\n", i, SDL_GetAudioDriver(i));
    }
    const char* driver_name = SDL_GetCurrentAudioDriver();
    if (driver_name) {
        UTIL_LogOutput(LOGLEVEL_VERBOSE, "Audio subsystem initialized; current driver is %s.\n", driver_name);
        if(SDL_strncmp(driver_name, "wasapi", 6)==0)
            gConfig.wAudioBufferSize = PAL_AUDIO_FORCE_BUFFER_SIZE_WASAPI;
    } else {
        UTIL_LogOutput(LOGLEVEL_VERBOSE, "Audio subsystem not initialized.\n");
    }
# if SDL_VERSION_ATLEAST(3,0,0)
	int i, num_devices;
	int iSelectedDeviceID = SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK;
	SDL_AudioDeviceID* devices = SDL_GetAudioPlaybackDevices(&num_devices);
	if (devices) {
		for (i = 0; i < num_devices; ++i) {
			SDL_AudioDeviceID instance_id = devices[i];
			UTIL_LogOutput(LOGLEVEL_VERBOSE, "Available audio device %d:%s\n", i, SDL_iconv_utf8_ucs2(SDL_GetAudioDeviceName(instance_id)));
		}
		if(gConfig.iAudioDevice >= 0 && gConfig.iAudioDevice < num_devices)
			iSelectedDeviceID = devices[gConfig.iAudioDevice];
		UTIL_LogOutput(LOGLEVEL_VERBOSE, "OpenAudio: requesting audio device: %s\n", (iSelectedDeviceID != SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK ? SDL_GetAudioDeviceName(iSelectedDeviceID) : "default"));
		SDL_free(devices);
	}
# else
    for( int i = 0; i<SDL_GetNumAudioDevices(0);i++)
    {
        UTIL_LogOutput(LOGLEVEL_VERBOSE, "Available audio device %d:%s\n", i, SDL_GetAudioDeviceName(i,0));
    }
    UTIL_LogOutput(LOGLEVEL_VERBOSE, "OpenAudio: requesting audio device: %s\n",(gConfig.iAudioDevice >= 0 ? SDL_GetAudioDeviceName(gConfig.iAudioDevice, 0) : "default"));
# endif
#endif


   //
   // Open the audio device.
   //
   gAudioDevice.spec.freq = gConfig.iSampleRate;
   gAudioDevice.spec.format = AUDIO_S16SYS;
   gAudioDevice.spec.channels = gConfig.iAudioChannels;

#if SDL_VERSION_ATLEAST(3,0,0)
   memcpy(&spec, &gAudioDevice.spec, sizeof(SDL_AudioSpec));
   SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, PAL_va(0, "%d", gConfig.wAudioBufferSize));
#else
   gAudioDevice.spec.samples = gConfig.wAudioBufferSize;
   gAudioDevice.spec.callback = AUDIO_FillBuffer;
#endif

   UTIL_LogOutput(LOGLEVEL_VERBOSE, "OpenAudio: requesting audio spec:freq %d, format %d, channels %d, samples %d\n", gAudioDevice.spec.freq, gAudioDevice.spec.format,  gAudioDevice.spec.channels);

   if (SDL_OpenAudio(&gAudioDevice.spec, &spec) < 0)
   {
      UTIL_LogOutput(LOGLEVEL_VERBOSE, "OpenAudio ERROR: %s, got spec:freq %d, format %d, channels %d, samples %d\n", SDL_GetError(), spec.freq, spec.format, spec.channels);
      //
      // Failed
      //
      return -3;
   }
   else
   {
      UTIL_LogOutput(LOGLEVEL_VERBOSE, "OpenAudio succeed, got spec:freq %d, format %d, channels %d, samples %d\n", spec.freq, spec.format, spec.channels);
//hackhack
#if SDL_VERSION_ATLEAST(3,0,0) && __EMSCRIPTEN__
# define MULTIPLIER 10
#else
# define MULTIPLIER 1
#endif
      gAudioDevice.pSoundBuffer = malloc(gConfig.wAudioBufferSize * MULTIPLIER * gConfig.iAudioChannels * sizeof(short));
   }

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
       gAudioDevice.pMusPlayer = RIX_Init(UTIL_GetFullPathName(PAL_BUFFER_SIZE_ARGS(0), gConfig.pszGamePath, "mus.mkf"));
	   break;
   case MUSIC_MP3:
	   gAudioDevice.pMusPlayer = MP3_Init();
	   break;
   case MUSIC_OGG:
	   gAudioDevice.pMusPlayer = OGG_Init();
	   break;
   case MUSIC_OPUS:
	   gAudioDevice.pMusPlayer = OPUS_Init();
	   break;
   case MUSIC_MIDI:
	   if (gConfig.eMIDISynth == SYNTH_TIMIDITY)
		   gAudioDevice.pMusPlayer = TIMIDITY_Init();
	   else if (gConfig.eMIDISynth == SYNTH_TINYSOUNDFONT)
		   gAudioDevice.pMusPlayer = TSF_Init();
	   break;
   default:
	   break;
   }

   //
   // Initialize the CD audio.
   //
   switch (gConfig.eCDType)
   {
   case CD_SDLCD:
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
   case CD_MP3:
	   gAudioDevice.pCDPlayer = MP3_Init();
	   break;
   case CD_OGG:
	   gAudioDevice.pCDPlayer = OGG_Init();
	   break;
   case CD_OPUS:
	   gAudioDevice.pCDPlayer = OPUS_Init();
	   break;
   default:
      gAudioDevice.pCDPlayer = NULL;
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

   if (gConfig.eMIDISynth == SYNTH_NATIVE && gConfig.eMusicType == MUSIC_MIDI)
   {
      MIDI_Play(0, FALSE);
   }

   gAudioDevice.fOpened = FALSE;
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
   if(gConfig.eMIDISynth == SYNTH_NATIVE)
   MIDI_SetVolume(gConfig.iMusicVolume);
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
   if(gConfig.eMIDISynth == SYNTH_NATIVE)
   MIDI_SetVolume(gConfig.iMusicVolume);
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
   // Unlike musics that use the 'load as required' strategy, sound player
   // load the entire sound file at once, which may cause about 0.5s or longer
   // latency for large sound files. To prevent this latency affects audio playing,
   // the mutex lock is obtained inside the SOUND_Play function rather than here.
   if (gAudioDevice.pSoundPlayer)
   {
      gAudioDevice.pSoundPlayer->Play(gAudioDevice.pSoundPlayer, abs(iSoundNum), FALSE, 0.0f);
   }
}

VOID
AUDIO_PlayMusic(
   INT       iNumRIX,
   BOOL      fLoop,
   FLOAT     flFadeTime
)
{
	if (iNumRIX > 0)
	{
		//
		// Stop the current CD music.
		//
		AUDIO_PlayCDTrack(-1);
	}

   if (gConfig.eMIDISynth == SYNTH_NATIVE && gConfig.eMusicType == MUSIC_MIDI)
   {
      MIDI_Play(iNumRIX, fLoop);
      return;
   }

   AUDIO_Lock();
   if (gAudioDevice.pMusPlayer)
   {
      gAudioDevice.pMusPlayer->Play(gAudioDevice.pMusPlayer, iNumRIX, fLoop, flFadeTime);
   }
   AUDIO_Unlock();
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
                      special case: -2: do NOTHING

  Return value:

    TRUE if the track can be played, FALSE if not.

--*/
{
	BOOL ret = FALSE;
	if (iNumTrack > 0)
	{
		AUDIO_PlayMusic(-1, FALSE, 0);
	}
   if (iNumTrack == -2 && gAudioDevice.pCDPlayer->iMusic > PAL_CDTRACK_BASE )
   {
       return TRUE;
   }
#if PAL_HAS_SDLCD
   if (gAudioDevice.pCD != NULL)
   {
      if (CD_INDRIVE(SDL_CDStatus(gAudioDevice.pCD)))
      {
         SDL_CDStop(gAudioDevice.pCD);

         if (iNumTrack != -1)
         {
            if (SDL_CDPlayTracks(gAudioDevice.pCD, iNumTrack - 1, 0, 1, 0) == 0)
            {
               return TRUE;
            }
         }
      }
   }
#endif
   AUDIO_Lock();
   if (gAudioDevice.pCDPlayer)
   {
	   if (iNumTrack != -1)
	   {
		   ret = gAudioDevice.pCDPlayer->Play(gAudioDevice.pCDPlayer, PAL_CDTRACK_BASE + iNumTrack, TRUE, 0);
	   }
	   else
	   {
		   ret = gAudioDevice.pCDPlayer->Play(gAudioDevice.pCDPlayer, -1, FALSE, 0);
	   }
   }
   AUDIO_Unlock();

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

void
AUDIO_Lock(
	void
)
{
	SDL_LockAudio();
}

void
AUDIO_Unlock(
	void
)
{
	SDL_UnlockAudio();
}
