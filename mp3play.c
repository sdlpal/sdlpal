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

#include "util.h"
#include "global.h"
#include "palcfg.h"
#include "audio.h"
#include "players.h"
#include "resampler.h"

#if PAL_HAS_MP3

#define DR_MP3_IMPLEMENTATION
#define DR_MP3_NO_STDIO
#include "3rd/dr_mp3.h"

typedef struct tagMP3PLAYER
{
	AUDIOPLAYER_COMMONS;

	SDL_RWops       *rw;
	drmp3            mp3;
	short            sBuffer[4096];
	int              iBufPos;
	int              iBufLen;
	int              nChannels;
	long             lSampleRate;
	void            *resampler[2];
	BOOL             fReady;
	BOOL             fRewind;
	BOOL             fUseResampler;
	BOOL             fMp3Opened;
} MP3PLAYER, *LPMP3PLAYER;

static size_t
mp3_read_cb(void *pUserData, void *pBufferOut, size_t bytesToRead)
{
	return SDL_RWread((SDL_RWops *)pUserData, pBufferOut, 1, bytesToRead);
}

static drmp3_bool32
mp3_seek_cb(void *pUserData, int offset, drmp3_seek_origin origin)
{
	int whence;
	switch (origin) {
	case DRMP3_SEEK_SET: whence = RW_SEEK_SET; break;
	case DRMP3_SEEK_CUR: whence = RW_SEEK_CUR; break;
	default: return DRMP3_FALSE;
	}
	return SDL_RWseek((SDL_RWops *)pUserData, offset, whence) >= 0 ? DRMP3_TRUE : DRMP3_FALSE;
}

static VOID
MP3_Close(LPMP3PLAYER player)
{
	int i;
	for (i = 0; i < 2; i++)
	{
		if (player->resampler[i])
			resampler_clear(player->resampler[i]);
	}
	if (player->fMp3Opened)
	{
		drmp3_uninit(&player->mp3);
		player->fMp3Opened = FALSE;
	}
	player->iBufPos = player->iBufLen = 0;
	player->fReady = FALSE;
	player->fRewind = FALSE;
}

static BOOL
MP3_Open(LPMP3PLAYER player)
{
	if (player->fReady)
		MP3_Close(player);

	SDL_RWseek(player->rw, 0, RW_SEEK_SET);

	if (!drmp3_init(&player->mp3, mp3_read_cb, mp3_seek_cb, NULL, NULL, player->rw, NULL))
		return FALSE;

	player->fMp3Opened = TRUE;
	player->nChannels = (int)player->mp3.channels;
	player->lSampleRate = (long)player->mp3.sampleRate;
	player->iBufPos = player->iBufLen = 0;
	player->fRewind = FALSE;
	player->fUseResampler = (player->lSampleRate != (long)gConfig.iSampleRate);

	if (player->fUseResampler)
	{
		int quality = AUDIO_IsIntegerConversion(player->lSampleRate) ? RESAMPLER_QUALITY_MIN : RESAMPLER_QUALITY_MAX;
		int i;
		for (i = 0; i < 2; i++)
		{
			resampler_set_quality(player->resampler[i], quality);
			resampler_set_rate(player->resampler[i], (double)player->lSampleRate / (double)gConfig.iSampleRate);
			resampler_clear(player->resampler[i]);
		}
	}

	player->fReady = TRUE;
	return TRUE;
}

