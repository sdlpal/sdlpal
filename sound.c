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

#include "palcommon.h"
#include "global.h"
#include "palcfg.h"
#include "audio.h"
#include "players.h"
#include "util.h"
#include "resampler.h"
#include "midi.h"
#include "riff.h"
#include <math.h>

typedef struct tagWAVESPEC
{
	int                 size;
	int                 freq;
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_AudioFormat     format;
#else
	uint16_t            format;
#endif
	uint8_t             channels;
	uint8_t             align;
} WAVESPEC;

typedef const void * (*SoundLoader)(LPCBYTE, DWORD, WAVESPEC *);
typedef int(*ResampleMixer)(void *[2], const void *, const WAVESPEC *, void *, int, const void **);

typedef struct tagWAVEDATA
{
	struct tagWAVEDATA *next;

	void               *resampler[2];	/* The resampler used for sound data */
	ResampleMixer       ResampleMix;
	const void         *base;
	const void         *current;
	const void         *end;
	WAVESPEC            spec;
} WAVEDATA;

typedef struct tagSOUNDPLAYER
{
	AUDIOPLAYER_COMMONS;

	FILE               *mkf;		/* File pointer to the MKF file */
	SoundLoader         LoadSound;	/* The function pointer for load WAVE/VOC data */
	WAVEDATA            soundlist;
	int                 cursounds;
} SOUNDPLAYER, *LPSOUNDPLAYER;

static const void *
SOUND_LoadWAVEData(
	LPCBYTE                lpData,
	DWORD                  dwLen,
	WAVESPEC              *lpSpec
)
/*++
  Purpose:

    Return the WAVE data pointer inside the input buffer.

  Parameters:

    [IN]  lpData - pointer to the buffer of the WAVE file.

    [IN]  dwLen - length of the buffer of the WAVE file.

    [OUT] lpSpec - pointer to the SDL_AudioSpec structure, which contains
                    some basic information about the WAVE file.

  Return value:

    Pointer to the WAVE data inside the input buffer, NULL if failed.
--*/
{
	const RIFFHeader      *lpRiff   = (const RIFFHeader *)lpData;
	const RIFFChunkHeader *lpChunk  = NULL;
	const WAVEFormatPCM   *lpFormat = NULL;
	const uint8_t         *lpWaveData = NULL;
	uint32_t len,type;

	if (dwLen < sizeof(RIFFHeader) || SDL_SwapLE32(lpRiff->signature) != RIFF_RIFF ||
		SDL_SwapLE32(lpRiff->type) != RIFF_WAVE || dwLen < SDL_SwapLE32(lpRiff->length) + 8)
	{
		return NULL;
	}

	lpChunk = (const RIFFChunkHeader *)(lpRiff + 1); dwLen -= sizeof(RIFFHeader);
	while (dwLen >= sizeof(RIFFChunkHeader))
	{
        len = SDL_SwapLE32(lpChunk->length);
        type = SDL_SwapLE32(lpChunk->type);
		if (dwLen >= sizeof(RIFFChunkHeader) + len)
			dwLen -= sizeof(RIFFChunkHeader) + len;
		else
			return NULL;

		switch (type)
		{
		case WAVE_fmt:
			lpFormat = (const WAVEFormatPCM *)(lpChunk + 1);
			if (len != sizeof(WAVEFormatPCM) || lpFormat->wFormatTag != SDL_SwapLE16(0x0001))
			{
				return NULL;
			}
			break;
		case WAVE_data:
			lpWaveData = (const uint8_t *)(lpChunk + 1);
			dwLen = 0;
			break;
		}
		lpChunk = (const RIFFChunkHeader *)((const uint8_t *)(lpChunk + 1) + len);
	}

	if (lpFormat == NULL || lpWaveData == NULL)
	{
		return NULL;
	}

	lpSpec->channels = SDL_SwapLE16(lpFormat->nChannels);
	lpSpec->format = (SDL_SwapLE16(lpFormat->wBitsPerSample) == 16) ? AUDIO_S16 : AUDIO_U8;
	lpSpec->freq = SDL_SwapLE32(lpFormat->nSamplesPerSec);
	lpSpec->size = len;
	lpSpec->align = SDL_SwapLE16(lpFormat->nChannels) * SDL_SwapLE16(lpFormat->wBitsPerSample) >> 3;

	return lpWaveData;
}

