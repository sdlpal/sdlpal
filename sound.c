/* -*- mode: c; tab-width: 4; c-basic-offset: 3; c-file-style: "linux" -*- */
//
// Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
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
#include "sound.h"
#include "players.h"
#include "util.h"
#include "resampler.h"
#include "midi.h"
#include <math.h>

#if PAL_HAS_OGG
#include <vorbis/codec.h>
#endif

static BOOL  gSndOpened = FALSE;

BOOL         g_fNoSound = FALSE;
BOOL         g_fNoMusic = FALSE;

int          g_iCurrChannel = 0;

#define PAL_CDTRACK_BASE    10000

typedef LPCBYTE(*FNLoadSoundData)(LPCBYTE, DWORD, SDL_AudioSpec *);

typedef struct tagSNDPLAYER
{
   FILE                     *mkf;
   SDL_AudioSpec             spec;
   SDL_mutex                *mtx;
   LPBYTE                    buf[2], pos[2];
   INT                       audio_len[2];
   void                     *resampler;
   MUSICPLAYER              *pMusPlayer;
   MUSICPLAYER              *pCDPlayer;
#if PAL_HAS_SDLCD
   SDL_CD                   *pCD;
#endif
   FNLoadSoundData           LoadSoundData;
} SNDPLAYER;

static SNDPLAYER gSndPlayer;

typedef struct tagRIFFHEADER
{
	DWORD   riff_sig;	/* 'RIFF' */
	DWORD   data_length;	/* Total length minus eight, little-endian */
	DWORD   riff_type;	/* 'WAVE' */
} RIFFHEADER, *LPRIFFHEADER;
typedef const RIFFHEADER *LPCRIFFHEADER;

typedef struct tagRIFFCHUNK
{
	DWORD   chunk_type;	/* 'fmt ' and so on */
	DWORD   chunk_length;	/* Total chunk length minus eight, little-endian */
} RIFFCHUNK, *LPRIFFCHUNK;
typedef const RIFFCHUNK *LPCRIFFCHUNK;

typedef struct tagWAVEFORMATPCM
{
	WORD    wFormatTag;        /* format type */
	WORD    nChannels;         /* number of channels (i.e. mono, stereo, etc.) */
	DWORD   nSamplesPerSec;    /* sample rate */
	DWORD   nAvgBytesPerSec;   /* for buffer estimation */
	WORD    nBlockAlign;       /* block size of data */
	WORD    wBitsPerSample;
} WAVEFORMATPCM, *LPWAVEFORMATPCM;
typedef const WAVEFORMATPCM *LPCWAVEFORMATPCM;

static LPCBYTE
SOUND_LoadWAVEData(
	LPCBYTE                lpData,
	DWORD                  dwLen,
	SDL_AudioSpec         *lpSpec
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
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#	define RIFF		'RIFF'
#	define WAVE		'WAVE'
#	define FMT		'fmt '
#	define DATA		'data'
#	define PCM     0x0100
#else
#	define RIFF		'FFIR'
#	define WAVE		'EVAW'
#	define FMT		' tmf'
#	define DATA		'atad'
#	define PCM     0x0001
#endif
	LPCRIFFHEADER lpRiff = (LPCRIFFHEADER)lpData;
	LPCRIFFCHUNK lpChunk;
	LPCWAVEFORMATPCM lpFormat = NULL;
	LPCBYTE lpWaveData = NULL;
	DWORD len;

	if (dwLen < sizeof(RIFFHEADER) || lpRiff->riff_sig != RIFF || lpRiff->riff_type != WAVE || dwLen < SDL_SwapLE32(lpRiff->data_length) + 8)
	{
		return NULL;
	}

	lpChunk = (LPCRIFFCHUNK)(lpRiff + 1); dwLen -= sizeof(RIFFHEADER);
	while (dwLen >= sizeof(RIFFCHUNK))
	{
		len = SDL_SwapLE32(lpChunk->chunk_length);
		if (dwLen >= sizeof(RIFFCHUNK) + len)
			dwLen -= sizeof(RIFFCHUNK) + len;
		else
			return NULL;

		switch (lpChunk->chunk_type)
		{
		case FMT:
			lpFormat = (LPCWAVEFORMATPCM)(lpChunk + 1);
			if (len != sizeof(WAVEFORMATPCM) || lpFormat->wFormatTag != PCM)
			{
				return NULL;
			}
			break;
		case DATA:
			lpWaveData = (LPCBYTE)(lpChunk + 1);
			dwLen = 0;
			break;
		}
		lpChunk = (LPCRIFFCHUNK)((LPCBYTE)(lpChunk + 1) + len);
	}

	if (lpFormat == NULL || lpWaveData == NULL)
	{
		return NULL;
	}

	lpSpec->channels = lpFormat->nChannels;
	lpSpec->format = (lpFormat->wBitsPerSample == 16) ? AUDIO_S16 : AUDIO_U8;
	lpSpec->freq = lpFormat->nSamplesPerSec;
	lpSpec->size = len;

	return lpWaveData;

#undef RIFF
#undef WAVE
#undef FMT
}

