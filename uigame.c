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

#include "main.h"

static BOOL __buymenu_firsttime_render;

static WORD GetSavedTimes(int iSaveSlot)
{
	FILE *fp = UTIL_OpenFileAtPath(gConfig.pszSavePath, PAL_va(0, "%d.rpg", iSaveSlot));
	WORD wSavedTimes = 0;
	if (fp != NULL)
	{
		if (fread(&wSavedTimes, sizeof(WORD), 1, fp) == 1)
			wSavedTimes = SDL_SwapLE16(wSavedTimes);
		else
			wSavedTimes = 0;
		fclose(fp);
	}
	return wSavedTimes;
}

VOID
PAL_DrawOpeningMenuBackground(
   VOID
)
/*++
  Purpose:

    Draw the background of the main menu.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   LPBYTE        buf;

   buf = (LPBYTE)malloc(320 * 200);
   if (buf == NULL)
   {
      return;
   }

   //
   // Read the picture from fbp.mkf.
   //
   PAL_MKFDecompressChunk(buf, 320 * 200, MAINMENU_BACKGROUND_FBPNUM, gpGlobals->f.fpFBP);

   //
   // ...and blit it to the screen buffer.
   //
   PAL_FBPBlitToSurface(buf, gpScreen);
   VIDEO_UpdateScreen(NULL);

   free(buf);
}

INT
PAL_OpeningMenu(
   VOID
)
/*++
  Purpose:

    Show the opening menu.

  Parameters:

    None.

  Return value:

    Which saved slot to load from (1-5). 0 to start a new game.

--*/
{
   WORD          wItemSelected;
   WORD          wDefaultItem     = 0;
   INT           w[2] = { PAL_WordWidth(MAINMENU_LABEL_NEWGAME), PAL_WordWidth(MAINMENU_LABEL_LOADGAME) };

   MENUITEM      rgMainMenuItem[2] = {
      // value   label                     enabled   position
      {  0,      MAINMENU_LABEL_NEWGAME,   TRUE,     PAL_XY(125 - (w[0] > 4 ? (w[0] - 4) * 8 : 0), 95)  },
      {  1,      MAINMENU_LABEL_LOADGAME,  TRUE,     PAL_XY(125 - (w[1] > 4 ? (w[1] - 4) * 8 : 0), 112) }
   };

   //
   // Play the background music
   //
   AUDIO_PlayMusic(RIX_NUM_OPENINGMENU, TRUE, 1);

   //
   // Draw the background
   //
   PAL_DrawOpeningMenuBackground();
   PAL_FadeIn(0, FALSE, 1);

   while (TRUE)
   {
      //
      // Activate the menu
      //
      wItemSelected = PAL_ReadMenu(NULL, rgMainMenuItem, 2, wDefaultItem, MENUITEM_COLOR);

      if (wItemSelected == 0 || wItemSelected == MENUITEM_VALUE_CANCELLED)
      {
         //
         // Start a new game
         //
         wItemSelected = 0;
         break;
      }
      else
      {
         //
         // Load game
         //
         VIDEO_BackupScreen(gpScreen);
         wItemSelected = PAL_SaveSlotMenu(1);
         VIDEO_RestoreScreen(gpScreen);
         VIDEO_UpdateScreen(NULL);
         if (wItemSelected != MENUITEM_VALUE_CANCELLED)
         {
            break;
         }
         wDefaultItem = 1;
      }
   }

   //
   // Fade out the screen and the music
   //
   AUDIO_PlayMusic(0, FALSE, 1);
   PAL_FadeOut(1);

   if (wItemSelected == 0)
   {
      PAL_PlayAVI("3.avi");
   }

   return (INT)wItemSelected;
}

INT
PAL_SaveSlotMenu(
   WORD        wDefaultSlot
)
/*++
  Purpose:

    Show the load game menu.

  Parameters:

    [IN]  wDefaultSlot - default save slot number (1-5).

  Return value:

    Which saved slot to load from (1-5). MENUITEM_VALUE_CANCELLED if cancelled.

--*/
{
   LPBOX           rgpBox[5];
   int             i, w = PAL_WordMaxWidth(LOADMENU_LABEL_SLOT_FIRST, 5);
   int             dx = (w > 4) ? (w - 4) * 16 : 0;
   WORD            wItemSelected;

   MENUITEM        rgMenuItem[5];

   const SDL_Rect  rect = { 195 - dx, 7, 120 + dx, 190 };

   //
   // Create the boxes and create the menu items
   //
   for (i = 0; i < 5; i++)
   {
      // Fix render problem with shadow
      rgpBox[i] = PAL_CreateSingleLineBox(PAL_XY(195 - dx, 7 + 38 * i), 6 + (w > 4 ? w - 4 : 0), FALSE);

      rgMenuItem[i].wValue = i + 1;
      rgMenuItem[i].fEnabled = TRUE;
      rgMenuItem[i].wNumWord = LOADMENU_LABEL_SLOT_FIRST + i;
	  rgMenuItem[i].pos = PAL_XY(210 - dx, 17 + 38 * i);
   }

   //
   // Draw the numbers of saved times
   //
   for (i = 1; i <= 5; i++)
   {
      //
      // Draw the number
      //
      PAL_DrawNumber((UINT)GetSavedTimes(i), 4, PAL_XY(270, 38 * i - 17),
         kNumColorYellow, kNumAlignRight);
   }

   //
   // Activate the menu
   //
   wItemSelected = PAL_ReadMenu(NULL, rgMenuItem, 5, wDefaultSlot - 1, MENUITEM_COLOR);

   //
   // Delete the boxes
   //
   for (i = 0; i < 5; i++)
   {
      PAL_DeleteBox(rgpBox[i]);
   }

   VIDEO_UpdateScreen(&rect);

   return wItemSelected;
}

static
WORD
PAL_SelectionMenu(
	int   nWords,
	int   nDefault,
	WORD  wItems[]
)
/*++
  Purpose:

    Show a common selection box.

  Parameters:

    [IN]  nWords - number of emnu items.
	[IN]  nDefault - index of default item.
	[IN]  wItems - item word array.

  Return value:

    User-selected index.

--*/
{
	LPBOX           rgpBox[4];
	MENUITEM        rgMenuItem[4];
	int             w[4] = {
		(nWords >= 1 && wItems[0]) ? PAL_WordWidth(wItems[0]) : 1,
		(nWords >= 2 && wItems[1]) ? PAL_WordWidth(wItems[1]) : 1,
		(nWords >= 3 && wItems[2]) ? PAL_WordWidth(wItems[2]) : 1,
		(nWords >= 4 && wItems[3]) ? PAL_WordWidth(wItems[3]) : 1 };
	int             dx[4] = { (w[0] - 1) * 16, (w[1] - 1) * 16, (w[2] - 1) * 16, (w[3] - 1) * 16 }, i;
	PAL_POS         pos[4] = { PAL_XY(145, 110), PAL_XY(220 + dx[0], 110), PAL_XY(145, 160), PAL_XY(220 + dx[2], 160) };
	WORD            wReturnValue;

	const SDL_Rect  rect = { 130, 100, 125 + max(dx[0] + dx[1], dx[2] + dx[3]), 100 };

	for (i = 0; i < nWords; i++)
		if (nWords > i && !wItems[i])
			return MENUITEM_VALUE_CANCELLED;

	//
	// Create menu items
	//
	for (i = 0; i < nWords; i++)
	{
		rgMenuItem[i].fEnabled = TRUE;
		rgMenuItem[i].pos = pos[i];
		rgMenuItem[i].wValue = i;
		rgMenuItem[i].wNumWord = wItems[i];
	}

	//
	// Create the boxes
	//
	dx[1] = dx[0]; dx[3] = dx[2]; dx[0] = dx[2] = 0;
	for (i = 0; i < nWords; i++)
	{
		rgpBox[i] = PAL_CreateSingleLineBox(PAL_XY(130 + 75 * (i % 2) + dx[i], 100 + 50 * (i / 2)), w[i] + 1, TRUE);
	}

	//
	// Activate the menu
	//
	wReturnValue = PAL_ReadMenu(NULL, rgMenuItem, nWords, nDefault, MENUITEM_COLOR);

	//
	// Delete the boxes
	//
	for (i = 0; i < nWords; i++)
	{
		PAL_DeleteBox(rgpBox[i]);
	}

	VIDEO_UpdateScreen(&rect);

	return wReturnValue;
}

