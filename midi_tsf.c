/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2022, SDLPAL development team.
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

#include "main.h"

#define TSF_IMPLEMENTATION
#include "tsf.h"
#define TML_IMPLEMENTATION
#include "tml.h"

typedef struct tagTSFPLAYER
{
	AUDIOPLAYER_COMMONS;

	tsf* pTinySoundFont;
	double fSec;
	tml_message* pMidiMessage, *pCurMidiMsg;
	BOOL fNoteOn;
} TSFPLAYER, * LPTSFPLAYER;

static VOID
TSF_Close(
	VOID* object
)
{
	LPTSFPLAYER player = (LPTSFPLAYER)object;
	if (object) {
		player->fNoteOn = FALSE;
		if (player->pCurMidiMsg)
			tml_free(player->pCurMidiMsg);
		player->pCurMidiMsg = player->pMidiMessage = NULL;
	}
}

static VOID TSF_LoadMIDI(LPTSFPLAYER player)
{
	tml_free(player->pCurMidiMsg);
	player->pMidiMessage = NULL;
	player->fSec = 0;

	if (gConfig.fIsWIN95)
		player->pCurMidiMsg = player->pMidiMessage = tml_load_filename(UTIL_GetFullPathName(PAL_BUFFER_SIZE_ARGS(0), gConfig.pszGamePath, PAL_va(1, "Musics%s%.3d.mid", PAL_NATIVE_PATH_SEPARATOR, player->iMusic)));

	if (!player->pMidiMessage)
	{
		FILE* fp = NULL;
		uint8_t* buf = NULL;
		int      size;

		if ((fp = UTIL_OpenFile("midi.mkf")) != NULL)
		{
			if ((size = PAL_MKFGetChunkSize(player->iMusic, fp)) > 0 &&
				(buf = (uint8_t*)UTIL_malloc(size)))
			{
				PAL_MKFReadChunk(buf, size, player->iMusic, fp);
			}
			fclose(fp);
		}

		if (buf)
		{
			player->pCurMidiMsg = player->pMidiMessage = tml_load_memory(buf, size);
			free(buf);
		}
	}
}

static VOID
TSF_FillBuffer(
	VOID       *object,
	LPBYTE      stream,
	INT         len
)
{
	LPTSFPLAYER player = (LPTSFPLAYER)object;
	if (player && player->fNoteOn) {
 		SDL_AudioSpec* pSpec = AUDIO_GetDeviceSpec();
		int SampleBlock, SampleCount = (len / (pSpec->channels * sizeof(short)));
		for (SampleBlock = TSF_RENDER_EFFECTSAMPLEBLOCK; SampleCount; SampleCount -= SampleBlock, stream += (SampleBlock * (pSpec->channels * sizeof(short))))
		{
			if (SampleBlock > SampleCount) SampleBlock = SampleCount;

			for (player->fSec += SampleBlock * (1000.0 / pSpec->freq); player->pMidiMessage && player->fSec >= player->pMidiMessage->time; player->pMidiMessage = player->pMidiMessage->next)
			{
				switch (player->pMidiMessage->type)
				{
				case TML_PROGRAM_CHANGE: 
					tsf_channel_set_presetnumber(player->pTinySoundFont, player->pMidiMessage->channel, player->pMidiMessage->program, (player->pMidiMessage->channel == 9));
					break;
				case TML_NOTE_ON: 
					tsf_channel_note_on(player->pTinySoundFont, player->pMidiMessage->channel, player->pMidiMessage->key, player->pMidiMessage->velocity / 127.0f);
					break;
				case TML_NOTE_OFF: 
					tsf_channel_note_off(player->pTinySoundFont, player->pMidiMessage->channel, player->pMidiMessage->key);
					break;
				case TML_PITCH_BEND: 
					tsf_channel_set_pitchwheel(player->pTinySoundFont, player->pMidiMessage->channel, player->pMidiMessage->pitch_bend);
					break;
				case TML_CONTROL_CHANGE:
					tsf_channel_midi_control(player->pTinySoundFont, player->pMidiMessage->channel, player->pMidiMessage->control, player->pMidiMessage->control_value);
					break;
				}
			}

			tsf_render_short(player->pTinySoundFont, (short*)stream, SampleBlock, 0);
		}
		if (player->pMidiMessage == NULL)
			TSF_LoadMIDI(player);
	}
}

static BOOL
TSF_Play(
	VOID	   *object,
	int			iNum,
	BOOL		fLoop,
	FLOAT       flFadeTime
)
{
	LPTSFPLAYER player = (LPTSFPLAYER)object;

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

	TSF_Close(player);

	player->iMusic = iNum;

	if (player->iMusic == 0)
		return TRUE;

	tsf_reset(player->pTinySoundFont);

	TSF_LoadMIDI(player);

	if (player->pMidiMessage)
	{
		player->fNoteOn = TRUE;
		return TRUE;
	}
	else
		return FALSE;
}

static VOID
TSF_Shutdown(
	VOID* object
)
{
	LPTSFPLAYER player = (LPTSFPLAYER)object;
	if (player) {
		TSF_Close(player);

        if(player->pTinySoundFont)
			tsf_close(player->pTinySoundFont);
        player->pTinySoundFont = NULL;

		free(object);
	}
}

LPAUDIOPLAYER
TSF_Init(
	VOID
)
{
	LPTSFPLAYER player;
	if ((player = (LPTSFPLAYER)malloc(sizeof(TSFPLAYER))) != NULL)
	{
		memset(player, 0, sizeof(TSFPLAYER));

		player->FillBuffer = TSF_FillBuffer;
		player->Play = TSF_Play;
		player->Shutdown = TSF_Shutdown;

		player->pTinySoundFont = NULL;
		player->fSec = 0.0f;
		player->pMidiMessage = player->pCurMidiMsg = NULL;
		player->fNoteOn = FALSE;

		player->iMusic = -1;
		player->fLoop = FALSE;

		if (!player->pTinySoundFont)
		{
			player->pTinySoundFont = tsf_load_filename(UTIL_IsAbsolutePath(gConfig.pszSoundBank) ? gConfig.pszSoundBank : UTIL_GetFullPathName(PAL_BUFFER_SIZE_ARGS(0), gConfig.pszGamePath, gConfig.pszSoundBank));
			tsf_channel_set_bank_preset(player->pTinySoundFont, 9, 128, 0);
			SDL_AudioSpec* pSpec = AUDIO_GetDeviceSpec();
			tsf_set_output(player->pTinySoundFont, TSF_STEREO_INTERLEAVED, pSpec->freq, 0.0f);
			tsf_set_volume(player->pTinySoundFont, 1.0);
		}
		if (!player->pTinySoundFont)
		{
			TerminateOnError("Could not load SoundFont\n");
			return NULL;
		}
	}
	return (LPAUDIOPLAYER)player;
}