typedef struct tagVOCHEADER
{
	char    signature[0x14];	/* "Creative Voice File\x1A" */
	WORD    data_offset;		/* little endian */
	WORD	version;
	WORD	version_checksum;
} VOCHEADER, *LPVOCHEADER;
typedef const VOCHEADER *LPCVOCHEADER;

static LPCBYTE
SOUND_LoadVOCData(
	LPCBYTE                lpData,
	DWORD                  dwLen,
	SDL_AudioSpec         *lpSpec
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

			return lpData + 6;
		}
		else
		{
			lpData += len + 4;
		}
	}

	return NULL;
}

static void
SOUND_ResampleU8(
	LPCBYTE                lpData,
	const SDL_AudioSpec   *lpSpec,
	LPBYTE                 lpBuffer,
	DWORD                  dwLen,
	void                  *resampler
	)
/*++
	Purpose:

	Resample 8-bit unsigned PCM data into 16-bit signed (little-endian) PCM data.

	Parameters:

	[IN]  lpData - pointer to the buffer of the input PCM data.

	[IN]  lpSpec - pointer to the SDL_AudioSpec structure, which contains
                   some basic information about the input PCM data.

	[IN]  lpBuffer - pointer of the buffer of the output PCM data.

	[IN]  dwLen - length of the buffer of the output PCM data, should be exactly
                  the number of bytes needed of the resampled data.

	[IN]  resampler - pointer of the resampler instance.

	Return value:

	None.
--*/
{
	int src_samples = lpSpec->size / lpSpec->channels, i;

	for (i = 0; i < lpSpec->channels; i++)
	{
		LPCBYTE src = lpData + i;
		short *dst = (short *)lpBuffer + i;
		int channel_len = dwLen / lpSpec->channels, total_bytes = 0;

		resampler_clear(resampler);
		while (total_bytes < channel_len && src_samples > 0)
		{
			int to_write, j;
			to_write = resampler_get_free_count(resampler);
			if (to_write > src_samples) to_write = src_samples;
			for (j = 0; j < to_write; j++)
			{
				resampler_write_sample(resampler, (*src ^ 0x80) << 8);
				src += lpSpec->channels;
			}
			src_samples -= to_write;
			while (total_bytes < channel_len && resampler_get_sample_count(resampler) > 0)
			{
				*dst = SDL_SwapLE16(resampler_get_and_remove_sample(resampler));
				dst += lpSpec->channels; total_bytes += (SDL_AUDIO_BITSIZE(AUDIO_S16) >> 3);
			}
		}
		/* Flush resampler's output buffer */
		while (total_bytes < channel_len)
		{
			int j, to_write = resampler_get_free_count(resampler);
			for (j = 0; j < to_write; j++)
				resampler_write_sample(resampler, (src[-lpSpec->channels] ^ 0x80) << 8);
			while (total_bytes < channel_len && resampler_get_sample_count(resampler) > 0)
			{
				*dst = SDL_SwapLE16(resampler_get_and_remove_sample(resampler));
				dst += lpSpec->channels; total_bytes += (SDL_AUDIO_BITSIZE(AUDIO_S16) >> 3);
			}
		}
	}
}

