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

   MENUITEM      rgMainMenuItem[2] = {
      // value   label                     enabled   position
      {  0,      MAINMENU_LABEL_NEWGAME,   TRUE,     PAL_XY(125, 95)  },
      {  1,      MAINMENU_LABEL_LOADGAME,  TRUE,     PAL_XY(125, 112) }
   };

   //
   // Play the background music
   //
   PAL_PlayMUS(RIX_NUM_OPENINGMENU, TRUE, 1);

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
         wItemSelected = PAL_SaveSlotMenu(1);
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
   PAL_PlayMUS(0, FALSE, 1);
   PAL_FadeOut(1);

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
   int             i;
   FILE           *fp;
   WORD            wItemSelected;
   WORD            wSavedTimes;

   MENUITEM        rgMenuItem[5];

   const SDL_Rect  rect = {195, 7, 120, 190};

   //
   // Create the boxes and create the menu items
   //
   for (i = 0; i < 5; i++)
   {
      rgpBox[i] = PAL_CreateSingleLineBox(PAL_XY(195, 7 + 38 * i), 6, TRUE);

      rgMenuItem[i].wValue = i + 1;
      rgMenuItem[i].fEnabled = TRUE;
      rgMenuItem[i].wNumWord = LOADMENU_LABEL_SLOT_FIRST + i;
      rgMenuItem[i].pos = PAL_XY(210, 17 + 38 * i);
   }

   //
   // Draw the numbers of saved times
   //
   for (i = 1; i <= 5; i++)
   {
      fp = fopen(va("%s%d%s", PAL_SAVE_PREFIX, i, ".rpg"), "rb");
      if (fp == NULL)
      {
         wSavedTimes = 0;
      }
      else
      {
         fread(&wSavedTimes, sizeof(WORD), 1, fp);
         wSavedTimes = SWAP16(wSavedTimes);
         fclose(fp);
      }

      //
      // Draw the number
      //
      PAL_DrawNumber((UINT)wSavedTimes, 4, PAL_XY(270, 38 * i - 17),
         kNumColorYellow, kNumAlignRight);
   }

   VIDEO_UpdateScreen(&rect);

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
   LPBOX           rgpBox[2];
   MENUITEM        rgMenuItem[2];
   int             i;
   WORD            wReturnValue;

   const SDL_Rect  rect = {130, 100, 125, 50};

   //
   // Create menu items
   //
   rgMenuItem[0].fEnabled = TRUE;
   rgMenuItem[0].pos = PAL_XY(145, 110);
   rgMenuItem[0].wValue = 0;
   rgMenuItem[0].wNumWord = CONFIRMMENU_LABEL_NO;

   rgMenuItem[1].fEnabled = TRUE;
   rgMenuItem[1].pos = PAL_XY(220, 110);
   rgMenuItem[1].wValue = 1;
   rgMenuItem[1].wNumWord = CONFIRMMENU_LABEL_YES;

   //
   // Create the boxes
   //
   for (i = 0; i < 2; i++)
   {
      rgpBox[i] = PAL_CreateSingleLineBox(PAL_XY(130 + 75 * i, 100), 2, TRUE);
   }

   VIDEO_UpdateScreen(&rect);

   //
   // Activate the menu
   //
   wReturnValue = PAL_ReadMenu(NULL, rgMenuItem, 2, 0, MENUITEM_COLOR);

   //
   // Delete the boxes
   //
   for (i = 0; i < 2; i++)
   {
      PAL_DeleteBox(rgpBox[i]);
   }

   VIDEO_UpdateScreen(&rect);

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
   LPBOX           rgpBox[2];
   MENUITEM        rgMenuItem[2];
   int             i;
   WORD            wReturnValue;
   const SDL_Rect  rect = {130, 100, 125, 50};

   //
   // Create menu items
   //
   rgMenuItem[0].fEnabled = TRUE;
   rgMenuItem[0].pos = PAL_XY(145, 110);
   rgMenuItem[0].wValue = 0;
   rgMenuItem[0].wNumWord = SWITCHMENU_LABEL_DISABLE;

   rgMenuItem[1].fEnabled = TRUE;
   rgMenuItem[1].pos = PAL_XY(220, 110);
   rgMenuItem[1].wValue = 1;
   rgMenuItem[1].wNumWord = SWITCHMENU_LABEL_ENABLE;

   //
   // Create the boxes
   //
   for (i = 0; i < 2; i++)
   {
      rgpBox[i] = PAL_CreateSingleLineBox(PAL_XY(130 + 75 * i, 100), 2, TRUE);
   }

   VIDEO_UpdateScreen(&rect);

   //
   // Activate the menu
   //
   wReturnValue = PAL_ReadMenu(NULL, rgMenuItem, 2, fEnabled ? 1 : 0, MENUITEM_COLOR);

   //
   // Delete the boxes
   //
   for (i = 0; i < 2; i++)
   {
      PAL_DeleteBox(rgpBox[i]);
   }

   VIDEO_UpdateScreen(&rect);

   if (wReturnValue == MENUITEM_VALUE_CANCELLED)
   {
      return fEnabled;
   }

   return (wReturnValue == 0) ? FALSE : TRUE;
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
   VIDEO_UpdateScreen(&rect);

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
   PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(10, 10), 0, FALSE, FALSE);

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
   int                 iSlot, i, iSavedTimes;
   FILE               *fp;
   const SDL_Rect      rect = {40, 60, 100, 135};

   //
   // Create menu items
   //