typedef struct tagVOCHEADER
{
	char    signature[0x14];	/* "Creative Voice File\x1A" */
	WORD    data_offset;		/* little endian */
	WORD	version;
	WORD	version_checksum;
} VOCHEADER, *LPVOCHEADER;
typedef const VOCHEADER *LPCVOCHEADER;

static const void *
SOUND_LoadVOCData(
	LPCBYTE                lpData,
	DWORD                  dwLen,
	WAVESPEC              *lpSpec
)
/*++
  Purpose:

    Return the VOC data pointer inside the input buffer. Currently supports type 01 block only.

  Parameters:

    [IN]  lpData - pointer to the buffer of the VOC file.

    [IN]  dwLen - length of the buffer of the VOC file.

    [OUT] lpSpec - pointer to the SDL_AudioSpec structure, which contains
                   some basic information about the VOC file.

  Return value:

    Pointer to the WAVE data inside the input buffer, NULL if failed.

    Reference: http://sox.sourceforge.net/AudioFormats-11.html
--*/
{
	LPCVOCHEADER lpVOC = (LPCVOCHEADER)lpData;

	if (dwLen < sizeof(VOCHEADER) || memcmp(lpVOC->signature, "Creative Voice File\x1A", 0x14) || SDL_SwapLE16(lpVOC->data_offset) >= dwLen)
	{
		return NULL;
	}

	lpData += SDL_SwapLE16(lpVOC->data_offset);
	dwLen -= SDL_SwapLE16(lpVOC->data_offset);

	while (dwLen && *lpData)
	{
		DWORD len;
		if (dwLen >= 4)
		{
			len = lpData[1] | (lpData[2] << 8) | (lpData[3] << 16);
			if (dwLen >= len + 4)
				dwLen -= len + 4;
			else
				return NULL;
		}
		else
		{
			return NULL;
		}
		if (*lpData == 0x01)
		{
			if (lpData[5] != 0) return NULL;	/* Only 8-bit is supported */

			lpSpec->format = AUDIO_U8;
			lpSpec->channels = 1;
			lpSpec->freq = ((1000000 / (256 - lpData[4]) + 99) / 100) * 100; /* Round to next 100Hz */
			lpSpec->size = len - 2;
			lpSpec->align = 1;

			return lpData + 6;
		}
		else
		{
			lpData += len + 4;
		}
	}

	return NULL;
}