static void
SOUND_ResampleS16(
	LPCBYTE                lpData,
	const SDL_AudioSpec   *lpSpec,
	LPBYTE                 lpBuffer,
	DWORD                  dwLen,
	void                  *resampler
	)
/*++
	Purpose:

	Resample 16-bit signed (little-endian) PCM data into 16-bit signed (little-endian) PCM data.

	Parameters:

	[IN]  lpData - pointer to the buffer of the input PCM data.

	[IN]  lpSpec - pointer to the SDL_AudioSpec structure, which contains
                   some basic information about the input PCM data.

	[IN]  lpBuffer - pointer of the buffer of the output PCM data.

	[IN]  dwLen - length of the buffer of the output PCM data, should be exactly
                  the number of bytes needed of the resampled data.

	[IN]  resampler - pointer of the resampler instance.

	Return value:

	None.
--*/
{
	int src_samples = lpSpec->size / lpSpec->channels / 2, i;

	for (i = 0; i < lpSpec->channels; i++)
	{
		const short *src = (short *)lpData + i;
		short *dst = (short *)lpBuffer + i;
		int channel_len = dwLen / lpSpec->channels, total_bytes = 0;

		resampler_clear(resampler);
		while (total_bytes < channel_len && src_samples > 0)
		{
			int to_write, j;
			to_write = resampler_get_free_count(resampler);
			if (to_write > src_samples) to_write = src_samples;
			for (j = 0; j < to_write; j++)
			{
				resampler_write_sample(resampler, SDL_SwapLE16(*src));
				src += lpSpec->channels;
			}
			src_samples -= to_write;
			while (total_bytes < channel_len && resampler_get_sample_count(resampler) > 0)
			{
				*dst = SDL_SwapLE16(resampler_get_and_remove_sample(resampler));
				dst += lpSpec->channels; total_bytes += (SDL_AUDIO_BITSIZE(AUDIO_S16) >> 3);
			}
		}
		/* Flush resampler's output buffer */
		while (total_bytes < channel_len)
		{
			int j, to_write = resampler_get_free_count(resampler);
			short val = SDL_SwapLE16(src[-lpSpec->channels]);
			for (j = 0; j < to_write; j++)
				resampler_write_sample(resampler, val);
			while (total_bytes < channel_len && resampler_get_sample_count(resampler) > 0)
			{
				*dst = SDL_SwapLE16(resampler_get_and_remove_sample(resampler));
				dst += lpSpec->channels; total_bytes += (SDL_AUDIO_BITSIZE(AUDIO_S16) >> 3);
			}
		}
	}
}