WORD
PAL_TripleMenu(
   WORD  wThirdWord
)
/*++
  Purpose:

    Show a triple-selection box.

  Parameters:

    None.

  Return value:

    User-selected index.

--*/
{
   WORD wItems[3] = { CONFIRMMENU_LABEL_NO, CONFIRMMENU_LABEL_YES, wThirdWord };
   return PAL_SelectionMenu(3, 0, wItems);
}

BOOL
PAL_ConfirmMenu(
   VOID
)
/*++
  Purpose:

    Show a "Yes or No?" confirm box.

  Parameters:

    None.

  Return value:

    TRUE if user selected Yes, FALSE if selected No.

--*/
{
   WORD wItems[2] = { CONFIRMMENU_LABEL_NO, CONFIRMMENU_LABEL_YES };
   WORD wReturnValue = PAL_SelectionMenu(2, 0, wItems);

   return (wReturnValue == MENUITEM_VALUE_CANCELLED || wReturnValue == 0) ? FALSE : TRUE;
}

BOOL
PAL_SwitchMenu(
   BOOL      fEnabled
)
/*++
  Purpose:

    Show a "Enable/Disable" selection box.

  Parameters:

    [IN]  fEnabled - whether the option is originally enabled or not.

  Return value:

    TRUE if user selected "Enable", FALSE if selected "Disable".

--*/
{
   WORD wItems[2] = { SWITCHMENU_LABEL_DISABLE, SWITCHMENU_LABEL_ENABLE };
   WORD wReturnValue = PAL_SelectionMenu(2, fEnabled ? 1 : 0, wItems);
   return (wReturnValue == MENUITEM_VALUE_CANCELLED) ? fEnabled : ((wReturnValue == 0) ? FALSE : TRUE);
}

#ifndef PAL_CLASSIC

static VOID
PAL_BattleSpeedMenu(
   VOID
)
/*++
  Purpose:

    Show the Battle Speed selection box.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   LPBOX           lpBox;
   WORD            wReturnValue;
   const SDL_Rect  rect = {131, 100, 165, 50};

   MENUITEM        rgMenuItem[5] = {
      { 1,   BATTLESPEEDMENU_LABEL_1,       TRUE,   PAL_XY(145, 110) },
      { 2,   BATTLESPEEDMENU_LABEL_2,       TRUE,   PAL_XY(170, 110) },
      { 3,   BATTLESPEEDMENU_LABEL_3,       TRUE,   PAL_XY(195, 110) },
      { 4,   BATTLESPEEDMENU_LABEL_4,       TRUE,   PAL_XY(220, 110) },
      { 5,   BATTLESPEEDMENU_LABEL_5,       TRUE,   PAL_XY(245, 110) },
   };

   //
   // Create the boxes
   //
   lpBox = PAL_CreateSingleLineBox(PAL_XY(131, 100), 8, TRUE);

   //
   // Activate the menu
   //
   wReturnValue = PAL_ReadMenu(NULL, rgMenuItem, 5, gpGlobals->bBattleSpeed - 1,
      MENUITEM_COLOR);

   //
   // Delete the boxes
   //
   PAL_DeleteBox(lpBox);

   VIDEO_UpdateScreen(&rect);

   if (wReturnValue != MENUITEM_VALUE_CANCELLED)
   {
      gpGlobals->bBattleSpeed = wReturnValue;
   }
}

#endif

LPBOX
PAL_ShowCash(
   DWORD      dwCash
)
/*++
  Purpose:

    Show the cash amount at the top left corner of the screen.

  Parameters:

    [IN]  dwCash - amount of cash.

  Return value:

    pointer to the saved screen part.

--*/
{
   LPBOX     lpBox;

   //
   // Create the box.
   //
   lpBox = PAL_CreateSingleLineBox(PAL_XY(0, 0), 5, TRUE);
   if (lpBox == NULL)
   {
      return NULL;
   }

   //
   // Draw the text label.
   //
   PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(10, 10), 0, FALSE, FALSE, FALSE);

   //
   // Draw the cash amount.
   //
   PAL_DrawNumber(dwCash, 6, PAL_XY(49, 14), kNumColorYellow, kNumAlignRight);

   return lpBox;
}

static VOID
PAL_SystemMenu_OnItemChange(
   WORD        wCurrentItem
)
/*++
  Purpose:

    Callback function when user selected another item in the system menu.

  Parameters:

    [IN]  wCurrentItem - current selected item.

  Return value:

    None.

--*/
{
   gpGlobals->iCurSystemMenuItem = wCurrentItem - 1;
}