static VOID
MP3_FillBuffer(
	VOID       *object,
	LPBYTE      buf,
	INT         len
)
{
	LPMP3PLAYER player = (LPMP3PLAYER)object;

	if (!player->fReady) return;

	while (len > 0)
	{
		if (player->fRewind)
		{
			/* Reopen decoder for clean rewind */
			drmp3_uninit(&player->mp3);
			player->fMp3Opened = FALSE;
			SDL_RWseek(player->rw, 0, RW_SEEK_SET);
			if (!drmp3_init(&player->mp3, mp3_read_cb, mp3_seek_cb, NULL, NULL, player->rw, NULL))
			{
				player->fReady = FALSE;
				return;
			}
			player->fMp3Opened = TRUE;
			player->iBufPos = player->iBufLen = 0;
			player->fRewind = FALSE;
			if (player->fUseResampler)
			{
				resampler_clear(player->resampler[0]);
				resampler_clear(player->resampler[1]);
			}
		}

		if (player->iBufPos >= player->iBufLen)
		{
			drmp3_uint64 framesRead;
			int maxFrames = (int)(sizeof(player->sBuffer) / sizeof(short)) / player->nChannels;

			framesRead = drmp3_read_pcm_frames_s16(&player->mp3, (drmp3_uint64)maxFrames, player->sBuffer);
			if (framesRead == 0)
			{
				if (player->fLoop)
				{
					player->fRewind = TRUE;
					continue;
				}
				else
				{
					player->fReady = FALSE;
					return;
				}
			}
			player->iBufLen = (int)(framesRead * player->nChannels);
			player->iBufPos = 0;
		}

		if (player->fUseResampler)
		{
			while (player->iBufPos < player->iBufLen)
			{
				short sL, sR;
				int to_write = resampler_get_free_count(player->resampler[0]);
				if (to_write <= 0) break;
				if (player->nChannels >= 2)
				{
					sL = player->sBuffer[player->iBufPos++];
					sR = player->sBuffer[player->iBufPos++];
					player->iBufPos += (player->nChannels - 2);
				}
				else
				{
					sL = sR = player->sBuffer[player->iBufPos++];
				}
				resampler_write_sample(player->resampler[0], sL);
				resampler_write_sample(player->resampler[1], sR);
			}

			{
				int resampler_count = resampler_get_sample_count(player->resampler[0]);
				int frame_size = gConfig.iAudioChannels * sizeof(short);
				int frames_needed = len / frame_size;
				int frames = (resampler_count < frames_needed) ? resampler_count : frames_needed;
				short *dst = (short *)buf;
				int i;
				for (i = 0; i < frames; i++)
				{
					if (gConfig.iAudioChannels > 1)
					{
						*dst++ = (short)resampler_get_and_remove_sample(player->resampler[0]);
						*dst++ = (short)resampler_get_and_remove_sample(player->resampler[1]);
					}
					else
					{
						int l = resampler_get_and_remove_sample(player->resampler[0]);
						int r = resampler_get_and_remove_sample(player->resampler[1]);
						*dst++ = (short)((l + r) / 2);
					}
				}
				len -= frames * frame_size;
				buf += frames * frame_size;
			}
		}
		else
		{
			int frames_avail = (player->iBufLen - player->iBufPos) / player->nChannels;
			int frame_size = gConfig.iAudioChannels * sizeof(short);
			int frames_needed = (int)((DWORD)len / frame_size);
			int frames = (frames_avail < frames_needed) ? frames_avail : frames_needed;
			short *dst = (short *)buf;
			int i;

			for (i = 0; i < frames; i++)
			{
				short sL, sR;
				if (player->nChannels >= 2)
				{
					sL = player->sBuffer[player->iBufPos++];
					sR = player->sBuffer[player->iBufPos++];
					player->iBufPos += (player->nChannels - 2);
				}
				else
				{
					sL = sR = player->sBuffer[player->iBufPos++];
				}
				if (gConfig.iAudioChannels > 1)
				{
					*dst++ = sL;
					*dst++ = sR;
				}
				else
				{
					*dst++ = (short)((sL + sR) / 2);
				}
			}
			len -= frames * frame_size;
			buf += frames * frame_size;
		}
	}
}

static BOOL
MP3_Play(
	VOID       *object,
	INT         iNum,
	BOOL        fLoop,
	FLOAT       flFadeTime
)
{
	LPMP3PLAYER player = (LPMP3PLAYER)object;

	player->fLoop = fLoop;

	if (iNum == player->iMusic)
		return TRUE;

	MP3_Close(player);
	if (player->rw)
	{
		SDL_RWclose(player->rw);
		player->rw = NULL;
	}
	player->iMusic = iNum;

	if (iNum == -1) return TRUE;

	player->rw = SDL_RWFromFile(UTIL_GetFullPathName(PAL_BUFFER_SIZE_ARGS(0), gConfig.pszGamePath,
		PAL_va(1, "mp3%s%.2d.mp3", PAL_NATIVE_PATH_SEPARATOR, iNum)), "rb");
	if (!player->rw)
		return FALSE;

	if (!MP3_Open(player))
	{
		if (player->rw)
		{
			SDL_RWclose(player->rw);
			player->rw = NULL;
		}
		return FALSE;
	}

	return TRUE;
}

static VOID
MP3_Shutdown(
	VOID       *object
)
{
	LPMP3PLAYER player = (LPMP3PLAYER)object;
	int i;

	MP3_Close(player);

	if (player->rw)
	{
		SDL_RWclose(player->rw);
		player->rw = NULL;
	}

	for (i = 0; i < 2; i++)
	{
		if (player->resampler[i])
		{
			resampler_delete(player->resampler[i]);
			player->resampler[i] = NULL;
		}
	}

	free(player);
}

LPAUDIOPLAYER
MP3_Init(VOID)
{
	LPMP3PLAYER player;

	player = (LPMP3PLAYER)calloc(1, sizeof(MP3PLAYER));
	if (player)
	{
		player->FillBuffer = MP3_FillBuffer;
		player->Play = MP3_Play;
		player->Shutdown = MP3_Shutdown;

		player->resampler[0] = resampler_create();
		player->resampler[1] = resampler_create();
		if (!player->resampler[0] || !player->resampler[1])
		{
			MP3_Shutdown(player);
			return NULL;
		}
	}

	return (LPAUDIOPLAYER)player;
}

#else

LPAUDIOPLAYER
MP3_Init(VOID)
{
	return NULL;
}

#endif
