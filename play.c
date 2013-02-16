/* -*- mode: c; tab-width: 4; c-basic-offset: 3; c-file-style: "linux" -*- */
//
// Copyright (c) 2008, Wei Mingzhi <whistler_wmz@users.sf.net>.
// All rights reserved.
//
// Portions based on PALx Project by palxex.
// Copyright (c) 2006, Pal Lockheart <palxex@gmail.com>.
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
PAL_GameUpdate(
   BOOL       fTrigger
)
/*++
  Purpose:

    The main game logic routine. Update the status of everything.

  Parameters:

    [IN]  fTrigger - whether to process trigger events or not.

  Return value:

    None.

--*/
{
   WORD            wEventObjectID, wDir;
   int             i;
   LPEVENTOBJECT   p;

   //
   // Check for trigger events
   //
   if (fTrigger)
   {
      //
      // Check if we are entering a new scene
      //
      if (gpGlobals->fEnteringScene)
      {
         //
         // Run the script for entering the scene
         //
         gpGlobals->fEnteringScene = FALSE;

         i = gpGlobals->wNumScene - 1;
         gpGlobals->g.rgScene[i].wScriptOnEnter =
            PAL_RunTriggerScript(gpGlobals->g.rgScene[i].wScriptOnEnter, 0xFFFF);

         if (gpGlobals->fEnteringScene || gpGlobals->fGameStart)
         {
            //
            // Don't go further as we're switching to another scene
            //
            return;
         }

         PAL_ClearKeyState();
         PAL_MakeScene();
      }

      //
      // Update the vanish time for all event objects
      //
      for (wEventObjectID = 0; wEventObjectID < gpGlobals->g.nEventObject; wEventObjectID++)
      {
         p = &gpGlobals->g.lprgEventObject[wEventObjectID];

         if (p->sVanishTime != 0)
         {
            p->sVanishTime += ((p->sVanishTime < 0) ? 1 : -1);
         }
      }

      //
      // Loop through all event objects in the current scene
      //
      for (wEventObjectID = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex + 1;
         wEventObjectID <= gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex;
         wEventObjectID++)
      {
         p = &gpGlobals->g.lprgEventObject[wEventObjectID - 1];

         if (p->sVanishTime != 0)
         {
            continue;
         }

         if (p->sState < 0)
         {
            if (p->x < PAL_X(gpGlobals->viewport) ||
               p->x > PAL_X(gpGlobals->viewport) + 320 ||
               p->y < PAL_Y(gpGlobals->viewport) ||
               p->y > PAL_Y(gpGlobals->viewport) + 320)
            {
               p->sState = abs(p->sState);
               p->wCurrentFrameNum = 0;
            }
         }
         else if (p->sState > 0 && p->wTriggerMode >= kTriggerTouchNear)
         {
            //
            // This event object can be triggered without manually exploring
            //
            if (abs(PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset) - p->x) +
               abs(PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset) - p->y) * 2 <
               (p->wTriggerMode - kTriggerTouchNear) * 32 + 16)
            {
               //
               // Player is in the trigger zone.
               //

               if (p->nSpriteFrames)
               {
                  //
                  // The sprite has multiple frames. Try to adjust the direction.
                  //
                  int                xOffset, yOffset;

                  p->wCurrentFrameNum = 0;

                  xOffset = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset) - p->x;
                  yOffset = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset) - p->y;

                  if (xOffset > 0)
                  {
                     p->wDirection = ((yOffset > 0) ? kDirEast : kDirNorth);
                  }
                  else
                  {
                     p->wDirection = ((yOffset > 0) ? kDirSouth : kDirWest);
                  }

                  //
                  // Redraw the scene
                  //
                  PAL_UpdatePartyGestures(FALSE);

                  PAL_MakeScene();
                  VIDEO_UpdateScreen(NULL);
               }

               //
               // Execute the script.
               //
               p->wTriggerScript = PAL_RunTriggerScript(p->wTriggerScript, wEventObjectID);

               PAL_ClearKeyState();

               if (gpGlobals->fEnteringScene || gpGlobals->fGameStart)
               {
                  //
                  // Don't go further on scene switching
                  //
                  return;
               }
            }
         }
      }
   }

   //
   // Run autoscript for each event objects
   //
   for (wEventObjectID = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex + 1;
      wEventObjectID <= gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex;
      wEventObjectID++)
   {
      p = &gpGlobals->g.lprgEventObject[wEventObjectID - 1];

      if (p->sState > 0 && p->sVanishTime == 0)
      {
         WORD wScriptEntry = p->wAutoScript;
         if (wScriptEntry != 0)
         {
            p->wAutoScript = PAL_RunAutoScript(wScriptEntry, wEventObjectID);
            if (gpGlobals->fEnteringScene || gpGlobals->fGameStart)
            {
               //
               // Don't go further on scene switching
               //
               return;
            }
         }
      }

      //
      // Check if the player is in the way
      //
      if (fTrigger && p->sState >= kObjStateBlocker && p->wSpriteNum != 0 &&
         abs(p->x - PAL_X(gpGlobals->viewport) - PAL_X(gpGlobals->partyoffset)) +
         abs(p->y - PAL_Y(gpGlobals->viewport) - PAL_Y(gpGlobals->partyoffset)) * 2 <= 12)
      {
         //
         // Player is in the way, try to move a step
         //
         wDir = (p->wDirection + 1) % 4;
         for (i = 0; i < 4; i++)
         {
            int              x, y;
            PAL_POS          pos;

            x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
            y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

            x += ((wDir == kDirWest || wDir == kDirSouth) ? -16 : 16);
            y += ((wDir == kDirWest || wDir == kDirNorth) ? -8 : 8);

            pos = PAL_XY(x, y);

            if (!PAL_CheckObstacle(pos, TRUE, 0))
            {
               //
               // move here
               //
               gpGlobals->viewport = PAL_XY(
                  PAL_X(pos) - PAL_X(gpGlobals->partyoffset),
                  PAL_Y(pos) - PAL_Y(gpGlobals->partyoffset));

               break;
            }

            wDir = (wDir + 1) % 4;
         }
      }
   }

   gpGlobals->dwFrameNum++;
}

