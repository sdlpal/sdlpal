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

static struct MAGICITEM
{
   WORD         wMagic;
   WORD         wMP;
   BOOL         fEnabled;
} rgMagicItem[MAX_PLAYER_MAGICS];

static int     g_iNumMagic = 0;
static int     g_iCurrentItem = 0;
static WORD    g_wPlayerMP = 0;

WORD
PAL_MagicSelectionMenuUpdate(
   VOID
)
/*++
  Purpose:

    Update the magic selection menu.

  Parameters:

    None.

  Return value:

    The selected magic. 0 if cancelled, 0xFFFF if not confirmed.

--*/
{
#ifndef PAL_WIN95
   int         i, j, k;
   BYTE        bColor;
#else
   int         i, j, k, line;
   BYTE        bColor;
   WORD        wScript;
#endif

   //
   // Check for inputs
   //
   if (g_InputState.dwKeyPress & kKeyUp)
   {
      g_iCurrentItem -= 3;
   }
   else if (g_InputState.dwKeyPress & kKeyDown)
   {
      g_iCurrentItem += 3;
   }
   else if (g_InputState.dwKeyPress & kKeyLeft)
   {
      g_iCurrentItem--;
   }
   else if (g_InputState.dwKeyPress & kKeyRight)
   {
      g_iCurrentItem++;
   }
   else if (g_InputState.dwKeyPress & kKeyPgUp)
   {
      g_iCurrentItem -= 3 * 5;
   }
   else if (g_InputState.dwKeyPress & kKeyPgDn)
   {
      g_iCurrentItem += 3 * 5;
   }
   else if (g_InputState.dwKeyPress & kKeyMenu)
   {
      return 0;
   }

   //
   // Make sure the current menu item index is in bound
   //
   if (g_iCurrentItem < 0)
   {
      g_iCurrentItem = 0;
   }
   else if (g_iCurrentItem >= g_iNumMagic)
   {
      g_iCurrentItem = g_iNumMagic - 1;
   }

   //
   // Create the box.
   //
   PAL_CreateBox(PAL_XY(10, 42), 4, 16, 1, FALSE);

#ifndef PAL_WIN95
   if (gpGlobals->lpObjectDesc == NULL)
   {
      //
      // Draw the cash amount.
      //
      PAL_CreateSingleLineBox(PAL_XY(0, 0), 5, FALSE);
      PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(10, 10), 0, FALSE, FALSE);
      PAL_DrawNumber(gpGlobals->dwCash, 6, PAL_XY(49, 14), kNumColorYellow, kNumAlignRight);

      //
      // Draw the MP of the selected magic.
      //
      PAL_CreateSingleLineBox(PAL_XY(215, 0), 5, FALSE);
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH),
         gpScreen, PAL_XY(260, 14));
      PAL_DrawNumber(rgMagicItem[g_iCurrentItem].wMP, 4, PAL_XY(230, 14),
         kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(g_wPlayerMP, 4, PAL_XY(265, 14), kNumColorCyan, kNumAlignRight);
   }
   else
   {
      char szDesc[512], *next;
      const char *d = PAL_GetObjectDesc(gpGlobals->lpObjectDesc, rgMagicItem[g_iCurrentItem].wMagic);

      //
      // Draw the magic description.
      //
      if (d != NULL)
      {
         k = 3;
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

            PAL_DrawText(d, PAL_XY(100, k), DESCTEXT_COLOR, TRUE, FALSE);
            k += 16;

            if (next == NULL)
            {
               break;
            }

            d = next;
         }
      }

      //
      // Draw the MP of the selected magic.
      //
      PAL_CreateSingleLineBox(PAL_XY(0, 0), 5, FALSE);
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH),
         gpScreen, PAL_XY(45, 14));
      PAL_DrawNumber(rgMagicItem[g_iCurrentItem].wMP, 4, PAL_XY(15, 14),
         kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(g_wPlayerMP, 4, PAL_XY(50, 14), kNumColorCyan, kNumAlignRight);
   }
#else
   wScript = gpGlobals->g.rgObject[rgMagicItem[g_iCurrentItem].wMagic].item.wScriptDesc;
   line = 0;
   while (wScript && gpGlobals->g.lprgScriptEntry[wScript].wOperation != 0)
   {
      if (gpGlobals->g.lprgScriptEntry[wScript].wOperation == 0xFFFF)
      {
         wScript = PAL_RunAutoScript(wScript, line);
         line++;
      }
      else
      {
         wScript = PAL_RunAutoScript(wScript, 0);
      }
   }

   //
   // Draw the MP of the selected magic.
   //
   PAL_CreateSingleLineBox(PAL_XY(0, 0), 5, FALSE);
   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH),
      gpScreen, PAL_XY(45, 14));
   PAL_DrawNumber(rgMagicItem[g_iCurrentItem].wMP, 4, PAL_XY(15, 14),
      kNumColorYellow, kNumAlignRight);
   PAL_DrawNumber(g_wPlayerMP, 4, PAL_XY(50, 14), kNumColorCyan, kNumAlignRight);
