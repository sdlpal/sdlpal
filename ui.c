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

#include "main.h"

LPSPRITE      gpSpriteUI = NULL;

INT
PAL_InitUI(
   VOID
)
/*++
  Purpose:

    Initialze the UI subsystem.

  Parameters:

    None.

  Return value:

    0 = success, -1 = fail.

--*/
{
   int        iSize;

   //
   // Load the UI sprite.
   //
   iSize = PAL_MKFGetChunkSize(CHUNKNUM_SPRITEUI, gpGlobals->f.fpDATA);
   if (iSize < 0)
   {
      return -1;
   }

   gpSpriteUI = (LPSPRITE)calloc(1, iSize);
   if (gpSpriteUI == NULL)
   {
      return -1;
   }

   PAL_MKFReadChunk(gpSpriteUI, iSize, CHUNKNUM_SPRITEUI, gpGlobals->f.fpDATA);

   return 0;
}

VOID
PAL_FreeUI(
   VOID
)
/*++
  Purpose:

    Shutdown the UI subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   if (gpSpriteUI != NULL)
   {
      free(gpSpriteUI);
      gpSpriteUI = NULL;
   }
}

LPBOX
PAL_CreateBox(
   PAL_POS        pos,
   INT            nRows,
   INT            nColumns,
   INT            iStyle,
   BOOL           fSaveScreen
)
/*++
  Purpose:

    Create a box on the screen.

  Parameters:

    [IN]  pos - position of the box.

    [IN]  nRows - number of rows of the box.

    [IN]  nColumns - number of columns of the box.

    [IN]  iStyle - style of the box (0 or 1).

    [IN]  fSaveScreen - whether save the used screen area or not.

  Return value:

    Pointer to a BOX structure. NULL if failed.
    If fSaveScreen is false, then always returns NULL.

--*/
{
   int              i, j, x, m, n;
   LPCBITMAPRLE     rglpBorderBitmap[3][3];
   LPBOX            lpBox = NULL;
   SDL_Surface     *save;
   SDL_Rect         rect;

   //
   // Get the bitmaps
   //
   for (i = 0; i < 3; i++)
   {
      for (j = 0; j < 3; j++)
      {
         rglpBorderBitmap[i][j] = PAL_SpriteGetFrame(gpSpriteUI, i * 3 + j + iStyle * 9);
      }
   }

   rect.x = PAL_X(pos);
   rect.y = PAL_Y(pos);
   rect.w = 0;
   rect.h = 0;

   //
   // Get the total width and total height of the box
   //
   for (i = 0; i < 3; i++)
   {
      if (i == 1)
      {
         rect.w += PAL_RLEGetWidth(rglpBorderBitmap[0][i]) * nColumns;
         rect.h += PAL_RLEGetHeight(rglpBorderBitmap[i][0]) * nRows;
      }
      else
      {
         rect.w += PAL_RLEGetWidth(rglpBorderBitmap[0][i]);
         rect.h += PAL_RLEGetHeight(rglpBorderBitmap[i][0]);
      }
   }

   if (fSaveScreen)
   {
      //
      // Save the used part of the screen
      //
      save = SDL_CreateRGBSurface(gpScreen->flags, rect.w, rect.h, 8,
         gpScreen->format->Rmask, gpScreen->format->Gmask,
         gpScreen->format->Bmask, gpScreen->format->Amask);

      if (save == NULL)
      {
         return NULL;
      }

      lpBox = (LPBOX)calloc(1, sizeof(BOX));
      if (lpBox == NULL)
      {
         SDL_FreeSurface(save);
         return NULL;
      }

#if SDL_VERSION_ATLEAST(2,0,0)
      SDL_SetSurfacePalette(save, gpScreen->format->palette);
#else
      SDL_SetPalette(save, SDL_LOGPAL | SDL_PHYSPAL, VIDEO_GetPalette(), 0, 256);
#endif
      SDL_BlitSurface(gpScreen, &rect, save, NULL);

      lpBox->lpSavedArea = save;
      lpBox->pos = pos;
      lpBox->wWidth = rect.w;
      lpBox->wHeight = rect.h;
   }

   //
   // Border takes 2 additional rows and columns...
   //
   nRows += 2;
   nColumns += 2;

   //
   // Draw the box
   //
   for (i = 0; i < nRows; i++)
   {
      x = rect.x;
      m = (i == 0) ? 0 : ((i == nRows - 1) ? 2 : 1);

      for (j = 0; j < nColumns; j++)
      {
         n = (j == 0) ? 0 : ((j == nColumns - 1) ? 2 : 1);
         PAL_RLEBlitToSurface(rglpBorderBitmap[m][n], gpScreen, PAL_XY(x, rect.y));
         x += PAL_RLEGetWidth(rglpBorderBitmap[m][n]);
      }

      rect.y += PAL_RLEGetHeight(rglpBorderBitmap[m][0]);
   }

   return lpBox;
}