#ifdef PAL_CLASSIC
   MENUITEM        rgSystemMenuItem[5] =
   {
      // value  label                      enabled   pos
      { 1,      SYSMENU_LABEL_SAVE,        TRUE,     PAL_XY(53, 72) },
      { 2,      SYSMENU_LABEL_LOAD,        TRUE,     PAL_XY(53, 72 + 18) },
      { 3,      SYSMENU_LABEL_MUSIC,       TRUE,     PAL_XY(53, 72 + 36) },
      { 4,      SYSMENU_LABEL_SOUND,       TRUE,     PAL_XY(53, 72 + 54) },
      { 5,      SYSMENU_LABEL_QUIT,        TRUE,     PAL_XY(53, 72 + 72) },
   };
#else
   MENUITEM        rgSystemMenuItem[6] =
   {
      // value  label                      enabled   pos
      { 1,      SYSMENU_LABEL_SAVE,        TRUE,     PAL_XY(53, 72) },
      { 2,      SYSMENU_LABEL_LOAD,        TRUE,     PAL_XY(53, 72 + 18) },
      { 3,      SYSMENU_LABEL_MUSIC,       TRUE,     PAL_XY(53, 72 + 36) },
      { 4,      SYSMENU_LABEL_SOUND,       TRUE,     PAL_XY(53, 72 + 54) },
      { 5,      SYSMENU_LABEL_BATTLEMODE,  TRUE,     PAL_XY(53, 72 + 72) },
      { 6,      SYSMENU_LABEL_QUIT,        TRUE,     PAL_XY(53, 72 + 90) },
   };
#endif

   //
   // Create the menu box.
   //
#ifdef PAL_CLASSIC
   lpMenuBox = PAL_CreateBox(PAL_XY(40, 60), 4, 3, 0, TRUE);
#else
   lpMenuBox = PAL_CreateBox(PAL_XY(40, 60), 5, 3, 0, TRUE);
#endif
   VIDEO_UpdateScreen(&rect);

   //
   // Perform the menu.
   //
#ifdef PAL_CLASSIC
   wReturnValue = PAL_ReadMenu(PAL_SystemMenu_OnItemChange, rgSystemMenuItem, 5,
      gpGlobals->iCurSystemMenuItem, MENUITEM_COLOR);
#else
   wReturnValue = PAL_ReadMenu(PAL_SystemMenu_OnItemChange, rgSystemMenuItem, 6,
      gpGlobals->iCurSystemMenuItem, MENUITEM_COLOR);