#endif


   //
   // Draw the texts of the current page
   //
   i = g_iCurrentItem / 3 * 3 - 3 * 2;
   if (i < 0)
   {
      i = 0;
   }

   for (j = 0; j < 5; j++)
   {
      for (k = 0; k < 3; k++)
      {
         bColor = MENUITEM_COLOR;

         if (i >= g_iNumMagic)
         {
            //
            // End of the list reached
            //
            j = 5;
            break;
         }

         if (i == g_iCurrentItem)
         {
            if (rgMagicItem[i].fEnabled)
            {
               bColor = MENUITEM_COLOR_SELECTED;
            }
            else
            {
               bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
            }
         }
         else if (!rgMagicItem[i].fEnabled)
         {
            bColor = MENUITEM_COLOR_INACTIVE;
         }

         //
         // Draw the text
         //
         PAL_DrawText(PAL_GetWord(rgMagicItem[i].wMagic),
            PAL_XY(35 + k * 87, 54 + j * 18), bColor, TRUE, FALSE);

         //
         // Draw the cursor on the current selected item
         //
         if (i == g_iCurrentItem)
         {
            PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR),
               gpScreen, PAL_XY(60 + k * 87, 64 + j * 18));
         }

         i++;
      }
   }

   if (g_InputState.dwKeyPress & kKeySearch)
   {
      if (rgMagicItem[g_iCurrentItem].fEnabled)
      {
         j = g_iCurrentItem % 3;
         k = (g_iCurrentItem < 3 * 2) ? (g_iCurrentItem / 3) : 2;

         j = 35 + j * 87;
         k = 54 + k * 18;

         PAL_DrawText(PAL_GetWord(rgMagicItem[g_iCurrentItem].wMagic), PAL_XY(j, k),
            MENUITEM_COLOR_CONFIRMED, FALSE, TRUE);

         return rgMagicItem[g_iCurrentItem].wMagic;
      }
   }

   return 0xFFFF;
}

VOID
PAL_MagicSelectionMenuInit(
   WORD         wPlayerRole,
   BOOL         fInBattle,
   WORD         wDefaultMagic
)
/*++
  Purpose:

    Initialize the magic selection menu.

  Parameters:

    [IN]  wPlayerRole - the player ID.

    [IN]  fInBattle - TRUE if in battle, FALSE if not.

    [IN]  wDefaultMagic - the default magic item.

  Return value:

    None.

--*/
{
   WORD       w;
   int        i, j;

   g_iCurrentItem = 0;
   g_iNumMagic = 0;

   g_wPlayerMP = gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole];

   //
   // Put all magics of this player to the array
   //
   for (i = 0; i < MAX_PLAYER_MAGICS; i++)
   {
      w = gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole];
      if (w != 0)
      {
         rgMagicItem[g_iNumMagic].wMagic = w;

         w = gpGlobals->g.rgObject[w].magic.wMagicNumber;
         rgMagicItem[g_iNumMagic].wMP = gpGlobals->g.lprgMagic[w].wCostMP;

         rgMagicItem[g_iNumMagic].fEnabled = TRUE;

         if (rgMagicItem[g_iNumMagic].wMP > g_wPlayerMP)
         {
            rgMagicItem[g_iNumMagic].fEnabled = FALSE;
         }

         w = gpGlobals->g.rgObject[rgMagicItem[g_iNumMagic].wMagic].magic.wFlags;
         if (fInBattle)
         {
            if (!(w & kMagicFlagUsableInBattle))
            {
               rgMagicItem[g_iNumMagic].fEnabled = FALSE;
            }
         }
         else
         {
            if (!(w & kMagicFlagUsableOutsideBattle))
            {
               rgMagicItem[g_iNumMagic].fEnabled = FALSE;
            }
         }

         g_iNumMagic++;
      }
   }

   //
   // Sort the array
   //
   for (i = 0; i < g_iNumMagic - 1; i++)
   {
      BOOL fCompleted = TRUE;

      for (j = 0; j < g_iNumMagic - 1 - i; j++)
      {
         if (rgMagicItem[j].wMagic > rgMagicItem[j + 1].wMagic)
         {
            struct MAGICITEM t = rgMagicItem[j];
            rgMagicItem[j] = rgMagicItem[j + 1];
            rgMagicItem[j + 1] = t;

            fCompleted = FALSE;
         }
      }

      if (fCompleted)
      {
         break;
      }
   }

   //
   // Place the cursor to the default item
   //
   for (i = 0; i < g_iNumMagic; i++)
   {
      if (rgMagicItem[i].wMagic == wDefaultMagic)
      {
         g_iCurrentItem = i;
         break;
      }
   }
}

WORD
PAL_MagicSelectionMenu(
   WORD         wPlayerRole,
   BOOL         fInBattle,
   WORD         wDefaultMagic
)
/*++
  Purpose:

    Show the magic selection menu.

  Parameters:

    [IN]  wPlayerRole - the player ID.

    [IN]  fInBattle - TRUE if in battle, FALSE if not.

    [IN]  wDefaultMagic - the default magic item.

  Return value:

    The selected magic. 0 if cancelled.

--*/
{
   WORD            w;
   int             i;
   DWORD           dwTime;

   PAL_MagicSelectionMenuInit(wPlayerRole, fInBattle, wDefaultMagic);
   PAL_ClearKeyState();

   dwTime = SDL_GetTicks();

   while (TRUE)
   {
      PAL_MakeScene();

      w = 45;

      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         PAL_PlayerInfoBox(PAL_XY(w, 165), gpGlobals->rgParty[i].wPlayerRole, 100,
            TIMEMETER_COLOR_DEFAULT, FALSE);
         w += 78;
      }

      w = PAL_MagicSelectionMenuUpdate();
      VIDEO_UpdateScreen(NULL);

      PAL_ClearKeyState();

      if (w != 0xFFFF)
      {
         return w;
      }

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
   }

   return 0; // should not really reach here
}