static int
SOUND_ResampleMix_U8_Mono_Mono(
	void                  *resampler[2],
	const void            *lpData,
	const WAVESPEC        *lpSpec,
	void                  *lpBuffer,
	int                    iBufLen,
	const void           **llpData
)
/*++
  Purpose:

    Resample 8-bit unsigned mono PCM data into 16-bit signed (native-endian) mono PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
	int src_samples = lpSpec->size;
	const uint8_t * src = (const uint8_t *)lpData;
	short *dst = (short *)lpBuffer;
	int channel_len = iBufLen, total_bytes = 0;

	while (total_bytes < channel_len && src_samples > 0)
	{
		int j, to_write = resampler_get_free_count(resampler[0]);
		if (to_write > src_samples) to_write = src_samples;
		for (j = 0; j < to_write; j++)
			resampler_write_sample(resampler[0], (*src++ ^ 0x80) << 8);
		src_samples -= to_write;
		while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
		{
			int sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
			*dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
			total_bytes += sizeof(short);
			resampler_remove_sample(resampler[0]);
		}
	}

	if (llpData) *llpData = src;
	return total_bytes;
}

static int
SOUND_ResampleMix_U8_Mono_Stereo(
	void                  *resampler[2],
	const void            *lpData,
	const WAVESPEC        *lpSpec,
	void                  *lpBuffer,
	int                    iBufLen,
	const void           **llpData
)
/*++
  Purpose:

    Resample 8-bit unsigned mono PCM data into 16-bit signed (native-endian) stereo PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
	int src_samples = lpSpec->size;
	const uint8_t * src = (const uint8_t *)lpData;
	short *dst = (short *)lpBuffer;
	int channel_len = iBufLen >> 1, total_bytes = 0;

	while (total_bytes < channel_len && src_samples > 0)
	{
		int j, to_write = resampler_get_free_count(resampler[0]);
		if (to_write > src_samples) to_write = src_samples;
		for (j = 0; j < to_write; j++)
			resampler_write_sample(resampler[0], (*src++ ^ 0x80) << 8);
		src_samples -= to_write;
		while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
		{
			int sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
			dst[0] = dst[1] = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
			total_bytes += sizeof(short); dst += 2;
			resampler_remove_sample(resampler[0]);
		}
	}

	if (llpData) *llpData = src;
	return total_bytes;
}

static int
SOUND_ResampleMix_U8_Stereo_Mono(
	void                  *resampler[2],
	const void            *lpData,
	const WAVESPEC        *lpSpec,
	void                  *lpBuffer,
	int                    iBufLen,
	const void           **llpData
)
/*++
  Purpose:

    Resample 8-bit unsigned stereo PCM data into 16-bit signed (native-endian) mono PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
	int src_samples = lpSpec->size >> 1;
	const uint8_t * src = (const uint8_t *)lpData;
	short *dst = (short *)lpBuffer;
	int channel_len = iBufLen, total_bytes = 0;

	while (total_bytes < channel_len && src_samples > 0)
	{
		int j, to_write = resampler_get_free_count(resampler[0]);
		if (to_write > src_samples) to_write = src_samples;
		for (j = 0; j < to_write; j++)
		{
			resampler_write_sample(resampler[0], (*src++ ^ 0x80) << 8);
			resampler_write_sample(resampler[1], (*src++ ^ 0x80) << 8);
		}
		src_samples -= to_write;
		while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
		{
			int sample = (((resampler_get_sample(resampler[0]) >> 8) + (resampler_get_sample(resampler[1]) >> 8)) >> 1) + *dst;
			*dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
			total_bytes += sizeof(short);
			resampler_remove_sample(resampler[0]);
			resampler_remove_sample(resampler[1]);
		}
	}

	if (llpData) *llpData = src;
	return total_bytes;
}

static int
SOUND_ResampleMix_U8_Stereo_Stereo(
	void                  *resampler[2],
	const void            *lpData,
	const WAVESPEC        *lpSpec,
	void                  *lpBuffer,
	int                    iBufLen,
	const void           **llpData
)
/*++
  Purpose:

    Resample 8-bit unsigned stereo PCM data into 16-bit signed (native-endian) stereo PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
	int src_samples = lpSpec->size >> 1;
	const uint8_t * src = (const uint8_t *)lpData;
	short *dst = (short *)lpBuffer;
	int channel_len = iBufLen >> 1, total_bytes = 0;

	while (total_bytes < channel_len && src_samples > 0)
	{
		int j, to_write = resampler_get_free_count(resampler[0]);
		if (to_write > src_samples) to_write = src_samples;
		for (j = 0; j < to_write; j++)
		{
			resampler_write_sample(resampler[0], (*src++ ^ 0x80) << 8);
			resampler_write_sample(resampler[1], (*src++ ^ 0x80) << 8);
		}
		src_samples -= to_write;
		while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
		{
			int sample;
			sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
			*dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
			sample = (resampler_get_sample(resampler[1]) >> 8) + *dst;
			*dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
			total_bytes += sizeof(short);
			resampler_remove_sample(resampler[0]);
			resampler_remove_sample(resampler[1]);
		}
	}

	if (llpData) *llpData = src;
	return total_bytes;
}

static int
SOUND_ResampleMix_S16_Mono_Mono(
	void                  *resampler[2],
	const void            *lpData,
	const WAVESPEC        *lpSpec,
	void                  *lpBuffer,
	int                    iBufLen,
	const void           **llpData
)
/*++
  Purpose:

    Resample 16-bit signed (little-endian) mono PCM data into 16-bit signed (native-endian) mono PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
	int src_samples = lpSpec->size >> 1;
	const short * src = (const short *)lpData;
	short *dst = (short *)lpBuffer;
	int channel_len = iBufLen, total_bytes = 0;

	while (total_bytes < channel_len && src_samples > 0)
	{
		int j, to_write = resampler_get_free_count(resampler[0]);
		if (to_write > src_samples) to_write = src_samples;
		for (j = 0; j < to_write; j++)
			resampler_write_sample(resampler[0], SDL_SwapLE16(*src++));
		src_samples -= to_write;
		while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
		{
			int sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
			*dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
			total_bytes += sizeof(short);
			resampler_remove_sample(resampler[0]);
		}
	}

	if (llpData) *llpData = src;
	return total_bytes;
}

static int
SOUND_ResampleMix_S16_Mono_Stereo(
	void                  *resampler[2],
	const void            *lpData,
	const WAVESPEC        *lpSpec,
	void                  *lpBuffer,
	int                    iBufLen,
	const void           **llpData
)
/*++
  Purpose:

    Resample 16-bit signed (little-endian) mono PCM data into 16-bit signed (native-endian) stereo PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
	int src_samples = lpSpec->size >> 1;
	const short * src = (const short *)lpData;
	short *dst = (short *)lpBuffer;
	int channel_len = iBufLen >> 1, total_bytes = 0;

	while (total_bytes < channel_len && src_samples > 0)
	{
		int j, to_write = resampler_get_free_count(resampler[0]);
		if (to_write > src_samples) to_write = src_samples;
		for (j = 0; j < to_write; j++)
			resampler_write_sample(resampler[0], SDL_SwapLE16(*src++));
		src_samples -= to_write;
		while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
		{
			int sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
			dst[0] = dst[1] = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
			total_bytes += sizeof(short); dst += 2;
			resampler_remove_sample(resampler[0]);
		}
	}

	if (llpData) *llpData = src;
	return total_bytes;
}

static int
SOUND_ResampleMix_S16_Stereo_Mono(
	void                  *resampler[2],
	const void            *lpData,
	const WAVESPEC        *lpSpec,
	void                  *lpBuffer,
	int                    iBufLen,
	const void           **llpData
)
/*++
  Purpose:

    Resample 16-bit signed (little-endian) stereo PCM data into 16-bit signed (native-endian) mono PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
	int src_samples = lpSpec->size >> 2;
	const short * src = (const short *)lpData;
	short *dst = (short *)lpBuffer;
	int channel_len = iBufLen, total_bytes = 0;

	while (total_bytes < channel_len && src_samples > 0)
	{
		int j, to_write = resampler_get_free_count(resampler[0]);
		if (to_write > src_samples) to_write = src_samples;
		for (j = 0; j < to_write; j++)
		{
			resampler_write_sample(resampler[0], SDL_SwapLE16(*src++));
			resampler_write_sample(resampler[1], SDL_SwapLE16(*src++));
		}
		src_samples -= to_write;
		while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
		{
			int sample = (((resampler_get_sample(resampler[0]) >> 8) + (resampler_get_sample(resampler[1]) >> 8)) >> 1) + *dst;
			*dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
			total_bytes += sizeof(short);
			resampler_remove_sample(resampler[0]);
			resampler_remove_sample(resampler[1]);
		}
	}

	if (llpData) *llpData = src;
	return total_bytes;
}

static int
SOUND_ResampleMix_S16_Stereo_Stereo(
	void                  *resampler[2],
	const void            *lpData,
	const WAVESPEC        *lpSpec,
	void                  *lpBuffer,
	int                    iBufLen,
	const void           **llpData
)
/*++
  Purpose:

    Resample 16-bit signed (little-endian) stereo PCM data into 16-bit signed (native-endian) stereo PCM data.

  Parameters:

    [IN]  resampler - array of pointers to the resampler instance.

    [IN]  lpData - pointer to the buffer of the input PCM data.

    [IN]  lpSpec - pointer to the WAVESPEC structure, which contains
                   some basic information about the input PCM data.

    [IN]  lpBuffer - pointer of the buffer of the output PCM data.

    [IN]  iBufLen - length of the buffer of the output PCM data.

    [OUT] llpData - pointer to receive the pointer of remaining input PCM data.

  Return value:

    The number of output buffer used, in bytes.
--*/
{
	int src_samples = lpSpec->size >> 2;
	const short * src = (const short *)lpData;
	short *dst = (short *)lpBuffer;
	int channel_len = iBufLen >> 1, total_bytes = 0;

	while (total_bytes < channel_len && src_samples > 0)
	{
		int j, to_write = resampler_get_free_count(resampler[0]);
		if (to_write > src_samples) to_write = src_samples;
		for (j = 0; j < to_write; j++)
		{
			resampler_write_sample(resampler[0], SDL_SwapLE16(*src++));
			resampler_write_sample(resampler[1], SDL_SwapLE16(*src++));
		}
		src_samples -= to_write;
		while (total_bytes < channel_len && resampler_get_sample_count(resampler[0]) > 0)
		{
			int sample;
			sample = (resampler_get_sample(resampler[0]) >> 8) + *dst;
			*dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
			sample = (resampler_get_sample(resampler[1]) >> 8) + *dst;
			*dst++ = (sample <= 32767) ? ((sample >= -32768) ? sample : -32768) : 32767;
			total_bytes += sizeof(short);
			resampler_remove_sample(resampler[0]);
			resampler_remove_sample(resampler[1]);
		}
	}

	if (llpData) *llpData = src;
	return total_bytes;
}


