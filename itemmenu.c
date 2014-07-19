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

static int     g_iNumInventory = 0;
static WORD    g_wItemFlags = 0;
static BOOL    g_fNoDesc = FALSE;

WORD
PAL_ItemSelectMenuUpdate(
   VOID
)
/*++
  Purpose:

    Initialize the item selection menu.

  Parameters:

    None.

  Return value:

    The object ID of the selected item. 0 if cancelled, 0xFFFF if not confirmed.

--*/
{
#ifndef PAL_WIN95
   int                i, j, k;
   WORD               wObject;
#else
   int                i, j, k, line;
   WORD               wObject, wScript;
#endif
   BYTE               bColor;
   static BYTE        bufImage[2048];
   static WORD        wPrevImageIndex = 0xFFFF;

   //
   // Process input
   //
   if (g_InputState.dwKeyPress & kKeyUp)
   {
      gpGlobals->iCurInvMenuItem -= 3;
   }
   else if (g_InputState.dwKeyPress & kKeyDown)
   {
      gpGlobals->iCurInvMenuItem += 3;
   }
   else if (g_InputState.dwKeyPress & kKeyLeft)
   {
      gpGlobals->iCurInvMenuItem--;
   }
   else if (g_InputState.dwKeyPress & kKeyRight)
   {
      gpGlobals->iCurInvMenuItem++;
   }
   else if (g_InputState.dwKeyPress & kKeyPgUp)
   {
      gpGlobals->iCurInvMenuItem -= 3 * 7;
   }
   else if (g_InputState.dwKeyPress & kKeyPgDn)
   {
      gpGlobals->iCurInvMenuItem += 3 * 7;
   }
   else if (g_InputState.dwKeyPress & kKeyMenu)
   {
      return 0;
   }

   //
   // Make sure the current menu item index is in bound
   //
   if (gpGlobals->iCurInvMenuItem < 0)
   {
      gpGlobals->iCurInvMenuItem = 0;
   }
   else if (gpGlobals->iCurInvMenuItem >= g_iNumInventory)
   {
      gpGlobals->iCurInvMenuItem = g_iNumInventory - 1;
   }

   //
   // Redraw the box
   //
   PAL_CreateBox(PAL_XY(2, 0), 6, 17, 1, FALSE);

   //
   // Draw the texts in the current page
   //
   i = gpGlobals->iCurInvMenuItem / 3 * 3 - 3 * 4;
   if (i < 0)
   {
      i = 0;
   }

   for (j = 0; j < 7; j++)
   {
      for (k = 0; k < 3; k++)
      {
         wObject = gpGlobals->rgInventory[i].wItem;
         bColor = MENUITEM_COLOR;

         if (i >= MAX_INVENTORY || wObject == 0)
         {
            //
            // End of the list reached
            //
            j = 7;
            break;
         }

         if (i == gpGlobals->iCurInvMenuItem)
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
         PAL_DrawText(PAL_GetWord(wObject), PAL_XY(15 + k * 100, 12 + j * 18),
            bColor, TRUE, FALSE);

         //
         // Draw the cursor on the current selected item
         //
         if (i == gpGlobals->iCurInvMenuItem)
         {
            PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR),
               gpScreen, PAL_XY(40 + k * 100, 22 + j * 18));
         }

         //
         // Draw the amount of this item
         //
		 if ((SHORT)gpGlobals->rgInventory[i].nAmount - (SHORT)gpGlobals->rgInventory[i].nAmountInUse > 1)
		 {
            PAL_DrawNumber(gpGlobals->rgInventory[i].nAmount - gpGlobals->rgInventory[i].nAmountInUse,
               2, PAL_XY(96 + k * 100, 17 + j * 18), kNumColorCyan, kNumAlignRight);
		 }

         i++;
      }
   }

   //
   // Draw the picture of current selected item
   //
   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen,
      PAL_XY(5, 140));

   wObject = gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].wItem;

   if (gpGlobals->g.rgObject[wObject].item.wBitmap != wPrevImageIndex)
   {
      if (PAL_MKFReadChunk(bufImage, 2048,
         gpGlobals->g.rgObject[wObject].item.wBitmap, gpGlobals->f.fpBALL) > 0)
      {
         wPrevImageIndex = gpGlobals->g.rgObject[wObject].item.wBitmap;
      }
      else
      {
         wPrevImageIndex = 0xFFFF;
      }
   }

   if (wPrevImageIndex != 0xFFFF)
   {
      PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(12, 148));
   }

   //
   // Draw the description of the selected item
   //