static BOOL
PAL_SystemMenu(
   VOID
)
/*++
  Purpose:

    Show the system menu.

  Parameters:

    None.

  Return value:

    TRUE if user made some operations in the menu, FALSE if user cancelled.

--*/
{
   LPBOX               lpMenuBox;
   WORD                wReturnValue;
   int                 iSlot, i;
   const SDL_Rect      rect = {40, 60, 280, 135};

   //
   // Create menu items
   //
   const MENUITEM      rgSystemMenuItem[] =
   {
      // value  label                        enabled   pos
      { 1,      SYSMENU_LABEL_SAVE,          TRUE,     PAL_XY(53, 72) },
      { 2,      SYSMENU_LABEL_LOAD,          TRUE,     PAL_XY(53, 72 + 18) },
      { 3,      SYSMENU_LABEL_MUSIC,         TRUE,     PAL_XY(53, 72 + 36) },
      { 4,      SYSMENU_LABEL_SOUND,         TRUE,     PAL_XY(53, 72 + 54) },
      { 5,      SYSMENU_LABEL_QUIT,          TRUE,     PAL_XY(53, 72 + 72) },
#if !defined(PAL_CLASSIC)
      { 6,      SYSMENU_LABEL_BATTLEMODE,    TRUE,     PAL_XY(53, 72 + 90) },
#endif
   };
   const int           nSystemMenuItem = sizeof(rgSystemMenuItem) / sizeof(MENUITEM);

   //
   // Create the menu box.
   //
   lpMenuBox = PAL_CreateBox(PAL_XY(40, 60), nSystemMenuItem - 1, PAL_MenuTextMaxWidth(rgSystemMenuItem, nSystemMenuItem) - 1, 0, TRUE);

   //
   // Perform the menu.
   //
   wReturnValue = PAL_ReadMenu(PAL_SystemMenu_OnItemChange, rgSystemMenuItem, nSystemMenuItem, gpGlobals->iCurSystemMenuItem, MENUITEM_COLOR);

   if (wReturnValue == MENUITEM_VALUE_CANCELLED)
   {
      //
      // User cancelled the menu
      //
      PAL_DeleteBox(lpMenuBox);
      VIDEO_UpdateScreen(&rect);
      return FALSE;
   }

   switch (wReturnValue)
   {
   case 1:
      //
      // Save game
      //
      iSlot = PAL_SaveSlotMenu(gpGlobals->bCurrentSaveSlot);

      if (iSlot != MENUITEM_VALUE_CANCELLED)
      {
         WORD wSavedTimes = 0;
         gpGlobals->bCurrentSaveSlot = (BYTE)iSlot;

         for (i = 1; i <= 5; i++)
         {
            WORD curSavedTimes = GetSavedTimes(i);
            if (curSavedTimes > wSavedTimes)
            {
               wSavedTimes = curSavedTimes;
            }
         }
         PAL_SaveGame(iSlot, wSavedTimes + 1);
      }
      break;

   case 2:
      //
      // Load game
      //
      iSlot = PAL_SaveSlotMenu(gpGlobals->bCurrentSaveSlot);
      if (iSlot != MENUITEM_VALUE_CANCELLED)
      {
         AUDIO_PlayMusic(0, FALSE, 1);
         PAL_FadeOut(1);
         PAL_InitGameData(iSlot);
      }
      break;

   case 3:
      //
      // Music
      //
      AUDIO_EnableMusic(PAL_SwitchMenu(AUDIO_MusicEnabled()));
      if (gConfig.eMusicType == MUSIC_MIDI)
      {
         AUDIO_PlayMusic(AUDIO_MusicEnabled() ? gpGlobals->wNumMusic : 0, AUDIO_MusicEnabled(), 0);
      }
      break;

   case 4:
      //
      // Sound
      //
      AUDIO_EnableSound(PAL_SwitchMenu(AUDIO_SoundEnabled()));
      break;

   case 5:
      //
      // Quit
      //
      PAL_QuitGame();
      break;

#if !defined(PAL_CLASSIC)
   case 6:
      //
      // Battle Mode
      //
      PAL_BattleSpeedMenu();
      break;
#endif
   }

   PAL_DeleteBox(lpMenuBox);
   return TRUE;
}

VOID
PAL_InGameMagicMenu(
   VOID
)
/*++
  Purpose:

    Show the magic menu.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   MENUITEM         rgMenuItem[MAX_PLAYERS_IN_PARTY];
   int              i, y;
   static WORD      w;
   WORD             wMagic;
   const SDL_Rect   rect = {35, 62, 285, 90};

   //
   // Draw the player info boxes
   //
   y = 45;

   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      PAL_PlayerInfoBox(PAL_XY(y, 165), gpGlobals->rgParty[i].wPlayerRole, 100,
         TIMEMETER_COLOR_DEFAULT, TRUE);
      y += 78;
   }

   y = 75;

   //
   // Generate one menu items for each player in the party
   //
   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      assert(i <= MAX_PLAYERS_IN_PARTY);

      rgMenuItem[i].wValue = i;
      rgMenuItem[i].wNumWord =
         gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[i].wPlayerRole];
      rgMenuItem[i].fEnabled =
         (gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole] > 0);
      rgMenuItem[i].pos = PAL_XY(48, y);

      y += 18;
   }

   //
   // Draw the box
   //
   PAL_CreateBox(PAL_XY(35, 62), gpGlobals->wMaxPartyMemberIndex, PAL_MenuTextMaxWidth(rgMenuItem, sizeof(rgMenuItem)/sizeof(MENUITEM)) - 1, 0, FALSE);

   w = PAL_ReadMenu(NULL, rgMenuItem, gpGlobals->wMaxPartyMemberIndex + 1, w, MENUITEM_COLOR);

   if (w == MENUITEM_VALUE_CANCELLED)
   {
      return;
   }

   wMagic = 0;

   while (TRUE)
   {
      wMagic = PAL_MagicSelectionMenu(gpGlobals->rgParty[w].wPlayerRole, FALSE, wMagic);
      if (wMagic == 0)
      {
         break;
      }

      if (gpGlobals->g.rgObject[wMagic].magic.wFlags & kMagicFlagApplyToAll)
      {
         gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse =
            PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse, 0);

         if (g_fScriptSuccess)
         {
            gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess =
               PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess, 0);

            if(g_fScriptSuccess)
               gpGlobals->g.PlayerRoles.rgwMP[gpGlobals->rgParty[w].wPlayerRole] -=
               gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wMagic].magic.wMagicNumber].wCostMP;
         }

         if (gpGlobals->fNeedToFadeIn)
         {
            PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
            gpGlobals->fNeedToFadeIn = FALSE;
         }
      }
      else
      {
         //
         // Need to select which player to use the magic on.
         //
         WORD       wPlayer = 0;
         SDL_Rect   rect;

         while (wPlayer != MENUITEM_VALUE_CANCELLED)
         {
            //
            // Redraw the player info boxes first
            //
            y = 45;

            for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
            {
               PAL_PlayerInfoBox(PAL_XY(y, 165), gpGlobals->rgParty[i].wPlayerRole, 100,
                  TIMEMETER_COLOR_DEFAULT, TRUE);
               y += 78;
            }

            //
            // Draw the cursor on the selected item
            //
            rect.x = 70 + 78 * wPlayer;
            rect.y = 193;
            rect.w = 9;
            rect.h = 6;

            PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR),
               gpScreen, PAL_XY(rect.x, rect.y));

            VIDEO_UpdateScreen(&rect);

            while (TRUE)
            {
               PAL_ClearKeyState();
               PAL_ProcessEvent();

               if (g_InputState.dwKeyPress & kKeyMenu)
               {
                  wPlayer = MENUITEM_VALUE_CANCELLED;
                  break;
               }
               else if (g_InputState.dwKeyPress & kKeySearch)
               {
                  gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse =
                     PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse,
                        gpGlobals->rgParty[wPlayer].wPlayerRole);

                  if (g_fScriptSuccess)
                  {
                     gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess =
                        PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess,
                           gpGlobals->rgParty[wPlayer].wPlayerRole);

                     if (g_fScriptSuccess)
                     {
                        gpGlobals->g.PlayerRoles.rgwMP[gpGlobals->rgParty[w].wPlayerRole] -=
                           gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wMagic].magic.wMagicNumber].wCostMP;

                        //
                        // Check if we have run out of MP
                        //
                        if (gpGlobals->g.PlayerRoles.rgwMP[gpGlobals->rgParty[w].wPlayerRole] <
                           gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wMagic].magic.wMagicNumber].wCostMP)
                        {
                           //
                           // Don't go further if run out of MP
                           //
                           wPlayer = MENUITEM_VALUE_CANCELLED;
                        }
                     }
                  }

                  break;
               }
               else if (g_InputState.dwKeyPress & (kKeyLeft | kKeyUp))
               {
                  if (wPlayer > 0)
                  {
                     wPlayer--;
                     break;
                  }
               }
               else if (g_InputState.dwKeyPress & (kKeyRight | kKeyDown))
               {
                  if (wPlayer < gpGlobals->wMaxPartyMemberIndex)
                  {
                     wPlayer++;
                     break;
                  }
               }

               SDL_Delay(1);
            }
         }
      }

      //
      // Redraw the player info boxes
      //
      y = 45;

      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         PAL_PlayerInfoBox(PAL_XY(y, 165), gpGlobals->rgParty[i].wPlayerRole, 100,
            TIMEMETER_COLOR_DEFAULT, TRUE);
         y += 78;
      }
   }
}

static VOID
PAL_InventoryMenu(
   VOID
)
/*++
  Purpose:

    Show the inventory menu.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   static WORD      w = 0;
   const SDL_Rect   rect = {30, 60, 290, 60};

   MENUITEM        rgMenuItem[2] =
   {
      // value  label                     enabled   pos
      { 1,      INVMENU_LABEL_EQUIP,      TRUE,     PAL_XY(43, 73) },
      { 2,      INVMENU_LABEL_USE,        TRUE,     PAL_XY(43, 73 + 18) },
   };

   PAL_CreateBox(PAL_XY(30, 60), 1, PAL_MenuTextMaxWidth(rgMenuItem, sizeof(rgMenuItem)/sizeof(MENUITEM)) - 1, 0, FALSE);

   w = PAL_ReadMenu(NULL, rgMenuItem, 2, w - 1, MENUITEM_COLOR);

   switch (w)
   {
   case 1:
      PAL_GameEquipItem();
      break;

   case 2:
      PAL_GameUseItem();
      break;
   }
}

static VOID
PAL_InGameMenu_OnItemChange(
   WORD        wCurrentItem
)
/*++
  Purpose:

    Callback function when user selected another item in the in-game menu.

  Parameters:

    [IN]  wCurrentItem - current selected item.

  Return value:

    None.

--*/
{
   gpGlobals->iCurMainMenuItem = wCurrentItem - 1;
}

