/* -*- mode: c; tab-width: 4; c-basic-offset: 3; c-file-style: "linux" -*- */
//
// Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
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

#define MAX_SPRITE_TO_DRAW         2048

typedef struct tagSPRITE_TO_DRAW
{
   LPCBITMAPRLE     lpSpriteFrame; // pointer to the frame bitmap
   PAL_POS          pos;           // position on the scene
   int              iLayer;        // logical layer
} SPRITE_TO_DRAW;

static SPRITE_TO_DRAW    g_rgSpriteToDraw[MAX_SPRITE_TO_DRAW];
static int               g_nSpriteToDraw;

static VOID
PAL_AddSpriteToDraw(
   LPCBITMAPRLE     lpSpriteFrame,
   int              x,
   int              y,
   int              iLayer
)
/*++
   Purpose:

     Add a sprite to our list of drawing.

   Parameters:

     [IN]  lpSpriteFrame - the bitmap of the sprite frame.

     [IN]  x - the X coordinate on the screen.

     [IN]  y - the Y coordinate on the screen.

     [IN]  iLayer - the layer of the sprite.

   Return value:

     None.

--*/
{
   assert(g_nSpriteToDraw < MAX_SPRITE_TO_DRAW);

   g_rgSpriteToDraw[g_nSpriteToDraw].lpSpriteFrame = lpSpriteFrame;
   g_rgSpriteToDraw[g_nSpriteToDraw].pos = PAL_XY(x, y);
   g_rgSpriteToDraw[g_nSpriteToDraw].iLayer = iLayer;

   g_nSpriteToDraw++;
}

static VOID
PAL_CalcCoverTiles(
   SPRITE_TO_DRAW         *lpSpriteToDraw
)
/*++
   Purpose:

     Calculate all the tiles which may cover the specified sprite. Add the tiles
     into our list as well.

   Parameters:

     [IN]  lpSpriteToDraw - pointer to SPRITE_TO_DRAW struct.

   Return value:

     None.

--*/
{
   int             x, y, i, l, iTileHeight;
   LPCBITMAPRLE    lpTile;

   const int       sx = PAL_X(gpGlobals->viewport) + PAL_X(lpSpriteToDraw->pos);
   const int       sy = PAL_Y(gpGlobals->viewport) + PAL_Y(lpSpriteToDraw->pos);
   const int       sh = ((sx % 32) ? 1 : 0);

   const int       width = PAL_RLEGetWidth(lpSpriteToDraw->lpSpriteFrame);
   const int       height = PAL_RLEGetHeight(lpSpriteToDraw->lpSpriteFrame);

   int             dx = 0;
   int             dy = 0;
   int             dh = 0;

   //
   // Loop through all the tiles in the area of the sprite.
   //
   for (y = (sy - height - 15) / 16; y <= sy / 16; y++)
   {
      for (x = (sx - width / 2) / 32; x <= (sx + width / 2) / 32; x++)
      {
         for (i = ((x == (sx - width / 2) / 32) ? 0 : 3); i < 5; i++)
         {
            //
            // Scan tiles in the following form (* = to scan):
            //
            // . . . * * * . . .
            //  . . . * * . . . .
            //
            switch (i)
            {
               case 0:
                  dx = x;
                  dy = y;
                  dh = sh;
                  break;

               case 1:
                  dx = x - 1;
                  break;

               case 2:
                  dx = (sh ? x : (x - 1));
                  dy = (sh ? (y + 1) : y);
                  dh = 1 - sh;
                  break;

               case 3:
                  dx = x + 1;
                  dy = y;
                  dh = sh;
                  break;

               case 4:
                  dx = (sh ? (x + 1) : x);
                  dy = (sh ? (y + 1) : y);
                  dh = 1 - sh;
                  break;
            }

            for (l = 0; l < 2; l++)
            {
               lpTile = PAL_MapGetTileBitmap(dx, dy, dh, l, PAL_GetCurrentMap());
               iTileHeight = (signed char)PAL_MapGetTileHeight(dx, dy, dh, l, PAL_GetCurrentMap());

               //
               // Check if this tile may cover the sprites
               //
               if (lpTile != NULL && iTileHeight > 0 && (dy + iTileHeight) * 16 + dh * 8 >= sy)
               {
                  //
                  // This tile may cover the sprite
                  //
                  PAL_AddSpriteToDraw(lpTile,
                     dx * 32 + dh * 16 - 16 - PAL_X(gpGlobals->viewport),
                     dy * 16 + dh * 8 + 7 + l + iTileHeight * 8 - PAL_Y(gpGlobals->viewport),
                     iTileHeight * 8 + l);
               }
            }
         }
      }
   }
}

