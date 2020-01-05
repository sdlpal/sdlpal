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
// Portions based on PalLibrary by Lou Yihua <louyihua@21cn.com>.
// Copyright (c) 2006-2007, Lou Yihua.
//

#include "main.h"

static INT
PAL_RNGReadFrame(
   LPBYTE          lpBuffer,
   UINT            uiBufferSize,
   UINT            uiRngNum,
   UINT            uiFrameNum,
   FILE           *fpRngMKF
)
/*++
  Purpose:

    Read a frame from a RNG animation.

  Parameters:

    [OUT] lpBuffer - pointer to the destination buffer.

    [IN]  uiBufferSize - size of the destination buffer.

    [IN]  uiRngNum - the number of the RNG animation in the MKF archive.

    [IN]  uiFrameNum - frame number in the RNG animation.

    [IN]  fpRngMKF - pointer to the fopen'ed MKF file.

  Return value:

    Integer value which indicates the size of the chunk.
    -1 if there are error in parameters.
    -2 if buffer size is not enough.

--*/
{
   UINT         uiOffset       = 0;
   UINT         uiSubOffset    = 0;
   UINT         uiNextOffset   = 0;
   UINT         uiChunkCount   = 0;
   INT          iChunkLen      = 0;

   if (lpBuffer == NULL || fpRngMKF == NULL || uiBufferSize == 0)
   {
      return -1;
   }

   //
   // Get the total number of chunks.
   //
   uiChunkCount = PAL_MKFGetChunkCount(fpRngMKF);
   if (uiRngNum >= uiChunkCount)
   {
      return -1;
   }

   //
   // Get the offset of the chunk.
   //
   fseek(fpRngMKF, 4 * uiRngNum, SEEK_SET);
   PAL_fread(&uiOffset, sizeof(UINT), 1, fpRngMKF);
   PAL_fread(&uiNextOffset, sizeof(UINT), 1, fpRngMKF);
   uiOffset = SDL_SwapLE32(uiOffset);
   uiNextOffset = SDL_SwapLE32(uiNextOffset);

   //
   // Get the length of the chunk.
   //
   iChunkLen = uiNextOffset - uiOffset;
   if (iChunkLen != 0)
   {
      fseek(fpRngMKF, uiOffset, SEEK_SET);
   }
   else
   {
      return -1;
   }

   //
   // Get the number of sub chunks.
   //
   PAL_fread(&uiChunkCount, sizeof(UINT), 1, fpRngMKF);
   uiChunkCount = (SDL_SwapLE32(uiChunkCount) - 4) / 4;
   if (uiFrameNum >= uiChunkCount)
   {
      return -1;
   }

   //
   // Get the offset of the sub chunk.
   //
   fseek(fpRngMKF, uiOffset + 4 * uiFrameNum, SEEK_SET);
   PAL_fread(&uiSubOffset, sizeof(UINT), 1, fpRngMKF);
   PAL_fread(&uiNextOffset, sizeof(UINT), 1, fpRngMKF);
   uiSubOffset = SDL_SwapLE32(uiSubOffset);
   uiNextOffset = SDL_SwapLE32(uiNextOffset);

   //
   // Get the length of the sub chunk.
   //
   iChunkLen = uiNextOffset - uiSubOffset;
   if ((UINT)iChunkLen > uiBufferSize)
   {
      return -2;
   }

   if (iChunkLen != 0)
   {
      fseek(fpRngMKF, uiOffset + uiSubOffset, SEEK_SET);
      return (int)fread(lpBuffer, 1, iChunkLen, fpRngMKF);
   }

   return -1;
}

