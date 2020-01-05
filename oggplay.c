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
// oggplay.h: Player of OGG files.
//   @Author: Lou Yihua <louyihua@21cn.com>, 2015-07-28.
//

#include "util.h"
#include "global.h"
#include "palcfg.h"
#include "players.h"
#include "audio.h"
#include <math.h>

#if PAL_HAS_OGG
#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>
#include "resampler.h"

#define FLAG_OY 0x01
#define FLAG_VI 0x02
#define FLAG_VC 0x04
#define FLAG_OS 0x08
#define FLAG_VD 0x10
#define FLAG_VB 0x20

#define STAGE_PAGEOUT    1
#define STAGE_PACKETOUT  2
#define STAGE_PCMOUT	 3
#define STAGE_REWIND     4

#define OGG_BUFFER_LENGTH 4096

typedef struct tagOGGPLAYER
{
	AUDIOPLAYER_COMMONS;

	ogg_sync_state   oy; /* sync and verify incoming physical bitstream */
	ogg_stream_state os; /* take physical pages, weld into a logical stream of packets */
	ogg_page         og; /* one Ogg bitstream page. Vorbis packets are inside */
	vorbis_info      vi; /* struct that stores all the static vorbis bitstream settings */
	vorbis_comment   vc; /* struct that stores all the bitstream user comments */
	vorbis_dsp_state vd; /* central working state for the packet->PCM decoder */
	vorbis_block     vb; /* local working space for packet->PCM decode */

	FILE            *fp;
	void            *resampler[2];
	INT              iFlags;
	INT              iStage;
	INT              nChannels;
	BOOL             fReady;
	BOOL             fUseResampler;
} OGGPLAYER, *LPOGGPLAYER;

PAL_FORCE_INLINE ogg_int16_t OGG_GetSample(float pcm)
{
	int val = (int)(floor(pcm * 32767.f + .5f));
	/* might as well guard against clipping */
	if (val > 32767) {
		val = 32767;
	}
	else if (val < -32768) {
		val = -32768;
	}
	return (ogg_int16_t)val;
}

PAL_FORCE_INLINE void OGG_FillResample(LPOGGPLAYER player, ogg_int16_t* stream)
{
	if (gConfig.iAudioChannels == 2) {
		stream[0] = resampler_get_and_remove_sample(player->resampler[0]);
		stream[1] = (player->vi.channels > 1) ? resampler_get_and_remove_sample(player->resampler[1]) : stream[0];
	}
	else {
		if (player->vi.channels > 1) {
			*stream = (short)((int)(resampler_get_and_remove_sample(player->resampler[0]) + resampler_get_and_remove_sample(player->resampler[1])) >> 1);
		}
		else {
			*stream = resampler_get_and_remove_sample(player->resampler[0]);
		}
	}
}

static void OGG_Cleanup(LPOGGPLAYER player)
{
	int i;
	for (i = 0; i < gConfig.iAudioChannels; i++) resampler_clear(player->resampler[0]);
	/* Do various cleanups */
	if (player->iFlags & FLAG_VB) vorbis_block_clear(&player->vb);
	if (player->iFlags & FLAG_VD) vorbis_dsp_clear(&player->vd);
	if (player->iFlags & FLAG_OS) ogg_stream_clear(&player->os);
	if (player->iFlags & FLAG_VC) vorbis_comment_clear(&player->vc);
	if (player->iFlags & FLAG_VI) vorbis_info_clear(&player->vi);  /* must be called last */
	if (player->iFlags & FLAG_OY) ogg_sync_clear(&player->oy);
	player->iFlags = player->iStage = 0;
	player->fReady = FALSE;
}


