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
#include "palcfg.h"
#include "sound.h"
#include "players.h"
#include "util.h"
#include "resampler.h"
#include "midi.h"
#include <math.h>

#if PAL_HAS_OGG
#include <vorbis/codec.h>
#endif

#define PAL_CDTRACK_BASE    10000

typedef LPCBYTE(*LoaderFunction)(LPCBYTE, DWORD, SDL_AudioSpec *);

typedef struct tagWAVEPLAYER
{
	LoaderFunction           LoadSound;	/* The function pointer for load WAVE/VOC data */
	void                    *resampler;	/* The resampler used for sound data */
	short                   *buf;		/* Base address of the ring buffer */
	int                      buf_len;	/* Length of the ring buffer in samples */
	int                      pos;		/* Position in samples of the 'read' pointer */
	int                      len;		/* Number of vaild samples from the 'read' pointer */
} WAVEPLAYER;

typedef struct tagSNDPLAYER
{
   FILE                     *mkf;		/* File pointer to the MKF file */
   SDL_AudioSpec             spec;		/* Actual-used sound specification */
   SDL_AudioCVT              cvt;		/* Audio format conversion parameter */
   SDL_mutex                *mtx;		/* Mutex for preventing using destroyed objects */
   MUSICPLAYER              *pMusPlayer;
   MUSICPLAYER              *pCDPlayer;
#if PAL_HAS_SDLCD
   SDL_CD                   *pCD;
#endif
   WAVEPLAYER                WavePlayer;
   INT                       iMusicVolume;	/* The BGM volume ranged in [0, 128] for better performance */
   INT                       iSoundVolume;	/* The sound effect volume ranged in [0, 128] for better performance */
   BOOL                      fOpened;       /* Is the audio device opened? */
   BOOL                      fMusicEnabled; /* Is BGM enabled? */
   BOOL                      fSoundEnabled; /* Is sound effect enabled? */
} SNDPLAYER;

static SNDPLAYER gSndPlayer;

PAL_FORCE_INLINE
void
AUDIO_MixNative(
	short     *dst,
	short     *src,
	int        samples
)
{
	while (samples > 0)
	{
		int val = *src++ + *dst;
		if (val > SHRT_MAX)
			*dst++ = SHRT_MAX;
		else if (val < SHRT_MIN)
			*dst++ = SHRT_MIN;
		else
			*dst++ = (short)val;
		samples--;
	}
}

PAL_FORCE_INLINE
void
AUDIO_AdjustVolume(
	short     *srcdst,
	int        iVolume,
	int        samples
)
{
	if (iVolume == SDL_MIX_MAXVOLUME) return;
	if (iVolume == 0) { memset(srcdst, 0, samples << 1); return; }
	while (samples > 0)
	{
		*srcdst = *srcdst * iVolume / SDL_MIX_MAXVOLUME;
		samples--; srcdst++;
	}
}

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

    Resample 8-bit unsigned PCM data into 16-bit signed (native-endian) PCM data.

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
				*dst = resampler_get_and_remove_sample(resampler);
				dst += lpSpec->channels; total_bytes += (SDL_AUDIO_BITSIZE(AUDIO_S16SYS) >> 3);
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
				*dst = resampler_get_and_remove_sample(resampler);
				dst += lpSpec->channels; total_bytes += (SDL_AUDIO_BITSIZE(AUDIO_S16SYS) >> 3);
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

    Resample 16-bit signed (little-endian) PCM data into 16-bit signed (native-endian) PCM data.

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
				*dst = resampler_get_and_remove_sample(resampler);
				dst += lpSpec->channels; total_bytes += (SDL_AUDIO_BITSIZE(AUDIO_S16SYS) >> 3);
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
				*dst = resampler_get_and_remove_sample(resampler);
				dst += lpSpec->channels; total_bytes += (SDL_AUDIO_BITSIZE(AUDIO_S16SYS) >> 3);
			}
		}
	}
}

