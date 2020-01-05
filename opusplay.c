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
// opusplay.c: Player for opus files.
//   @Author: Soar Qin <soarchin@gmail.com>, 2019-12-13.
//

#include "util.h"
#include "global.h"
#include "palcfg.h"
#include "players.h"
#include "audio.h"
#include <math.h>

#if PAL_HAS_OPUS
#include "resampler.h"
#include "opusfile.h"

enum {
    opus_sample_buffer_count = 120*48*2,
};

typedef struct tagOPUSPLAYER
{
    AUDIOPLAYER_COMMONS;

    OggOpusFile     *fp;
    opus_int16       sBuffer[opus_sample_buffer_count];
    int              iBufPos, iBufLen;

    void            *resampler[2];
    int              iLink;
    int              fReady;
    int              fRewind;
    int              fUseResampler;
} OPUSPLAYER, *LPOPUSPLAYER;

PAL_FORCE_INLINE opus_int16 OPUS_GetSample(float pcm)
{
    int val = (int)(floor(pcm * 32767.f + .5f));
    /* might as well guard against clipping */
    if (val > 32767) {
        val = 32767;
    }
    else if (val < -32768) {
        val = -32768;
    }
    return (opus_int16)val;
}

PAL_FORCE_INLINE void OPUS_FillResample(LPOPUSPLAYER player, opus_int16* stream, int count)
{
    int i;
    if (gConfig.iAudioChannels == 2) {
        for (i = count; i; --i) {
            *stream++ = resampler_get_and_remove_sample(player->resampler[0]);
            *stream++ = resampler_get_and_remove_sample(player->resampler[1]);
        }
    }
    else {
        for (i = count; i; --i) {
            *stream++ = (short)((int)(resampler_get_and_remove_sample(player->resampler[0])
                + resampler_get_and_remove_sample(player->resampler[1])) >> 1);
        }
    }
}

static void OPUS_Cleanup(LPOPUSPLAYER player)
{
    int i;
    for (i = 0; i < gConfig.iAudioChannels; i++) resampler_clear(player->resampler[0]);
    player->iBufPos = player->iBufLen = 0;
    player->iLink = -1;
    player->fReady = FALSE;
    player->fRewind = FALSE;
}


static BOOL OPUS_Rewind(LPOPUSPLAYER player)
{
    OPUS_Cleanup(player);
    op_raw_seek(player->fp, 0);
    player->iLink = op_current_link(player->fp);
    player->fUseResampler = 48000 != gConfig.iSampleRate;
    if (player->fUseResampler) {
        int i;
        double factor = 48000. / (double)gConfig.iSampleRate;
        for (i = 0; i < 2; i++)
        {
            resampler_set_quality(player->resampler[i], AUDIO_IsIntegerConversion(48000) ? RESAMPLER_QUALITY_MIN : gConfig.iResampleQuality);
            resampler_set_rate(player->resampler[i], factor);
            resampler_clear(player->resampler[i]);
        }
    }
    player->fRewind = FALSE;
    return (player->fReady = TRUE);
}

