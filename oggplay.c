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
// oggplay.h: Player of OGG files.
//   @Author: Lou Yihua <louyihua@21cn.com>, 2015-07-28.
//            Soar Qin <soarchin@gmail.com>, 2026-04-14.
//

#include "util.h"
#include "global.h"
#include "palcfg.h"
#include "audio.h"
#include "players.h"
#include "resampler.h"

#if PAL_HAS_OGG

#include "3rd/stb_vorbis.c"

typedef struct tagOGGPLAYER
{
	AUDIOPLAYER_COMMONS;

	unsigned char   *pFileData;
	int              nFileLen;
	stb_vorbis      *vf;
	short            sBuffer[4096];
	int              iBufPos;
	int              iBufLen;
	int              nChannels;
	long             lSampleRate;
	void            *resampler[2];
	BOOL             fReady;
	BOOL             fRewind;
	BOOL             fUseResampler;
} OGGPLAYER, *LPOGGPLAYER;

static VOID
OGG_Close(LPOGGPLAYER player)
{
	int i;
	for (i = 0; i < 2; i++)
	{
		if (player->resampler[i])
			resampler_clear(player->resampler[i]);
	}
	if (player->vf)
	{
		stb_vorbis_close(player->vf);
		player->vf = NULL;
	}
	if (player->pFileData)
	{
		free(player->pFileData);
		player->pFileData = NULL;
		player->nFileLen = 0;
	}
	player->iBufPos = player->iBufLen = 0;
	player->fReady = FALSE;
	player->fRewind = FALSE;
}

static BOOL
OGG_Open(LPOGGPLAYER player)
{
	stb_vorbis_info info;
	int error;

	if (player->fReady)
		OGG_Close(player);

	if (!player->pFileData || player->nFileLen <= 0)
		return FALSE;

	player->vf = stb_vorbis_open_memory(player->pFileData, player->nFileLen, &error, NULL);
	if (!player->vf)
		return FALSE;

	info = stb_vorbis_get_info(player->vf);
	player->nChannels = info.channels;
	player->lSampleRate = (long)info.sample_rate;
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
OGG_FillBuffer(
	VOID       *object,
	LPBYTE      buf,
	INT         len
)
{
	LPOGGPLAYER player = (LPOGGPLAYER)object;

	if (!player->fReady) return;

	while (len > 0)
	{
		if (player->fRewind)
		{
			stb_vorbis_seek_start(player->vf);
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
			int samplesPerChannel = (int)(sizeof(player->sBuffer) / sizeof(short)) / player->nChannels;
			int ret = stb_vorbis_get_samples_short_interleaved(
				player->vf, player->nChannels, player->sBuffer, samplesPerChannel * player->nChannels);
			if (ret > 0)
			{
				player->iBufLen = ret * player->nChannels;
				player->iBufPos = 0;
			}
			else
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
		}

		if (player->fUseResampler)
		{
			/* Feed sBuffer samples into resampler */
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

			/* Drain resampler output into buf */
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
OGG_Play(
	VOID       *object,
	INT         iNum,
	BOOL        fLoop,
	FLOAT       flFadeTime
)
{
	LPOGGPLAYER player = (LPOGGPLAYER)object;
	SDL_RWops *rw;
	Sint64 fileSize;

	player->fLoop = fLoop;

	if (iNum == player->iMusic)
		return TRUE;

	OGG_Close(player);
	player->iMusic = iNum;

	if (iNum == -1) return TRUE;

	rw = SDL_RWFromFile(UTIL_GetFullPathName(PAL_BUFFER_SIZE_ARGS(0), gConfig.pszGamePath,
		PAL_va(1, "ogg%s%.2d.ogg", PAL_NATIVE_PATH_SEPARATOR, iNum)), "rb");
	if (!rw)
		return FALSE;

	/* Load entire file into memory for stb_vorbis */
	SDL_RWseek(rw, 0, RW_SEEK_END);
	fileSize = SDL_RWtell(rw);
	SDL_RWseek(rw, 0, RW_SEEK_SET);
	if (fileSize <= 0)
	{
		SDL_RWclose(rw);
		return FALSE;
	}

	player->pFileData = (unsigned char *)malloc((size_t)fileSize);
	if (!player->pFileData)
	{
		SDL_RWclose(rw);
		return FALSE;
	}

	player->nFileLen = (int)SDL_RWread(rw, player->pFileData, 1, (size_t)fileSize);
	SDL_RWclose(rw);

	if (player->nFileLen <= 0)
	{
		free(player->pFileData);
		player->pFileData = NULL;
		return FALSE;
	}

	if (!OGG_Open(player))
	{
		OGG_Close(player);
		return FALSE;
	}

	return TRUE;
}

static VOID
OGG_Shutdown(
	VOID       *object
)
{
	LPOGGPLAYER player = (LPOGGPLAYER)object;
	int i;

	OGG_Close(player);

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
OGG_Init(VOID)
{
	LPOGGPLAYER player;

	player = (LPOGGPLAYER)calloc(1, sizeof(OGGPLAYER));
	if (player)
	{
		player->FillBuffer = OGG_FillBuffer;
		player->Play = OGG_Play;
		player->Shutdown = OGG_Shutdown;

		player->resampler[0] = resampler_create();
		player->resampler[1] = resampler_create();
		if (!player->resampler[0] || !player->resampler[1])
		{
			OGG_Shutdown(player);
			return NULL;
		}
	}

	return (LPAUDIOPLAYER)player;
}

#else

LPAUDIOPLAYER
OGG_Init(VOID)
{
	return NULL;
}

#endif
