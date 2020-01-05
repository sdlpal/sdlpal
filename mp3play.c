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

#include "util.h"
#include "global.h"
#include "palcfg.h"
#include "audio.h"
#include "players.h"

#if PAL_HAS_MP3
#include "libmad/music_mad.h"
#include "resampler.h"

typedef struct tagMP3PLAYER
{
	AUDIOPLAYER_COMMONS;

	mad_data           *pMP3;
} MP3PLAYER, *LPMP3PLAYER;

static VOID MP3_Close(
	LPMP3PLAYER player
	)
{
	if (player->pMP3)
	{
		mad_stop(player->pMP3);
		mad_closeFile(player->pMP3);

		player->pMP3 = NULL;
	}
}

static VOID
MP3_FillBuffer(
	VOID       *object,
	LPBYTE      stream,
	INT         len
	)
{
	LPMP3PLAYER player = (LPMP3PLAYER)object;
	if (player->pMP3) {
		if (!mad_isPlaying(player->pMP3) && player->fLoop)
		{
			mad_seek(player->pMP3, 0);
			mad_start(player->pMP3);
		}

		if (mad_isPlaying(player->pMP3))
			mad_getSamples(player->pMP3, stream, len);
	}
}

static VOID
MP3_Shutdown(
	VOID       *object
	)
{
	if (object)
	{
		MP3_Close((LPMP3PLAYER)object);
		free(object);
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

	//
	// Check for NULL pointer.
	//
	if (player == NULL)
	{
		return FALSE;
	}

	player->fLoop = fLoop;

	if (iNum == player->iMusic)
	{
		return TRUE;
	}

	MP3_Close(player);

	player->iMusic = iNum;

	if (iNum > 0)
	{
		player->pMP3 = mad_openFile(UTIL_GetFullPathName(PAL_BUFFER_SIZE_ARGS(0), gConfig.pszGamePath, PAL_va(1, "mp3%s%.2d.mp3", PAL_NATIVE_PATH_SEPARATOR, iNum)), AUDIO_GetDeviceSpec(), gConfig.iResampleQuality);

		if (player->pMP3)
		{
			mad_start(player->pMP3);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else
	{
		return TRUE;
	}
}

LPAUDIOPLAYER
MP3_Init(
	VOID
)
{
	LPMP3PLAYER player;
	if ((player = (LPMP3PLAYER)malloc(sizeof(MP3PLAYER))) != NULL)
	{
		player->FillBuffer = MP3_FillBuffer;
		player->Play = MP3_Play;
		player->Shutdown = MP3_Shutdown;

		player->pMP3 = NULL;
		player->iMusic = -1;
		player->fLoop = FALSE;
	}
	return (LPAUDIOPLAYER)player;
}

#else

LPAUDIOPLAYER
MP3_Init(
	VOID
)
{
	return NULL;
}

#endif