VOID
PAL_InGameMenu(
   VOID
)
/*++
  Purpose:

    Show the in-game main menu.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   LPBOX                lpCashBox, lpMenuBox;
   WORD                 wReturnValue;
   const SDL_Rect       rect = {0, 0, 320, 185};
   
   // Fix render problem with shadow
   VIDEO_BackupScreen(gpScreen);

   //
   // Create menu items
   //
   MENUITEM        rgMainMenuItem[4] =
   {
      // value  label                      enabled   pos
      { 1,      GAMEMENU_LABEL_STATUS,     TRUE,     PAL_XY(16, 50) },
      { 2,      GAMEMENU_LABEL_MAGIC,      TRUE,     PAL_XY(16, 50 + 18) },
      { 3,      GAMEMENU_LABEL_INVENTORY,  TRUE,     PAL_XY(16, 50 + 36) },
      { 4,      GAMEMENU_LABEL_SYSTEM,     TRUE,     PAL_XY(16, 50 + 54) },
   };

   //
   // Display the cash amount.
   //
   lpCashBox = PAL_ShowCash(gpGlobals->dwCash);

   //
   // Create the menu box.
   //
   // Fix render problem with shadow
   lpMenuBox = PAL_CreateBox(PAL_XY(3, 37), 3, PAL_MenuTextMaxWidth(rgMainMenuItem, 4) - 1, 0, FALSE);

   //
   // Process the menu
   //
   while (TRUE)
   {
      wReturnValue = PAL_ReadMenu(PAL_InGameMenu_OnItemChange, rgMainMenuItem, 4,
         gpGlobals->iCurMainMenuItem, MENUITEM_COLOR);

      if (wReturnValue == MENUITEM_VALUE_CANCELLED)
      {
         break;
      }

      switch (wReturnValue)
      {
      case 1:
         //
         // Status
         //
         PAL_PlayerStatus();
         goto out;

      case 2:
         //
         // Magic
         //
         PAL_InGameMagicMenu();
         goto out;

      case 3:
         //
         // Inventory
         //
         PAL_InventoryMenu();
         goto out;

      case 4:
         //
         // System
         //
         if (PAL_SystemMenu())
         {
            goto out;
         }
         break;
      }
   }

out:
   //
   // Remove the boxes.
   //
   PAL_DeleteBox(lpCashBox);
   PAL_DeleteBox(lpMenuBox);

   // Fix render problem with shadow
   VIDEO_RestoreScreen(gpScreen);
}

VOID
PAL_PlayerStatus(
   VOID
)
/*++
  Purpose:

    Show the player status.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   PAL_LARGE BYTE   bufBackground[320 * 200];
   PAL_LARGE BYTE   bufImage[16384];
   PAL_LARGE BYTE   bufImageBox[50 * 49];
   int              labels0[] = {
      STATUS_LABEL_EXP, STATUS_LABEL_LEVEL, STATUS_LABEL_HP,
      STATUS_LABEL_MP
   };
   int              labels1[] = {
      STATUS_LABEL_EXP_LAYOUT, STATUS_LABEL_LEVEL_LAYOUT, STATUS_LABEL_HP_LAYOUT,
      STATUS_LABEL_MP_LAYOUT
   };
   int              labels[] = {
      STATUS_LABEL_ATTACKPOWER, STATUS_LABEL_MAGICPOWER, STATUS_LABEL_RESISTANCE,
      STATUS_LABEL_DEXTERITY, STATUS_LABEL_FLEERATE
   };
   int              iCurrent;
   int              iPlayerRole;
   int              i, j;
   WORD             w;

   PAL_MKFDecompressChunk(bufBackground, 320 * 200, STATUS_BACKGROUND_FBPNUM, gpGlobals->f.fpFBP);
   iCurrent = 0;

   if (gConfig.fUseCustomScreenLayout)
   {
      for (i = 0; i < 49; i++)
      {
         memcpy(&bufImageBox[i * 50], &bufBackground[(i + 39) * 320 + 247], 50);
      }
      for (i = 0; i < 49; i++)
      {
         memcpy(&bufBackground[(i + 125) * 320 + 81], &bufBackground[(i + 125) * 320 + 81 - 50], 50);
         memcpy(&bufBackground[(i + 141) * 320 + 141], &bufBackground[(i + 141) * 320 + 81 - 50], 50);
         memcpy(&bufBackground[(i + 133) * 320 + 201], &bufBackground[(i + 133) * 320 + 81 - 50], 50);
         memcpy(&bufBackground[(i + 101) * 320 + 251], &bufBackground[(i + 101) * 320 + 81 - 50], 50);
         memcpy(&bufBackground[(i + 39) * 320 + 247], &bufBackground[(i + 39) * 320 + 189 - 50], 50);
         if (i > 0) memcpy(&bufBackground[(i - 1) * 320 + 189], &bufBackground[(i - 1) * 320 + 189 - 50], 50);
      }
      for(i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
      {
         short x = PAL_X(gConfig.ScreenLayout.RoleEquipImageBoxes[i]);
         short y = PAL_Y(gConfig.ScreenLayout.RoleEquipImageBoxes[i]);
         short sx = (x < 0) ? -x : 0, sy = (y < 0) ? -y : 0, d = (x > 270) ? x - 270 : 0;
         if (sx >= 50 || sy >= 49 || x >= 320 || y >= 200) continue;
         for (; sy < 49 && y + sy < 200; sy++)
         {
            memcpy(&bufBackground[(y + sy) * 320 + x + sx], &bufImageBox[sy * 50 + sx], 50 - sx - d);
         }
      }
   }

   while (iCurrent >= 0 && iCurrent <= gpGlobals->wMaxPartyMemberIndex)
   {
      iPlayerRole = gpGlobals->rgParty[iCurrent].wPlayerRole;

      //
      // Draw the background image
      //
      PAL_FBPBlitToSurface(bufBackground, gpScreen);

      //
      // Draw the image of player role
      //
      if (PAL_MKFReadChunk(bufImage, 16384, gpGlobals->g.PlayerRoles.rgwAvatar[iPlayerRole], gpGlobals->f.fpRGM) > 0)
      {
         PAL_RLEBlitToSurface(bufImage, gpScreen, gConfig.ScreenLayout.RoleImage);
      }

      //
      // Draw the equipments
      //
      for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
      {
         int offset;

         w = gpGlobals->g.PlayerRoles.rgwEquipment[i][iPlayerRole];

         if (w == 0)
         {
            continue;
         }

         //
         // Draw the image
         //
         if (PAL_MKFReadChunk(bufImage, 16384,
            gpGlobals->g.rgObject[w].item.wBitmap, gpGlobals->f.fpBALL) > 0)
         {
            PAL_RLEBlitToSurface(bufImage, gpScreen,
               PAL_XY_OFFSET(gConfig.ScreenLayout.RoleEquipImageBoxes[i], 1, 1));
         }

         //
         // Draw the text label
         //
         offset = PAL_WordWidth(w) * 16;
         if (PAL_X(gConfig.ScreenLayout.RoleEquipNames[i]) + offset > 320)
         {
            offset = 320 - PAL_X(gConfig.ScreenLayout.RoleEquipNames[i]) - offset;
         }
         else
         {
            offset = 0;
         }
         int index = &gConfig.ScreenLayout.RoleEquipNames[i] - gConfig.ScreenLayoutArray;
         BOOL fShadow = (gConfig.ScreenLayoutFlag[index] & DISABLE_SHADOW) ? FALSE : TRUE;
         BOOL fUse8x8Font = (gConfig.ScreenLayoutFlag[index] & USE_8x8_FONT) ? TRUE : FALSE;
         PAL_DrawText(PAL_GetWord(w), PAL_XY_OFFSET(gConfig.ScreenLayout.RoleEquipNames[i], offset, 0), STATUS_COLOR_EQUIPMENT, fShadow, FALSE, fUse8x8Font);
      }

      //
      // Draw the text labels
      //
      for (i = 0; i < sizeof(labels0) / sizeof(int); i++)
      {
         int index = labels1[i];
         BOOL fShadow = (gConfig.ScreenLayoutFlag[index] & DISABLE_SHADOW) ? FALSE : TRUE;
         BOOL fUse8x8Font = (gConfig.ScreenLayoutFlag[index] & USE_8x8_FONT) ? TRUE : FALSE;
         PAL_DrawText(PAL_GetWord(labels0[i]), *(&gConfig.ScreenLayout.RoleExpLabel+i), MENUITEM_COLOR, fShadow, FALSE, fUse8x8Font);
      }
      for (i = 0; i < sizeof(labels) / sizeof(int); i++)
      {
         int index = &gConfig.ScreenLayout.RoleStatusLabels[i] - gConfig.ScreenLayoutArray;
         BOOL fShadow = (gConfig.ScreenLayoutFlag[index] & DISABLE_SHADOW) ? FALSE : TRUE;
         BOOL fUse8x8Font = (gConfig.ScreenLayoutFlag[index] & USE_8x8_FONT) ? TRUE : FALSE;
         PAL_DrawText(PAL_GetWord(labels[i]), gConfig.ScreenLayout.RoleStatusLabels[i], MENUITEM_COLOR, fShadow, FALSE, fUse8x8Font);
      }

      PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[iPlayerRole]),
         gConfig.ScreenLayout.RoleName, MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);

      //
      // Draw the stats
      //
      if (gConfig.ScreenLayout.RoleExpSlash != 0)
	  {
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, gConfig.ScreenLayout.RoleExpSlash);
      }
      if (gConfig.ScreenLayout.RoleHPSlash != 0)
	  {
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, gConfig.ScreenLayout.RoleHPSlash);
      }
      if (gConfig.ScreenLayout.RoleMPSlash != 0)
	  {
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, gConfig.ScreenLayout.RoleMPSlash);
      }

      PAL_DrawNumber(gpGlobals->Exp.rgPrimaryExp[iPlayerRole].wExp, 5,
         gConfig.ScreenLayout.RoleCurrExp, kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.rgLevelUpExp[gpGlobals->g.PlayerRoles.rgwLevel[iPlayerRole]], 5,
         gConfig.ScreenLayout.RoleNextExp, kNumColorCyan, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwLevel[iPlayerRole], 2,
         gConfig.ScreenLayout.RoleLevel, kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwHP[iPlayerRole], 4,
         gConfig.ScreenLayout.RoleCurHP, kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxHP[iPlayerRole], 4,
         gConfig.ScreenLayout.RoleMaxHP, kNumColorBlue, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMP[iPlayerRole], 4,
         gConfig.ScreenLayout.RoleCurMP, kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxMP[iPlayerRole], 4,
         gConfig.ScreenLayout.RoleMaxMP, kNumColorBlue, kNumAlignRight);

      PAL_DrawNumber(PAL_GetPlayerAttackStrength(iPlayerRole), 4,
         gConfig.ScreenLayout.RoleStatusValues[0], kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerMagicStrength(iPlayerRole), 4,
         gConfig.ScreenLayout.RoleStatusValues[1], kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerDefense(iPlayerRole), 4,
         gConfig.ScreenLayout.RoleStatusValues[2], kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerDexterity(iPlayerRole), 4,
         gConfig.ScreenLayout.RoleStatusValues[3], kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerFleeRate(iPlayerRole), 4,
         gConfig.ScreenLayout.RoleStatusValues[4], kNumColorYellow, kNumAlignRight);

      //
      // Draw all poisons
      //
      for (i = j = 0; i < MAX_POISONS; i++)
      {
         w = gpGlobals->rgPoisonStatus[i][iCurrent].wPoisonID;

         if (w != 0 && gpGlobals->g.rgObject[w].poison.wPoisonLevel <= 3)
         {
            PAL_DrawText(PAL_GetWord(w), gConfig.ScreenLayout.RolePoisonNames[j++], (BYTE)(gpGlobals->g.rgObject[w].poison.wColor + 10), TRUE, FALSE, FALSE);
         }
      }

      //
      // Update the screen
      //
      VIDEO_UpdateScreen(NULL);

      //
      // Wait for input
      //
      PAL_ClearKeyState();

      while (TRUE)
      {
         UTIL_Delay(1);

         if (g_InputState.dwKeyPress & kKeyMenu)
         {
            iCurrent = -1;
            break;
         }
         else if (g_InputState.dwKeyPress & (kKeyLeft | kKeyUp))
         {
            iCurrent--;
            break;
         }
         else if (g_InputState.dwKeyPress & (kKeyRight | kKeyDown | kKeySearch))
         {
            iCurrent++;
            break;
         }
      }
   }
}

WORD
PAL_ItemUseMenu(
   WORD           wItemToUse
)
/*++
  Purpose:

    Show the use item menu.

  Parameters:

    [IN]  wItemToUse - the object ID of the item to use.

  Return value:

    The selected player to use the item onto.
    MENUITEM_VALUE_CANCELLED if user cancelled.

--*/
{
   BYTE           bColor, bSelectedColor;
   PAL_LARGE BYTE bufImage[2048];
   DWORD          dwColorChangeTime;
   static WORD    wSelectedPlayer = 0;
   SDL_Rect       rect = {110, 2, 200, 180};
   int            i;

   bSelectedColor = MENUITEM_COLOR_SELECTED_FIRST;
   dwColorChangeTime = 0;

   while (TRUE)
   {
      if (wSelectedPlayer > gpGlobals->wMaxPartyMemberIndex)
      {
         wSelectedPlayer = 0;
      }

      //
      // Draw the box
      //
      PAL_CreateBox(PAL_XY(110, 2), 7, 9, 0, FALSE);

      //
      // Draw the stats of the selected player
      //
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_LEVEL), PAL_XY(200, 16),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_HP), PAL_XY(200, 34),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_MP), PAL_XY(200, 52),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_ATTACKPOWER), PAL_XY(200, 70),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_MAGICPOWER), PAL_XY(200, 88),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_RESISTANCE), PAL_XY(200, 106),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_DEXTERITY), PAL_XY(200, 124),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_FLEERATE), PAL_XY(200, 142),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE, FALSE);

      i = gpGlobals->rgParty[wSelectedPlayer].wPlayerRole;

      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwLevel[i], 4, PAL_XY(240, 20),
         kNumColorYellow, kNumAlignRight);

      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
         PAL_XY(263, 38));
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxHP[i], 4,
         PAL_XY(261, 40), kNumColorBlue, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwHP[i], 4,
         PAL_XY(240, 37), kNumColorYellow, kNumAlignRight);

      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
         PAL_XY(263, 56));
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxMP[i], 4,
         PAL_XY(261, 58), kNumColorBlue, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMP[i], 4,
         PAL_XY(240, 55), kNumColorYellow, kNumAlignRight);

      PAL_DrawNumber(PAL_GetPlayerAttackStrength(i), 4, PAL_XY(240, 74),
         kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerMagicStrength(i), 4, PAL_XY(240, 92),
         kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerDefense(i), 4, PAL_XY(240, 110),
         kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerDexterity(i), 4, PAL_XY(240, 128),
         kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerFleeRate(i), 4, PAL_XY(240, 146),
         kNumColorYellow, kNumAlignRight);

      //
      // Draw the names of the players in the party
      //
      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         if (i == wSelectedPlayer)
         {
            bColor = bSelectedColor;
         }
         else
         {
            bColor = MENUITEM_COLOR;
         }

         PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[i].wPlayerRole]),
            PAL_XY(125, 16 + 20 * i), bColor, TRUE, FALSE, FALSE);
      }

      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen,
         PAL_XY(120, 80));

      i = PAL_GetItemAmount(wItemToUse);

      if (i > 0)
      {
         //
         // Draw the picture of the item
         //
         if (PAL_MKFReadChunk(bufImage, 2048,
            gpGlobals->g.rgObject[wItemToUse].item.wBitmap, gpGlobals->f.fpBALL) > 0)
         {
            PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(127, 88));
         }

         //
         // Draw the amount and label of the item
         //
         PAL_DrawText(PAL_GetWord(wItemToUse), PAL_XY(116, 143), STATUS_COLOR_EQUIPMENT, TRUE, FALSE, FALSE);
         PAL_DrawNumber(i, 2, PAL_XY(170, 133), kNumColorCyan, kNumAlignRight);
      }

      //
      // Update the screen area
      //
      VIDEO_UpdateScreen(&rect);

      //
      // Wait for key
      //
      PAL_ClearKeyState();

      while (TRUE)
      {
         //
         // See if we should change the highlight color
         //
         if (SDL_TICKS_PASSED(SDL_GetTicks(), dwColorChangeTime))
         {
            if ((WORD)bSelectedColor + 1 >=
               (WORD)MENUITEM_COLOR_SELECTED_FIRST + MENUITEM_COLOR_SELECTED_TOTALNUM)
            {
               bSelectedColor = MENUITEM_COLOR_SELECTED_FIRST;
            }
            else
            {
               bSelectedColor++;
            }

            dwColorChangeTime = SDL_GetTicks() + (600 / MENUITEM_COLOR_SELECTED_TOTALNUM);

            //
            // Redraw the selected item.
            //
            PAL_DrawText(
               PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[gpGlobals->rgParty[wSelectedPlayer].wPlayerRole]),
               PAL_XY(125, 16 + 20 * wSelectedPlayer), bSelectedColor, FALSE, TRUE, FALSE);
         }

         PAL_ProcessEvent();

         if (g_InputState.dwKeyPress != 0)
         {
            break;
         }

         SDL_Delay(1);
      }

      if (i <= 0)
      {
         return MENUITEM_VALUE_CANCELLED;
      }

      if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
      {
         wSelectedPlayer--;
      }
      else if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
      {
         if (wSelectedPlayer < gpGlobals->wMaxPartyMemberIndex)
         {
            wSelectedPlayer++;
         }
      }
      else if (g_InputState.dwKeyPress & kKeyMenu)
      {
         break;
      }
      else if (g_InputState.dwKeyPress & kKeySearch)
      {
         return gpGlobals->rgParty[wSelectedPlayer].wPlayerRole;
      }
   }

   return MENUITEM_VALUE_CANCELLED;
}