LPBOX
PAL_CreateSingleLineBox(
   PAL_POS        pos,
   INT            nLen,
   BOOL           fSaveScreen
)
/*++
  Purpose:

    Create a single-line box on the screen.

  Parameters:

    [IN]  pos - position of the box.

    [IN]  nLen - length of the box.

    [IN]  fSaveScreen - whether save the used screen area or not.

  Return value:

    Pointer to a BOX structure. NULL if failed.
    If fSaveScreen is false, then always returns NULL.

--*/
{
   static const int      iNumLeftSprite   = 44;
   static const int      iNumMidSprite    = 45;
   static const int      iNumRightSprite  = 46;

   LPCBITMAPRLE          lpBitmapLeft;
   LPCBITMAPRLE          lpBitmapMid;
   LPCBITMAPRLE          lpBitmapRight;
   SDL_Surface          *save;
   SDL_Rect              rect;
   LPBOX                 lpBox = NULL;
   int                   i;

   //
   // Get the bitmaps
   //
   lpBitmapLeft = PAL_SpriteGetFrame(gpSpriteUI, iNumLeftSprite);
   lpBitmapMid = PAL_SpriteGetFrame(gpSpriteUI, iNumMidSprite);
   lpBitmapRight = PAL_SpriteGetFrame(gpSpriteUI, iNumRightSprite);

   rect.x = PAL_X(pos);
   rect.y = PAL_Y(pos);

   //
   // Get the total width and total height of the box
   //
   rect.w = PAL_RLEGetWidth(lpBitmapLeft) + PAL_RLEGetWidth(lpBitmapRight);
   rect.w += PAL_RLEGetWidth(lpBitmapMid) * nLen;
   rect.h = PAL_RLEGetHeight(lpBitmapLeft);

   if (fSaveScreen)
   {
      //
      // Save the used part of the screen
      //
      save = SDL_CreateRGBSurface(gpScreen->flags, rect.w, rect.h, 8,
         gpScreen->format->Rmask, gpScreen->format->Gmask,
         gpScreen->format->Bmask, gpScreen->format->Amask);

      if (save == NULL)
      {
         return NULL;
      }

      lpBox = (LPBOX)calloc(1, sizeof(BOX));
      if (lpBox == NULL)
      {
         SDL_FreeSurface(gpScreen);
         return NULL;
      }

#if SDL_VERSION_ATLEAST(2,0,0)
      SDL_SetSurfacePalette(save, gpScreen->format->palette);
#else
      SDL_SetPalette(save, SDL_PHYSPAL | SDL_LOGPAL, VIDEO_GetPalette(), 0, 256);
#endif
      SDL_BlitSurface(gpScreen, &rect, save, NULL);

      lpBox->pos = pos;
      lpBox->lpSavedArea = save;
      lpBox->wHeight = (WORD)rect.w;
      lpBox->wWidth = (WORD)rect.h;
   }

   //
   // Draw the box
   //
   PAL_RLEBlitToSurface(lpBitmapLeft, gpScreen, pos);

   rect.x += PAL_RLEGetWidth(lpBitmapLeft);

   for (i = 0; i < nLen; i++)
   {
      PAL_RLEBlitToSurface(lpBitmapMid, gpScreen, PAL_XY(rect.x, rect.y));
      rect.x += PAL_RLEGetWidth(lpBitmapMid);
   }

   PAL_RLEBlitToSurface(lpBitmapRight, gpScreen, PAL_XY(rect.x, rect.y));

   return lpBox;
}

VOID
PAL_DeleteBox(
   LPBOX          lpBox
)
/*++
  Purpose:

    Delete a box and restore the saved part of the screen.

  Parameters:

    [IN]  lpBox - pointer to the BOX struct.

  Return value:

    None.

--*/
{
   SDL_Rect        rect;

   //
   // Check for NULL pointer.
   //
   if (lpBox == NULL)
   {
      return;
   }

   //
   // Restore the saved screen part
   //
   rect.x = PAL_X(lpBox->pos);
   rect.y = PAL_Y(lpBox->pos);
   rect.w = lpBox->wWidth;
   rect.h = lpBox->wHeight;

   SDL_BlitSurface(lpBox->lpSavedArea, NULL, gpScreen, &rect);

   //
   // Free the memory used by the box
   //
   SDL_FreeSurface(lpBox->lpSavedArea);
   free(lpBox);
}