static VOID
PAL_SceneDrawSprites(
   VOID
)
/*++
   Purpose:

     Draw all the sprites to scene.

   Parameters:

     None.

   Return value:

     None.

--*/
{
   int i, x, y, vy;

   g_nSpriteToDraw = 0;

   //
   // Put all the sprites to be drawn into our array.
   //

   //
   // Players
   //
   for (i = 0; i <= (short)gpGlobals->wMaxPartyMemberIndex + gpGlobals->nFollower; i++)
   {
      LPCBITMAPRLE lpBitmap =
         PAL_SpriteGetFrame(PAL_GetPlayerSprite((BYTE)i), gpGlobals->rgParty[i].wFrame);

      if (lpBitmap == NULL)
      {
         continue;
      }

      //
      // Add it to our array
      //
      PAL_AddSpriteToDraw(lpBitmap,
         gpGlobals->rgParty[i].x - PAL_RLEGetWidth(lpBitmap) / 2,
         gpGlobals->rgParty[i].y + gpGlobals->wLayer + 10,
         gpGlobals->wLayer + 6);

      //
      // Calculate covering tiles on the map
      //
      PAL_CalcCoverTiles(&g_rgSpriteToDraw[g_nSpriteToDraw - 1]);
   }

   //
   // Event Objects (Monsters/NPCs/others)
   //
   for (i = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex;
      i < gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex; i++)
   {
      LPCBITMAPRLE     lpFrame;
      LPCSPRITE        lpSprite;

      LPEVENTOBJECT    lpEvtObj = &(gpGlobals->g.lprgEventObject[i]);

      int              iFrame;

      if (lpEvtObj->sState == kObjStateHidden || lpEvtObj->sVanishTime > 0 ||
         lpEvtObj->sState < 0)
      {
         continue;
      }

      //
      // Get the sprite
      //
      lpSprite = PAL_GetEventObjectSprite((WORD)i + 1);
      if (lpSprite == NULL)
      {
         continue;
      }

      iFrame = lpEvtObj->wCurrentFrameNum;
      if (lpEvtObj->nSpriteFrames == 3)
      {
         //
         // walking character
         //
         if (iFrame == 2)
         {
            iFrame = 0;
         }

         if (iFrame == 3)
         {
            iFrame = 2;
         }
      }

      lpFrame = PAL_SpriteGetFrame(lpSprite,
         lpEvtObj->wDirection * lpEvtObj->nSpriteFrames + iFrame);

      if (lpFrame == NULL)
      {
         continue;
      }

      //
      // Calculate the coordinate and check if outside the screen
      //
      x = (SHORT)lpEvtObj->x - PAL_X(gpGlobals->viewport);
      x -= PAL_RLEGetWidth(lpFrame) / 2;

      if (x >= 320 || x < -(int)PAL_RLEGetWidth(lpFrame))
      {
         //
         // outside the screen; skip it
         //
         continue;
      }

      y = (SHORT)lpEvtObj->y - PAL_Y(gpGlobals->viewport);
      y += lpEvtObj->sLayer * 8 + 9;

      vy = y - PAL_RLEGetHeight(lpFrame) - lpEvtObj->sLayer * 8 + 2;
      if (vy >= 200 || vy < -(int)PAL_RLEGetHeight(lpFrame))
      {
         //
         // outside the screen; skip it
         //
         continue;
      }

      //
      // Add it into the array
      //
      PAL_AddSpriteToDraw(lpFrame, x, y, lpEvtObj->sLayer * 8 + 2);

      //
      // Calculate covering map tiles
      //
      PAL_CalcCoverTiles(&g_rgSpriteToDraw[g_nSpriteToDraw - 1]);
   }

   //
   // All sprites are now in our array; sort them by their vertical positions.
   //
   for (x = 0; x < g_nSpriteToDraw - 1; x++)
   {
      SPRITE_TO_DRAW           tmp;
      BOOL                     fSwap = FALSE;

      for (y = 0; y < g_nSpriteToDraw - 1 - x; y++)
      {
         if (PAL_Y(g_rgSpriteToDraw[y].pos) > PAL_Y(g_rgSpriteToDraw[y + 1].pos))
         {
            fSwap = TRUE;

            tmp = g_rgSpriteToDraw[y];
            g_rgSpriteToDraw[y] = g_rgSpriteToDraw[y + 1];
            g_rgSpriteToDraw[y + 1] = tmp;
         }
      }

      if (!fSwap)
      {
         break;
      }
   }

   //
   // Draw all the sprites to the screen.
   //
   for (i = 0; i < g_nSpriteToDraw; i++)
   {
      SPRITE_TO_DRAW *p = &g_rgSpriteToDraw[i];

      x = PAL_X(p->pos);
      y = PAL_Y(p->pos) - PAL_RLEGetHeight(p->lpSpriteFrame) - p->iLayer;

      PAL_RLEBlitToSurface(p->lpSpriteFrame, gpScreen, PAL_XY(x, y));
   }
}