static VOID
PAL_BuyMenu_OnItemChange(
   WORD           wCurrentItem
)
/*++
  Purpose:

    Callback function which is called when player selected another item
    in the buy menu.

  Parameters:

    [IN]  wCurrentItem - current item on the menu, indicates the object ID of
                         the currently selected item.

  Return value:

    None.

--*/
{
   const SDL_Rect      rect = {20, 8, 300, 175};
   int                 i, n;
   PAL_LARGE BYTE      bufImage[2048];

   if( __buymenu_firsttime_render )
      PAL_RLEBlitToSurfaceWithShadow(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen, PAL_XY(35+6, 8+6), TRUE);
   //
   // Draw the picture of current selected item
   //
   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen,
      PAL_XY(35, 8));

   if (PAL_MKFReadChunk(bufImage, 2048,
      gpGlobals->g.rgObject[wCurrentItem].item.wBitmap, gpGlobals->f.fpBALL) > 0)
   {
      if( __buymenu_firsttime_render )
         PAL_RLEBlitToSurfaceWithShadow(bufImage, gpScreen, PAL_XY(42+6, 16+6), TRUE);
      PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(42, 16));
   }

   //
   // See how many of this item we have in the inventory
   //
   n = 0;

   for (i = 0; i < MAX_INVENTORY; i++)
   {
      if (gpGlobals->rgInventory[i].wItem == 0)
      {
         break;
      }
      else if (gpGlobals->rgInventory[i].wItem == wCurrentItem)
      {
         n = gpGlobals->rgInventory[i].nAmount;
         break;
      }
   }

   if( __buymenu_firsttime_render )
      PAL_CreateSingleLineBoxWithShadow(PAL_XY(20, 105), 5, FALSE, 6);
   else
   //
   // Draw the amount of this item in the inventory
   //
   PAL_CreateSingleLineBoxWithShadow(PAL_XY(20, 105), 5, FALSE, 0);
   PAL_DrawText(PAL_GetWord(BUYMENU_LABEL_CURRENT), PAL_XY(30, 115), 0, FALSE, FALSE, FALSE);
   PAL_DrawNumber(n, 6, PAL_XY(69, 119), kNumColorYellow, kNumAlignRight);

   if( __buymenu_firsttime_render )
      PAL_CreateSingleLineBoxWithShadow(PAL_XY(20, 145), 5, FALSE, 6);
   else
   //
   // Draw the cash amount
   //
   PAL_CreateSingleLineBoxWithShadow(PAL_XY(20, 145), 5, FALSE, 0);
   PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(30, 155), 0, FALSE, FALSE, FALSE);
   PAL_DrawNumber(gpGlobals->dwCash, 6, PAL_XY(69, 159), kNumColorYellow, kNumAlignRight);

   VIDEO_UpdateScreen(&rect);
   
   __buymenu_firsttime_render = FALSE;
}