static VOID SDLCALL
AUDIO_FillBuffer(
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
#if SDL_VERSION_ATLEAST(2,0,0)
   memset(stream, 0, len);
#endif

   SDL_mutexP(gSndPlayer.mtx);

   gSndPlayer.cvt.buf = stream;
   gSndPlayer.cvt.len = len;

   //
   // Play music
   //
   if (gSndPlayer.fMusicEnabled && gSndPlayer.iMusicVolume > 0)
   {
      if (gSndPlayer.pMusPlayer)
      {
         gSndPlayer.pMusPlayer->FillBuffer(gSndPlayer.pMusPlayer, stream, len);
      }

      if (gSndPlayer.pCDPlayer)
      {
         gSndPlayer.pCDPlayer->FillBuffer(gSndPlayer.pCDPlayer, stream, len);
      }

      //
      // Adjust volume for music
      //
      AUDIO_AdjustVolume((short *)stream, gSndPlayer.iMusicVolume, len >> 1);
   }

   //
   // Play sound
   //
   if (gSndPlayer.fSoundEnabled && gSndPlayer.WavePlayer.len > 0 && gSndPlayer.iSoundVolume > 0)
   {
      //
      // Mix as much sound data as possible
      //
      WAVEPLAYER *player = &gSndPlayer.WavePlayer;
      int mixlen = min(player->len, len >> 1);
      if (player->pos + mixlen > player->buf_len)
      {
         AUDIO_MixNative((short *)stream, player->buf + player->pos, player->buf_len - player->pos);
         stream += (player->buf_len - player->pos) << 1; memset(player->buf + player->pos, 0, (player->buf_len - player->pos) << 1);
		 AUDIO_MixNative((short *)stream, player->buf, player->pos + mixlen - player->buf_len);
         stream += (player->pos + mixlen - player->buf_len) << 1; memset(player->buf, 0, (player->pos + mixlen - player->buf_len) << 1);
      }
      else
      {
         AUDIO_MixNative((short *)stream, player->buf + player->pos, mixlen);
         stream += mixlen << 1; memset(player->buf + player->pos, 0, mixlen << 1);
      }
      player->pos = (player->pos + mixlen) % player->buf_len; player->len -= mixlen; len -= (mixlen << 1);
   }

   //
   // Convert audio from native byte-order to actual byte-order
   //
   SDL_ConvertAudio(&gSndPlayer.cvt);

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
	LoaderFunction func[2];
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
			gSndPlayer.WavePlayer.LoadSound = func[i];
			break;
		}
	}
}

