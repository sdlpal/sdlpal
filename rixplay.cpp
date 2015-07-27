/* -*- mode: c++; tab-width: 4; c-basic-offset: 3; c-file-style: "linux" -*- */
//
// Copyright (c) 2008, Wei Mingzhi <whistler_wmz@users.sf.net>.
// All rights reserved.
//
// This program is free software: you can redistribute it and/or modify
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

#include "rixplay.h"
#include "sound.h"

#define OPL_SAMPLERATE       49716

#if OPL_SAMPLERATE != PAL_SAMPLE_RATE
#define USE_RESAMPLER        1
#endif
#if PAL_CHANNELS == 2
#define USE_SURROUNDOPL      1
#endif
#define USE_DEMUOPL          1

#if USE_RESAMPLER
#include "resampler.h"
#endif

#include "adplug/opl.h"
#if USE_DEMUOPL
#include "adplug/demuopl.h"
#else
#include "adplug/emuopl.h"
#endif
#if USE_SURROUNDOPL
#include "adplug/surroundopl.h"
#endif
#include "adplug/rix.h"

extern "C" BOOL g_fNoMusic;
#ifdef __SYMBIAN32__
extern "C" INT  g_iVolume;
#endif

typedef struct tagRIXPLAYER
{
   tagRIXPLAYER() : iCurrentMusic(-1) {}
   Copl                      *opl;
   CrixPlayer                *rix;
   void                      *resampler[2];
   INT                        iCurrentMusic; // current playing music number
   INT                        iNextMusic; // the next music number to switch to
   DWORD                      dwStartFadeTime;
   DWORD                      dwFadeLength;
   enum { NONE, FADE_IN, FADE_OUT } FadeType; // fade in or fade out ?
   BOOL                       fLoop;
   BOOL                       fNextLoop;
   BYTE                       buf[PAL_SAMPLE_RATE / 70 * sizeof(short) * PAL_CHANNELS];
   LPBYTE                     pos;
} RIXPLAYER, *LPRIXPLAYER;

static LPRIXPLAYER gpRixPlayer = NULL;

