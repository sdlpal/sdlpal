/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
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

static int     g_iNumInventory = 0;
static WORD    g_wItemFlags = 0;
static BOOL    g_fNoDesc = FALSE;

WORD
PAL_ItemSelectMenuUpdate(
   BOOL                      fIsInvMenu
)
/*++
  Purpose:

    Initialize the item selection menu.

  Parameters:

    [IN]  fIsInvMenu - True if the current menu is inventory, FALSE if the current menu is sell menu.

  Return value:

    The object ID of the selected item. 0 if cancelled, 0xFFFF if not confirmed.

--*/
{
   int                i, j, k, line, item_delta;
   int               *iCurMenuItem = (fIsInvMenu) ? &gpGlobals->iCurInvMenuItem : &gpGlobals->iCurSellMenuItem;
   WORD               wObject, wScript;
   BYTE               bColor;
   static BYTE        bufImage[2048];
   const int          iItemsPerLine = 32 / gConfig.dwWordLength;
   const int          iItemTextWidth = 8 * gConfig.dwWordLength + 20;
   const int          iLinesPerPage = 7 - gConfig.ScreenLayout.ExtraItemDescLines;
   const int          iCursorXOffset = gConfig.dwWordLength * 5 / 2;
   const int          iAmountXOffset = gConfig.dwWordLength * 8 + 1;
   const int          iPageLineOffset = (iLinesPerPage + 1) / 2;
   const int          iPictureYOffset = (gConfig.ScreenLayout.ExtraItemDescLines > 1) ? (gConfig.ScreenLayout.ExtraItemDescLines - 1) * 16 : 0;
   PAL_POS            cursorPos = PAL_XY(15 + iCursorXOffset, 22);

   //
   // Process input
   //
   if (g_InputState.dwKeyPress & kKeyUp)
   {
      item_delta = -iItemsPerLine;
   }
   else if (g_InputState.dwKeyPress & kKeyDown)
   {
      item_delta = iItemsPerLine;
   }
   else if (g_InputState.dwKeyPress & kKeyLeft)
   {
      item_delta = -1;
   }
   else if (g_InputState.dwKeyPress & kKeyRight)
   {
      item_delta = 1;
   }
   else if (g_InputState.dwKeyPress & kKeyPgUp)
   {
      item_delta = -(iItemsPerLine * iLinesPerPage);
   }
   else if (g_InputState.dwKeyPress & kKeyPgDn)
   {
      item_delta = iItemsPerLine * iLinesPerPage;
   }
   else if (g_InputState.dwKeyPress & kKeyHome)
   {
      item_delta = -(*iCurMenuItem);
   }
   else if (g_InputState.dwKeyPress & kKeyEnd)
   {
      item_delta = g_iNumInventory - (*iCurMenuItem) - 1;
   }
   else if (g_InputState.dwKeyPress & kKeyMenu)
   {
      return 0;
   }
   else
   {
      item_delta = 0;
   }

   //
   // Make sure the current menu item index is in bound
   //
   if ((*iCurMenuItem) + item_delta < 0)
      (*iCurMenuItem) = 0;
   else if ((*iCurMenuItem) + item_delta >= g_iNumInventory)
      (*iCurMenuItem) = g_iNumInventory-1;
   else
      (*iCurMenuItem) += item_delta;

   //
   // Redraw the box
   //
   PAL_CreateBoxWithShadow(PAL_XY(2, 0), iLinesPerPage - 1, 17, 1, FALSE, 0);

   //
   // Draw the texts in the current page
   //
   i = (*iCurMenuItem) / iItemsPerLine * iItemsPerLine - iItemsPerLine * iPageLineOffset;
   if (i < 0)
   {
      i = 0;
   }

   const int xBase = 0, yBase = 140;

   for (j = 0; j < iLinesPerPage; j++)
   {
      for (k = 0; k < iItemsPerLine; k++)
      {
         wObject = gpGlobals->rgInventory[i].wItem;
         bColor = MENUITEM_COLOR;

         if (i >= MAX_INVENTORY || wObject == 0)
         {
            //
            // End of the list reached
            //
            j = iLinesPerPage;
            break;
         }

         if (i == (*iCurMenuItem))
         {
            if (!(gpGlobals->g.rgObject[wObject].item.wFlags & g_wItemFlags) ||
               (SHORT)gpGlobals->rgInventory[i].nAmount <= (SHORT)gpGlobals->rgInventory[i].nAmountInUse)
            {
               //
               // This item is not selectable
               //
               bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
            }
            else
            {
               //
               // This item is selectable
               //
               if (gpGlobals->rgInventory[i].nAmount == 0)
               {
                  bColor = MENUITEM_COLOR_EQUIPPEDITEM;
               }
               else
               {
                  bColor = MENUITEM_COLOR_SELECTED;
               }
            }
         }
         else if (!(gpGlobals->g.rgObject[wObject].item.wFlags & g_wItemFlags) ||
            (SHORT)gpGlobals->rgInventory[i].nAmount <= (SHORT)gpGlobals->rgInventory[i].nAmountInUse)
         {
            //
            // This item is not selectable
            //
            bColor = MENUITEM_COLOR_INACTIVE;
         }
         else if (gpGlobals->rgInventory[i].nAmount == 0)
         {
            bColor = MENUITEM_COLOR_EQUIPPEDITEM;
         }

         //
         // Draw the text
         //
         PAL_DrawText(PAL_GetWord(wObject), PAL_XY(15 + k * iItemTextWidth, 12 + j * 18), bColor, TRUE, FALSE, FALSE);

         if (i == (*iCurMenuItem))
         {
            cursorPos = PAL_XY(15 + iCursorXOffset + k * iItemTextWidth, 22 + j * 18);

            //
            // Draw the picture of current selected item
            //
            PAL_RLEBlitToSurfaceWithShadow(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen,
               PAL_XY(xBase + 5, yBase + 5 - iPictureYOffset), TRUE);
            PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen,
               PAL_XY(xBase, yBase - iPictureYOffset));

            if (PAL_MKFReadChunk(bufImage, 2048,
               gpGlobals->g.rgObject[wObject].item.wBitmap, gpGlobals->f.fpBALL) > 0)
            {
               PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(xBase + 8, yBase + 7 - iPictureYOffset));
            }
         }

         //
         // Draw the amount of this item
         //
         if ((SHORT)gpGlobals->rgInventory[i].nAmount - (SHORT)gpGlobals->rgInventory[i].nAmountInUse > 1)
         {
            PAL_DrawNumber(gpGlobals->rgInventory[i].nAmount - gpGlobals->rgInventory[i].nAmountInUse,
               2, PAL_XY(15 + iAmountXOffset + k * iItemTextWidth, 17 + j * 18), kNumColorCyan, kNumAlignRight);
         }

         i++;
      }
   }

   //
   // Draw the cursor on the current selected item
   //
   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR), gpScreen, cursorPos);

   wObject = gpGlobals->rgInventory[*iCurMenuItem].wItem;

   //
   // Draw the description of the selected item
   //
   if (!gConfig.fIsWIN95)
   {
      if (!g_fNoDesc && gpGlobals->lpObjectDesc != NULL)
	  {
         WCHAR szDesc[512], *next;
         const WCHAR *d = PAL_GetObjectDesc(gpGlobals->lpObjectDesc, wObject);

         if (d != NULL)
         {
            k = 150 - gConfig.ScreenLayout.ExtraItemDescLines * 16;
            wcscpy(szDesc, d);
            d = szDesc;

            while (TRUE)
            {
               next = wcschr(d, '*');
               if (next != NULL)
               {
                  *next++ = '\0';
               }

               PAL_DrawText(d, PAL_XY(75, k), DESCTEXT_COLOR, TRUE, FALSE, FALSE);
               k += 16;

               if (next == NULL)
               {
                  break;
               }

               d = next;
            }
         }
      }
   }
   else
   {
      if (!g_fNoDesc)
      {
         wScript = gpGlobals->g.rgObject[wObject].item.wScriptDesc;
         line = 0;
         while (wScript && gpGlobals->g.lprgScriptEntry[wScript].wOperation != 0)
         {
            if (gpGlobals->g.lprgScriptEntry[wScript].wOperation == 0xFFFF)
            {
               int line_incr = (gpGlobals->g.lprgScriptEntry[wScript].rgwOperand[1] != 1) ? 1 : 0;
               wScript = PAL_RunAutoScript(wScript, PAL_ITEM_DESC_BOTTOM | line);
               line += line_incr;
            }
            else
            {
               wScript = PAL_RunAutoScript(wScript, 0);
            }
         }
      }
   }

   if (g_InputState.dwKeyPress & kKeySearch)
   {
      if ((gpGlobals->g.rgObject[wObject].item.wFlags & g_wItemFlags) &&
         (SHORT)gpGlobals->rgInventory[*iCurMenuItem].nAmount >
         (SHORT)gpGlobals->rgInventory[*iCurMenuItem].nAmountInUse)
      {
         if (gpGlobals->rgInventory[*iCurMenuItem].nAmount > 0)
         {
            j = ((*iCurMenuItem) < iItemsPerLine * iPageLineOffset) ? ((*iCurMenuItem) / iItemsPerLine) : iPageLineOffset;
            k = (*iCurMenuItem) % iItemsPerLine;

            PAL_DrawText(PAL_GetWord(wObject), PAL_XY(15 + k * iItemTextWidth, 12 + j * 18), MENUITEM_COLOR_CONFIRMED, FALSE, FALSE, FALSE);

            //
            // Draw the cursor on the current selected item
            //
            PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR), gpScreen, cursorPos);
         }

         return wObject;
      }
   }

   return 0xFFFF;
}