static VOID SDLCALL
SOUND_FillAudio(
   LPVOID          udata,
   LPBYTE          stream,
   INT             len
)
/*++
  Purpose:

    SDL sound callback function.

  Parameters:

    [IN]  udata - pointer to user-defined parameters (Not used).

    [OUT] stream - pointer to the stream buffer.

    [IN]  len - Length of the buffer.

  Return value:

    None.

--*/
{
   int        i;

#if SDL_VERSION_ATLEAST(2,0,0)
   memset(stream, 0, len);
#endif

   //
   // Play music
   //
   if (!g_fNoMusic)
   {
	   SDL_mutexP(gSndPlayer.mtx);
	   if (gSndPlayer.pMusPlayer)
	   {
		   gSndPlayer.pMusPlayer->FillBuffer(gSndPlayer.pMusPlayer, stream, len);
	   }

	   if (gSndPlayer.pCDPlayer)
	   {
		   gSndPlayer.pCDPlayer->FillBuffer(gSndPlayer.pCDPlayer, stream, len);
	   }
	   SDL_mutexV(gSndPlayer.mtx);
   }

   //
   // No current playing sound
   //
   if (g_fNoSound)
   {
      return;
   }

   SDL_mutexP(gSndPlayer.mtx);

   for (i = 0; i < 2; i++)
   {
      //
      // Only play if we have data left
      //
      if (gSndPlayer.buf[i] == NULL)
      {
         continue;
      }

      if (gSndPlayer.audio_len[i] == 0)
      {
         //
         // Delete the audio buffer from memory
         //
         free(gSndPlayer.buf[i]);
         gSndPlayer.buf[i] = NULL;
         continue;
      }

      //
      // Mix as much data as possible
      //
      len = (len > gSndPlayer.audio_len[i]) ? gSndPlayer.audio_len[i] : len;
      SDL_MixAudio(stream, gSndPlayer.pos[i], len, gConfig.iVolume);
      gSndPlayer.pos[i] += len;
      gSndPlayer.audio_len[i] -= len;
   }
   SDL_mutexV(gSndPlayer.mtx);
}

static VOID
SOUND_LoadMKF(
	VOID
	)
/*++
  Purpose:

    Load MKF contents into memory.

  Parameters:

    None.

  Return value:

    None.

--*/
{
	char *mkfs[2];
	FNLoadSoundData func[2];
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
		gSndPlayer.mkf = UTIL_OpenFile(mkfs[i]);
		if (gSndPlayer.mkf)
		{
			gSndPlayer.LoadSoundData = func[i];
			break;
		}
	}
}

INT
SOUND_OpenAudio(
   VOID
)
/*++
  Purpose:

    Initialize the audio subsystem.

  Parameters:

    None.

  Return value:

    0 if succeed, others if failed.

--*/
{
   SDL_AudioSpec spec;

   if (gSndOpened)
   {
      //
      // Already opened
      //
      return -1;
   }

   gSndOpened = FALSE;

   //
   // Load the MKF file.
   //
   SOUND_LoadMKF();
   if (gSndPlayer.mkf == NULL)
   {
      return -2;
   }

   //
   // Initialize the resampler
   //
   resampler_init();
   gSndPlayer.resampler = resampler_create();

   //
   // Open the sound subsystem.
   //
   gSndPlayer.spec.freq = gConfig.iSampleRate;
   gSndPlayer.spec.format = AUDIO_S16;
   gSndPlayer.spec.channels = gConfig.iAudioChannels;
   gSndPlayer.spec.samples = gConfig.wAudioBufferSize;
   gSndPlayer.spec.callback = SOUND_FillAudio;

   if (SDL_OpenAudio(&gSndPlayer.spec, &spec) < 0)
   {
      //
      // Failed
      //
      return -3;
   }

   memcpy(&gSndPlayer.spec, &spec, sizeof(SDL_AudioSpec));

   gSndPlayer.buf[0] = NULL;
   gSndPlayer.pos[0] = NULL;
   gSndPlayer.audio_len[0] = 0;

   gSndPlayer.buf[1] = NULL;
   gSndPlayer.pos[1] = NULL;
   gSndPlayer.audio_len[1] = 0;

   gSndPlayer.mtx = SDL_CreateMutex();
   gSndOpened = TRUE;

   //
   // Initialize the music subsystem.
   //
   switch (gConfig.eMusicType)
   {
   case MUSIC_RIX:
	   if (!(gSndPlayer.pMusPlayer = RIX_Init(va("%s%s", PAL_PREFIX, "mus.mkf"))))
	   {
		   gSndPlayer.pMusPlayer = RIX_Init(va("%s%s", PAL_PREFIX, "MUS.MKF"));
	   }
	   break;
   case MUSIC_MP3:
#if PAL_HAS_MP3
	   gSndPlayer.pMusPlayer = MP3_Init(NULL);
#else
	   gSndPlayer.pMusPlayer = NULL;
#endif
	   break;
   case MUSIC_OGG:
#if PAL_HAS_OGG
	   gSndPlayer.pMusPlayer = OGG_Init(NULL);
#else
	   gSndPlayer.pMusPlayer = NULL;
#endif
	   break;
   case MUSIC_MIDI:
	   gSndPlayer.pMusPlayer = NULL;
	   break;
   }

   //
   // Initialize the CD audio.
   //
   switch (gConfig.eCDType)
   {
   case MUSIC_SDLCD:
   {
#if PAL_HAS_SDLCD
	   int i;
	   gSndPlayer.pCD = NULL;

	   for (i = 0; i < SDL_CDNumDrives(); i++)
	   {
		   gSndPlayer.pCD = SDL_CDOpen(i);
		   if (gSndPlayer.pCD != NULL)
		   {
			   if (!CD_INDRIVE(SDL_CDStatus(gSndPlayer.pCD)))
			   {
				   SDL_CDClose(gSndPlayer.pCD);
				   gSndPlayer.pCD = NULL;
			   }
			   else
			   {
				   break;
			   }
		   }
	   }
#endif
	   gSndPlayer.pCDPlayer = NULL;
	   break;
   }
   case MUSIC_MP3:
#if PAL_HAS_MP3
	   gSndPlayer.pCDPlayer = MP3_Init(NULL);
#else
	   gSndPlayer.pCDPlayer = NULL;
#endif
	   break;
   case MUSIC_OGG:
#if PAL_HAS_OGG
	   gSndPlayer.pCDPlayer = OGG_Init(NULL);
#else
	   gSndPlayer.pCDPlayer = NULL;
#endif
	   break;
   }

   //
   // Let the callback function run so that musics will be played.
   //
   SDL_PauseAudio(0);

   return 0;
}