static VOID
OPUS_FillBuffer(
    VOID       *object,
    LPBYTE      stream,
    INT         len
)
{
    int total_bytes;
    int bytes_per_sample;
    LPOPUSPLAYER player = (LPOPUSPLAYER)object;
    if (!player->fReady) {
        return;
    }

    total_bytes = 0;
    bytes_per_sample = gConfig.iAudioChannels * sizeof(opus_int16);
    while (total_bytes < len) {
        opus_int16 *samples;
        if (!player->fRewind && player->iBufLen == 0) {
            int read_count = op_read_stereo(player->fp, player->sBuffer, opus_sample_buffer_count);
            if (read_count == 0) {
                if (player->fLoop)
                    player->fRewind = TRUE;
            } else if (read_count < 0) {
                if (read_count == OP_HOLE) {
                    /* Hole detected! Corrupt file segment? */
                    continue;
                }
                /* Stop playing on other errors */
                player->fReady = FALSE;
                return;
            } else {
                player->iBufLen = read_count * 2;
            }
        }
        if (player->fUseResampler) {
            int i;
            int resampler_count;
            int fill_count;
            int buf_count = player->iBufLen - player->iBufPos;
            int to_write = resampler_get_free_count(player->resampler[0])*2;
            if (buf_count > to_write) {
                buf_count = to_write;
            }
            samples = player->sBuffer + player->iBufPos;
            for (i = buf_count; i; i -= 2) {
                resampler_write_sample(player->resampler[0], *samples++);
                resampler_write_sample(player->resampler[1], *samples++);
            }
            player->iBufPos += buf_count;
            if (player->iBufPos == player->iBufLen) {
                player->iBufPos = player->iBufLen = 0;
            }
            resampler_count = resampler_get_sample_count(player->resampler[0]);
            if (resampler_count==0) {
                if (player->fRewind) {
                    OPUS_Rewind(player);
                }
                continue;
            }
            fill_count = (len - total_bytes) / bytes_per_sample;
            if (fill_count > resampler_count) fill_count = resampler_count;
            OPUS_FillResample(player, (opus_int16 *)(stream + total_bytes), fill_count);
            total_bytes += fill_count * bytes_per_sample;
        } else {
            if (player->fRewind) {
                OPUS_Rewind(player);
                continue;
            }
            int i;
            opus_int16 *ptr = (opus_int16 *)(stream + total_bytes);
            opus_int16 *inptr = player->sBuffer + player->iBufPos;
            int buf_count = (player->iBufLen - player->iBufPos) / 2;
            int out_count = (len - total_bytes) / bytes_per_sample;
            int stereo = gConfig.iAudioChannels > 1;
            if (out_count < buf_count) {
                player->iBufPos += out_count * 2;
            } else {
                out_count = buf_count;
                player->iBufPos = player->iBufLen = 0;
            }
            if (stereo) {
                memcpy(ptr, inptr, out_count * bytes_per_sample);
            } else {
                for (i = out_count; i; i--) {
                    *ptr++ = (opus_int16)(((int32_t)inptr[0] + (int32_t)inptr[1])) / 2;
                    inptr += 2;
                }
            }
            total_bytes += out_count * bytes_per_sample;
        }
    }
}

static BOOL
OPUS_Play(
    VOID       *object,
    INT         iNum,
    BOOL        fLoop,
    FLOAT       flFadeTime
)
{
    LPOPUSPLAYER player = (LPOPUSPLAYER)object;
    static char internal_buffer[PAL_GLOBAL_BUFFER_SIZE];

    int ret;

    if (player == NULL)
    {
        return FALSE;
    }

    player->fLoop = fLoop;

    if (iNum == player->iMusic)
    {
        return TRUE;
    }

    player->fReady = FALSE;
    OPUS_Cleanup(player);
    if (player->fp)
    {
        op_free(player->fp);
        player->fp = NULL;
    }

    player->iMusic = iNum;

    if (iNum == -1)
    {
        return TRUE;
    }

    if (iNum == 0)
    {
        return FALSE;
    }

    player->fp = op_open_file(UTIL_GetFullPathName(internal_buffer, PAL_GLOBAL_BUFFER_SIZE, gConfig.pszGamePath, PAL_va(0, "opus%s%.2d.opus", PAL_NATIVE_PATH_SEPARATOR, iNum)), &ret);
    if (player->fp == NULL)
    {
        return FALSE;
    }

    if (!OPUS_Rewind(player))
    {
        op_free(player->fp);
        player->fp = NULL;
        return FALSE;
    }

    return TRUE;
}

static VOID
OPUS_Shutdown(
    VOID       *object
)
{
    if (object)
    {
        LPOPUSPLAYER player = (LPOPUSPLAYER)object;
        OPUS_Cleanup(player);
        resampler_delete(player->resampler[0]);
        resampler_delete(player->resampler[1]);
        op_free(player->fp);
        free(player);
    }
}

LPAUDIOPLAYER
OPUS_Init(
    VOID
)
{
    LPOPUSPLAYER player;
    if ((player = (LPOPUSPLAYER)malloc(sizeof(OPUSPLAYER))) != NULL)
    {
        memset(player, 0, sizeof(OPUSPLAYER));

        player->FillBuffer = OPUS_FillBuffer;
        player->Play = OPUS_Play;
        player->Shutdown = OPUS_Shutdown;

        player->iLink = -1;
        player->iMusic = -1;

        player->resampler[0] = resampler_create();
        if (player->resampler[0])
        {
            player->resampler[1] = resampler_create();
            if (player->resampler[1] == NULL)
            {
                resampler_delete(player->resampler[0]);
                player->resampler[0] = NULL;
            }
        }
    }
    return (LPAUDIOPLAYER)player;
}

#else

LPAUDIOPLAYER
OPUS_Init(
	VOID
)
{
	return NULL;
}

#endif