VOID
PAL_BuyMenu(
   WORD           wStoreNum
)
/*++
  Purpose:

    Show the buy item menu.

  Parameters:

    [IN]  wStoreNum - number of the store to buy items from.

  Return value:

    None.

--*/
{
   MENUITEM        rgMenuItem[MAX_STORE_ITEM];
   int             i, y;
   WORD            w;
   SDL_Rect        rect = {125, 8, 190, 190};

   //
   // create the menu items
   //
   y = 22;

   for (i = 0; i < MAX_STORE_ITEM; i++)
   {
      if (gpGlobals->g.lprgStore[wStoreNum].rgwItems[i] == 0)
      {
         break;
      }

      rgMenuItem[i].wValue = gpGlobals->g.lprgStore[wStoreNum].rgwItems[i];
      rgMenuItem[i].wNumWord = gpGlobals->g.lprgStore[wStoreNum].rgwItems[i];
      rgMenuItem[i].fEnabled = TRUE;
      rgMenuItem[i].pos = PAL_XY(150, y);

      y += 18;
   }

   //
   // Draw the box
   //
   PAL_CreateBox(PAL_XY(125, 8), 8, 8, 1, FALSE);

   //
   // Draw the number of prices
   //
   for (y = 0; y < i; y++)
   {
      w = gpGlobals->g.rgObject[rgMenuItem[y].wValue].item.wPrice;
      PAL_DrawNumber(w, 6, PAL_XY(235, 25 + y * 18), kNumColorCyan, kNumAlignRight);
   }

   w = 0;
   __buymenu_firsttime_render = TRUE;

   while (TRUE)
   {
      w = PAL_ReadMenu(PAL_BuyMenu_OnItemChange, rgMenuItem, i, w, MENUITEM_COLOR);

      if (w == MENUITEM_VALUE_CANCELLED)
      {
         break;
      }

      if (gpGlobals->g.rgObject[w].item.wPrice <= gpGlobals->dwCash)
      {
         if (PAL_ConfirmMenu())
         {
            //
            // Player bought an item
            //
            gpGlobals->dwCash -= gpGlobals->g.rgObject[w].item.wPrice;
            PAL_AddItemToInventory(w, 1);
         }
      }

      //
      // Place the cursor to the current item on next loop
      //
      for (y = 0; y < i; y++)
      {
         if (w == rgMenuItem[y].wValue)
         {
            w = y;
            break;
         }
      }
   }
}