INT
AUDIO_OpenDevice(
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

   if (gSndPlayer.fOpened)
   {
      //
      // Already opened
      //
      return -1;
   }

   gSndPlayer.fOpened = FALSE;
   gSndPlayer.fMusicEnabled = TRUE;
   gSndPlayer.fSoundEnabled = TRUE;
   gSndPlayer.iMusicVolume = gConfig.iMusicVolume * SDL_MIX_MAXVOLUME / PAL_MAX_VOLUME;
   gSndPlayer.iSoundVolume = gConfig.iSoundVolume * SDL_MIX_MAXVOLUME / PAL_MAX_VOLUME;

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
   gSndPlayer.WavePlayer.resampler = resampler_create();

   //
   // Open the sound subsystem.
   //
   gSndPlayer.spec.freq = gConfig.iSampleRate;
   gSndPlayer.spec.format = AUDIO_S16;
   gSndPlayer.spec.channels = gConfig.iAudioChannels;
   gSndPlayer.spec.samples = gConfig.wAudioBufferSize;
   gSndPlayer.spec.callback = AUDIO_FillBuffer;

   if (SDL_OpenAudio(&gSndPlayer.spec, &spec) < 0)
   {
      //
      // Failed
      //
      return -3;
   }
   else
      gSndPlayer.spec = spec;

   SDL_BuildAudioCVT(&gSndPlayer.cvt, AUDIO_S16SYS, spec.channels, spec.freq, spec.format, spec.channels, spec.freq);

   gSndPlayer.WavePlayer.buf = NULL;
   gSndPlayer.WavePlayer.buf_len = 0;
   gSndPlayer.WavePlayer.pos = 0;
   gSndPlayer.WavePlayer.len = 0;

   gSndPlayer.mtx = SDL_CreateMutex();
   gSndPlayer.fOpened = TRUE;

   //
   // Initialize the music subsystem.
   //
   switch (gConfig.eMusicType)
   {
   case MUSIC_RIX:
	   if (!(gSndPlayer.pMusPlayer = RIX_Init(va("%s%s", gConfig.pszGamePath, "mus.mkf"))))
	   {
		   gSndPlayer.pMusPlayer = RIX_Init(va("%s%s", gConfig.pszGamePath, "MUS.MKF"));
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
AUDIO_CloseDevice(
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

   if (gSndPlayer.WavePlayer.buf != NULL)
   {
      free(gSndPlayer.WavePlayer.buf);
      gSndPlayer.WavePlayer.buf = NULL;
	  gSndPlayer.WavePlayer.buf_len = 0;
	  gSndPlayer.WavePlayer.pos = 0;
	  gSndPlayer.WavePlayer.len = 0;
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
      AUDIO_PlayCDTrack(-1);
      SDL_CDClose(gSndPlayer.pCD);
   }
#endif

   if (gConfig.eMusicType == MUSIC_MIDI) MIDI_Play(0, FALSE);

   if (gSndPlayer.WavePlayer.resampler)
   {
      resampler_delete(gSndPlayer.WavePlayer.resampler);
	  gSndPlayer.WavePlayer.resampler = NULL;
   }

   SDL_mutexV(gSndPlayer.mtx);
   SDL_DestroyMutex(gSndPlayer.mtx);
}

SDL_AudioSpec*
AUDIO_GetDeviceSpec(
	VOID
)
{
	return &gSndPlayer.spec;
}

static INT
AUDIO_ChangeVolumeByValue(
   INT   *iVolume,
   INT    iValue
)
{
   *iVolume += iValue;
   if (*iVolume > PAL_MAX_VOLUME)
      *iVolume = PAL_MAX_VOLUME;
   else if (*iVolume < 0)
      *iVolume = 0;
   return *iVolume;
}

VOID
AUDIO_IncreaseVolume(
   VOID
)
/*++
  Purpose:

    Increase global volume by 3%.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   AUDIO_ChangeVolumeByValue(&gConfig.iMusicVolume, 3);
   AUDIO_ChangeVolumeByValue(&gConfig.iSoundVolume, 3);
   gSndPlayer.iMusicVolume = gConfig.iMusicVolume * SDL_MIX_MAXVOLUME / PAL_MAX_VOLUME;
   gSndPlayer.iSoundVolume = gConfig.iSoundVolume * SDL_MIX_MAXVOLUME / PAL_MAX_VOLUME;
}

VOID
AUDIO_DecreaseVolume(
   VOID
)
/*++
  Purpose:

    Decrease global volume by 3%.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   AUDIO_ChangeVolumeByValue(&gConfig.iMusicVolume, -3);
   AUDIO_ChangeVolumeByValue(&gConfig.iSoundVolume, -3);
   gSndPlayer.iMusicVolume = gConfig.iMusicVolume * SDL_MIX_MAXVOLUME / PAL_MAX_VOLUME;
   gSndPlayer.iSoundVolume = gConfig.iSoundVolume * SDL_MIX_MAXVOLUME / PAL_MAX_VOLUME;
}

VOID
AUDIO_PlaySound(
   INT    iSoundNum
)
/*++
  Purpose:

    Play a sound in voc.mkf/sounds.mkf file.

  Parameters:

    [IN]  iSoundNum - number of the sound; the absolute value is used.

  Return value:

    None.

--*/
{
   SDL_AudioCVT    wavecvt;
   SDL_AudioSpec   wavespec;
   LPBYTE          buf, bufdec;
   LPCBYTE         bufsrc;
   int             len;

   if (!gSndPlayer.fOpened || !gSndPlayer.fSoundEnabled)
   {
      return;
   }

   if (iSoundNum < 0)
   {
      iSoundNum = -iSoundNum;
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

   bufsrc = gSndPlayer.WavePlayer.LoadSound(buf, len, &wavespec);
   if (bufsrc == NULL)
   {
	   free(buf);
	   return;
   }

   if (wavespec.freq != gSndPlayer.spec.freq)
   {
	   /* Resampler is needed */
	   //if (!AUDIO_IsIntegerConversion(wavespec.freq))
	   //{
		   resampler_set_quality(gSndPlayer.WavePlayer.resampler, AUDIO_IsIntegerConversion(wavespec.freq) ? RESAMPLER_QUALITY_MIN : gConfig.iResampleQuality);
		   resampler_set_rate(gSndPlayer.WavePlayer.resampler, (double)wavespec.freq / (double)gSndPlayer.spec.freq);
		   len = (int)ceil(wavespec.size * (double)gSndPlayer.spec.freq / (double)wavespec.freq) * (SDL_AUDIO_BITSIZE(AUDIO_S16SYS) / SDL_AUDIO_BITSIZE(wavespec.format));
		   if (len >= wavespec.channels * 2 && (bufdec = malloc(len)))
		   {
			   if (wavespec.format == AUDIO_S16)
				   SOUND_ResampleS16(bufsrc, &wavespec, bufdec, len, gSndPlayer.WavePlayer.resampler);
			   else
				   SOUND_ResampleU8(bufsrc, &wavespec, bufdec, len, gSndPlayer.WavePlayer.resampler);
			   /* Free the original buffer and reset the pointer for simpler later operations */
			   free(buf); buf = bufdec;
			   wavespec.format = AUDIO_S16SYS;
			   wavespec.freq = gSndPlayer.spec.freq;
		   }
		   else
		   {
			   free(buf);
			   return;
		   }
	   //}
	   //else
	   //{
		  // SDL_BuildAudioCVT(&wavecvt, wavespec.format, wavespec.channels, wavespec.freq, wavespec.format, wavespec.channels, gSndPlayer.spec.freq);
		  // if (wavecvt.len_mult > 1)
		  // {
			 //  wavecvt.buf = (LPBYTE)malloc(wavespec.size * wavecvt.len_mult);
			 //  memcpy(wavecvt.buf, bufsrc, wavespec.size);
		  // }
		  // else
			 //  wavecvt.buf = bufsrc;
		  // wavecvt.len = wavespec.size;
		  // SDL_ConvertAudio(&wavecvt);
		  // if (wavecvt.len_mult > 1)
		  // {
			 //  free(buf);
			 //  buf = wavecvt.buf;
		  // }
		  // bufdec = wavecvt.buf;
		  // len = (int)(wavespec.size * wavecvt.len_ratio);
		  // wavespec.freq = gSndPlayer.spec.freq;
	   //}
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
	   AUDIO_S16SYS, gSndPlayer.spec.channels, gSndPlayer.spec.freq) < 0)
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
   if (SDL_ConvertAudio(&wavecvt) == 0)
   {
      WAVEPLAYER *player = &gSndPlayer.WavePlayer;

      wavecvt.len = (int)(wavecvt.len * wavecvt.len_ratio) >> 1;

      AUDIO_AdjustVolume((short *)wavecvt.buf, gSndPlayer.iSoundVolume, wavecvt.len);

      SDL_mutexP(gSndPlayer.mtx);

      //
      // Check if the current sound buffer is large enough
      //
      if (gSndPlayer.WavePlayer.buf_len < wavecvt.len)
      {
         if (player->pos + player->len > player->buf_len)
         {
            short *old_buf = player->buf;
            player->buf = (short *)malloc(wavecvt.len << 1);
            memcpy(player->buf, old_buf + player->pos, (player->buf_len - player->pos) << 1);
            memcpy(player->buf + player->buf_len - player->pos, old_buf, (player->pos + player->len - player->buf_len) << 1);
            player->pos = 0; free(old_buf);
         }
         else
            player->buf = (short *)realloc(player->buf, wavecvt.len << 1);
         memset(player->buf + player->pos + player->len, 0, ((player->buf_len = wavecvt.len) - player->pos - player->len) << 1);
      }

      //
      // Mix the current sound buffer with newly played sound and adjust the length of valid data
      //
      if (player->pos + wavecvt.len > player->buf_len)
      {
         AUDIO_MixNative(player->buf + player->pos, (short *)wavecvt.buf, player->buf_len - player->pos);
         AUDIO_MixNative(player->buf, (short *)wavecvt.buf + player->buf_len - player->pos, player->pos + wavecvt.len - player->buf_len);
      }
      else
         AUDIO_MixNative(player->buf + player->pos, (short *)wavecvt.buf, wavecvt.len);
      player->len = max(player->len, wavecvt.len);

      SDL_mutexV(gSndPlayer.mtx);
   }

   free(wavecvt.buf);
}

VOID
AUDIO_PlayMusic(
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
AUDIO_PlayCDTrack(
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
            AUDIO_PlayMusic(-1, FALSE, 0);

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
		   AUDIO_PlayMusic(-1, FALSE, 0);
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

VOID
AUDIO_EnableMusic(
   BOOL   fEnable
)
{
   gSndPlayer.fMusicEnabled = fEnable;
}

BOOL
AUDIO_MusicEnabled(
   VOID
)
{
   return gSndPlayer.fMusicEnabled;
}

VOID
AUDIO_EnableSound(
   BOOL   fEnable
)
{
	gSndPlayer.fSoundEnabled = fEnable;
}

BOOL
AUDIO_SoundEnabled(
   VOID
)
{
   return gSndPlayer.fSoundEnabled;
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
