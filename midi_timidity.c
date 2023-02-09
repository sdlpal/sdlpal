/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2023, SDLPAL development team.
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

#include "timidity.h"

typedef struct tagTIMIDITYPLAYER
{
	AUDIOPLAYER_COMMONS;

	MidiSong	*pSong;
} TIMIDITYPLAYER, *LPTIMIDITYPLAYER;

static VOID TIMIDITY_Close(
	LPTIMIDITYPLAYER player
)
{
	if (player->pSong)
	{
		Timidity_FreeSong(player->pSong);
		player->pSong = NULL;
	}
}

static VOID
TIMIDITY_FillBuffer(
	VOID       *object,
	LPBYTE      stream,
	INT         len
)
{
	LPTIMIDITYPLAYER player = (LPTIMIDITYPLAYER)object;
	if (player->pSong) {
		if (!player->pSong->playing && player->fLoop)
		{
			Timidity_Seek(player->pSong, 0);
			Timidity_Start(player->pSong);
		}

		if (player->pSong->playing)
			Timidity_PlaySome(player->pSong, stream, len);
	}
}

static VOID
TIMIDITY_Shutdown(
	VOID       *object
	)
{
	LPTIMIDITYPLAYER player = (LPTIMIDITYPLAYER)object;
	if (player)
	{
		TIMIDITY_Close(player);
		free(player);
	}
	Timidity_Exit();
}

static BOOL
TIMIDITY_Play(
	VOID       *object,
	INT         iNum,
	BOOL        fLoop,
	FLOAT       flFadeTime
	)
{
	LPTIMIDITYPLAYER player = (LPTIMIDITYPLAYER)object;

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

	TIMIDITY_Close(player);

	player->iMusic = iNum;

	if (iNum > 0)
	{
		if (gConfig.fIsWIN95)
		{
			SDL_RWops* rw = SDL_RWFromFile(PAL_CombinePath(0, gConfig.pszGamePath, PAL_va(1, "Musics%s%.3d.mid", PAL_NATIVE_PATH_SEPARATOR, iNum)), "rb");
			if (rw)
			{
				player->pSong = Timidity_LoadSong(rw, AUDIO_GetDeviceSpec());
				SDL_RWclose(rw);
			}
		}
		
		if (!player->pSong)
		{
			FILE    *fp  = NULL;
			uint8_t *buf = NULL;
			int      size;

			if ((fp = UTIL_OpenFile("midi.mkf")) != NULL)
			{
				if ((size = PAL_MKFGetChunkSize(iNum, fp)) > 0 &&
					(buf = (uint8_t*)UTIL_malloc(size)))
				{
					PAL_MKFReadChunk(buf, size, iNum, fp);
				}
				fclose(fp);
			}

			if (buf)
			{
				SDL_RWops *rw = SDL_RWFromConstMem(buf, size);
				player->pSong = Timidity_LoadSong(rw, AUDIO_GetDeviceSpec());
				SDL_RWclose(rw);
				free(buf);
			}
		}

		if (player->pSong)
		{
			Timidity_Start(player->pSong);
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
TIMIDITY_Init(
	VOID
)
{
	Timidity_Init(gConfig.pszGamePath, gConfig.pszSoundBank);

	LPTIMIDITYPLAYER player;
	if ((player = (LPTIMIDITYPLAYER)malloc(sizeof(TIMIDITYPLAYER))) != NULL)
	{
		memset(player, 0, sizeof(TIMIDITYPLAYER));

		player->FillBuffer = TIMIDITY_FillBuffer;
		player->Play = TIMIDITY_Play;
		player->Shutdown = TIMIDITY_Shutdown;

		player->pSong = NULL;
		player->iMusic = -1;
		player->fLoop = FALSE;
	}
	return (LPAUDIOPLAYER)player;
}