static BOOL
SOUND_Play(
   VOID  *object,
   INT    iSoundNum,
   BOOL   fLoop,
   FLOAT  flFadeTime
)
/*++
  Purpose:

    Play a sound in voc.mkf/sounds.mkf file.

  Parameters:

    [IN]  object - Pointer to the SOUNDPLAYER instance.
    [IN]  iSoundNum - number of the sound; the absolute value is used.
    [IN]  fLoop - Not used, should be zero.
    [IN]  flFadeTime - Not used, should be zero.

  Return value:

    None.

--*/
{
	LPSOUNDPLAYER  player = (LPSOUNDPLAYER)object;
	const SDL_AudioSpec *devspec = AUDIO_GetDeviceSpec();
	WAVESPEC         wavespec;
	ResampleMixer    mixer;
	WAVEDATA        *cursnd;
	void            *buf;
	const void      *snddata;
	int              len, i;

	//
	// Check for NULL pointer.
	//
	if (player == NULL)
	{
		return FALSE;
	}

	//
	// Get the length of the sound file.
	//
	len = PAL_MKFGetChunkSize(iSoundNum, player->mkf);
	if (len <= 0)
	{
		return FALSE;
	}

	buf = malloc(len);
	if (buf == NULL)
	{
		return FALSE;
	}

	//
	// Read the sound file from the MKF archive.
	//
	PAL_MKFReadChunk(buf, len, iSoundNum, player->mkf);

	snddata = player->LoadSound(buf, len, &wavespec);
	if (snddata == NULL)
	{
		free(buf);
		return FALSE;
	}

	if (wavespec.channels == 1 && devspec->channels == 1)
		mixer = (wavespec.format == AUDIO_S16) ? SOUND_ResampleMix_S16_Mono_Mono : SOUND_ResampleMix_U8_Mono_Mono;
	else if (wavespec.channels == 1 && devspec->channels == 2)
		mixer = (wavespec.format == AUDIO_S16) ? SOUND_ResampleMix_S16_Mono_Stereo : SOUND_ResampleMix_U8_Mono_Stereo;
	else if (wavespec.channels == 2 && devspec->channels == 1)
		mixer = (wavespec.format == AUDIO_S16) ? SOUND_ResampleMix_S16_Stereo_Mono : SOUND_ResampleMix_U8_Stereo_Mono;
	else if (wavespec.channels == 2 && devspec->channels == 2)
		mixer = (wavespec.format == AUDIO_S16) ? SOUND_ResampleMix_S16_Stereo_Stereo : SOUND_ResampleMix_U8_Stereo_Stereo;
	else
	{
		free(buf);
		return FALSE;
	}

	AUDIO_Lock();

	cursnd = &player->soundlist;
	while (cursnd->next && cursnd->base)
		cursnd = cursnd->next;
	if (cursnd->base)
	{
		WAVEDATA *obj = (WAVEDATA *)malloc(sizeof(WAVEDATA));
		memset(obj, 0, sizeof(WAVEDATA));
		cursnd->next = obj;
		cursnd = cursnd->next;
	}

	for (i = 0; i < wavespec.channels; i++)
	{
		if (!cursnd->resampler[i])
			cursnd->resampler[i] = resampler_create();
		else
			resampler_clear(cursnd->resampler[i]);
		resampler_set_quality(cursnd->resampler[i], AUDIO_IsIntegerConversion(wavespec.freq) ? RESAMPLER_QUALITY_MIN : gConfig.iResampleQuality);
		resampler_set_rate(cursnd->resampler[i], (double)wavespec.freq / (double)devspec->freq);
	}

	cursnd->base = buf;
	cursnd->current = snddata;
	cursnd->end = (const uint8_t *)snddata + wavespec.size;
	cursnd->spec = wavespec;
	cursnd->ResampleMix = mixer;
	player->cursounds++;

	AUDIO_Unlock();

	return TRUE;
}