WORD
PAL_ReadMenu(
   LPITEMCHANGED_CALLBACK    lpfnMenuItemChanged,
   LPMENUITEM                rgMenuItem,
   INT                       nMenuItem,
   WORD                      wDefaultItem,
   BYTE                      bLabelColor
)
/*++
  Purpose:

    Execute a menu.

  Parameters:

    [IN]  lpfnMenuItemChanged - Callback function which is called when user
                                changed the current menu item.

    [IN]  rgMenuItem - Array of the menu items.

    [IN]  nMenuItem - Number of menu items.

    [IN]  wDefaultItem - default item index.

    [IN]  bLabelColor - color of the labels.

  Return value:

    Return value of the selected menu item. MENUITEM_VALUE_CANCELLED if cancelled.

--*/
{
   int               i;
   WORD              wCurrentItem    = (wDefaultItem < nMenuItem) ? wDefaultItem : 0;

   //
   // Draw all the menu texts.
   //
   for (i = 0; i < nMenuItem; i++)
   {
      BYTE bColor = bLabelColor;

      if (!rgMenuItem[i].fEnabled)
      {
         if (i == wCurrentItem)
         {
            bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
         }
         else
         {
            bColor = MENUITEM_COLOR_INACTIVE;
         }
      }

      PAL_DrawText(PAL_GetWord(rgMenuItem[i].wNumWord), rgMenuItem[i].pos,
         bColor, TRUE, TRUE);
   }

   if (lpfnMenuItemChanged != NULL)
   {
      (*lpfnMenuItemChanged)(rgMenuItem[wDefaultItem].wValue);
   }

   while (TRUE)
   {
      PAL_ClearKeyState();

      //
      // Redraw the selected item if needed.
      //
      if (rgMenuItem[wCurrentItem].fEnabled)
      {
         PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
            rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED, FALSE, TRUE);
      }

      PAL_ProcessEvent();

      if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
      {
         //
         // User pressed the down or right arrow key
         //
         if (rgMenuItem[wCurrentItem].fEnabled)
         {
            //
            // Dehighlight the unselected item.
            //
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
               rgMenuItem[wCurrentItem].pos, bLabelColor, FALSE, TRUE);
         }
         else
         {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
               rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_INACTIVE, FALSE, TRUE);
         }

         wCurrentItem++;

         if (wCurrentItem >= nMenuItem)
         {
            wCurrentItem = 0;
         }

         //
         // Highlight the selected item.
         //
         if (rgMenuItem[wCurrentItem].fEnabled)
         {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
               rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED, FALSE, TRUE);
         }
         else
         {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
               rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED_INACTIVE, FALSE, TRUE);
         }

         if (lpfnMenuItemChanged != NULL)
         {
            (*lpfnMenuItemChanged)(rgMenuItem[wCurrentItem].wValue);
         }
      }
      else if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
      {
         //
         // User pressed the up or left arrow key
         //
         if (rgMenuItem[wCurrentItem].fEnabled)
         {
            //
            // Dehighlight the unselected item.
            //
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
               rgMenuItem[wCurrentItem].pos, bLabelColor, FALSE, TRUE);
         }
         else
         {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
               rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_INACTIVE, FALSE, TRUE);
         }

         if (wCurrentItem > 0)
         {
            wCurrentItem--;
         }
         else
         {
            wCurrentItem = nMenuItem - 1;
         }

         //
         // Highlight the selected item.
         //
         if (rgMenuItem[wCurrentItem].fEnabled)
         {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
               rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED, FALSE, TRUE);
         }
         else
         {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
               rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED_INACTIVE, FALSE, TRUE);
         }

         if (lpfnMenuItemChanged != NULL)
         {
            (*lpfnMenuItemChanged)(rgMenuItem[wCurrentItem].wValue);
         }
      }
      else if (g_InputState.dwKeyPress & kKeyMenu)
      {
         //
         // User cancelled
         //
         if (rgMenuItem[wCurrentItem].fEnabled)
         {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
               rgMenuItem[wCurrentItem].pos, bLabelColor, FALSE, TRUE);
         }
         else
         {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
               rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_INACTIVE, FALSE, TRUE);
         }

         break;
      }
      else if (g_InputState.dwKeyPress & kKeySearch)
      {
         //
         // User pressed Enter
         //
         if (rgMenuItem[wCurrentItem].fEnabled)
         {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
               rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_CONFIRMED, FALSE, TRUE);

            return rgMenuItem[wCurrentItem].wValue;
         }
      }

      //
      // Use delay function to avoid high CPU usage.
      //
      SDL_Delay(50);
   }

   return MENUITEM_VALUE_CANCELLED;
}