VOID
PAL_ItemSelectMenuInit(
   WORD                      wItemFlags
)
/*++
  Purpose:

    Initialize the item selection menu.

  Parameters:

    [IN]  wItemFlags - flags for usable item.

  Return value:

    None.

--*/
{
   int           i, j;
   WORD          w;

   g_wItemFlags = wItemFlags;

   //
   // Compress the inventory
   //
   PAL_CompressInventory();

   //
   // Count the total number of items in inventory
   //
   g_iNumInventory = 0;
   while (g_iNumInventory < MAX_INVENTORY &&
      gpGlobals->rgInventory[g_iNumInventory].wItem != 0)
   {
      g_iNumInventory++;
   }

   //
   // Also add usable equipped items to the list
   //
   if ((wItemFlags & kItemFlagUsable) && !gpGlobals->fInBattle)
   {
      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         w = gpGlobals->rgParty[i].wPlayerRole;

         for (j = 0; j < MAX_PLAYER_EQUIPMENTS; j++)
         {
            if (gpGlobals->g.rgObject[gpGlobals->g.PlayerRoles.rgwEquipment[j][w]].item.wFlags & kItemFlagUsable)
            {
               if (g_iNumInventory < MAX_INVENTORY)
               {
                  gpGlobals->rgInventory[g_iNumInventory].wItem = gpGlobals->g.PlayerRoles.rgwEquipment[j][w];
                  gpGlobals->rgInventory[g_iNumInventory].nAmount = 0;
                  gpGlobals->rgInventory[g_iNumInventory].nAmountInUse = (WORD)-1;

                  g_iNumInventory++;
               }
            }
         }
      }
   }
}

