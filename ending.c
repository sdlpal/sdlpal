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

#include "main.h"

static WORD g_wCurEffectSprite = 0;

VOID
PAL_EndingSetEffectSprite(
   WORD         wSpriteNum
)
/*++
  Purpose:

    Set the effect sprite of the ending.

  Parameters:

    [IN]  wSpriteNum - the number of the sprite.

  Return value:

    None.

--*/
{
   g_wCurEffectSprite = wSpriteNum;
}

VOID
PAL_ShowFBP(
   WORD         wChunkNum,
   WORD         wFade
)
/*++
  Purpose:

    Draw an FBP picture to the screen.

  Parameters:

    [IN]  wChunkNum - number of chunk in fbp.mkf file.

    [IN]  wFade - fading speed of showing the picture.

  Return value:

    None.

--*/
{
   PAL_LARGE BYTE            buf[320 * 200];
   PAL_LARGE BYTE            bufSprite[320 * 200];
   const int                 rgIndex[6] = {0, 3, 1, 5, 2, 4};
   SDL_Surface              *p;
   int                       i, j, k;
   BYTE                      a, b;

   if (PAL_MKFDecompressChunk(buf, 320 * 200, wChunkNum, gpGlobals->f.fpFBP) <= 0)
   {
      memset(buf, 0, sizeof(buf));
   }

   if (g_wCurEffectSprite != 0)
   {
      PAL_MKFDecompressChunk(bufSprite, 320 * 200, g_wCurEffectSprite, gpGlobals->f.fpMGO);
   }

   if (wFade)
   {
      wFade++;
      wFade *= 10;

      p = SDL_CreateRGBSurface(gpScreen->flags & ~SDL_HWSURFACE, 320, 200, 8,
         gpScreen->format->Rmask, gpScreen->format->Gmask,
         gpScreen->format->Bmask, gpScreen->format->Amask);

      PAL_FBPBlitToSurface(buf, p);
      VIDEO_BackupScreen();

      for (i = 0; i < 16; i++)
      {
         for (j = 0; j < 6; j++)
         {
            //
            // Blend the pixels in the 2 buffers, and put the result into the
            // backup buffer
            //
            for (k = rgIndex[j]; k < gpScreen->pitch * gpScreen->h; k += 6)
            {
               a = ((LPBYTE)(p->pixels))[k];
               b = ((LPBYTE)(gpScreenBak->pixels))[k];

               if (i > 0)
               {
                  if ((a & 0x0F) > (b & 0x0F))
                  {
                     b++;
                  }
                  else if ((a & 0x0F) < (b & 0x0F))
                  {
                     b--;
                  }
               }

               ((LPBYTE)(gpScreenBak->pixels))[k] = ((a & 0xF0) | (b & 0x0F));
            }

            SDL_BlitSurface(gpScreenBak, NULL, gpScreen, NULL);

            if (g_wCurEffectSprite != 0)
            {
               int f = SDL_GetTicks() / 150;
               PAL_RLEBlitToSurface(PAL_SpriteGetFrame(bufSprite, f % PAL_SpriteGetNumFrames(bufSprite)),
                  gpScreen, PAL_XY(0, 0));
            }

            VIDEO_UpdateScreen(NULL);
            UTIL_Delay(wFade);
         }
      }

      SDL_FreeSurface(p);
   }

   //
   // HACKHACK: to make the ending show correctly
   //
   if (wChunkNum != 49)
   {
      PAL_FBPBlitToSurface(buf, gpScreen);
   }

   VIDEO_UpdateScreen(NULL);
}