#ifndef PAL_WIN95
   if (!g_fNoDesc && gpGlobals->lpObjectDesc != NULL)
   {
      char szDesc[512], *next;
      const char *d = PAL_GetObjectDesc(gpGlobals->lpObjectDesc, wObject);

      if (d != NULL)
      {
         k = 150;
         strcpy(szDesc, d);
         d = szDesc;

         while (TRUE)
         {
            next = strchr(d, '*');
            if (next != NULL)
            {
               *next = '\0';
               next++;
            }

            PAL_DrawText(d, PAL_XY(75, k), DESCTEXT_COLOR, TRUE, FALSE);
            k += 16;

            if (next == NULL)
            {
               break;
            }

            d = next;
         }
      }
   }
#else
   if (!g_fNoDesc)
   {
      wScript = gpGlobals->g.rgObject[wObject].item.wScriptDesc;
      line = 0;
      while (wScript && gpGlobals->g.lprgScriptEntry[wScript].wOperation != 0)
      {
         if (gpGlobals->g.lprgScriptEntry[wScript].wOperation == 0xFFFF)
         {
            wScript = PAL_RunAutoScript(wScript, (1 << 15) | line);
            line++;
         }
         else
         {
            wScript = PAL_RunAutoScript(wScript, 0);
         }
      }
   }
#endif

   if (g_InputState.dwKeyPress & kKeySearch)
   {
      if ((gpGlobals->g.rgObject[wObject].item.wFlags & g_wItemFlags) &&
         (SHORT)gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].nAmount >
         (SHORT)gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].nAmountInUse)
      {
         if (gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].nAmount > 0)
         {
            j = (gpGlobals->iCurInvMenuItem < 3 * 4) ? (gpGlobals->iCurInvMenuItem / 3) : 4;
            k = gpGlobals->iCurInvMenuItem % 3;

            PAL_DrawText(PAL_GetWord(wObject), PAL_XY(15 + k * 100, 12 + j * 18),
               MENUITEM_COLOR_CONFIRMED, FALSE, FALSE);
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
   WORD             w;
   DWORD            dwTime;

   PAL_ItemSelectMenuInit(wItemFlags);
   iPrevIndex = gpGlobals->iCurInvMenuItem;

   PAL_ClearKeyState();

   if (lpfnMenuItemChanged != NULL)
   {
      g_fNoDesc = TRUE;
      (*lpfnMenuItemChanged)(gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].wItem);
   }

   dwTime = SDL_GetTicks();

   while (TRUE)
   {
      if (lpfnMenuItemChanged == NULL)
      {
         PAL_MakeScene();
      }

      w = PAL_ItemSelectMenuUpdate();
      VIDEO_UpdateScreen(NULL);

      PAL_ClearKeyState();

      PAL_ProcessEvent();
      while (SDL_GetTicks() < dwTime)
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

      if (iPrevIndex != gpGlobals->iCurInvMenuItem)
      {
         if (gpGlobals->iCurInvMenuItem >= 0 && gpGlobals->iCurInvMenuItem < MAX_INVENTORY)
         {
            if (lpfnMenuItemChanged != NULL)
            {
               (*lpfnMenuItemChanged)(gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].wItem);
            }
         }

         iPrevIndex = gpGlobals->iCurInvMenuItem;
      }
   }

   assert(FALSE);
   return 0; // should not really reach here
}