WORD
PAL_ItemSelectMenu(
   LPITEMCHANGED_CALLBACK    lpfnMenuItemChanged,
   WORD                      wItemFlags
)
/*++
  Purpose:

    Show the item selection menu.

  Parameters:

    [IN]  lpfnMenuItemChanged - Callback function which is called when user
                                changed the current menu item.

    [IN]  wItemFlags - flags for usable item.

  Return value:

    The object ID of the selected item. 0 if cancelled.

--*/
{
   int              iPrevIndex;
   int             *iCurMenuItem = (wItemFlags != kItemFlagSellable) ? &gpGlobals->iCurInvMenuItem : &gpGlobals->iCurSellMenuItem;
   WORD             w;
   DWORD            dwTime;

   // Special handling in Win version, no longer requiring cursor position recording
   if (gConfig.fIsWIN95 && wItemFlags == kItemFlagSellable) *iCurMenuItem = 0;

   PAL_ItemSelectMenuInit(wItemFlags);
   iPrevIndex = *iCurMenuItem;

   PAL_ClearKeyState();

   if (lpfnMenuItemChanged != NULL)
   {
      g_fNoDesc = TRUE;
      (*lpfnMenuItemChanged)(gpGlobals->rgInventory[*iCurMenuItem].wItem);
   }

   dwTime = SDL_GetTicks();

   while (TRUE)
   {
      if (lpfnMenuItemChanged == NULL)
      {
         PAL_MakeScene();
      }

      w = PAL_ItemSelectMenuUpdate(wItemFlags != kItemFlagSellable);
      VIDEO_UpdateScreen(NULL);

      PAL_ClearKeyState();

      PAL_ProcessEvent();
      while (!SDL_TICKS_PASSED(SDL_GetTicks(), dwTime))
      {
         PAL_ProcessEvent();
         if (g_InputState.dwKeyPress != 0)
         {
            break;
         }
         SDL_Delay(5);
      }

      dwTime = SDL_GetTicks() + FRAME_TIME;

      if (w != 0xFFFF)
      {
         g_fNoDesc = FALSE;
         return w;
      }

      if (iPrevIndex != (*iCurMenuItem))
      {
         if ((*iCurMenuItem) >= 0 && (*iCurMenuItem) < MAX_INVENTORY)
         {
            if (lpfnMenuItemChanged != NULL)
            {
               (*lpfnMenuItemChanged)(gpGlobals->rgInventory[(*iCurMenuItem)].wItem);
            }
         }

         iPrevIndex = (*iCurMenuItem);
      }
   }

   assert(FALSE);
   return 0; // should not really reach here
}
