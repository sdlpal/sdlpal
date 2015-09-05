/* -*- mode: c++; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
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

#include "util.h"
#include "global.h"

#if PAL_HAS_MP3

#include "sound.h"
#include "players.h"
#include "resampler.h"
#include "libmad/music_mad.h"

typedef struct tagMP3PLAYER
{
	MUSICPLAYER_FUNCTIONS;

	mad_data           *pMP3;
	INT                 iMusic;
	BOOL                fLoop;
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
		player->pMP3->volume = gConfig.iVolume * 3 / 4;

		mad_getSamples(player->pMP3, stream, len);

		if (!mad_isPlaying(player->pMP3) && player->fLoop)
		{
			mad_seek(player->pMP3, 0);
			mad_start(player->pMP3);

			mad_getSamples(player->pMP3, stream, len);
		}
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

	if (iNum > 0)
	{
		if ((player->pMP3 = mad_openFile(va("%s/mp3/%.2d.mp3", PAL_PREFIX, iNum), SOUND_GetAudioSpec(), gConfig.iResampleQuality)) == NULL)
		{
			player->pMP3 = mad_openFile(va("%s/MP3/%.2d.MP3", PAL_PREFIX, iNum), SOUND_GetAudioSpec(), gConfig.iResampleQuality);
		}

		if (player->pMP3)
		{
			player->iMusic = iNum;
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

LPMUSICPLAYER
MP3_Init(
	LPCSTR szFileName
	)
{
	LPMP3PLAYER player;
	if (player = (LPMP3PLAYER)malloc(sizeof(MP3PLAYER)))
	{
		player->FillBuffer = MP3_FillBuffer;
		player->Play = MP3_Play;
		player->Shutdown = MP3_Shutdown;

		player->pMP3 = NULL;
		player->iMusic = -1;
		player->fLoop = FALSE;
		return (LPMUSICPLAYER)player;
	}
	else
	{
		return NULL;
	}
}

#endif