VOID
PAL_ScrollFBP(
   WORD         wChunkNum,
   WORD         wScrollSpeed,
   BOOL         fScrollDown
)
/*++
  Purpose:

    Scroll up an FBP picture to the screen.

  Parameters:

    [IN]  wChunkNum - number of chunk in fbp.mkf file.

    [IN]  wScrollSpeed - scrolling speed of showing the picture.

    [IN]  fScrollDown - TRUE if scroll down, FALSE if scroll up.

  Return value:

    None.

--*/
{
   SDL_Surface          *p;
   PAL_LARGE BYTE        buf[320 * 200];
   PAL_LARGE BYTE        bufSprite[320 * 200];
   int                   i, l;
   SDL_Rect              rect, dstrect;

   if (PAL_MKFDecompressChunk(buf, 320 * 200, wChunkNum, gpGlobals->f.fpFBP) <= 0)
   {
      return;
   }

   if (g_wCurEffectSprite != 0)
   {
      PAL_MKFDecompressChunk(bufSprite, 320 * 200, g_wCurEffectSprite, gpGlobals->f.fpMGO);
   }

   p = SDL_CreateRGBSurface(gpScreen->flags & ~SDL_HWSURFACE, 320, 200, 8,
      gpScreen->format->Rmask, gpScreen->format->Gmask,
      gpScreen->format->Bmask, gpScreen->format->Amask);

   if (p == NULL)
   {
      return;
   }

   VIDEO_BackupScreen();
   PAL_FBPBlitToSurface(buf, p);

   if (wScrollSpeed == 0)
   {
      wScrollSpeed = 1;
   }

   rect.x = 0;
   rect.w = 320;
   dstrect.x = 0;
   dstrect.w = 320;

   for (l = 0; l < 220; l++)
   {
      i = l;
      if (i > 200)
      {
         i = 200;
      }

      if (fScrollDown)
      {
         rect.y = 0;
         dstrect.y = i;
         rect.h = 200 - i;
         dstrect.h = 200 - i;
      }
      else
      {
         rect.y = i;
         dstrect.y = 0;
         rect.h = 200 - i;
         dstrect.h = 200 - i;
      }

      SDL_BlitSurface(gpScreenBak, &rect, gpScreen, &dstrect);

      if (fScrollDown)
      {
         rect.y = 200 - i;
         dstrect.y = 0;
         rect.h = i;
         dstrect.h = i;
      }
      else
      {
         rect.y = 0;
         dstrect.y = 200 - i;
         rect.h = i;
         dstrect.h = i;
      }

      SDL_BlitSurface(p, &rect, gpScreen, &dstrect);

      PAL_ApplyWave(gpScreen);

      if (g_wCurEffectSprite != 0)
      {
         int f = SDL_GetTicks() / 150;
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(bufSprite, f % PAL_SpriteGetNumFrames(bufSprite)),
            gpScreen, PAL_XY(0, 0));
      }

      VIDEO_UpdateScreen(NULL);

      if (gpGlobals->fNeedToFadeIn)
      {
         PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
         gpGlobals->fNeedToFadeIn = FALSE;
      }

      UTIL_Delay(800 / wScrollSpeed);
   }

   SDL_BlitSurface(p, NULL, gpScreen, NULL);
   SDL_FreeSurface(p);
   VIDEO_UpdateScreen(NULL);
}

VOID
PAL_EndingAnimation(
   VOID
)
/*++
  Purpose:

    Show the ending animation.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   LPBYTE            buf;
   LPBYTE            bufGirl;
   SDL_Surface      *pUpper;
   SDL_Surface      *pLower;
   SDL_Rect          srcrect, dstrect;

   int               yPosGirl = 180;
   int               i;

   buf = (LPBYTE)UTIL_calloc(1, 64000);
   bufGirl = (LPBYTE)UTIL_calloc(1, 6000);

   pUpper = SDL_CreateRGBSurface(gpScreen->flags & ~SDL_HWSURFACE, 320, 200, 8,
      gpScreen->format->Rmask, gpScreen->format->Gmask,
      gpScreen->format->Bmask, gpScreen->format->Amask);

   pLower = SDL_CreateRGBSurface(gpScreen->flags & ~SDL_HWSURFACE, 320, 200, 8,
      gpScreen->format->Rmask, gpScreen->format->Gmask,
      gpScreen->format->Bmask, gpScreen->format->Amask);

   PAL_MKFDecompressChunk(buf, 64000, 61, gpGlobals->f.fpFBP);
   PAL_FBPBlitToSurface(buf, pUpper);

   PAL_MKFDecompressChunk(buf, 64000, 62, gpGlobals->f.fpFBP);
   PAL_FBPBlitToSurface(buf, pLower);

   PAL_MKFDecompressChunk(buf, 64000, 571, gpGlobals->f.fpMGO);
   PAL_MKFDecompressChunk(bufGirl, 6000, 572, gpGlobals->f.fpMGO);

   srcrect.x = 0;
   dstrect.x = 0;
   srcrect.w = 320;
   dstrect.w = 320;

   gpGlobals->wScreenWave = 2;

   for (i = 0; i < 400; i++)
   {
      //
      // Draw the background
      //
      srcrect.y = 0;
      srcrect.h = 200 - i / 2;

      dstrect.y = i / 2;
      dstrect.h = 200 - i / 2;

      SDL_BlitSurface(pLower, &srcrect, gpScreen, &dstrect);

      srcrect.y = 200 - i / 2;
      srcrect.h = i / 2;

      dstrect.y = 0;
      dstrect.h = i / 2;

      SDL_BlitSurface(pUpper, &srcrect, gpScreen, &dstrect);

      PAL_ApplyWave(gpScreen);

      //
      // Draw the beast
      //
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(buf, 0), gpScreen, PAL_XY(0, -400 + i));
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(buf, 1), gpScreen, PAL_XY(0, -200 + i));

      //
      // Draw the girl
      //
      yPosGirl -= i & 1;
      if (yPosGirl < 80)
      {
         yPosGirl = 80;
      }

      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(bufGirl, (SDL_GetTicks() / 50) % 4),
         gpScreen, PAL_XY(220, yPosGirl));

      //
      // Update the screen
      //
      VIDEO_UpdateScreen(NULL);
      if (gpGlobals->fNeedToFadeIn)
      {
         PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
         gpGlobals->fNeedToFadeIn = FALSE;
      }

      UTIL_Delay(50);
   }

   gpGlobals->wScreenWave = 0;

   SDL_FreeSurface(pUpper);
   SDL_FreeSurface(pLower);

   free(buf);
   free(bufGirl);
}