VOID
RIX_FillBuffer(
	LPBYTE     stream,
	INT        len
	)
	/*++
	  Purpose:

		Fill the background music into the sound buffer. Called by the SDL sound
		callback function only (sound.c: SOUND_FillAudio).

	  Parameters:

		[OUT] stream - pointer to the stream buffer.

		[IN]  len - Length of the buffer.

	  Return value:

		None.

	--*/
{
	INT       i, l, volume = SDL_MIX_MAXVOLUME / 2;
	UINT      t = SDL_GetTicks();

#ifdef __SYMBIAN32__
	volume = g_iVolume / 2;
#endif

	if (gpRixPlayer == NULL)
	{
		//
		// Not initialized
		//
		return;
	}

	//
	// fading in or fading out
	//
	switch (gpRixPlayer->FadeType)
	{
	case RIXPLAYER::FADE_IN:
		if (t >= gpRixPlayer->dwStartFadeTime + gpRixPlayer->dwFadeLength)
		{
			gpRixPlayer->FadeType = RIXPLAYER::NONE;
		}
		else
		{
			volume = (INT)(volume * (t - gpRixPlayer->dwStartFadeTime) / (FLOAT)gpRixPlayer->dwFadeLength);
		}
		break;
	case RIXPLAYER::FADE_OUT:
		if (gpRixPlayer->iCurrentMusic == -1 || t >= gpRixPlayer->dwStartFadeTime + gpRixPlayer->dwFadeLength)
		{
			//
			// There is no current playing music, or fading time has passed.
			// Start playing the next one.
			//
			if (gpRixPlayer->iNextMusic > 0)
			{
				gpRixPlayer->iCurrentMusic = gpRixPlayer->iNextMusic;
				gpRixPlayer->fLoop = gpRixPlayer->fNextLoop;
				gpRixPlayer->FadeType = RIXPLAYER::FADE_IN;
				gpRixPlayer->dwStartFadeTime = t;
				gpRixPlayer->opl->init();
				gpRixPlayer->rix->rewind(gpRixPlayer->iCurrentMusic);
				if (gpRixPlayer->resampler[0]) resampler_clear(gpRixPlayer->resampler[0]);
				if (gpRixPlayer->resampler[1]) resampler_clear(gpRixPlayer->resampler[1]);
			}
			else
			{
				gpRixPlayer->iCurrentMusic = -1;
				gpRixPlayer->FadeType = RIXPLAYER::NONE;
			}
			return;
		}
		else
		{
			volume = (INT)(volume * (1.0f - (t - gpRixPlayer->dwStartFadeTime) / (FLOAT)gpRixPlayer->dwFadeLength));
		}
		break;
	default:
		if (gpRixPlayer->iCurrentMusic <= 0)
		{
			//
			// No current playing music
			//
			return;
		}
	}

	//
	// Fill the buffer with sound data
	//
	while (len > 0)
	{
		if (gpRixPlayer->pos == NULL ||
			gpRixPlayer->pos - gpRixPlayer->buf >= (int)sizeof(gpRixPlayer->buf))
		{
			gpRixPlayer->pos = gpRixPlayer->buf;
			if (!gpRixPlayer->rix->update())
			{
				if (!gpRixPlayer->fLoop)
				{
					//
					// Not loop, simply terminate the music
					//
					gpRixPlayer->iCurrentMusic = -1;
					gpRixPlayer->FadeType = RIXPLAYER::NONE;
					return;
				}
				gpRixPlayer->rix->rewind(gpRixPlayer->iCurrentMusic);
				if (!gpRixPlayer->rix->update())
				{
					//
					// Something must be wrong
					//
					gpRixPlayer->iCurrentMusic = -1;
					gpRixPlayer->FadeType = RIXPLAYER::NONE;
					return;
				}
			}
			int sample_count = PAL_SAMPLE_RATE / 70;
			if (gpRixPlayer->resampler[0])
			{
				unsigned int samples_written = 0;
				short tempBuf[64 * PAL_CHANNELS]; // hard code on resampler defination
				short *finalBuf = (short*)gpRixPlayer->buf;

				while (sample_count)
				{
					int to_write = resampler_get_free_count(gpRixPlayer->resampler[0]);
					if (to_write)
					{
						gpRixPlayer->opl->update(tempBuf, to_write);
						for (int i = 0; i < to_write; i++)
						{
							resampler_write_sample(gpRixPlayer->resampler[0], tempBuf[i * 2]);
							resampler_write_sample(gpRixPlayer->resampler[1], tempBuf[i * 2 + 1]);
						}
					}

					finalBuf[samples_written++] = resampler_get_and_remove_sample(gpRixPlayer->resampler[0]);
					finalBuf[samples_written++] = resampler_get_and_remove_sample(gpRixPlayer->resampler[1]);
					--sample_count;
				}
			}
			else
			{
				gpRixPlayer->opl->update((short *)(gpRixPlayer->buf), sample_count);
			}
		}

		l = sizeof(gpRixPlayer->buf) - (gpRixPlayer->pos - gpRixPlayer->buf);
		if (len < l)
		{
			l = len;
		}

		//
		// Put audio data into buffer and adjust volume
		// WARNING: for signed 16-bit little-endian only
		//
		for (i = 0; i < (int)(l / sizeof(SHORT)); i++)
		{
			SHORT s = SWAP16((int)(*(SHORT *)(gpRixPlayer->pos)) * volume / SDL_MIX_MAXVOLUME);

			*(SHORT *)(stream) = s;
			stream += sizeof(SHORT);

			gpRixPlayer->pos += sizeof(SHORT);
		}

		len -= l;
	}
}