static VOID
PAL_SellMenu_OnItemChange(
   WORD         wCurrentItem
)
/*++
  Purpose:

    Callback function which is called when player selected another item
    in the sell item menu.

  Parameters:

    [IN]  wCurrentItem - current item on the menu, indicates the object ID of
                         the currently selected item.

  Return value:

    None.

--*/
{
   //
   // Draw the cash amount
   //
   PAL_CreateSingleLineBoxWithShadow(PAL_XY(100, 150), 5, FALSE, 0);
   PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(110, 160), 0, FALSE, FALSE, FALSE);
   PAL_DrawNumber(gpGlobals->dwCash, 6, PAL_XY(149, 164), kNumColorYellow, kNumAlignRight);

   //
   // Draw the price
   //
   PAL_CreateSingleLineBoxWithShadow(PAL_XY(220, 150), 5, FALSE, 0);

   if (gpGlobals->g.rgObject[wCurrentItem].item.wFlags & kItemFlagSellable)
   {
      PAL_DrawText(PAL_GetWord(SELLMENU_LABEL_PRICE), PAL_XY(230, 160), 0, FALSE, FALSE, FALSE);
      PAL_DrawNumber(gpGlobals->g.rgObject[wCurrentItem].item.wPrice / 2, 6,
         PAL_XY(269, 164), kNumColorYellow, kNumAlignRight);
   }
}