VOID
SOUND_CloseAudio(
   VOID
)
/*++
  Purpose:

    Close the audio subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   SDL_CloseAudio();

   SDL_mutexP(gSndPlayer.mtx);

   if (gSndPlayer.buf[0] != NULL)
   {
      free(gSndPlayer.buf[0]);
      gSndPlayer.buf[0] = NULL;
   }

   if (gSndPlayer.buf[1] != NULL)
   {
      free(gSndPlayer.buf[1]);
      gSndPlayer.buf[1] = NULL;
   }

   if (gSndPlayer.mkf != NULL)
   {
      fclose(gSndPlayer.mkf);
      gSndPlayer.mkf = NULL;
   }

   if (gSndPlayer.pMusPlayer)
   {
	   gSndPlayer.pMusPlayer->Shutdown(gSndPlayer.pMusPlayer);
	   gSndPlayer.pMusPlayer = NULL;
   }

   if (gSndPlayer.pCDPlayer)
   {
	   gSndPlayer.pCDPlayer->Shutdown(gSndPlayer.pCDPlayer);
	   gSndPlayer.pCDPlayer = NULL;
   }

#if PAL_HAS_SDLCD
   if (gSndPlayer.pCD != NULL)
   {
      SOUND_PlayCDA(-1);
      SDL_CDClose(gSndPlayer.pCD);
   }
#endif

   if (gConfig.eMusicType == MUSIC_MIDI) MIDI_Play(0, FALSE);

   if (gSndPlayer.resampler)
   {
      resampler_delete(gSndPlayer.resampler);
	  gSndPlayer.resampler = NULL;
   }

   SDL_mutexV(gSndPlayer.mtx);
   SDL_DestroyMutex(gSndPlayer.mtx);
}

SDL_AudioSpec*
SOUND_GetAudioSpec(
	VOID
)
{
	return &gSndPlayer.spec;
}

VOID
SOUND_AdjustVolume(
   INT    iDirection
)
/*++
  Purpose:

    SDL sound volume adjust function.

  Parameters:

    [IN]  iDirection - value, Increase (>0) or decrease (<=0) 3% volume.

  Return value:

    None.

--*/
{
   if (iDirection > 0)
   {
      gConfig.iVolume += SDL_MIX_MAXVOLUME * 0.03;
      if (gConfig.iVolume > SDL_MIX_MAXVOLUME)
      {
		  gConfig.iVolume = SDL_MIX_MAXVOLUME;
      }
   }
   else
   {
      gConfig.iVolume -= SDL_MIX_MAXVOLUME * 0.03;
      if (gConfig.iVolume < 0)
      {
		  gConfig.iVolume = 0;
      }
   }
}

