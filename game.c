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

static VOID
PAL_GameStart(
   VOID
)
/*++
  Purpose:

    Do some initialization work when game starts (new game or load game).

  Parameters:

    None.

  Return value:

    None.

--*/
{
   PAL_SetLoadFlags(kLoadScene | kLoadPlayerSprite);

   if (!gpGlobals->fEnteringScene)
   {
      //
      // Fade in music if the player has loaded an old game.
      //
      AUDIO_PlayMusic(gpGlobals->wNumMusic, TRUE, 1);
   }

   gpGlobals->fNeedToFadeIn = TRUE;
   gpGlobals->dwFrameNum = 0;
}

VOID
PAL_GameMain(
   VOID
)
/*++
  Purpose:

    The game entry routine.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   DWORD       dwTime;

   //
   // Show the opening menu.
   //
   gpGlobals->bCurrentSaveSlot = (BYTE)PAL_OpeningMenu();
   gpGlobals->fInMainGame = TRUE;

   //
   // Initialize game data and set the flags to load the game resources.
   //
   PAL_InitGameData(gpGlobals->bCurrentSaveSlot);

   //
   // Run the main game loop.
   //
   dwTime = SDL_GetTicks();
   while (TRUE)
   {
      //
      // Do some initialization at game start.
      //
      if (gpGlobals->fGameStart)
      {
         PAL_GameStart();
         gpGlobals->fGameStart = FALSE;
      }

      //
      // Load the game resources if needed.
      //
      PAL_LoadResources();

      //
      // Clear the input state of previous frame.
      //
      PAL_ClearKeyState();

      //
      // Wait for the time of one frame. Accept input here.
      //
      PAL_DelayUntil(dwTime);

      //
      // Set the time of the next frame.
      //
      dwTime = SDL_GetTicks() + FRAME_TIME;

      //
      // Run the main frame routine.
      //
      PAL_StartFrame();
   }
}
