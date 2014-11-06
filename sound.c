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
#include "common.h"
#include "sound.h"
#include "rixplay.h"
#include "util.h"

#ifdef PAL_HAS_NATIVEMIDI
#include "midi.h"
#endif

#ifdef PAL_HAS_MP3
#include "libmad/music_mad.h"
#endif

static BOOL  gSndOpened = FALSE;

BOOL         g_fNoSound = FALSE;
BOOL         g_fNoMusic = FALSE;

#ifdef PAL_HAS_NATIVEMIDI
BOOL         g_fUseMidi = FALSE;
#endif

static BOOL  g_fUseWav = FALSE;

#ifdef __SYMBIAN32__
INT          g_iVolume  = SDL_MIX_MAXVOLUME * 0.1;
#endif

#ifdef PAL_CLASSIC
int          g_iCurrChannel = 0;
#endif

typedef struct tagSNDPLAYER
{
   FILE                     *mkf;
   SDL_AudioSpec             spec;
   SDL_mutex                *mtx;
   LPBYTE                    buf[2], pos[2];
   INT                       audio_len[2];
#ifdef PAL_HAS_CD
   SDL_CD                   *pCD;
#endif
#ifdef PAL_HAS_MP3
   mad_data                 *pMP3;
   BOOL                      fMP3Loop;
   INT                       iCurrentMP3;
   SDL_mutex                *lock;
#endif
} SNDPLAYER;

static SNDPLAYER gSndPlayer;

static SDL_AudioSpec *
SOUND_LoadVOCFromBuffer(
   LPCBYTE                lpVOC,
   DWORD                  dwLen,
   SDL_AudioSpec         *lpSpec,
   LPBYTE                *lppBuffer
)
/*++
  Purpose:

    Load a VOC file in a buffer. Currently supports type 01 block only.

  Parameters:

    [IN]  lpVOC - pointer to the buffer of the VOC file.

    [IN]  dwLen - length of the buffer of the VOC file.

    [OUT] lpSpec - pointer to the SDL_AudioSpec structure, which contains
                   some basic information about the VOC file.

    [OUT] lppBuffer - the output buffer.

  Return value:

    Pointer to the SDL_AudioSpec structure, NULL if failed.

--*/
{
   INT freq, len, x, i, l;
   SDL_RWops *rw;

   if (g_fUseWav)
   {
      rw = SDL_RWFromConstMem(lpVOC, dwLen);
      if (rw == NULL) return NULL;

      len = dwLen;

      SDL_LoadWAV_RW(rw, 1, lpSpec, lppBuffer, (Uint32 *)&len);
      lpSpec->size = len;

      return lpSpec;
   }
   else
   {
      //
      // Skip header
      //
      lpVOC += 0x1B;

      //
      // Length is 3 bytes long
      //
      len = (lpVOC[0] | (lpVOC[1] << 8) | (lpVOC[2] << 16)) - 2;
      lpVOC += 3;

      //
      // One byte for frequency
      //
      freq = 1000000 / (256 - *lpVOC);

#if 1

      lpVOC += 2;

      //
      // Convert the sample manually, as SDL doesn't like "strange" sample rates.
      //
      x = (INT)(len * ((FLOAT)PAL_SAMPLE_RATE / freq));

      *lppBuffer = (LPBYTE)calloc(1, x);
      if (*lppBuffer == NULL)
      {
         return NULL;
      }
      for (i = 0; i < x; i++)
      {
         l = (INT)(i * (freq / (FLOAT)PAL_SAMPLE_RATE));
         if (l >= len)
         {
            l = len - 1;
         }
         (*lppBuffer)[i] = lpVOC[l];
      }

      lpSpec->channels = 1;
      lpSpec->format = AUDIO_U8;
      lpSpec->freq = PAL_SAMPLE_RATE;
      lpSpec->size = x;

#else

      *lppBuffer = (unsigned char *)malloc(len);
      if (*lppBuffer == NULL)
      {
         return NULL;
      }

      lpSpec->channels = 1;
      lpSpec->format = AUDIO_U8;
      lpSpec->freq = freq;
      lpSpec->size = len;

      lpVOC += 2;
      memcpy(*lppBuffer, lpVOC, len);

#endif

      return lpSpec;
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
#ifdef PAL_HAS_MP3
      SDL_mutexP(gSndPlayer.lock);

      if (gSndPlayer.pMP3 != NULL)
      {
         mad_getSamples(gSndPlayer.pMP3, stream, len);

         if (!mad_isPlaying(gSndPlayer.pMP3) && gSndPlayer.fMP3Loop)
         {
            mad_seek(gSndPlayer.pMP3, 0);
            mad_start(gSndPlayer.pMP3);

            mad_getSamples(gSndPlayer.pMP3, stream, len);
         }
      }

      SDL_mutexV(gSndPlayer.lock);
#endif
      RIX_FillBuffer(stream, len);
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
#ifdef __SYMBIAN32__
      SDL_MixAudio(stream, gSndPlayer.pos[i], len, g_iVolume);
#else
      SDL_MixAudio(stream, gSndPlayer.pos[i], len, SDL_MIX_MAXVOLUME * 2 / 3);
#endif
      gSndPlayer.pos[i] += len;
      gSndPlayer.audio_len[i] -= len;
   }
   SDL_mutexV(gSndPlayer.mtx);
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
   gSndPlayer.mkf = UTIL_OpenFile("voc.mkf");
   if (gSndPlayer.mkf == NULL)
   {
      gSndPlayer.mkf = UTIL_OpenFile("sounds.mkf");
      if (gSndPlayer.mkf == NULL)
      {
         return -2;
      }
      g_fUseWav = TRUE;
   }
   else
   {
      g_fUseWav = FALSE;
   }

   //
   // Open the sound subsystem.
   //
   gSndPlayer.spec.freq = PAL_SAMPLE_RATE;
   gSndPlayer.spec.format = AUDIO_S16;
   gSndPlayer.spec.channels = PAL_CHANNELS;
   gSndPlayer.spec.samples = 1024;
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
   if (RIX_Init(va("%s%s", PAL_PREFIX, "mus.mkf")) < 0)
   {
      RIX_Init(va("%s%s", PAL_PREFIX, "MUS.MKF"));
   }

#ifdef PAL_HAS_CD
   //
   // Initialize the CD audio.
   //
   {
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
   }
#endif

#ifdef PAL_HAS_MP3
   gSndPlayer.iCurrentMP3 = -1;
   gSndPlayer.lock = SDL_CreateMutex();
#endif

   //
   // Let the callback function run so that musics will be played.
   //
   SDL_PauseAudio(0);

   return 0;
}