INT
RIX_Init(
   LPCSTR     szFileName
)
/*++
  Purpose:

    Initialize the RIX player subsystem.

  Parameters:

    [IN]  szFileName - Filename of the mus.mkf file.

  Return value:

    0 if success, -1 if cannot allocate memory, -2 if file not found.

--*/
{
   gpRixPlayer = new RIXPLAYER;
   if (gpRixPlayer == NULL)
   {
      return -1;
   }

#if USE_DEMUOPL
   typedef CDemuopl COpl;
#else
   typedef CEmuopl  COpl;
#endif

#if USE_SURROUNDOPL
   gpRixPlayer->opl = new CSurroundopl(new COpl(OPL_SAMPLERATE, true, false),
                                       new COpl(OPL_SAMPLERATE, true, false), true);
#  if USE_RESAMPLER
   resampler_init();
   gpRixPlayer->resampler[0] = resampler_create();
   gpRixPlayer->resampler[1] = resampler_create();

   resampler_set_quality( gpRixPlayer->resampler[0], RESAMPLER_QUALITY_MAX );
   resampler_set_quality( gpRixPlayer->resampler[1], RESAMPLER_QUALITY_MAX );

   resampler_set_rate( gpRixPlayer->resampler[0], OPL_SAMPLERATE / (double)PAL_SAMPLE_RATE );
   resampler_set_rate( gpRixPlayer->resampler[1], OPL_SAMPLERATE / (double)PAL_SAMPLE_RATE );
#  else
   gpRixPlayer->resampler[0] = NULL;
   gpRixPlayer->resampler[1] = NULL;
#  endif
#else
   gpRixPlayer->opl = new COpl(OPL_SAMPLERATE, true, PAL_CHANNELS == 2);
#endif

   if (gpRixPlayer->opl == NULL)
   {
      delete gpRixPlayer;
      return -1;
   }

   gpRixPlayer->rix = new CrixPlayer(gpRixPlayer->opl);
   if (gpRixPlayer->rix == NULL)
   {
      delete gpRixPlayer->opl;
      delete gpRixPlayer;
      return -1;
   }

   //
   // Load the MKF file.
   //
   if (!gpRixPlayer->rix->load(szFileName, CProvider_Filesystem()))
   {
      delete gpRixPlayer->rix;
      delete gpRixPlayer->opl;
      delete gpRixPlayer;
      gpRixPlayer = NULL;
      return -2;
   }

   //
   // Success.
   //
   gpRixPlayer->FadeType = RIXPLAYER::NONE;
   gpRixPlayer->iCurrentMusic = -1;
   gpRixPlayer->pos = NULL;
   gpRixPlayer->fLoop = FALSE;
   gpRixPlayer->fNextLoop = FALSE;

   return 0;
}

VOID
RIX_Shutdown(
   VOID
)
/*++
  Purpose:

    Shutdown the RIX player subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   if (gpRixPlayer != NULL)
   {
      delete gpRixPlayer->rix;
      delete gpRixPlayer->opl;
      delete gpRixPlayer;

      gpRixPlayer = NULL;
   }
}

VOID
RIX_Play(
   INT       iNumRIX,
   BOOL      fLoop,
   FLOAT     flFadeTime
)
/*++
  Purpose:

    Start playing the specified music.

  Parameters:

    [IN]  iNumRIX - number of the music. 0 to stop playing current music.

    [IN]  fLoop - Whether the music should be looped or not.

    [IN]  flFadeTime - the fade in/out time when switching music.

  Return value:

    None.

--*/
{
   //
   // Check for NULL pointer.
   //
   if (gpRixPlayer == NULL)
   {
      return;
   }

   //
   // Stop the current CD music.
   //
   SOUND_PlayCDA(-1);
    
   DWORD t = SDL_GetTicks();
   gpRixPlayer->fNextLoop = fLoop;

   if (iNumRIX == gpRixPlayer->iCurrentMusic && !g_fNoMusic)
   {
      return;
   }

   gpRixPlayer->iNextMusic = iNumRIX;
   gpRixPlayer->dwStartFadeTime = t;
   gpRixPlayer->dwFadeLength = (DWORD)(flFadeTime * 1000) / 2;
   gpRixPlayer->FadeType = RIXPLAYER::FADE_OUT;
}
