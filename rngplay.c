/* -*- mode: c; tab-width: 4; c-basic-offset: 3; c-file-style: "linux" -*- */
//
// Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
// All rights reserved.
//
// Portions based on PalLibrary by Lou Yihua <louyihua@21cn.com>.
// Copyright (c) 2006-2007, Lou Yihua.
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
   fread(&uiOffset, sizeof(UINT), 1, fpRngMKF);
   fread(&uiNextOffset, sizeof(UINT), 1, fpRngMKF);
   uiOffset = SWAP32(uiOffset);
   uiNextOffset = SWAP32(uiNextOffset);

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
   fread(&uiChunkCount, sizeof(UINT), 1, fpRngMKF);
   uiChunkCount = (SWAP32(uiChunkCount) - 4) / 4;
   if (uiFrameNum >= uiChunkCount)
   {
      return -1;
   }

   //
   // Get the offset of the sub chunk.
   //
   fseek(fpRngMKF, uiOffset + 4 * uiFrameNum, SEEK_SET);
   fread(&uiSubOffset, sizeof(UINT), 1, fpRngMKF);
   fread(&uiNextOffset, sizeof(UINT), 1, fpRngMKF);
   uiSubOffset = SWAP32(uiSubOffset);
   uiNextOffset = SWAP32(uiNextOffset);

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
      fread(lpBuffer, iChunkLen, 1, fpRngMKF);
   }
   else
   {
      return -1;
   }

   return iChunkLen;
}

static INT
PAL_RNGBlitToSurface(
   INT                      iNumRNG,
   INT                      iNumFrame,
   SDL_Surface             *lpDstSurface,
   FILE                    *fpRngMKF
)
/*++
  Purpose:

    Blit one frame in an RNG animation to an SDL surface.
    The surface should contain the last frame of the RNG, or blank if it's the first
    frame.

    NOTE: Assume the surface is already locked, and the surface is a 320x200 8-bit one.

  Parameters:

    [IN]  iNumRNG - The number of the animation in the MKF archive.

    [IN]  iNumFrame - The number of the frame in the animation.

    [OUT] lpDstSurface - pointer to the destination SDL surface.

    [IN]  fpRngMKF - Pointer to the fopen'ed rng.mkf file.

  Return value:

    0 = success, -1 = error.

--*/
{
   INT                   ptr         = 0;
   INT                   dst_ptr     = 0;
   BYTE                  data        = 0;
   WORD                  wdata       = 0;
   INT                   x, y, i, n;
   LPBYTE                rng         = NULL;
   LPBYTE                buf         = NULL;

   //
   // Check for invalid parameters.
   //
   if (lpDstSurface == NULL || iNumRNG < 0 || iNumFrame < 0)
   {
      return -1;
   }

   buf = (LPBYTE)calloc(1, 65000);
   if (buf == NULL)
   {
      return -1;
   }

   //
   // Read the frame.
   //
   if (PAL_RNGReadFrame(buf, 65000, iNumRNG, iNumFrame, fpRngMKF) < 0)
   {
      free(buf);
      return -1;
   }

   //
   // Decompress the frame.
   //
   rng = (LPBYTE)calloc(1, 65000);
   if (rng == NULL)
   {
      free(buf);
      return -1;
   }
   Decompress(buf, rng, 65000);
   free(buf);

   //
   // Draw the frame to the surface.
   // FIXME: Dirty and ineffective code, needs to be cleaned up
   //
   while (TRUE)
   {
      data = rng[ptr++];
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
   free(rng);
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
   UINT            iTime;
   int             iDelay = 800 / (iSpeed == 0 ? 16 : iSpeed);
   FILE           *fp;

   fp = UTIL_OpenRequiredFile("rng.mkf");

   for (; iStartFrame <= iEndFrame; iStartFrame++)
   {
      iTime = SDL_GetTicks() + iDelay;

      if (PAL_RNGBlitToSurface(iNumRNG, iStartFrame, gpScreen, fp) == -1)
      {
         //
         // Failed to get the frame, don't go further
         //
         fclose(fp);
         return;
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
      PAL_ProcessEvent();
      while (SDL_GetTicks() <= iTime)
      {
         PAL_ProcessEvent();
         SDL_Delay(1);
      }
   }

   fclose(fp);
}