#ifdef PSP
void
SOUND_ReloadVOC(
	void
)
{
   fclose(gSndPlayer.mkf);
   gSndPlayer.mkf = UTIL_OpenFile("voc.mkf");
   g_fUseWav = FALSE;
}
#endif

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
   SDL_mutexP(gSndPlayer.mtx);
   SDL_CloseAudio();

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

   SDL_DestroyMutex(gSndPlayer.mtx);

#ifdef PAL_HAS_MP3
   SDL_mutexP(gSndPlayer.lock);

   if (gSndPlayer.pMP3 != NULL)
   {
      mad_stop(gSndPlayer.pMP3);
      mad_closeFile(gSndPlayer.pMP3);
      gSndPlayer.pMP3 = NULL;
   }

   SDL_DestroyMutex(gSndPlayer.lock);
#endif

   RIX_Shutdown();

#ifdef PAL_HAS_CD
   if (gSndPlayer.pCD != NULL)
   {
      SOUND_PlayCDA(-1);
      SDL_CDClose(gSndPlayer.pCD);
   }
#endif

#ifdef PAL_HAS_NATIVEMIDI
   MIDI_Play(0, FALSE);
#endif
}

#ifdef __SYMBIAN32__

VOID
SOUND_AdjustVolume(
   INT    iDirectory
)
/*++
  Purpose:

    SDL sound volume adjust function.

  Parameters:

    [IN]  iDirectory - value, Increase (>0) or decrease (<=0) 3% volume.

  Return value:

    None.

--*/
{
   if (iDirectory > 0)
   {
      if (g_iVolume <= SDL_MIX_MAXVOLUME)
      {
         g_iVolume += SDL_MIX_MAXVOLUME * 0.03;
      }
      else
      {
         g_iVolume = SDL_MIX_MAXVOLUME;
      }
   }
   else
   {
      if (g_iVolume > 0)
      {
         g_iVolume -= SDL_MIX_MAXVOLUME * 0.03;
      }
      else
      {
         g_iVolume = 0;
      }
   }
}