VOID
SOUND_PlayChannel(
   INT    iSoundNum,
   INT    iChannel
)
/*++
  Purpose:

    Play a sound in voc.mkf file.

  Parameters:

    [IN]  iSoundNum - number of the sound.

    [IN]  iChannel - the number of channel (0 or 1).

  Return value:

    None.

--*/
{
   SDL_AudioCVT    wavecvt;
   SDL_AudioSpec   wavespec;
   LPBYTE          buf, bufdec;
   LPCBYTE         bufsrc;
   int             len;

   if (!gSndOpened || g_fNoSound)
   {
      return;
   }

   //
   // Stop playing current sound.
   //
   SDL_mutexP(gSndPlayer.mtx);
   if (gSndPlayer.buf[iChannel] != NULL)
   {
      free(gSndPlayer.buf[iChannel]);
      gSndPlayer.buf[iChannel] = NULL;
   }
   SDL_mutexV(gSndPlayer.mtx);

   if (iSoundNum < 0)
   {
      return;
   }

   //
   // Get the length of the sound file.
   //
   len = PAL_MKFGetChunkSize(iSoundNum, gSndPlayer.mkf);
   if (len <= 0)
   {
      return;
   }

   buf = (LPBYTE)malloc(len);
   if (buf == NULL)
   {
      return;
   }

   //
   // Read the sound file from the MKF archive.
   //
   PAL_MKFReadChunk(buf, len, iSoundNum, gSndPlayer.mkf);

   bufsrc = gSndPlayer.LoadSoundData(buf, len, &wavespec);
   if (bufsrc == NULL)
   {
	   free(buf);
	   return;
   }

   if (wavespec.freq != gSndPlayer.spec.freq)
   {
	   /* Resampler is needed */
	   resampler_set_quality(gSndPlayer.resampler, SOUND_IsIntegerConversion(wavespec.freq) ? RESAMPLER_QUALITY_MIN : gConfig.iResampleQuality);
	   resampler_set_rate(gSndPlayer.resampler, (double)wavespec.freq / (double)gSndPlayer.spec.freq);
	   len = (int)ceil(wavespec.size * (double)gSndPlayer.spec.freq / (double)wavespec.freq) * (SDL_AUDIO_BITSIZE(AUDIO_S16) / SDL_AUDIO_BITSIZE(wavespec.format));
	   if (len >= wavespec.channels * 2 && (bufdec = malloc(len)))
	   {
		   if (wavespec.format == AUDIO_S16)
			   SOUND_ResampleS16(bufsrc, &wavespec, bufdec, len, gSndPlayer.resampler);
		   else
			   SOUND_ResampleU8(bufsrc, &wavespec, bufdec, len, gSndPlayer.resampler);
		   /* Free the original buffer and reset the pointer for simpler later operations */
		   free(buf); buf = bufdec;
		   wavespec.format = AUDIO_S16;
		   wavespec.freq = gSndPlayer.spec.freq;
	   }
	   else
	   {
		   free(buf);
		   return;
	   }
   }
   else
   {
	   bufdec = (LPBYTE)bufsrc;
	   len = wavespec.size;
   }

   //
   // Build the audio converter and create conversion buffers
   //
   if (SDL_BuildAudioCVT(&wavecvt, wavespec.format, wavespec.channels, wavespec.freq,
      gSndPlayer.spec.format, gSndPlayer.spec.channels, gSndPlayer.spec.freq) < 0)
   {
      free(buf);
      return;
   }

   wavecvt.len = len & ~((SDL_AUDIO_BITSIZE(wavespec.format) >> 3) * wavespec.channels - 1);
   wavecvt.buf = (LPBYTE)malloc(wavecvt.len * wavecvt.len_mult);
   if (wavecvt.buf == NULL)
   {
      free(buf);
      return;
   }
   memcpy(wavecvt.buf, bufdec, len);
   free(buf);

   //
   // Run the audio converter
   //
   if (SDL_ConvertAudio(&wavecvt) < 0)
   {
      free(wavecvt.buf);
      return;
   }

   SDL_mutexP(gSndPlayer.mtx);
   if (gSndPlayer.buf[iChannel] != NULL)
   {
	   free(gSndPlayer.buf[iChannel]);
	   gSndPlayer.buf[iChannel] = NULL;
   }
   gSndPlayer.buf[iChannel] = wavecvt.buf;
   gSndPlayer.audio_len[iChannel] = wavecvt.len * wavecvt.len_mult;
   gSndPlayer.pos[iChannel] = wavecvt.buf;
   SDL_mutexV(gSndPlayer.mtx);
}

