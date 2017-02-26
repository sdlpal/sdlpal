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

typedef struct tagRESOURCES
{
   BYTE             bLoadFlags;

   LPPALMAP         lpMap;                                      // current loaded map
   LPSPRITE        *lppEventObjectSprites;                      // event object sprites
   int              nEventObject;                               // number of event objects

   LPSPRITE         rglpPlayerSprite[MAX_PLAYERS_IN_PARTY + 1]; // player sprites
} RESOURCES, *LPRESOURCES;

static LPRESOURCES gpResources = NULL;

static VOID
PAL_FreeEventObjectSprites(
   VOID
)
/*++
  Purpose:

    Free all sprites of event objects on the scene.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int i;

   if (gpResources->lppEventObjectSprites != NULL)
   {
      for (i = 0; i < gpResources->nEventObject; i++)
      {
         free(gpResources->lppEventObjectSprites[i]);
      }

      free(gpResources->lppEventObjectSprites);

      gpResources->lppEventObjectSprites = NULL;
      gpResources->nEventObject = 0;
   }
}

static VOID
PAL_FreePlayerSprites(
   VOID
)
/*++
  Purpose:

    Free all player sprites.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int i;

   for (i = 0; i < MAX_PLAYERS_IN_PARTY + 1; i++)
   {
      free(gpResources->rglpPlayerSprite[i]);
      gpResources->rglpPlayerSprite[i] = NULL;
   }
}

VOID
PAL_InitResources(
   VOID
)
/*++
  Purpose:

    Initialze the resource manager.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   gpResources = (LPRESOURCES)UTIL_calloc(1, sizeof(RESOURCES));
}

VOID
PAL_FreeResources(
   VOID
)
/*++
  Purpose:

    Free all loaded resources.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   if (gpResources != NULL)
   {
      //
      // Free all loaded sprites
      //
      PAL_FreePlayerSprites();
      PAL_FreeEventObjectSprites();

      //
      // Free map
      //
      PAL_FreeMap(gpResources->lpMap);

      //
      // Delete the instance
      //
      free(gpResources);
   }

   gpResources = NULL;
}

VOID
PAL_SetLoadFlags(
   BYTE       bFlags
)
/*++
  Purpose:

    Set flags to load resources.

  Parameters:

    [IN]  bFlags - flags to be set.

  Return value:

    None.

--*/
{
   if (gpResources == NULL)
   {
      return;
   }

   gpResources->bLoadFlags |= bFlags;
}