VOID
PAL_ApplyWave(
   SDL_Surface         *lpSurface
)
/*++
   Purpose:

     Apply screen waving effect when needed.

   Parameters:

     [OUT] lpSurface - the surface to be proceed.

   Return value:

     None.

--*/
{
   int                  wave[32];
   int                  i, a, b;
   static int           index = 0;
   LPBYTE               p;
   BYTE                 buf[320];

   gpGlobals->wScreenWave += gpGlobals->sWaveProgression;

   if (gpGlobals->wScreenWave == 0 || gpGlobals->wScreenWave >= 256)
   {
      //
      // No need to wave the screen
      //
      gpGlobals->wScreenWave = 0;
      gpGlobals->sWaveProgression = 0;
      return;
   }

   //
   // Calculate the waving offsets.
   //
   a = 0;
   b = 60 + 8;

   for (i = 0; i < 16; i++)
   {
      b -= 8;
      a += b;

      //
      // WARNING: assuming the screen width is 320
      //
      wave[i] = a * gpGlobals->wScreenWave / 256;
      wave[i + 16] = 320 - wave[i];
   }

   //
   // Apply the effect.
   // WARNING: only works with 320x200 8-bit surface.
   //
   a = index;
   p = (LPBYTE)(lpSurface->pixels);

   //
   // Loop through all lines in the screen buffer.
   //
   for (i = 0; i < 200; i++)
   {
      b = wave[a];

      if (b > 0)
      {
         //
         // Do a shift on the current line with the calculated offset.
         //
         memcpy(buf, p, b);
         //memmove(p, p + b, 320 - b);
         memmove(p, &p[b], 320 - b);
         //memcpy(p + 320 - b, buf, b);
         memcpy(&p[320 - b], buf, b);
      }

      a = (a + 1) % 32;
      p += lpSurface->pitch;
   }

   index = (index + 1) % 32;
}