VOID
PAL_SellMenu(
   VOID
)
/*++
  Purpose:

    Show the sell item menu.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   WORD      w;

   while (TRUE)
   {
      w = PAL_ItemSelectMenu(PAL_SellMenu_OnItemChange, kItemFlagSellable);
      if (w == 0)
      {
         break;
      }

      if (PAL_ConfirmMenu())
      {
         if (PAL_AddItemToInventory(w, -1))
         {
            gpGlobals->dwCash += gpGlobals->g.rgObject[w].item.wPrice / 2;
         }
      }
   }
}

VOID
PAL_EquipItemMenu(
   WORD        wItem
)
/*++
  Purpose:

    Show the menu which allow players to equip the specified item.

  Parameters:

    [IN]  wItem - the object ID of the item.

  Return value:

    None.

--*/
{
   PAL_LARGE BYTE   bufBackground[320 * 200];
   PAL_LARGE BYTE   bufImageBox[72 * 72];
   PAL_LARGE BYTE   bufImage[2048];
   WORD             w;
   int              iCurrentPlayer, i;
   BYTE             bColor, bSelectedColor;
   DWORD            dwColorChangeTime;

   gpGlobals->wLastUnequippedItem = wItem;

   PAL_MKFDecompressChunk(bufBackground, 320 * 200, EQUIPMENU_BACKGROUND_FBPNUM,
      gpGlobals->f.fpFBP);

   if (gConfig.fUseCustomScreenLayout)
   {
      int x = PAL_X(gConfig.ScreenLayout.EquipImageBox);
      int y = PAL_Y(gConfig.ScreenLayout.EquipImageBox);
      for (i = 8; i < 72; i++)
      {
         memcpy(&bufBackground[i * 320 + 92], &bufBackground[(i + 128) * 320 + 92], 32);
         memcpy(&bufBackground[(i + 64) * 320 + 92], &bufBackground[(i + 128) * 320 + 92], 32);
      }
      for (i = 9; i < 90; i++)
      {
         memcpy(&bufBackground[i * 320 + 226], &bufBackground[(i + 104) * 320 + 226], 32);
      }
      for (i = 99; i < 113; i++)
      {
         memcpy(&bufBackground[i * 320 + 226], &bufBackground[(i + 16) * 320 + 226], 32);
      }
      for (i = 8; i < 80; i++)
      {
         memcpy(&bufImageBox[(i - 8) * 72], &bufBackground[i * 320 + 8], 72);
         memcpy(&bufBackground[i * 320 + 8], &bufBackground[(i + 72) * 320 + 8], 72);
      }
      for (i = 0; i < 72; i++)
      {
         memcpy(&bufBackground[(i + y) * 320 + x], &bufImageBox[i * 72], 72);
      }
   }

   iCurrentPlayer = 0;
   bSelectedColor = MENUITEM_COLOR_SELECTED_FIRST;
   dwColorChangeTime = SDL_GetTicks() + (600 / MENUITEM_COLOR_SELECTED_TOTALNUM);

   while (TRUE)
   {
      wItem = gpGlobals->wLastUnequippedItem;

      //
      // Draw the background
      //
      PAL_FBPBlitToSurface(bufBackground, gpScreen);

      //
      // Draw the item picture
      //
      if (PAL_MKFReadChunk(bufImage, 2048,
         gpGlobals->g.rgObject[wItem].item.wBitmap, gpGlobals->f.fpBALL) > 0)
      {
         PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY_OFFSET(gConfig.ScreenLayout.EquipImageBox, 8, 8));
      }

      if (gConfig.fUseCustomScreenLayout)
      {
         int labels1[] = { STATUS_LABEL_ATTACKPOWER, STATUS_LABEL_MAGICPOWER, STATUS_LABEL_RESISTANCE, STATUS_LABEL_DEXTERITY, STATUS_LABEL_FLEERATE };
         int labels2[] = { EQUIP_LABEL_HEAD, EQUIP_LABEL_SHOULDER, EQUIP_LABEL_BODY, EQUIP_LABEL_HAND, EQUIP_LABEL_FOOT, EQUIP_LABEL_NECK };
		 for (i = 0; i < sizeof(labels1) / sizeof(int); i++)
         {
            int index = &gConfig.ScreenLayout.EquipStatusLabels[i] - gConfig.ScreenLayoutArray;
            BOOL fShadow = (gConfig.ScreenLayoutFlag[index] & DISABLE_SHADOW) ? FALSE : TRUE;
            BOOL fUse8x8Font = (gConfig.ScreenLayoutFlag[index] & USE_8x8_FONT) ? TRUE : FALSE;
            PAL_DrawText(PAL_GetWord(labels1[i]), gConfig.ScreenLayoutArray[index], MENUITEM_COLOR, fShadow, FALSE, fUse8x8Font);
         }
		 for (i = 0; i < sizeof(labels2) / sizeof(int); i++)
         {
            int index = &gConfig.ScreenLayout.EquipLabels[i] - gConfig.ScreenLayoutArray;
            BOOL fShadow = (gConfig.ScreenLayoutFlag[index] & DISABLE_SHADOW) ? FALSE : TRUE;
            BOOL fUse8x8Font = (gConfig.ScreenLayoutFlag[index] & USE_8x8_FONT) ? TRUE : FALSE;
            PAL_DrawText(PAL_GetWord(labels2[i]), gConfig.ScreenLayoutArray[index], MENUITEM_COLOR, fShadow, FALSE, fUse8x8Font);
         }
      }

      //
      // Draw the current equipment of the selected player
      //
      w = gpGlobals->rgParty[iCurrentPlayer].wPlayerRole;
      for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
      {
         if (gpGlobals->g.PlayerRoles.rgwEquipment[i][w] != 0)
         {
            PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwEquipment[i][w]),
				gConfig.ScreenLayout.EquipNames[i], MENUITEM_COLOR, TRUE, FALSE, FALSE);
         }
      }

      //
      // Draw the stats of the currently selected player
      //
      PAL_DrawNumber(PAL_GetPlayerAttackStrength(w), 4, gConfig.ScreenLayout.EquipStatusValues[0], kNumColorCyan, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerMagicStrength(w), 4, gConfig.ScreenLayout.EquipStatusValues[1], kNumColorCyan, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerDefense(w), 4, gConfig.ScreenLayout.EquipStatusValues[2], kNumColorCyan, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerDexterity(w), 4, gConfig.ScreenLayout.EquipStatusValues[3], kNumColorCyan, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerFleeRate(w), 4, gConfig.ScreenLayout.EquipStatusValues[4], kNumColorCyan, kNumAlignRight);

      //
      // Draw a box for player selection
      //
      PAL_CreateBox(gConfig.ScreenLayout.EquipRoleListBox, gpGlobals->wMaxPartyMemberIndex, PAL_WordMaxWidth(36, 4) - 1, 0, FALSE);

      //
      // Draw the label of players
      //
      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         w = gpGlobals->rgParty[i].wPlayerRole;

         if (iCurrentPlayer == i)
         {
            if (gpGlobals->g.rgObject[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
            {
               bColor = bSelectedColor;
            }
            else
            {
               bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
            }
         }
         else
         {
            if (gpGlobals->g.rgObject[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
            {
               bColor = MENUITEM_COLOR;
            }
            else
            {
               bColor = MENUITEM_COLOR_INACTIVE;
            }
         }

         PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]),
            PAL_XY_OFFSET(gConfig.ScreenLayout.EquipRoleListBox, 13, 13 + 18 * i), bColor, TRUE, FALSE, FALSE);
      }

      //
      // Draw the text label and amount of the item
      //
      if (wItem != 0)
      {
         PAL_DrawText(PAL_GetWord(wItem), gConfig.ScreenLayout.EquipItemName, MENUITEM_COLOR_CONFIRMED, TRUE, FALSE, FALSE);
         PAL_DrawNumber(PAL_GetItemAmount(wItem), 2, gConfig.ScreenLayout.EquipItemAmount, kNumColorCyan, kNumAlignRight);
      }

      //
      // Update the screen
      //
      VIDEO_UpdateScreen(NULL);

      //
      // Accept input
      //
      PAL_ClearKeyState();

      while (TRUE)
      {
         PAL_ProcessEvent();

         //
         // See if we should change the highlight color
         //
         if (SDL_TICKS_PASSED(SDL_GetTicks(), dwColorChangeTime))
         {
            if ((WORD)bSelectedColor + 1 >=
               (WORD)MENUITEM_COLOR_SELECTED_FIRST + MENUITEM_COLOR_SELECTED_TOTALNUM)
            {
               bSelectedColor = MENUITEM_COLOR_SELECTED_FIRST;
            }
            else
            {
               bSelectedColor++;
            }

            dwColorChangeTime = SDL_GetTicks() + (600 / MENUITEM_COLOR_SELECTED_TOTALNUM);

            //
            // Redraw the selected item if needed.
            //
            w = gpGlobals->rgParty[iCurrentPlayer].wPlayerRole;

            if (gpGlobals->g.rgObject[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
            {
               PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[w]),
                  PAL_XY_OFFSET(gConfig.ScreenLayout.EquipRoleListBox, 13, 13 + 18 * iCurrentPlayer), bSelectedColor, TRUE, TRUE, FALSE);
            }
         }

         if (g_InputState.dwKeyPress != 0)
         {
            break;
         }

         SDL_Delay(1);
      }

      if (wItem == 0)
      {
         return;
      }

      if (g_InputState.dwKeyPress & (kKeyUp | kKeyLeft))
      {
         iCurrentPlayer--;
         if (iCurrentPlayer < 0)
         {
            iCurrentPlayer = 0;
         }
      }
      else if (g_InputState.dwKeyPress & (kKeyDown | kKeyRight))
      {
         iCurrentPlayer++;
         if (iCurrentPlayer > gpGlobals->wMaxPartyMemberIndex)
         {
            iCurrentPlayer = gpGlobals->wMaxPartyMemberIndex;
         }
      }
      else if (g_InputState.dwKeyPress & kKeyMenu)
      {
         return;
      }
      else if (g_InputState.dwKeyPress & kKeySearch)
      {
         w = gpGlobals->rgParty[iCurrentPlayer].wPlayerRole;

         if (gpGlobals->g.rgObject[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
         {
            //
            // Run the equip script
            //
            gpGlobals->g.rgObject[wItem].item.wScriptOnEquip =
               PAL_RunTriggerScript(gpGlobals->g.rgObject[wItem].item.wScriptOnEquip,
                  gpGlobals->rgParty[iCurrentPlayer].wPlayerRole);
         }
      }
   }
}

VOID
PAL_QuitGame(
   VOID
)
{
#if PAL_HAS_CONFIG_PAGE
	WORD wReturnValue = PAL_TripleMenu(SYSMENU_LABEL_LAUNCHSETTING);
#else
	WORD wReturnValue = PAL_ConfirmMenu(); // No config menu available
#endif
	if (wReturnValue == 1 || wReturnValue == 2)
	{
		if (wReturnValue == 2) gConfig.fLaunchSetting = TRUE;
		PAL_SaveConfig();		// Keep the fullscreen state
		AUDIO_PlayMusic(0, FALSE, 2);
		PAL_FadeOut(2);
		PAL_Shutdown(0);
	}
}