static BOOL OGG_Rewind(LPOGGPLAYER player)
{
	ogg_packet       op; /* one raw packet of data for decode */
	char *buffer;
	int i, bytes;

	OGG_Cleanup(player);

	fseek(player->fp, 0, SEEK_SET);

	ogg_sync_init(&player->oy); player->iFlags = FLAG_OY;

	/* grab some data at the head of the stream. We want the first page
	(which is guaranteed to be small and only contain the Vorbis
	stream initial header) We need the first page to get the stream
	serialno. */

	/* submit a 4k block to libvorbis' Ogg layer */
	buffer = ogg_sync_buffer(&player->oy, OGG_BUFFER_LENGTH);
	bytes = fread(buffer, 1, OGG_BUFFER_LENGTH, player->fp);
	ogg_sync_wrote(&player->oy, bytes);

	/* Get the first page. */
	if (ogg_sync_pageout(&player->oy, &player->og) != 1) {
		/* have we simply run out of data?  If so, we're done. */
		/* error case.  Must not be Vorbis data */
		OGG_Cleanup(player);
		return (player->fReady = FALSE);
	}

	/* Get the serial number and set up the rest of decode. */
	/* serialno first; use it to set up a logical stream */
	ogg_stream_init(&player->os, ogg_page_serialno(&player->og));
	player->iFlags |= FLAG_OS;

	/* extract the initial header from the first page and verify that the
	Ogg bitstream is in fact Vorbis data */

	/* I handle the initial header first instead of just having the code
	read all three Vorbis headers at once because reading the initial
	header is an easy way to identify a Vorbis bitstream and it's
	useful to see that functionality seperated out. */

	vorbis_info_init(&player->vi); player->iFlags |= FLAG_VI;
	vorbis_comment_init(&player->vc); player->iFlags |= FLAG_VC;
	if (ogg_stream_pagein(&player->os, &player->og)<0) {
		/* error; stream version mismatch perhaps */
		OGG_Cleanup(player);
		return (player->fReady = FALSE);
	}

	if (ogg_stream_packetout(&player->os, &op) != 1) {
		/* no page? must not be vorbis */
		OGG_Cleanup(player);
		return (player->fReady = FALSE);
	}

	if (vorbis_synthesis_headerin(&player->vi, &player->vc, &op)<0) {
		/* error case; not a vorbis header */
		OGG_Cleanup(player);
		return (player->fReady = FALSE);
	}

	/* At this point, we're sure we're Vorbis. We've set up the logical
	(Ogg) bitstream decoder. Get the comment and codebook headers and
	set up the Vorbis decoder */

	/* The next two packets in order are the comment and codebook headers.
	They're likely large and may span multiple pages. Thus we read
	and submit data until we get our two packets, watching that no
	pages are missing. If a page is missing, error out; losing a
	header page is the only place where missing data is fatal. */

	i = 0;
	while (i < 2) {
		while (i < 2) {
			int result = ogg_sync_pageout(&player->oy, &player->og);
			if (result == 0)break; /* Need more data */
								   /* Don't complain about missing or corrupt data yet. We'll
								   catch it at the packet output phase */
			if (result == 1) {
				ogg_stream_pagein(&player->os, &player->og); /* we can ignore any errors here
															 as they'll also become apparent
															 at packetout */
				while (i < 2) {
					result = ogg_stream_packetout(&player->os, &op);
					if (result == 0)break;
					if (result < 0) {
						/* Uh oh; data at some point was corrupted or missing!
						We can't tolerate that in a header.  Die. */
						OGG_Cleanup(player);
						return (player->fReady = FALSE);
					}
					result = vorbis_synthesis_headerin(&player->vi, &player->vc, &op);
					if (result < 0) {
						OGG_Cleanup(player);
						return (player->fReady = FALSE);
					}
					i++;
				}
			}
		}
		/* no harm in not checking before adding more */
		buffer = ogg_sync_buffer(&player->oy, OGG_BUFFER_LENGTH);
		bytes = fread(buffer, 1, OGG_BUFFER_LENGTH, player->fp);
		if (bytes == 0 && i < 2) {
			OGG_Cleanup(player);
			return (player->fReady = FALSE);
		}
		ogg_sync_wrote(&player->oy, bytes);
	}

	if (vorbis_synthesis_init(&player->vd, &player->vi) == 0) { /* central decode state */
		vorbis_block_init(&player->vd, &player->vb);            /* local state for most of the decode
																so multiple block decodes can
																proceed in parallel. We could init
																multiple vorbis_block structures
																for vd here */
		player->iStage = STAGE_PAGEOUT;
		player->iFlags |= FLAG_VD | FLAG_VB;
		player->fUseResampler = (player->vi.rate != gConfig.iSampleRate);

		if (player->fUseResampler) {
			double factor = (double)player->vi.rate / (double)gConfig.iSampleRate;
			for (i = 0; i < min(player->vi.channels, 2); i++)
			{
				resampler_set_quality(player->resampler[i], AUDIO_IsIntegerConversion(player->vi.rate) ? RESAMPLER_QUALITY_MIN : gConfig.iResampleQuality);
				resampler_set_rate(player->resampler[i], factor);
				resampler_clear(player->resampler[i]);
			}
		}
		return (player->fReady = TRUE);
	}
	else {
		OGG_Cleanup(player);
		return (player->fReady = FALSE);
	}
}