#endif

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
   UINT            samplesize;
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
      LPBYTE p = gSndPlayer.buf[iChannel];
      gSndPlayer.buf[iChannel] = NULL;
      free(p);
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

   buf = (LPBYTE)calloc(len, 1);
   if (buf == NULL)
   {
      return;
   }

   //
   // Read the sound file from the MKF archive.
   //
   PAL_MKFReadChunk(buf, len, iSoundNum, gSndPlayer.mkf);

   SOUND_LoadVOCFromBuffer(buf, len, &wavespec, &bufdec);
   free(buf);

   //
   // Build the audio converter and create conversion buffers
   //
   if (SDL_BuildAudioCVT(&wavecvt, wavespec.format, wavespec.channels, wavespec.freq,
      gSndPlayer.spec.format, gSndPlayer.spec.channels, gSndPlayer.spec.freq) < 0)
   {
      free(bufdec);
      return;
   }

   samplesize = ((wavespec.format & 0xFF) / 8) * wavespec.channels;
   wavecvt.len = wavespec.size & ~(samplesize - 1);
   wavecvt.buf = (LPBYTE)malloc(wavecvt.len * wavecvt.len_mult);
   if (wavecvt.buf == NULL)
   {
      free(bufdec);
      return;
   }
   memcpy(wavecvt.buf, bufdec, wavespec.size);
   if (g_fUseWav)
   {
      SDL_FreeWAV(bufdec);
   }
   else
   {
      free(bufdec);
   }

   //
   // Run the audio converter
   //
   if (SDL_ConvertAudio(&wavecvt) < 0)
   {
      return;
   }

   SDL_mutexP(gSndPlayer.mtx);
   if (gSndPlayer.buf[iChannel] != NULL)
   {
	   LPBYTE p = gSndPlayer.buf[iChannel];
	   gSndPlayer.buf[iChannel] = NULL;
	   free(p);
   }
   gSndPlayer.buf[iChannel] = wavecvt.buf;
   gSndPlayer.audio_len[iChannel] = wavecvt.len * wavecvt.len_mult;
   gSndPlayer.pos[iChannel] = wavecvt.buf;
   SDL_mutexV(gSndPlayer.mtx);
}

VOID
PAL_PlayMUS(
   INT       iNumRIX,
   BOOL      fLoop,
   FLOAT     flFadeTime
)
{
#ifdef PAL_HAS_NATIVEMIDI
   if (g_fUseMidi)
   {
      MIDI_Play(iNumRIX, fLoop);
      return;
   }
#endif

#ifdef PAL_HAS_MP3
   SDL_mutexP(gSndPlayer.lock);

   if (gSndPlayer.pMP3 != NULL)
   {
      if (iNumRIX == gSndPlayer.iCurrentMP3 && !g_fNoMusic)
      {
         SDL_mutexV(gSndPlayer.lock);
         return;
      }

      mad_stop(gSndPlayer.pMP3);
      mad_closeFile(gSndPlayer.pMP3);

      gSndPlayer.pMP3 = NULL;
   }

   SDL_mutexV(gSndPlayer.lock);

   gSndPlayer.iCurrentMP3 = -1;

   if (iNumRIX > 0)
   {
      SDL_mutexP(gSndPlayer.lock);

      gSndPlayer.pMP3 = mad_openFile(va("%s/mp3/%.2d.mp3", PAL_PREFIX, iNumRIX), &gSndPlayer.spec);
      if (gSndPlayer.pMP3 != NULL)
      {
         RIX_Play(0, FALSE, flFadeTime);

         mad_start(gSndPlayer.pMP3);
         gSndPlayer.fMP3Loop = fLoop;
         gSndPlayer.iCurrentMP3 = iNumRIX;
         SDL_mutexV(gSndPlayer.lock);

         return;
      }

      SDL_mutexV(gSndPlayer.lock);
   }
#endif

   RIX_Play(iNumRIX, fLoop, flFadeTime);
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
#ifdef PAL_HAS_CD
   if (gSndPlayer.pCD != NULL)
   {
      if (CD_INDRIVE(SDL_CDStatus(gSndPlayer.pCD)))
      {
         SDL_CDStop(gSndPlayer.pCD);

         if (iNumTrack != -1)
         {
            PAL_PlayMUS(-1, FALSE, 0);

            if (SDL_CDPlayTracks(gSndPlayer.pCD, iNumTrack - 1, 0, 1, 0) == 0)
            {
               return TRUE;
            }
         }
      }
   }
#endif

   return FALSE;
}