VOID
PAL_MakeScene(
   VOID
)
/*++
   Purpose:

     Draw the scene of the current frame to the screen. Both the map and
     the sprites are handled here.

   Parameters:

     None.

   Return value:

     None.

--*/
{
   static SDL_Rect         rect = {0, 0, 320, 200};

   //
   // Step 1: Draw the complete map, for both of the layers.
   //
   rect.x = PAL_X(gpGlobals->viewport);
   rect.y = PAL_Y(gpGlobals->viewport);

   PAL_MapBlitToSurface(PAL_GetCurrentMap(), gpScreen, &rect, 0);
   PAL_MapBlitToSurface(PAL_GetCurrentMap(), gpScreen, &rect, 1);

   //
   // Step 2: Apply screen waving effects.
   //
   PAL_ApplyWave(gpScreen);

   //
   // Step 3: Draw all the sprites.
   //
   PAL_SceneDrawSprites();

   //
   // Check if we need to fade in.
   //
   if (gpGlobals->fNeedToFadeIn)
   {
      VIDEO_UpdateScreen(NULL);
      PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
      gpGlobals->fNeedToFadeIn = FALSE;
   }
}

BOOL
PAL_CheckObstacle(
   PAL_POS         pos,
   BOOL            fCheckEventObjects,
   WORD            wSelfObject
)
/*++
   Purpose:

     Check if the specified location has obstacle or not.

   Parameters:

     [IN]  pos - the position to check.

     [IN]  fCheckEventObjects - TRUE if check for event objects, FALSE if only
           check for the map.

     [IN]  wSelfObject - the event object which will be skipped.

   Return value:

     TRUE if the location is obstacle, FALSE if not.

--*/
{
   int x, y, h, xr, yr;

   if (PAL_X(pos) < 0 || PAL_X(pos) >= 2048 || PAL_Y(pos) < 0 || PAL_Y(pos) >= 2048)
   {
      return TRUE;
   }

   //
   // Check if the map tile at the specified position is blocking
   //
   x = PAL_X(pos) / 32;
   y = PAL_Y(pos) / 16;
   h = 0;

   xr = PAL_X(pos) % 32;
   yr = PAL_Y(pos) % 16;

   if (xr + yr * 2 >= 16)
   {
      if (xr + yr * 2 >= 48)
      {
         x++;
         y++;
      }
      else if (32 - xr + yr * 2 < 16)
      {
         x++;
      }
      else if (32 - xr + yr * 2 < 48)
      {
         h = 1;
      }
      else
      {
         y++;
      }
   }

   if (PAL_MapTileIsBlocked(x, y, h, PAL_GetCurrentMap()))
   {
      return TRUE;
   }

   if (fCheckEventObjects)
   {
      //
      // Loop through all event objects in the current scene
      //
      int i;
      for (i = gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex;
         i < gpGlobals->g.rgScene[gpGlobals->wNumScene].wEventObjectIndex; i++)
      {
         LPEVENTOBJECT p = &(gpGlobals->g.lprgEventObject[i]);
         if (i == wSelfObject - 1)
         {
            //
            // Skip myself
            //
            continue;
         }

         //
         // Is this object a blocking one?
         //
         if (p->sState >= kObjStateBlocker)
         {
            //
            // Check for collision
            //
            if (abs(p->x - PAL_X(pos)) + abs(p->y - PAL_Y(pos)) * 2 < 16)
            {
               return TRUE;
            }
         }
      }
   }

   return FALSE;
}