VOID
SOUND_PlayMUS(
   INT       iNumRIX,
   BOOL      fLoop,
   FLOAT     flFadeTime
)
{
   SDL_mutexP(gSndPlayer.mtx);
   if (gConfig.eMusicType == MUSIC_MIDI)
   {
      MIDI_Play(iNumRIX, fLoop);
   }
   else if (gSndPlayer.pMusPlayer)
   {
      gSndPlayer.pMusPlayer->Play(gSndPlayer.pMusPlayer, iNumRIX, fLoop, flFadeTime);
   }
   SDL_mutexV(gSndPlayer.mtx);
}

BOOL
SOUND_PlayCDA(
   INT    iNumTrack
)
/*++
  Purpose:

    Play a CD Audio Track.

  Parameters:

    [IN]  iNumTrack - number of the CD Audio Track.

  Return value:

    TRUE if the track can be played, FALSE if not.

--*/
{
	BOOL ret = FALSE;
#if PAL_HAS_SDLCD
   if (gSndPlayer.pCD != NULL)
   {
      if (CD_INDRIVE(SDL_CDStatus(gSndPlayer.pCD)))
      {
         SDL_CDStop(gSndPlayer.pCD);

         if (iNumTrack != -1)
         {
            SOUND_PlayMUS(-1, FALSE, 0);

            if (SDL_CDPlayTracks(gSndPlayer.pCD, iNumTrack - 1, 0, 1, 0) == 0)
            {
               return TRUE;
            }
         }
      }
   }
#endif
   SDL_mutexP(gSndPlayer.mtx);
   if (gSndPlayer.pCDPlayer)
   {
	   if (iNumTrack != -1)
	   {
		   SOUND_PlayMUS(-1, FALSE, 0);
		   ret = gSndPlayer.pCDPlayer->Play(gSndPlayer.pCDPlayer, PAL_CDTRACK_BASE + iNumTrack, TRUE, 0);
	   }
	   else
	   {
		   ret = gSndPlayer.pCDPlayer->Play(gSndPlayer.pCDPlayer, -1, FALSE, 0);
	   }
   }
   SDL_mutexV(gSndPlayer.mtx);

   return ret;
}

#ifdef PSP
void
SOUND_Reload(
	void
	)
{
	fclose(gSndPlayer.mkf);
	SOUND_LoadMKF();
}
#endif