static INT
PAL_RNGBlitToSurface(
   const uint8_t   *rng,
   int              length,
   SDL_Surface     *lpDstSurface
)
/*++
  Purpose:

    Blit one frame in an RNG animation to an SDL surface.
    The surface should contain the last frame of the RNG, or blank if it's the first
    frame.

    NOTE: Assume the surface is already locked, and the surface is a 320x200 8-bit one.

  Parameters:

    [IN]  rng - Pointer to the RNG data.

    [IN]  length - Length of the RNG data.

    [OUT] lpDstSurface - pointer to the destination SDL surface.

  Return value:

    0 = success, -1 = error.

--*/
{
   int                   ptr         = 0;
   int                   dst_ptr     = 0;
   uint16_t              wdata       = 0;
   int                   x, y, i, n;

   //
   // Check for invalid parameters.
   //
   if (lpDstSurface == NULL || length < 0)
   {
      return -1;
   }

   //
   // Draw the frame to the surface.
   // FIXME: Dirty and ineffective code, needs to be cleaned up
   //
   while (ptr < length)
   {
      uint8_t data = rng[ptr++];
      switch (data)
      {
      case 0x00:
      case 0x13:
         //
         // End
         //
         goto end;

      case 0x02:
         dst_ptr += 2;
         break;

      case 0x03:
         data = rng[ptr++];
         dst_ptr += (data + 1) * 2;
         break;

      case 0x04:
         wdata = rng[ptr] | (rng[ptr + 1] << 8);
         ptr += 2;
         dst_ptr += ((unsigned int)wdata + 1) * 2;
         break;

      case 0x0a:
         x = dst_ptr % 320;
         y = dst_ptr / 320;
         ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
         if (++x >= 320)
         {
            x = 0;
            ++y;
         }
         ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
         dst_ptr += 2;

      case 0x09:
         x = dst_ptr % 320;
         y = dst_ptr / 320;
         ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
         if (++x >= 320)
         {
            x = 0;
            ++y;
         }
         ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
         dst_ptr += 2;

      case 0x08:
         x = dst_ptr % 320;
         y = dst_ptr / 320;
         ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
         if (++x >= 320)
         {
            x = 0;
            ++y;
         }
         ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
         dst_ptr += 2;

      case 0x07:
         x = dst_ptr % 320;
         y = dst_ptr / 320;
         ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
         if (++x >= 320)
         {
            x = 0;
            ++y;
         }
         ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
         dst_ptr += 2;

      case 0x06:
         x = dst_ptr % 320;
         y = dst_ptr / 320;
         ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
         if (++x >= 320)
         {
            x = 0;
            ++y;
         }
         ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
         dst_ptr += 2;
         break;

      case 0x0b:
         data = *(rng + ptr++);
         for (i = 0; i <= data; i++)
         {
            x = dst_ptr % 320;
            y = dst_ptr / 320;
            ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
            if (++x >= 320)
            {
               x = 0;
               ++y;
            }
            ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
            dst_ptr += 2;
         }
         break;

      case 0x0c:
         wdata = rng[ptr] | (rng[ptr + 1] << 8);
         ptr += 2;
         for (i = 0; i <= wdata; i++)
         {
            x = dst_ptr % 320;
            y = dst_ptr / 320;
            ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
            if (++x >= 320)
            {
               x = 0;
               ++y;
            }
            ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr++];
            dst_ptr += 2;
         }
         break;

      case 0x0d:
      case 0x0e:
      case 0x0f:
      case 0x10:
         for (i = 0; i < data - (0x0d - 2); i++)
         {
            x = dst_ptr % 320;
            y = dst_ptr / 320;
            ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr];
            if (++x >= 320)
            {
               x = 0;
               ++y;
            }
            ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr + 1];
            dst_ptr += 2;
         }
         ptr += 2;
         break;

      case 0x11:
    	 data = *(rng + ptr++);
         for (i = 0; i <= data; i++)
         {
            x = dst_ptr % 320;
            y = dst_ptr / 320;
            ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr];
            if (++x >= 320)
            {
               x = 0;
               ++y;
            }
            ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr + 1];
            dst_ptr += 2;
         }
         ptr += 2;
         break;

      case 0x12:
         n = (rng[ptr] | (rng[ptr + 1] << 8)) + 1;
         ptr += 2;
         for (i = 0; i < n; i++)
         {
            x = dst_ptr % 320;
            y = dst_ptr / 320;
            ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr];
            if (++x >= 320)
            {
               x = 0;
               ++y;
            }
            ((LPBYTE)(lpDstSurface->pixels))[y * lpDstSurface->pitch + x] = rng[ptr + 1];
            dst_ptr += 2;
         }
         ptr += 2;
         break;
      }
   }

end:
   return 0;
}

VOID
PAL_RNGPlay(
   INT           iNumRNG,
   INT           iStartFrame,
   INT           iEndFrame,
   INT           iSpeed
)
/*++
  Purpose:

    Play a RNG movie.

  Parameters:

    [IN]  iNumRNG - number of the RNG movie.

    [IN]  iStartFrame - start frame number.

    [IN]  iEndFrame - end frame number.

    [IN]  iSpeed - speed of playing.

  Return value:

    None.

--*/
{
   double         iDelay = (double)SDL_GetPerformanceFrequency() / (iSpeed == 0 ? 16 : iSpeed);
   uint8_t        *rng = (uint8_t *)malloc(65000);
   uint8_t        *buf = (uint8_t *)malloc(65000);
   FILE           *fp = UTIL_OpenRequiredFile("rng.mkf");

   for (double iTime = SDL_GetPerformanceCounter(); rng && buf && iStartFrame != iEndFrame; iStartFrame++)
   {
	  iTime += iDelay;
      //
      // Read, decompress and render the frame
      //
      if (PAL_RNGReadFrame(buf, 65000, iNumRNG, iStartFrame, fp) < 0 ||
          PAL_RNGBlitToSurface(rng, Decompress(buf, rng, 65000), gpScreen) == -1)
      {
         //
         // Failed to get the frame, don't go further
         //
         break;
      }

      //
      // Update the screen
      //
      VIDEO_UpdateScreen(NULL);

      //
      // Fade in the screen if needed
      //
      if (gpGlobals->fNeedToFadeIn)
      {
         PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
         gpGlobals->fNeedToFadeIn = FALSE;
      }

      //
      // Delay for a while
      //
	  PAL_DelayUntilPC(iTime);
   }

   fclose(fp);
   free(rng);
   free(buf);
}
