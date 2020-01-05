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

#include <math.h>
#include "global.h"
#include "palcfg.h"
#include "players.h"
#include "audio.h"

#include "resampler.h"
#include "adplug/opl.h"
#include "adplug/emuopls.h"
#include "adplug/surroundopl.h"
#include "adplug/convertopl.h"
#include "adplug/rix.h"

typedef struct tagRIXPLAYER :
	public AUDIOPLAYER
{
   Copl                      *opl;
   CrixPlayer                *rix;
   void                      *resampler[2];
   BYTE                       buf[(PAL_MAX_SAMPLERATE + 69) / 70 * sizeof(short) * 2];
   LPBYTE                     pos;
   INT                        iNextMusic; // the next music number to switch to
   DWORD                      dwStartFadeTime;
   INT                        iTotalFadeOutSamples;
   INT                        iTotalFadeInSamples;
   INT                        iRemainingFadeSamples;
   enum { NONE, FADE_IN, FADE_OUT } FadeType; // fade in or fade out ?
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
	callback function only (audio.c: AUDIO_FillBuffer).

	Parameters:

	[OUT] stream - pointer to the stream buffer.

	[IN]  len - Length of the buffer.

	Return value:

	None.

--*/
{
	LPRIXPLAYER pRixPlayer = (LPRIXPLAYER)object;

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
				volume = SDL_MIX_MAXVOLUME;
			}
			else
			{
				volume = (INT)(SDL_MIX_MAXVOLUME * (1.0 - (double)pRixPlayer->iRemainingFadeSamples / pRixPlayer->iTotalFadeInSamples));
				delta_samples = (pRixPlayer->iTotalFadeInSamples / SDL_MIX_MAXVOLUME) & ~(gConfig.iAudioChannels - 1); vol_delta = 1;
			}
			break;
		case RIXPLAYER::FADE_OUT:
			if (pRixPlayer->iTotalFadeOutSamples == pRixPlayer->iRemainingFadeSamples && pRixPlayer->iTotalFadeOutSamples > 0)
			{
				UINT  now = SDL_GetTicks();
				INT   passed_samples = ((INT)(now - pRixPlayer->dwStartFadeTime) > 0) ? (INT)((now - pRixPlayer->dwStartFadeTime) * AUDIO_GetDeviceSpec()->freq / 1000) : 0;
				pRixPlayer->iRemainingFadeSamples -= passed_samples;
			}
			if (pRixPlayer->iMusic == -1 || pRixPlayer->iRemainingFadeSamples <= 0)
			{
				//
				// There is no current playing music, or fading time has passed.
				// Start playing the next one or stop playing.
				//
				if (pRixPlayer->iNextMusic > 0)
				{
					pRixPlayer->iMusic = pRixPlayer->iNextMusic;
					pRixPlayer->iNextMusic = -1;
					pRixPlayer->fLoop = pRixPlayer->fNextLoop;
					pRixPlayer->FadeType = RIXPLAYER::FADE_IN;
					if (pRixPlayer->iMusic > 0)
						pRixPlayer->dwStartFadeTime += pRixPlayer->iTotalFadeOutSamples * 1000 / gConfig.iSampleRate;
					else
						pRixPlayer->dwStartFadeTime = SDL_GetTicks();
					pRixPlayer->iTotalFadeOutSamples = 0;
					pRixPlayer->iRemainingFadeSamples = pRixPlayer->iTotalFadeInSamples;
					pRixPlayer->rix->rewind(pRixPlayer->iMusic);
					if (pRixPlayer->resampler[0]) resampler_clear(pRixPlayer->resampler[0]);
					if (pRixPlayer->resampler[1]) resampler_clear(pRixPlayer->resampler[1]);
					continue;
				}
				else
				{
					pRixPlayer->iMusic = -1;
					pRixPlayer->FadeType = RIXPLAYER::NONE;
					return;
				}
			}
			else
			{
				volume = (INT)(SDL_MIX_MAXVOLUME * ((double)pRixPlayer->iRemainingFadeSamples / pRixPlayer->iTotalFadeOutSamples));
				delta_samples = (pRixPlayer->iTotalFadeOutSamples / SDL_MIX_MAXVOLUME) & ~(gConfig.iAudioChannels - 1); vol_delta = -1;
			}
			break;
		default:
			if (pRixPlayer->iMusic <= 0)
			{
				//
				// No current playing music
				//
				return;
			}
			else
			{
				volume = SDL_MIX_MAXVOLUME;
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
						pRixPlayer->iMusic = -1;
						if (pRixPlayer->FadeType != RIXPLAYER::FADE_OUT && pRixPlayer->iNextMusic == -1)
						{
							pRixPlayer->FadeType = RIXPLAYER::NONE;
						}
						return;
					}
					pRixPlayer->rix->rewindReInit(pRixPlayer->iMusic, false);
					if (!pRixPlayer->rix->update())
					{
						//
						// Something must be wrong
						//
						pRixPlayer->iMusic = -1;
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
			if (pRixPlayer->FadeType != RIXPLAYER::NONE)
			{
				short* ptr = (short*)stream;
				for (int i = 0; i < l && pRixPlayer->iRemainingFadeSamples > 0; volume += vol_delta)
				{
					int j = 0;
					for (j = 0; i < l && j < delta_samples; i++, j++)
					{
						*ptr++ = *(short*)pRixPlayer->pos * volume / SDL_MIX_MAXVOLUME;
						pRixPlayer->pos += sizeof(short);
					}
					pRixPlayer->iRemainingFadeSamples -= j;
				}
				fContinue = (pRixPlayer->iRemainingFadeSamples > 0);
				len -= (LPBYTE)ptr - stream; stream = (LPBYTE)ptr;
			}
			else
			{
				memcpy(stream, pRixPlayer->pos, l * sizeof(short));
				pRixPlayer->pos += l * sizeof(short);
				stream += l * sizeof(short);
				len -= l * sizeof(short);
			}
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

	if (iNumRIX == pRixPlayer->iMusic && pRixPlayer->iNextMusic == -1)
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

LPAUDIOPLAYER
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
	if (!szFileName) return NULL;

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

	auto chip = (Copl::ChipType)gConfig.eOPLChip;
	if (chip == Copl::TYPE_OPL2 && gConfig.fUseSurroundOPL)
	{
		chip = Copl::TYPE_DUAL_OPL2;
	}

	Copl* opl = CEmuopl::CreateEmuopl((OPLCORE::TYPE)gConfig.eOPLCore, chip, gConfig.iOPLSampleRate);
	if (NULL == opl)
	{
		delete pRixPlayer;
		return NULL;
	}

	if (gConfig.fUseSurroundOPL)
	{
		Copl* tmpopl = new CSurroundopl(gConfig.iOPLSampleRate, gConfig.iSurroundOPLOffset, opl);
		if (NULL == tmpopl)
		{
			delete opl;
			delete pRixPlayer;
			return NULL;
		}
		opl = tmpopl;
	}

	pRixPlayer->opl = new CConvertopl(opl, true, gConfig.iAudioChannels == 2);
	if (pRixPlayer->opl == NULL)
	{
		delete opl;
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
	pRixPlayer->iMusic = pRixPlayer->iNextMusic = -1;
	pRixPlayer->pos = NULL;
	pRixPlayer->fLoop = FALSE;
	pRixPlayer->fNextLoop = FALSE;
	pRixPlayer->fReady = FALSE;

	return pRixPlayer;
}
