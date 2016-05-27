/* -*- mode: c++; tab-width: 4; c-basic-offset: 3; c-file-style: "linux" -*- */
//
// Copyright (c) 2008, Wei Mingzhi <whistler_wmz@users.sf.net>.
// All rights reserved.
// Modified by Lou Yihua <louyihua@21cn.com>, 2015.
//
// This file is part of SDLPAL.
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

#include <math.h>
#include "global.h"
#include "palcfg.h"
#include "players.h"
#include "sound.h"

#include "resampler.h"
#include "adplug/opl.h"
#include "adplug/demuopl.h"
#include "adplug/dbemuopl.h"
#include "adplug/emuopl.h"
#include "adplug/surroundopl.h"
#include "adplug/rix.h"

typedef struct tagRIXPLAYER :
	public MUSICPLAYER
{
   Copl                      *opl;
   CrixPlayer                *rix;
   void                      *resampler[2];
   BYTE                       buf[(PAL_MAX_SAMPLERATE + 69) / 70 * sizeof(short) * 2];
   LPBYTE                     pos;
   INT                        iCurrentMusic; // current playing music number
   INT                        iNextMusic; // the next music number to switch to
   DWORD                      dwStartFadeTime;
   INT                        iTotalFadeOutSamples;
   INT                        iTotalFadeInSamples;
   INT                        iRemainingFadeSamples;
   enum { NONE, FADE_IN, FADE_OUT } FadeType; // fade in or fade out ?
   BOOL                       fLoop;
   BOOL                       fNextLoop;
   BOOL                       fReady;
} RIXPLAYER, *LPRIXPLAYER;