static VOID
OGG_FillBuffer(
	VOID       *object,
	LPBYTE      stream,
	INT         len
	)
{
	LPOGGPLAYER player = (LPOGGPLAYER)object;

	if (player->fReady) {
		ogg_packet       op; /* one raw packet of data for decode */
		int total_bytes = 0, stage = player->iStage;

		while (total_bytes < len) {
			float **pcm;
			int samples, result;

			switch (stage)
			{
			case STAGE_PAGEOUT: /* PAGEOUT stage */
				result = ogg_sync_pageout(&player->oy, &player->og);
				if (result > 0) {
					/* can safely ignore errors at this point */
					ogg_stream_pagein(&player->os, &player->og);
					stage = STAGE_PACKETOUT;
				}
				else {
					if (result == 0) { /* need more data */
						char *buffer = ogg_sync_buffer(&player->oy, OGG_BUFFER_LENGTH);
						int bytes = fread(buffer, 1, OGG_BUFFER_LENGTH, player->fp);
						ogg_sync_wrote(&player->oy, bytes);
						stage = (bytes > 0) ? STAGE_PAGEOUT : STAGE_REWIND;
					}
					break;
				}
			case STAGE_PACKETOUT:
				result = ogg_stream_packetout(&player->os, &op);
				if (result > 0) {
					/* we have a packet.  Decode it */
					if (vorbis_synthesis(&player->vb, &op) == 0) { /* test for success! */
						vorbis_synthesis_blockin(&player->vd, &player->vb);
					}
					stage = STAGE_PCMOUT;
				}
				else {
					if (result == 0) { /* need more data */
						if (ogg_page_eos(&player->og)) {
							if (player->fLoop) {
								stage = STAGE_REWIND;
							}
							else {
								OGG_Cleanup(player);
								UTIL_CloseFile(player->fp);
								player->fp = NULL;
								return;
							}
						}
						else {
							stage = STAGE_PAGEOUT;
						}
					}
					break;
				}
			case STAGE_PCMOUT:
				if ((samples = vorbis_synthesis_pcmout(&player->vd, &pcm)) > 0) {
					int bout;
					if (player->fUseResampler) { /* Resampler is available and should be used */
						bout = 0;
						while (total_bytes < len && samples > 0) { /* Fill as many samples into resampler as possible */
							int i, j, to_write = resampler_get_free_count(player->resampler[0]);

							if (to_write > 0) {
								if (to_write >= samples) to_write = samples;

								for (i = 0; i < min(player->vi.channels, 2); i++) {
									float *mono = pcm[i] + bout;
									for (j = 0; j < to_write; j++) {
										resampler_write_sample(player->resampler[i], OGG_GetSample(mono[j]));
									}
								}
							}

							/* Fetch resampled samples if available */
							j = resampler_get_sample_count(player->resampler[0]);
							while (total_bytes < len && resampler_get_sample_count(player->resampler[0]) > 0) {
								OGG_FillResample(player, (ogg_int16_t *)(stream + total_bytes));
								total_bytes += gConfig.iAudioChannels * sizeof(ogg_int16_t);
							}

							samples -= to_write; bout += to_write;
						}
					}
					else {
						int i;
						ogg_int16_t *ptr = (ogg_int16_t *)(stream + total_bytes);
						bout = (len - total_bytes) / gConfig.iAudioChannels / sizeof(ogg_int16_t);
						if (bout > samples) bout = samples;
						for (i = 0; i < bout; i++) {
							if (gConfig.iAudioChannels == 2) {
								ptr[0] = OGG_GetSample(pcm[0][i]);
								ptr[1] = (player->vi.channels > 1) ? OGG_GetSample(pcm[1][i]) : ptr[0];
							}
							else {
								if (player->vi.channels > 1) {
									ptr[0] = (short)((int)(OGG_GetSample(pcm[0][i]) + OGG_GetSample(pcm[1][i])) >> 1);
								}
								else {
									ptr[0] = OGG_GetSample(pcm[0][i]);
								}
							}
							ptr += gConfig.iAudioChannels;
						}

						total_bytes += bout * gConfig.iAudioChannels * sizeof(ogg_int16_t);
					}
					/* tell libvorbis how many samples we actually consumed */
					vorbis_synthesis_read(&player->vd, bout);
				}
				else {
					stage = STAGE_PACKETOUT;
				}
				break;
			case STAGE_REWIND:
				if (player->vi.rate != gConfig.iSampleRate) { /* If there are samples in the resampler, fetch them first */
					while (total_bytes < len && resampler_get_sample_count(player->resampler[0]) > 0) {
						OGG_FillResample(player, (ogg_int16_t *)(stream + total_bytes));
						total_bytes += gConfig.iAudioChannels * sizeof(ogg_int16_t);
					}
					/* Break out if there are still samples in the resampler */
					if (resampler_get_sample_count(player->resampler[0]) > 0) break;
				}
				OGG_Rewind(player);
				stage = player->iStage;
				break;
			default:
				return;
			}
		}
		player->iStage = stage;
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

	player->fReady = FALSE;
	OGG_Cleanup(player);
	if (player->fp)
	{
		UTIL_CloseFile(player->fp);
		player->fp = NULL;
	}

    player->iMusic = iNum;

	if (iNum == -1)
	{
		return TRUE;
	}

	player->fp = UTIL_OpenFile(PAL_va(0, "ogg%s%.2d.ogg", PAL_NATIVE_PATH_SEPARATOR, iNum));
	if (player->fp == NULL)
	{
		return FALSE;
	}

	if (!OGG_Rewind(player))
	{
		UTIL_CloseFile(player->fp);
		player->fp = NULL;
		return FALSE;
	}

	return TRUE;
}

static VOID
OGG_Shutdown(
	VOID       *object
	)
{
	if (object)
	{
		LPOGGPLAYER player = (LPOGGPLAYER)object;
		OGG_Cleanup(player);
		resampler_delete(player->resampler[0]);
		resampler_delete(player->resampler[1]);
		UTIL_CloseFile(player->fp);
		free(player);
	}
}

LPAUDIOPLAYER
OGG_Init(
	VOID
)
{
	LPOGGPLAYER player;
	if ((player = (LPOGGPLAYER)malloc(sizeof(OGGPLAYER))) != NULL)
	{
		memset(player, 0, sizeof(OGGPLAYER));

		player->FillBuffer = OGG_FillBuffer;
		player->Play = OGG_Play;
		player->Shutdown = OGG_Shutdown;

		player->fp = NULL;
		player->iMusic = -1;
		player->iFlags = 0;
		player->iStage = 0;
		player->fLoop = FALSE;
		player->fReady = FALSE;
		player->fUseResampler = FALSE;

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
OGG_Init(
	VOID
)
{
	return NULL;
}

#endif