#endif

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
         gpGlobals->bCurrentSaveSlot = (BYTE)iSlot;

         iSavedTimes = 0;
         for (i = 1; i <= 5; i++)
         {
            fp = fopen(va("%s%d%s", PAL_SAVE_PREFIX, i, ".rpg"), "rb");
            if (fp != NULL)
            {
               WORD wSavedTimes;
               fread(&wSavedTimes, sizeof(WORD), 1, fp);
               fclose(fp);
               wSavedTimes = SWAP16(wSavedTimes);
               if ((int)wSavedTimes > iSavedTimes)
               {
                  iSavedTimes = wSavedTimes;
               }
            }
         }
         PAL_SaveGame(va("%s%d%s", PAL_SAVE_PREFIX, iSlot, ".rpg"), iSavedTimes + 1);
      }
      break;

   case 2:
      //
      // Load game
      //
      iSlot = PAL_SaveSlotMenu(gpGlobals->bCurrentSaveSlot);
      if (iSlot != MENUITEM_VALUE_CANCELLED)
      {
         PAL_PlayMUS(0, FALSE, 1);
         PAL_FadeOut(1);
         PAL_InitGameData(iSlot);
      }
      break;

   case 3:
      //
      // Music
      //
      g_fNoMusic = !PAL_SwitchMenu(!g_fNoMusic);
#ifdef PAL_HAS_NATIVEMIDI
      if (g_fUseMidi)
      {
         if (g_fNoMusic)
         {
            PAL_PlayMUS(0, FALSE, 0);
         }
         else
         {
            PAL_PlayMUS(gpGlobals->wNumMusic, TRUE, 0);
         }
      }
#endif
      break;

   case 4:
      //
      // Sound
      //
      g_fNoSound = !PAL_SwitchMenu(!g_fNoSound);
      break;

#ifndef PAL_CLASSIC
   case 5:
      //
      // Battle Mode
      //
      PAL_BattleSpeedMenu();
      break;

   case 6:
#else
   case 5:
#endif
      //
      // Quit
      //
      if (PAL_ConfirmMenu())
      {
         PAL_PlayMUS(0, FALSE, 2);
         PAL_FadeOut(2);
         PAL_Shutdown();
         exit(0);
      }
      break;
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
   const SDL_Rect   rect = {35, 62, 95, 90};

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
   PAL_CreateBox(PAL_XY(35, 62), gpGlobals->wMaxPartyMemberIndex, 2, 0, FALSE);
   VIDEO_UpdateScreen(&rect);

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
   const SDL_Rect   rect = {30, 60, 75, 60};

   MENUITEM        rgMenuItem[2] =
   {
      // value  label                     enabled   pos
      { 1,      INVMENU_LABEL_USE,        TRUE,     PAL_XY(43, 73) },
      { 2,      INVMENU_LABEL_EQUIP,      TRUE,     PAL_XY(43, 73 + 18) },
   };

   PAL_CreateBox(PAL_XY(30, 60), 1, 1, 0, FALSE);
   VIDEO_UpdateScreen(&rect);

   w = PAL_ReadMenu(NULL, rgMenuItem, 2, w - 1, MENUITEM_COLOR);

   switch (w)
   {
   case 1:
      PAL_GameUseItem();
      break;

   case 2:
      PAL_GameEquipItem();
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
   const SDL_Rect       rect = {0, 0, 150, 185};

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
   lpMenuBox = PAL_CreateBox(PAL_XY(3, 37), 3, 1, 0, TRUE);
   VIDEO_UpdateScreen(&rect);

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

   VIDEO_UpdateScreen(&rect);
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
   int              iCurrent;
   int              iPlayerRole;
   int              i, y;
   WORD             w;

   const int        rgEquipPos[MAX_PLAYER_EQUIPMENTS][2] = {
      {190, 0}, {248, 40}, {252, 102}, {202, 134}, {142, 142}, {82, 126}
   };

   PAL_MKFDecompressChunk(bufBackground, 320 * 200, STATUS_BACKGROUND_FBPNUM,
      gpGlobals->f.fpFBP);
   iCurrent = 0;

   while (iCurrent >= 0 && iCurrent <= gpGlobals->wMaxPartyMemberIndex)
   {
      iPlayerRole = gpGlobals->rgParty[iCurrent].wPlayerRole;

      //
      // Draw the background image
      //
      PAL_FBPBlitToSurface(bufBackground, gpScreen);

      //
      // Draw the text labels
      //
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_EXP), PAL_XY(6, 6), MENUITEM_COLOR, TRUE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_LEVEL), PAL_XY(6, 32), MENUITEM_COLOR, TRUE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_HP), PAL_XY(6, 54), MENUITEM_COLOR, TRUE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_MP), PAL_XY(6, 76), MENUITEM_COLOR, TRUE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_ATTACKPOWER), PAL_XY(6, 98), MENUITEM_COLOR, TRUE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_MAGICPOWER), PAL_XY(6, 118), MENUITEM_COLOR, TRUE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_RESISTANCE), PAL_XY(6, 138), MENUITEM_COLOR, TRUE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_DEXTERITY), PAL_XY(6, 158), MENUITEM_COLOR, TRUE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_FLEERATE), PAL_XY(6, 178), MENUITEM_COLOR, TRUE, FALSE);

      PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles.rgwName[iPlayerRole]),
         PAL_XY(110, 8), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE);

      //
      // Draw the stats
      //
      PAL_DrawNumber(gpGlobals->Exp.rgPrimaryExp[iPlayerRole].wExp, 5,
         PAL_XY(58, 6), kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.rgLevelUpExp[gpGlobals->g.PlayerRoles.rgwLevel[iPlayerRole]],
         5, PAL_XY(58, 15), kNumColorCyan, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwLevel[iPlayerRole], 2,
         PAL_XY(54, 35), kNumColorYellow, kNumAlignRight);
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
         PAL_XY(65, 58));
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
         PAL_XY(65, 80));
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwHP[iPlayerRole], 4, PAL_XY(42, 56),
         kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxHP[iPlayerRole], 4, PAL_XY(63, 61),
         kNumColorBlue, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMP[iPlayerRole], 4, PAL_XY(42, 78),
         kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(gpGlobals->g.PlayerRoles.rgwMaxMP[iPlayerRole], 4, PAL_XY(63, 83),
         kNumColorBlue, kNumAlignRight);

      PAL_DrawNumber(PAL_GetPlayerAttackStrength(iPlayerRole), 4,
         PAL_XY(42, 102), kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerMagicStrength(iPlayerRole), 4,
         PAL_XY(42, 122), kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerDefense(iPlayerRole), 4,
         PAL_XY(42, 142), kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerDexterity(iPlayerRole), 4,
         PAL_XY(42, 162), kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerFleeRate(iPlayerRole), 4,
         PAL_XY(42, 182), kNumColorYellow, kNumAlignRight);

      //
      // Draw the equipments
      //
      for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
      {
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
               PAL_XY(rgEquipPos[i][0], rgEquipPos[i][1]));
         }

         //
         // Draw the text label
         //
         PAL_DrawText(PAL_GetWord(w),
            PAL_XY(rgEquipPos[i][0] + 5, rgEquipPos[i][1] + 38), STATUS_COLOR_EQUIPMENT, TRUE, FALSE);
      }

      //
      // Draw the image of player role
      //
      if (PAL_MKFReadChunk(bufImage, 16384,
         gpGlobals->g.PlayerRoles.rgwAvatar[iPlayerRole], gpGlobals->f.fpRGM) > 0)
      {
         PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(110, 30));
      }

      //
      // Draw all poisons
      //
      y = 58;

      for (i = 0; i < MAX_POISONS; i++)
      {
         w = gpGlobals->rgPoisonStatus[i][iCurrent].wPoisonID;

         if (w != 0 &&
            gpGlobals->g.rgObject[w].poison.wPoisonLevel <= 3)
         {
            PAL_DrawText(PAL_GetWord(w), PAL_XY(185, y),
               (BYTE)(gpGlobals->g.rgObject[w].poison.wColor + 10), TRUE, FALSE);

            y += 18;
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
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_HP), PAL_XY(200, 34),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_MP), PAL_XY(200, 52),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_ATTACKPOWER), PAL_XY(200, 70),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_MAGICPOWER), PAL_XY(200, 88),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_RESISTANCE), PAL_XY(200, 106),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_DEXTERITY), PAL_XY(200, 124),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE);
      PAL_DrawText(PAL_GetWord(STATUS_LABEL_FLEERATE), PAL_XY(200, 142),
         ITEMUSEMENU_COLOR_STATLABEL, TRUE, FALSE);

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
            PAL_XY(125, 16 + 20 * i), bColor, TRUE, FALSE);
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
         PAL_DrawText(PAL_GetWord(wItemToUse), PAL_XY(116, 143), STATUS_COLOR_EQUIPMENT,
            TRUE, FALSE);
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
         if (SDL_GetTicks() > dwColorChangeTime)
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
               PAL_XY(125, 16 + 20 * wSelectedPlayer), bSelectedColor, FALSE, TRUE);
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
   const SDL_Rect      rect = {20, 8, 128, 175};
   int                 i, n;
   PAL_LARGE BYTE      bufImage[2048];

   //
   // Draw the picture of current selected item
   //
   PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen,
      PAL_XY(35, 8));

   if (PAL_MKFReadChunk(bufImage, 2048,
      gpGlobals->g.rgObject[wCurrentItem].item.wBitmap, gpGlobals->f.fpBALL) > 0)
   {
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

   //
   // Draw the amount of this item in the inventory
   //
   PAL_CreateSingleLineBox(PAL_XY(20, 105), 5, FALSE);
   PAL_DrawText(PAL_GetWord(BUYMENU_LABEL_CURRENT), PAL_XY(30, 115), 0, FALSE, FALSE);
   PAL_DrawNumber(n, 6, PAL_XY(69, 119), kNumColorYellow, kNumAlignRight);

   //
   // Draw the cash amount
   //
   PAL_CreateSingleLineBox(PAL_XY(20, 145), 5, FALSE);
   PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(30, 155), 0, FALSE, FALSE);
   PAL_DrawNumber(gpGlobals->dwCash, 6, PAL_XY(69, 159), kNumColorYellow, kNumAlignRight);

   VIDEO_UpdateScreen(&rect);
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

   VIDEO_UpdateScreen(&rect);

   w = 0;

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
   PAL_CreateSingleLineBox(PAL_XY(100, 150), 5, FALSE);
   PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(110, 160), 0, FALSE, FALSE);
   PAL_DrawNumber(gpGlobals->dwCash, 6, PAL_XY(149, 164), kNumColorYellow, kNumAlignRight);

   //
   // Draw the price
   //
   PAL_CreateSingleLineBox(PAL_XY(220, 150), 5, FALSE);

   if (gpGlobals->g.rgObject[wCurrentItem].item.wFlags & kItemFlagSellable)
   {
      PAL_DrawText(PAL_GetWord(SELLMENU_LABEL_PRICE), PAL_XY(230, 160), 0, FALSE, FALSE);
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
   PAL_LARGE BYTE   bufImage[2048];
   WORD             w;
   int              iCurrentPlayer, i;
   BYTE             bColor, bSelectedColor;
   DWORD            dwColorChangeTime;

   gpGlobals->wLastUnequippedItem = wItem;

   PAL_MKFDecompressChunk(bufBackground, 320 * 200, EQUIPMENU_BACKGROUND_FBPNUM,
      gpGlobals->f.fpFBP);

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
         PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(16, 16));
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
               PAL_XY(130, 11 + i * 22), MENUITEM_COLOR, TRUE, FALSE);
         }
      }

      //
      // Draw the stats of the currently selected player
      //
      PAL_DrawNumber(PAL_GetPlayerAttackStrength(w), 4, PAL_XY(260, 14),
         kNumColorCyan, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerMagicStrength(w), 4, PAL_XY(260, 36),
         kNumColorCyan, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerDefense(w), 4, PAL_XY(260, 58),
         kNumColorCyan, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerDexterity(w), 4, PAL_XY(260, 80),
         kNumColorCyan, kNumAlignRight);
      PAL_DrawNumber(PAL_GetPlayerFleeRate(w), 4, PAL_XY(260, 102),
         kNumColorCyan, kNumAlignRight);

      //
      // Draw a box for player selection
      //
      PAL_CreateBox(PAL_XY(2, 95), gpGlobals->wMaxPartyMemberIndex, 2, 0, FALSE);

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
            PAL_XY(15, 108 + 18 * i), bColor, TRUE, FALSE);
      }

      //
      // Draw the text label and amount of the item
      //
      if (wItem != 0)
      {
         PAL_DrawText(PAL_GetWord(wItem), PAL_XY(5, 70), MENUITEM_COLOR_CONFIRMED, TRUE, FALSE);
         PAL_DrawNumber(PAL_GetItemAmount(wItem), 2, PAL_XY(65, 73), kNumColorCyan, kNumAlignRight);
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
         if (SDL_GetTicks() > dwColorChangeTime)
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
                  PAL_XY(15, 108 + 18 * iCurrentPlayer), bSelectedColor, TRUE, TRUE);
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