VOID
PAL_DrawNumber(
   UINT            iNum,
   UINT            nLength,
   PAL_POS         pos,
   NUMCOLOR        color,
   NUMALIGN        align
)
/*++
  Purpose:

    Draw the specified number with the bitmaps in the UI sprite.

  Parameters:

    [IN]  iNum - the number to be drawn.

    [IN]  nLength - max. length of the number.

    [IN]  pos - position on the screen.

    [IN]  color - color of the number (yellow or blue).

    [IN]  align - align mode of the number.

  Return value:

    None.

--*/
{
   UINT          nActualLength, i;
   int           x, y;
   LPCBITMAPRLE  rglpBitmap[10];

   //
   // Get the bitmaps. Blue starts from 29, Cyan from 56, Yellow from 19.
   //
   x = (color == kNumColorBlue) ? 29 : ((color == kNumColorCyan) ? 56 : 19);

   for (i = 0; i < 10; i++)
   {
      rglpBitmap[i] = PAL_SpriteGetFrame(gpSpriteUI, (UINT)x + i);
   }

   i = iNum;
   nActualLength = 0;

   //
   // Calculate the actual length of the number.
   //
   while (i > 0)
   {
      i /= 10;
      nActualLength++;
   }

   if (nActualLength > nLength)
   {
      nActualLength = nLength;
   }
   else if (nActualLength == 0)
   {
      nActualLength = 1;
   }

   x = PAL_X(pos) - 6;
   y = PAL_Y(pos);

   switch (align)
   {
   case kNumAlignLeft:
      x += 6 * nActualLength;
      break;

   case kNumAlignMid:
      x += 3 * (nLength + nActualLength);
      break;

   case kNumAlignRight:
      x += 6 * nLength;
      break;
   }

   //
   // Draw the number.
   //
   while (nActualLength-- > 0)
   {
      PAL_RLEBlitToSurface(rglpBitmap[iNum % 10], gpScreen, PAL_XY(x, y));
      x -= 6;
      iNum /= 10;
   }
}

#ifndef PAL_WIN95

LPOBJECTDESC
PAL_LoadObjectDesc(
   LPCSTR         lpszFileName
)
/*++
  Purpose:

    Load the object description strings from file.

  Parameters:

    [IN]  lpszFileName - the filename to be loaded.

  Return value:

    Pointer to loaded data, in linked list form. NULL if unable to load.

--*/
{
   FILE                      *fp;
   PAL_LARGE char             buf[512];
   char                      *p;
   LPOBJECTDESC               lpDesc = NULL, pNew = NULL;
   unsigned int               i;

   fp = fopen(lpszFileName, "r");

   if (fp == NULL)
   {
      return NULL;
   }

   //
   // Load the description data
   //
   while (fgets(buf, 512, fp) != NULL)
   {
      p = strchr(buf, '=');
      if (p == NULL)
      {
         continue;
      }

      *p = '\0';
      p++;

      pNew = UTIL_calloc(1, sizeof(OBJECTDESC));

      sscanf(buf, "%x", &i);
      pNew->wObjectID = i;
      pNew->lpDesc = strdup(p);

      pNew->next = lpDesc;
      lpDesc = pNew;
   }

   fclose(fp);
   return lpDesc;
}

VOID
PAL_FreeObjectDesc(
   LPOBJECTDESC   lpObjectDesc
)
/*++
  Purpose:

    Free the object description data.

  Parameters:

    [IN]  lpObjectDesc - the description data to be freed.

  Return value:

    None.

--*/
{
   LPOBJECTDESC    p;

   while (lpObjectDesc != NULL)
   {
      p = lpObjectDesc->next;
      free(lpObjectDesc->lpDesc);
      free(lpObjectDesc);
      lpObjectDesc = p;
   }
}

LPCSTR
PAL_GetObjectDesc(
   LPOBJECTDESC   lpObjectDesc,
   WORD           wObjectID
)
/*++
  Purpose:

    Get the object description string from the linked list.

  Parameters:

    [IN]  lpObjectDesc - the description data linked list.

    [IN]  wObjectID - the object ID.

  Return value:

    The description string. NULL if the specified object ID
    is not found.

--*/
{
   while (lpObjectDesc != NULL)
   {
      if (lpObjectDesc->wObjectID == wObjectID)
      {
         return lpObjectDesc->lpDesc;
      }

      lpObjectDesc = lpObjectDesc->next;
   }

   return NULL;
}

#endif