VOID
PAL_UpdatePartyGestures(
   BOOL             fWalking
)
/*++
   Purpose:

     Update the gestures of all the party members.

   Parameters:

     [IN]  fWalking - whether the party is walking or not.

   Return value:

     None.

--*/
{
   static int       s_iThisStepFrame = 0;
   int              iStepFrameFollower = 0, iStepFrameLeader = 0;
   int              i;

   if (fWalking)
   {
      //
      // Update the gesture for party leader
      //
      s_iThisStepFrame = (s_iThisStepFrame + 1) % 4;
      if (s_iThisStepFrame & 1)
      {
         iStepFrameLeader = (s_iThisStepFrame + 1) / 2;
         iStepFrameFollower = 3 - iStepFrameLeader;
      }
      else
      {
         iStepFrameLeader = 0;
         iStepFrameFollower = 0;
      }

      gpGlobals->rgParty[0].x = PAL_X(gpGlobals->partyoffset);
      gpGlobals->rgParty[0].y = PAL_Y(gpGlobals->partyoffset);

      if (gpGlobals->g.PlayerRoles.rgwWalkFrames[gpGlobals->rgParty[0].wPlayerRole] == 4)
      {
         gpGlobals->rgParty[0].wFrame = gpGlobals->wPartyDirection * 4 + s_iThisStepFrame;
      }
      else
      {
         gpGlobals->rgParty[0].wFrame = gpGlobals->wPartyDirection * 3 + iStepFrameLeader;
      }

      //
      // Update the gestures and positions for other party members
      //
      for (i = 1; i <= (short)gpGlobals->wMaxPartyMemberIndex; i++)
      {
         gpGlobals->rgParty[i].x = gpGlobals->rgTrail[1].x - PAL_X(gpGlobals->viewport);
         gpGlobals->rgParty[i].y = gpGlobals->rgTrail[1].y - PAL_Y(gpGlobals->viewport);

         if (i == 2)
         {
            gpGlobals->rgParty[i].x +=
               (gpGlobals->rgTrail[1].wDirection == kDirEast || gpGlobals->rgTrail[1].wDirection == kDirWest) ? -16 : 16;
            gpGlobals->rgParty[i].y += 8;
         }
         else
         {
            gpGlobals->rgParty[i].x +=
               ((gpGlobals->rgTrail[1].wDirection == kDirWest || gpGlobals->rgTrail[1].wDirection == kDirSouth) ? 16 : -16);
            gpGlobals->rgParty[i].y +=
               ((gpGlobals->rgTrail[1].wDirection == kDirWest || gpGlobals->rgTrail[1].wDirection == kDirNorth) ? 8 : -8);
         }

         //
         // Adjust the position if there is obstacle
         //
         if (PAL_CheckObstacle(PAL_XY(gpGlobals->rgParty[i].x + PAL_X(gpGlobals->viewport),
            gpGlobals->rgParty[i].y + PAL_Y(gpGlobals->viewport)), TRUE, 0))
         {
            gpGlobals->rgParty[i].x = gpGlobals->rgTrail[1].x - PAL_X(gpGlobals->viewport);
            gpGlobals->rgParty[i].y = gpGlobals->rgTrail[1].y - PAL_Y(gpGlobals->viewport);
         }

         //
         // Update gesture for this party member
         //
         if (gpGlobals->g.PlayerRoles.rgwWalkFrames[gpGlobals->rgParty[i].wPlayerRole] == 4)
         {
            gpGlobals->rgParty[i].wFrame = gpGlobals->rgTrail[2].wDirection * 4 + s_iThisStepFrame;
         }
         else
         {
            gpGlobals->rgParty[i].wFrame = gpGlobals->rgTrail[2].wDirection * 3 + iStepFrameLeader;
         }
      }

      if (gpGlobals->nFollower > 0)
      {
         //
         // Update the position and gesture for the follower
         //
         gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + 1].x =
            gpGlobals->rgTrail[3].x - PAL_X(gpGlobals->viewport);
         gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + 1].y =
            gpGlobals->rgTrail[3].y - PAL_Y(gpGlobals->viewport);
         gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + 1].wFrame =
            gpGlobals->rgTrail[3].wDirection * 3 + iStepFrameFollower;
      }
   }
   else
   {
      //
      // Player is not moved. Use the "standing" gesture instead of "walking" one.
      //
      i = gpGlobals->g.PlayerRoles.rgwWalkFrames[gpGlobals->rgParty[0].wPlayerRole];
      if (i == 0)
      {
         i = 3;
      }
      gpGlobals->rgParty[0].wFrame = gpGlobals->wPartyDirection * i;

      for (i = 1; i <= (short)gpGlobals->wMaxPartyMemberIndex; i++)
      {
         int f = gpGlobals->g.PlayerRoles.rgwWalkFrames[gpGlobals->rgParty[i].wPlayerRole];
         if (f == 0)
         {
            f = 3;
         }
         gpGlobals->rgParty[i].wFrame = gpGlobals->rgTrail[2].wDirection * f;
      }

      if (gpGlobals->nFollower > 0)
      {
         gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + 1].wFrame =
            gpGlobals->rgTrail[3].wDirection * 3;
      }

      s_iThisStepFrame &= 2;
      s_iThisStepFrame ^= 2;
   }
}