VOID
PAL_GameUseItem(
   VOID
)
/*++
  Purpose:

    Allow player use an item in the game.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   WORD         wObject;

   while (TRUE)
   {
      wObject = PAL_ItemSelectMenu(NULL, kItemFlagUsable);

      if (wObject == 0)
      {
         return;
      }

      if (!(gpGlobals->g.rgObject[wObject].item.wFlags & kItemFlagApplyToAll))
      {
         //
         // Select the player to use the item on
         //
         WORD     wPlayer = 0;

         while (TRUE)
         {
            wPlayer = PAL_ItemUseMenu(wObject);

            if (wPlayer == MENUITEM_VALUE_CANCELLED)
            {
               break;
            }

            //
            // Run the script
            //
            gpGlobals->g.rgObject[wObject].item.wScriptOnUse =
               PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].item.wScriptOnUse, wPlayer);

            //
            // Remove the item if the item is consuming and the script succeeded
            //
            if ((gpGlobals->g.rgObject[wObject].item.wFlags & kItemFlagConsuming) &&
               g_fScriptSuccess)
            {
               PAL_AddItemToInventory(wObject, -1);
            }
         }
      }
      else
      {
         //
         // Run the script
         //
         gpGlobals->g.rgObject[wObject].item.wScriptOnUse =
            PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].item.wScriptOnUse, 0xFFFF);

         //
         // Remove the item if the item is consuming and the script succeeded
         //
         if ((gpGlobals->g.rgObject[wObject].item.wFlags & kItemFlagConsuming) &&
            g_fScriptSuccess)
         {
            PAL_AddItemToInventory(wObject, -1);
         }

         return;
      }
   }
}

VOID
PAL_GameEquipItem(
   VOID
)
/*++
  Purpose:

    Allow player equip an item in the game.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   WORD      wObject;

   while (TRUE)
   {
      wObject = PAL_ItemSelectMenu(NULL, kItemFlagEquipable);

      if (wObject == 0)
      {
         return;
      }

      PAL_EquipItemMenu(wObject);
   }
}

VOID
PAL_Search(
   VOID
)
/*++
  Purpose:

    Process searching trigger events.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int                x, y, xOffset, yOffset, dx, dy, dh, ex, ey, eh, i, k, l;
   LPEVENTOBJECT      p;
   PAL_POS            rgPos[13];

   //
   // Get the party location
   //
   x = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
   y = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

   if (gpGlobals->wPartyDirection == kDirNorth || gpGlobals->wPartyDirection == kDirEast)
   {
      xOffset = 16;
   }
   else
   {
      xOffset = -16;
   }

   if (gpGlobals->wPartyDirection == kDirEast || gpGlobals->wPartyDirection == kDirSouth)
   {
      yOffset = 8;
   }
   else
   {
      yOffset = -8;
   }

   rgPos[0] = PAL_XY(x, y);

   for (i = 0; i < 4; i++)
   {
      rgPos[i * 3 + 1] = PAL_XY(x + xOffset, y + yOffset);
      rgPos[i * 3 + 2] = PAL_XY(x, y + yOffset * 2);
      rgPos[i * 3 + 3] = PAL_XY(x + xOffset, y);
      x += xOffset;
      y += yOffset;
   }

   for (i = 0; i < 13; i++)
   {
      //
      // Convert to map location
      //
      dh = ((PAL_X(rgPos[i]) % 32) ? 1 : 0);
      dx = PAL_X(rgPos[i]) / 32;
      dy = PAL_Y(rgPos[i]) / 16;

      //
      // Loop through all event objects
      //
      for (k = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex;
         k < gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex; k++)
      {
         p = &(gpGlobals->g.lprgEventObject[k]);
         ex = p->x / 32;
         ey = p->y / 16;
         eh = ((p->x % 32) ? 1 : 0);

         if (p->sState <= 0 || p->wTriggerMode >= kTriggerTouchNear ||
            p->wTriggerMode * 6 - 4 < i || dx != ex || dy != ey || dh != eh)
         {
            continue;
         }

         //
         // Adjust direction/gesture for party members and the event object
         //
         if (p->nSpriteFrames * 4 > p->wCurrentFrameNum)
         {
            p->wCurrentFrameNum = 0; // use standing gesture
            p->wDirection = (gpGlobals->wPartyDirection + 2) % 4; // face the party

            for (l = 0; l <= gpGlobals->wMaxPartyMemberIndex; l++)
            {
               //
               // All party members should face the event object
               //
               gpGlobals->rgParty[l].wFrame = gpGlobals->wPartyDirection * 3;
            }

            //
            // Redraw everything
            //
            PAL_MakeScene();
            VIDEO_UpdateScreen(NULL);
         }

         //
         // Execute the script
         //
         p->wTriggerScript = PAL_RunTriggerScript(p->wTriggerScript, k + 1);

         //
         // Clear inputs and delay for a short time
         //
         UTIL_Delay(50);
         PAL_ClearKeyState();

         return; // don't go further
      }
   }
}

VOID
PAL_StartFrame(
   VOID
)
/*++
  Purpose:

    Starts a video frame. Called once per video frame.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   //
   // Run the game logic of one frame
   //
   PAL_GameUpdate(TRUE);
   if (gpGlobals->fEnteringScene)
   {
      return;
   }

   //
   // Update the positions and gestures of party members
   //
   PAL_UpdateParty();

   //
   // Update the scene
   //
   PAL_MakeScene();
   VIDEO_UpdateScreen(NULL);

   if (g_InputState.dwKeyPress & kKeyMenu)
   {
      //
      // Show the in-game menu
      //
      PAL_InGameMenu();
   }
   else if (g_InputState.dwKeyPress & kKeyUseItem)
   {
      //
      // Show the use item menu
      //
      PAL_GameUseItem();
   }
   else if (g_InputState.dwKeyPress & kKeyThrowItem)
   {
      //
      // Show the equipment menu
      //
      PAL_GameEquipItem();
   }
   else if (g_InputState.dwKeyPress & kKeyForce)
   {
      //
      // Show the magic menu
      //
      PAL_InGameMagicMenu();
   }
   else if (g_InputState.dwKeyPress & kKeyStatus)
   {
      //
      // Show the player status
      //
      PAL_PlayerStatus();
   }
   else if (g_InputState.dwKeyPress & kKeySearch)
   {
      //
      // Process search events
      //
      PAL_Search();
   }
   else if (g_InputState.dwKeyPress & kKeyFlee)
   {
      //
      // Quit Game
      //
      if (PAL_ConfirmMenu())
      {
         PAL_PlayMUS(0, FALSE, 2);
         PAL_FadeOut(2);
         PAL_Shutdown();
         exit(0);
      }
   }

   if (--gpGlobals->wChasespeedChangeCycles == 0)
   {
      gpGlobals->wChaseRange = 1;
   }
}

VOID
PAL_WaitForKey(
   WORD      wTimeOut
)
/*++
  Purpose:

    Wait for any key.

  Parameters:

    [IN]  wTimeOut - the maximum time of the waiting. 0 = wait forever.

  Return value:

    None.

--*/
{
   DWORD     dwTimeOut = SDL_GetTicks() + wTimeOut;

   PAL_ClearKeyState();

   while (wTimeOut == 0 || SDL_GetTicks() < dwTimeOut)
   {
      UTIL_Delay(5);

      if (g_InputState.dwKeyPress & (kKeySearch | kKeyMenu))
      {
         break;
      }
   }
}