VOID
PAL_LoadResources(
   VOID
)
/*++
  Purpose:

    Load the game resources if needed.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int                i, index, l, n;
   WORD               wPlayerID, wSpriteNum;

   if (gpResources == NULL || gpResources->bLoadFlags == 0)
   {
      return;
   }

   //
   // Load scene
   //
   if (gpResources->bLoadFlags & kLoadScene)
   {
      FILE              *fpMAP, *fpGOP;

      fpMAP = UTIL_OpenRequiredFile("map.mkf");
      fpGOP = UTIL_OpenRequiredFile("gop.mkf");

      if (gpGlobals->fEnteringScene)
      {
         gpGlobals->wScreenWave = 0;
         gpGlobals->sWaveProgression = 0;
      }

      //
      // Free previous loaded scene (sprites and map)
      //
      PAL_FreeEventObjectSprites();
      PAL_FreeMap(gpResources->lpMap);

      //
      // Load map
      //
      i = gpGlobals->wNumScene - 1;
      gpResources->lpMap = PAL_LoadMap(gpGlobals->g.rgScene[i].wMapNum,
         fpMAP, fpGOP);

      if (gpResources->lpMap == NULL)
      {
         fclose(fpMAP);
         fclose(fpGOP);

         TerminateOnError("PAL_LoadResources(): Fail to load map #%d (scene #%d) !",
            gpGlobals->g.rgScene[i].wMapNum, gpGlobals->wNumScene);
      }

      //
      // Load sprites
      //
      index = gpGlobals->g.rgScene[i].wEventObjectIndex;
      gpResources->nEventObject = gpGlobals->g.rgScene[i + 1].wEventObjectIndex;
      gpResources->nEventObject -= index;

      if (gpResources->nEventObject > 0)
      {
         gpResources->lppEventObjectSprites =
            (LPSPRITE *)UTIL_calloc(gpResources->nEventObject, sizeof(LPSPRITE));
      }

      for (i = 0; i < gpResources->nEventObject; i++, index++)
      {
         n = gpGlobals->g.lprgEventObject[index].wSpriteNum;
         if (n == 0)
         {
            //
            // this event object has no sprite
            //
            gpResources->lppEventObjectSprites[i] = NULL;
            continue;
         }

         l = PAL_MKFGetDecompressedSize(n, gpGlobals->f.fpMGO);

         gpResources->lppEventObjectSprites[i] = (LPSPRITE)UTIL_malloc(l);

         if (PAL_MKFDecompressChunk(gpResources->lppEventObjectSprites[i], l,
            n, gpGlobals->f.fpMGO) > 0)
         {
            gpGlobals->g.lprgEventObject[index].nSpriteFramesAuto =
               PAL_SpriteGetNumFrames(gpResources->lppEventObjectSprites[i]);
         }
      }

      gpGlobals->partyoffset = PAL_XY(160, 112);

      fclose(fpGOP);
      fclose(fpMAP);
   }

   //
   // Load player sprites
   //
   if (gpResources->bLoadFlags & kLoadPlayerSprite)
   {
      //
      // Free previous loaded player sprites
      //
      PAL_FreePlayerSprites();

      for (i = 0; i <= (short)gpGlobals->wMaxPartyMemberIndex; i++)
      {
         wPlayerID = gpGlobals->rgParty[i].wPlayerRole;
         assert(wPlayerID < MAX_PLAYER_ROLES);

         //
         // Load player sprite
         //
         wSpriteNum = gpGlobals->g.PlayerRoles.rgwSpriteNum[wPlayerID];

         l = PAL_MKFGetDecompressedSize(wSpriteNum, gpGlobals->f.fpMGO);

         gpResources->rglpPlayerSprite[i] = (LPSPRITE)UTIL_malloc(l);

         PAL_MKFDecompressChunk(gpResources->rglpPlayerSprite[i], l, wSpriteNum,
            gpGlobals->f.fpMGO);
      }

      if (gpGlobals->nFollower > 0)
      {
         //
         // Load the follower sprite
         //
         wSpriteNum = gpGlobals->rgParty[i].wPlayerRole;

         l = PAL_MKFGetDecompressedSize(wSpriteNum, gpGlobals->f.fpMGO);

         gpResources->rglpPlayerSprite[i] = (LPSPRITE)UTIL_malloc(l);

         PAL_MKFDecompressChunk(gpResources->rglpPlayerSprite[i], l, wSpriteNum,
            gpGlobals->f.fpMGO);
      }
   }

   //
   // Clear all of the load flags
   //
   gpResources->bLoadFlags = 0;
}

LPPALMAP
PAL_GetCurrentMap(
   VOID
)
/*++
  Purpose:

    Get the current loaded map.

  Parameters:

    None.

  Return value:

    Pointer to the current loaded map. NULL if no map is loaded.

--*/
{
   if (gpResources == NULL)
   {
      return NULL;
   }

   return gpResources->lpMap;
}

LPSPRITE
PAL_GetPlayerSprite(
   BYTE      bPlayerIndex
)
/*++
  Purpose:

    Get the player sprite.

  Parameters:

    [IN]  bPlayerIndex - index of player in party (starts from 0).

  Return value:

    Pointer to the player sprite.

--*/
{
   if (gpResources == NULL || bPlayerIndex > MAX_PLAYERS_IN_PARTY)
   {
      return NULL;
   }

   return gpResources->rglpPlayerSprite[bPlayerIndex];
}

LPSPRITE
PAL_GetEventObjectSprite(
   WORD      wEventObjectID
)
/*++
  Purpose:

    Get the sprite of the specified event object.

  Parameters:

    [IN]  wEventObjectID - the ID of event object.

  Return value:

    Pointer to the sprite.

--*/
{
   wEventObjectID -= gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex;
   wEventObjectID--;

   if (gpResources == NULL || wEventObjectID >= gpResources->nEventObject)
   {
      return NULL;
   }

   return gpResources->lppEventObjectSprites[wEventObjectID];
}