static VOID
RIX_FillBuffer(
	VOID      *object,
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
	LPRIXPLAYER pRixPlayer = (LPRIXPLAYER)object;
	static const int max_volume = PAL_MAX_VOLUME;

	if (pRixPlayer == NULL || !pRixPlayer->fReady)
	{
		//
		// Not initialized
		//
		return;
	}

	while (len > 0)
	{
		INT       volume, delta_samples = 0, vol_delta = 0;

		//
		// fading in or fading out
		//
		switch (pRixPlayer->FadeType)
		{
		case RIXPLAYER::FADE_IN:
			if (pRixPlayer->iRemainingFadeSamples <= 0)
			{
				pRixPlayer->FadeType = RIXPLAYER::NONE;
				volume = max_volume;
			}
			else
			{
				volume = (INT)(max_volume * (1.0 - (double)pRixPlayer->iRemainingFadeSamples / pRixPlayer->iTotalFadeInSamples));
				delta_samples = pRixPlayer->iTotalFadeInSamples / max_volume; vol_delta = 1;
			}
			break;
		case RIXPLAYER::FADE_OUT:
			if (pRixPlayer->iTotalFadeOutSamples == pRixPlayer->iRemainingFadeSamples && pRixPlayer->iTotalFadeOutSamples > 0)
			{
				UINT  now = SDL_GetTicks();
				INT   passed_samples = ((INT)(now - pRixPlayer->dwStartFadeTime) > 0) ? (INT)((now - pRixPlayer->dwStartFadeTime) * AUDIO_GetDeviceSpec()->freq / 1000) : 0;
				pRixPlayer->iRemainingFadeSamples -= passed_samples;
			}
			if (pRixPlayer->iCurrentMusic == -1 || pRixPlayer->iRemainingFadeSamples <= 0)
			{
				//
				// There is no current playing music, or fading time has passed.
				// Start playing the next one or stop playing.
				//
				if (pRixPlayer->iNextMusic > 0)
				{
					pRixPlayer->iCurrentMusic = pRixPlayer->iNextMusic;
					pRixPlayer->iNextMusic = -1;
					pRixPlayer->fLoop = pRixPlayer->fNextLoop;
					pRixPlayer->FadeType = RIXPLAYER::FADE_IN;
					if (pRixPlayer->iCurrentMusic > 0)
						pRixPlayer->dwStartFadeTime += pRixPlayer->iTotalFadeOutSamples * 1000 / gConfig.iSampleRate;
					else
						pRixPlayer->dwStartFadeTime = SDL_GetTicks();
					pRixPlayer->iTotalFadeOutSamples = 0;
					pRixPlayer->iRemainingFadeSamples = pRixPlayer->iTotalFadeInSamples;
					pRixPlayer->rix->rewind(pRixPlayer->iCurrentMusic);
					if (pRixPlayer->resampler[0]) resampler_clear(pRixPlayer->resampler[0]);
					if (pRixPlayer->resampler[1]) resampler_clear(pRixPlayer->resampler[1]);
					continue;
				}
				else
				{
					pRixPlayer->iCurrentMusic = -1;
					pRixPlayer->FadeType = RIXPLAYER::NONE;
					return;
				}
			}
			else
			{
				volume = (INT)(max_volume * ((double)pRixPlayer->iRemainingFadeSamples / pRixPlayer->iTotalFadeOutSamples));
				delta_samples = pRixPlayer->iTotalFadeOutSamples / max_volume; vol_delta = -1;
			}
			break;
		default:
			if (pRixPlayer->iCurrentMusic <= 0)
			{
				//
				// No current playing music
				//
				return;
			}
			else
			{
				volume = max_volume;
			}
		}

		//
		// Fill the buffer with sound data
		//
		int buf_max_len = gConfig.iSampleRate / 70 * gConfig.iAudioChannels * sizeof(short);
		bool fContinue = true;
		while (len > 0 && fContinue)
		{
			if (pRixPlayer->pos == NULL || pRixPlayer->pos - pRixPlayer->buf >= buf_max_len)
			{
				pRixPlayer->pos = pRixPlayer->buf;
				if (!pRixPlayer->rix->update())
				{
					if (!pRixPlayer->fLoop)
					{
						//
						// Not loop, simply terminate the music
						//
						pRixPlayer->iCurrentMusic = -1;
						if (pRixPlayer->FadeType != RIXPLAYER::FADE_OUT && pRixPlayer->iNextMusic == -1)
						{
							pRixPlayer->FadeType = RIXPLAYER::NONE;
						}
						return;
					}
					pRixPlayer->rix->rewind(pRixPlayer->iCurrentMusic, false);
					if (!pRixPlayer->rix->update())
					{
						//
						// Something must be wrong
						//
						pRixPlayer->iCurrentMusic = -1;
						pRixPlayer->FadeType = RIXPLAYER::NONE;
						return;
					}
				}
				int sample_count = gConfig.iSampleRate / 70;
				if (pRixPlayer->resampler[0])
				{
					unsigned int samples_written = 0;
					short *finalBuf = (short*)pRixPlayer->buf;

					while (sample_count)
					{
						int to_write = resampler_get_free_count(pRixPlayer->resampler[0]);
						if (to_write)
						{
							short *tempBuf = (short*)alloca(to_write * gConfig.iAudioChannels * sizeof(short));
							int temp_buf_read = 0;
							pRixPlayer->opl->update(tempBuf, to_write);
							for (int i = 0; i < to_write * gConfig.iAudioChannels; i++)
								resampler_write_sample(pRixPlayer->resampler[i % gConfig.iAudioChannels], tempBuf[temp_buf_read++]);
						}

						int to_get = resampler_get_sample_count(pRixPlayer->resampler[0]);
						if (to_get > sample_count) to_get = sample_count;
						for (int i = 0; i < to_get * gConfig.iAudioChannels; i++)
							finalBuf[samples_written++] = resampler_get_and_remove_sample(pRixPlayer->resampler[i % gConfig.iAudioChannels]);
						sample_count -= to_get;
					}
				}
				else
				{
					pRixPlayer->opl->update((short *)(pRixPlayer->buf), sample_count);
				}
			}

			int l = buf_max_len - (pRixPlayer->pos - pRixPlayer->buf);
			l = (l > len) ? len / sizeof(short) : l / sizeof(short);

			//
			// Put audio data into buffer and adjust volume
			//
			SHORT* ptr = (SHORT*)stream;
			if (pRixPlayer->FadeType == RIXPLAYER::NONE)
			{
				for (int i = 0; i < l; i++)
				{
					*ptr++ = *(SHORT *)pRixPlayer->pos * volume / max_volume;
					pRixPlayer->pos += sizeof(SHORT);
				}
			}
			else
			{
				for (int i = 0; i < l && pRixPlayer->iRemainingFadeSamples > 0; volume += vol_delta)
				{
					int j = 0;
					for (j = 0; i < l && j < delta_samples; i++, j++)
					{
						*ptr++ = *(SHORT *)pRixPlayer->pos * volume / max_volume;
						pRixPlayer->pos += sizeof(SHORT);
					}
					pRixPlayer->iRemainingFadeSamples -= j;
				}
				fContinue = (pRixPlayer->iRemainingFadeSamples > 0);
			}
			len -= (LPBYTE)ptr - stream;
			stream = (LPBYTE)ptr;
		}
	}
}