VOID
SOUND_Shutdown(
	VOID     *object
)
/*++
  Purpose:

    Shutdown the sound subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
	LPSOUNDPLAYER player = (LPSOUNDPLAYER)object;
	if (player)
	{
		WAVEDATA *cursnd = &player->soundlist;
		do
		{
			if (cursnd->resampler[0]) resampler_delete(cursnd->resampler[0]);
			if (cursnd->resampler[1]) resampler_delete(cursnd->resampler[1]);
			if (cursnd->base) free((void *)cursnd->base);
		} while ((cursnd = cursnd->next) != NULL);
		cursnd = player->soundlist.next;
		while (cursnd)
		{
			WAVEDATA *old = cursnd;
			cursnd = cursnd->next;
			free(old);
		}
		if (player->mkf) fclose(player->mkf);
	}
}

static VOID
SOUND_FillBuffer(
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
	LPSOUNDPLAYER player = (LPSOUNDPLAYER)object;
	if (player)
	{
		WAVEDATA *cursnd = &player->soundlist;
		int sounds = 0;
		do
		{
			if (cursnd->base)
			{
				cursnd->ResampleMix(cursnd->resampler, cursnd->current, &cursnd->spec, stream, len, &cursnd->current);
				cursnd->spec.size = (const uint8_t *)cursnd->end - (const uint8_t *)cursnd->current;
				if (cursnd->spec.size < cursnd->spec.align)
				{
					free((void *)cursnd->base);
					cursnd->base = cursnd->current = cursnd->end = NULL;
					player->cursounds--;
				}
				else
					sounds++;
			}
		} while ((cursnd = cursnd->next) && sounds < player->cursounds);
	}
}

LPAUDIOPLAYER
SOUND_Init(
	VOID
)
/*++
  Purpose:

    Initialize the sound subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
	char *mkfs[2];
	SoundLoader func[2];
	int i;

	if (gConfig.fIsWIN95)
	{
		mkfs[0] = "sounds.mkf"; func[0] = SOUND_LoadWAVEData;
		mkfs[1] = "voc.mkf"; func[1] = SOUND_LoadVOCData;
	}
	else
	{
		mkfs[0] = "voc.mkf"; func[0] = SOUND_LoadVOCData;
		mkfs[1] = "sounds.mkf"; func[1] = SOUND_LoadWAVEData;
	}

	for (i = 0; i < 2; i++)
	{
		FILE *mkf = UTIL_OpenFile(mkfs[i]);
		if (mkf)
		{
			LPSOUNDPLAYER player = (LPSOUNDPLAYER)malloc(sizeof(SOUNDPLAYER));
			memset(&player->soundlist, 0, sizeof(WAVEDATA));
			player->Play = SOUND_Play;
			player->FillBuffer = SOUND_FillBuffer;
			player->Shutdown = SOUND_Shutdown;

			player->LoadSound = func[i];
			player->mkf = mkf;
			player->soundlist.resampler[0] = resampler_create();
			player->soundlist.resampler[1] = resampler_create();
			player->cursounds = 0;
			return (LPAUDIOPLAYER)player;
		}
	}

	return NULL;
}