VOID
PAL_UpdateParty(
   VOID
)
/*++
   Purpose:

     Update the location and walking gesture of all the party members.

   Parameters:

     None.

   Return value:

     None.

--*/
{
   int              xSource, ySource, xTarget, yTarget, xOffset, yOffset, i;

   //
   // Has user pressed one of the arrow keys?
   //
   if (g_InputState.dir != kDirUnknown)
   {
      xOffset = ((g_InputState.dir == kDirWest || g_InputState.dir == kDirSouth) ? -16 : 16);
      yOffset = ((g_InputState.dir == kDirWest || g_InputState.dir == kDirNorth) ? -8 : 8);

      xSource = PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset);
      ySource = PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset);

      xTarget = xSource + xOffset;
      yTarget = ySource + yOffset;

      gpGlobals->wPartyDirection = g_InputState.dir;

      //
      // Check for obstacles on the destination location
      //
      if (!PAL_CheckObstacle(PAL_XY(xTarget, yTarget), TRUE, 0))
      {
         //
         // Player will actually be moved. Store trail.
         //
         for (i = 3; i >= 0; i--)
         {
            gpGlobals->rgTrail[i + 1] = gpGlobals->rgTrail[i];
         }

         gpGlobals->rgTrail[0].wDirection = g_InputState.dir;
         gpGlobals->rgTrail[0].x = xSource;
         gpGlobals->rgTrail[0].y = ySource;

         //
         // Move the viewport
         //
         gpGlobals->viewport =
            PAL_XY(PAL_X(gpGlobals->viewport) + xOffset, PAL_Y(gpGlobals->viewport) + yOffset);

         //
         // Update gestures
         //
         PAL_UpdatePartyGestures(TRUE);

         return; // don't go further
      }
   }

   PAL_UpdatePartyGestures(FALSE);
}

VOID
PAL_NPCWalkOneStep(
   WORD          wEventObjectID,
   INT           iSpeed
)
/*++
  Purpose:

    Move and animate the specified event object (NPC).

  Parameters:

    [IN]  wEventObjectID - the event object to move.

    [IN]  iSpeed - speed of the movement.

  Return value:

    None.

--*/
{
   LPEVENTOBJECT        p;

   //
   // Check for invalid parameters
   //
   if (wEventObjectID == 0 || wEventObjectID > gpGlobals->g.nEventObject)
   {
      return;
   }

   p = &(gpGlobals->g.lprgEventObject[wEventObjectID - 1]);

   //
   // Move the event object by the specified direction
   //
   p->x += ((p->wDirection == kDirWest || p->wDirection == kDirSouth) ? -2 : 2) * iSpeed;
   p->y += ((p->wDirection == kDirWest || p->wDirection == kDirNorth) ? -1 : 1) * iSpeed;

   //
   // Update the gesture
   //
   if (p->nSpriteFrames > 0)
   {
      p->wCurrentFrameNum++;
      p->wCurrentFrameNum %= (p->nSpriteFrames == 3 ? 4 : p->nSpriteFrames);
   }
   else if (p->nSpriteFramesAuto > 0)
   {
      p->wCurrentFrameNum++;
      p->wCurrentFrameNum %= p->nSpriteFramesAuto;
   }
}