static VOID
RIX_Shutdown(
	VOID     *object
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
	if (object != NULL)
	{
		LPRIXPLAYER pRixPlayer = (LPRIXPLAYER)object;
		pRixPlayer->fReady = FALSE;
		for (int i = 0; i < gConfig.iAudioChannels; i++)
			if (pRixPlayer->resampler[i])
				resampler_delete(pRixPlayer->resampler[i]);
		delete pRixPlayer->rix;
		delete pRixPlayer->opl;
		delete pRixPlayer;
	}
}

static BOOL
RIX_Play(
	VOID     *object,
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
	LPRIXPLAYER pRixPlayer = (LPRIXPLAYER)object;

	//
	// Check for NULL pointer.
	//
	if (pRixPlayer == NULL)
	{
		return FALSE;
	}

	//
	// Stop the current CD music.
	//
	AUDIO_PlayCDTrack(-1);

	if (iNumRIX == pRixPlayer->iCurrentMusic && pRixPlayer->iNextMusic == -1)
	{
		/* Will play the same music without any pending play changes,
		   just change the loop attribute */
		pRixPlayer->fLoop = fLoop;
		return TRUE;
	}

	if (pRixPlayer->FadeType != RIXPLAYER::FADE_OUT)
	{
		if (pRixPlayer->FadeType == RIXPLAYER::FADE_IN && pRixPlayer->iTotalFadeInSamples > 0 && pRixPlayer->iRemainingFadeSamples > 0)
		{
			pRixPlayer->dwStartFadeTime = SDL_GetTicks() - (int)((float)pRixPlayer->iRemainingFadeSamples / pRixPlayer->iTotalFadeInSamples * flFadeTime * (1000 / 2));
		}
		else
		{
			pRixPlayer->dwStartFadeTime = SDL_GetTicks();
		}
		pRixPlayer->iTotalFadeOutSamples = (int)round(flFadeTime / 2.0f * gConfig.iSampleRate) * gConfig.iAudioChannels;
		pRixPlayer->iRemainingFadeSamples = pRixPlayer->iTotalFadeOutSamples;
		pRixPlayer->iTotalFadeInSamples = pRixPlayer->iTotalFadeOutSamples;
	}
	else
	{
		pRixPlayer->iTotalFadeInSamples = (int)round(flFadeTime / 2.0f * gConfig.iSampleRate) * gConfig.iAudioChannels;
	}

	pRixPlayer->iNextMusic = iNumRIX;
	pRixPlayer->FadeType = RIXPLAYER::FADE_OUT;
	pRixPlayer->fNextLoop = fLoop;
	pRixPlayer->fReady = TRUE;

	return TRUE;
}

LPMUSICPLAYER
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
	LPRIXPLAYER pRixPlayer = new RIXPLAYER;
	if (pRixPlayer == NULL)
	{
		return NULL;
	}
	else
	{
		memset(pRixPlayer, 0, sizeof(RIXPLAYER));
		pRixPlayer->FillBuffer = RIX_FillBuffer;
		pRixPlayer->Shutdown = RIX_Shutdown;
		pRixPlayer->Play = RIX_Play;
	}

	if (gConfig.fUseSurroundOPL)
	{
		switch (gConfig.eOPLType)
		{
		case OPL_DOSBOX:
			pRixPlayer->opl = new CSurroundopl(
				new CDemuopl(gConfig.iOPLSampleRate, true, false),
				new CDemuopl(gConfig.iOPLSampleRate, true, false),
				true, gConfig.iOPLSampleRate, gConfig.iSurroundOPLOffset);
			break;
		case OPL_DOSBOX_NEW:
			pRixPlayer->opl = new CSurroundopl(
				new CDBemuopl(gConfig.iOPLSampleRate, true, false),
				new CDBemuopl(gConfig.iOPLSampleRate, true, false),
				true, gConfig.iOPLSampleRate, gConfig.iSurroundOPLOffset);
			break;
		case OPL_MAME:
			pRixPlayer->opl = new CSurroundopl(
				new CEmuopl(gConfig.iOPLSampleRate, true, false),
				new CEmuopl(gConfig.iOPLSampleRate, true, false),
				true, gConfig.iOPLSampleRate, gConfig.iSurroundOPLOffset);
			break;
		}
	}
	else
	{
		switch (gConfig.eOPLType)
		{
		case OPL_DOSBOX:
			pRixPlayer->opl = new CDemuopl(gConfig.iOPLSampleRate, true, gConfig.iAudioChannels == 2);
			break;
		case OPL_DOSBOX_NEW:
			pRixPlayer->opl = new CDBemuopl(gConfig.iOPLSampleRate, true, gConfig.iAudioChannels == 2);
			break;
		case OPL_MAME:
			pRixPlayer->opl = new CEmuopl(gConfig.iOPLSampleRate, true, gConfig.iAudioChannels == 2);
			break;
		}
	}

	if (pRixPlayer->opl == NULL)
	{
		delete pRixPlayer;
		return NULL;
	}

	pRixPlayer->rix = new CrixPlayer(pRixPlayer->opl);
	if (pRixPlayer->rix == NULL)
	{
		delete pRixPlayer->opl;
		delete pRixPlayer;
		return NULL;
	}

	//
	// Load the MKF file.
	//
	if (!pRixPlayer->rix->load(szFileName, CProvider_Filesystem()))
	{
		delete pRixPlayer->rix;
		delete pRixPlayer->opl;
		delete pRixPlayer;
		pRixPlayer = NULL;
		return NULL;
	}

	if (gConfig.iOPLSampleRate != gConfig.iSampleRate)
	{
		for (int i = 0; i < gConfig.iAudioChannels; i++)
		{
			pRixPlayer->resampler[i] = resampler_create();
			resampler_set_quality(pRixPlayer->resampler[i], AUDIO_IsIntegerConversion(gConfig.iOPLSampleRate) ? RESAMPLER_QUALITY_MIN : gConfig.iResampleQuality);
			resampler_set_rate(pRixPlayer->resampler[i], (double)gConfig.iOPLSampleRate / (double)gConfig.iSampleRate);
		}
	}

#if USE_RIX_EXTRA_INIT
	if (gConfig.pExtraFMRegs && gConfig.pExtraFMVals)
	{
		pRixPlayer->rix->set_extra_init(gConfig.pExtraFMRegs, gConfig.pExtraFMVals, gConfig.dwExtraLength);
	}
#endif

	//
	// Success.
	//
	pRixPlayer->FadeType = RIXPLAYER::NONE;
	pRixPlayer->iCurrentMusic = pRixPlayer->iNextMusic = -1;
	pRixPlayer->pos = NULL;
	pRixPlayer->fLoop = FALSE;
	pRixPlayer->fNextLoop = FALSE;
	pRixPlayer->fReady = FALSE;

	return pRixPlayer;
}
