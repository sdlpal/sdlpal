/* -*- mode: c; tab-width: 4; c-basic-offset: 3; c-file-style: "linux" -*- */
//
// Copyright (c) 2009, Pal_Bazzi.
//
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

#ifdef PSP

#include <math.h>
#include <pspkernel.h>
#include <pspctrl.h>
#include <SDL_thread.h>

PALINPUTSTATE            g_InputState;

BOOL                     g_fUseJoystick = TRUE;

static SceCtrlData pad;
static SDL_sem *pad_sem = 0;
static SDL_Thread *bthread = 0;
static int running = 0;
static unsigned int old_button=0;
static unsigned char old_x = 0, old_y = 0;

//
// Collect pad data about once per frame
//
int PSP_JoystickUpdate(void *data)
{
	while (running)
	{
		SDL_SemWait(pad_sem);
		sceCtrlPeekBufferPositive(&pad, 1); 
		SDL_SemPost(pad_sem);
		//
		// Delay 1/60th of a second 
		//
		sceKernelDelayThread(1000000 / 60);  
	}
	return 0;
}

void PAL_calc_Axes(
   unsigned char x,
   unsigned char y
)
{
   if(x<y && x+y<51)
   {
      g_InputState.dwKeyPress = kKeyLeft;
      g_InputState.prevdir = g_InputState.dir;
			g_InputState.dir = kDirWest;
			return;
   }
   if(x<y && x+y>51)
   {
      g_InputState.dwKeyPress = kKeyDown;
      g_InputState.prevdir = g_InputState.dir;
			g_InputState.dir = kDirSouth;
			return;
   }
   if(x>y && x+y<51)
   {
      g_InputState.dwKeyPress = kKeyUp;
      g_InputState.prevdir = g_InputState.dir;
			g_InputState.dir = kDirNorth;
			return;
   }
   if(x>y && x+y>51)
   {
      g_InputState.dwKeyPress = kKeyRight;
      g_InputState.prevdir = g_InputState.dir;
			g_InputState.dir = kDirEast;
			return;
	 }
	 g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
	 g_InputState.dir = kDirUnknown;
}

VOID
PAL_JoystickEventFilter(
   VOID
)
/*++
  Purpose:

    Handle joystick events.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   unsigned int button;
   unsigned char x, y;
 
   SDL_SemWait(pad_sem);
   button = pad.Buttons;
   x = pad.Lx;
   y = pad.Ly;
   SDL_SemPost(pad_sem);

   //
   //Axes
   //
   x /= 5;
   y /= 5;
   BOOL onCenter=(x>16 && x<32) && (y>16 && y<32);
   if(!onCenter)
   {
		 if(old_x != x || old_y != y) 
     {
		    PAL_calc_Axes(x,y);
		    old_y = y;
		    old_x = x;
		 }
	 }
	 else if (!button)
	 {
	   g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
	   g_InputState.dir = kDirUnknown;
	 }

   //
   //Buttons
   //
   int changed = (button != old_button);
   old_button = button;
   if(changed)
   {
	   if (button & PSP_CTRL_UP)
	   {
			 g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
			 g_InputState.dir = kDirNorth;
			 g_InputState.dwKeyPress = kKeyUp;
			 return;
	   } 
		 if (button & PSP_CTRL_DOWN)
		 {
		 	 g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
			 g_InputState.dir = kDirSouth;
			 g_InputState.dwKeyPress = kKeyDown;
			 return;
		 } 
		 if (button & PSP_CTRL_LEFT)
		 {
			 g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
			 g_InputState.dir = kDirWest;
			 g_InputState.dwKeyPress = kKeyLeft;
			 return;
		 } 
		 if (button & PSP_CTRL_RIGHT)
		 {
			 g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
			 g_InputState.dir = kDirEast;
			 g_InputState.dwKeyPress = kKeyRight;
			 return;
		 }   
		 if (button & PSP_CTRL_SQUARE)
		 {
		   g_InputState.dwKeyPress = kKeyForce;
			 return;
		 }
		 if (button & PSP_CTRL_TRIANGLE)
		 {
			g_InputState.dwKeyPress = kKeyThrowItem;
			return;
		 } 
		 if (button & PSP_CTRL_CIRCLE)
		 {
			 g_InputState.dwKeyPress = kKeySearch;
			 return;
		 } 
		 if (button & PSP_CTRL_CROSS)
		 {
			 g_InputState.dwKeyPress = kKeyMenu;
			 return;
		 } 
		 if (button & PSP_CTRL_START)
		 {
			 g_InputState.dwKeyPress = kKeySearch;
			 return;
		 }
		 if (button & PSP_CTRL_SELECT)
		 {
			 g_InputState.dwKeyPress = kKeyMenu;
			 return;
		 }
		 if (button & PSP_CTRL_LTRIGGER)
		 {
			 g_InputState.dwKeyPress = kKeyUseItem;
			 return;
		 }
		 if (button & PSP_CTRL_RTRIGGER)
		 {
			 g_InputState.dwKeyPress = kKeyRepeat;
			 return;
		 }
	   g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
	   g_InputState.dir = kDirUnknown;
   }
}


#if SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION <= 2
static int SDLCALL
PAL_EventFilter(
   const SDL_Event       *lpEvent
)
#else
static int SDLCALL
PAL_EventFilter(
   void                  *userdata,
   const SDL_Event       *lpEvent
)
#endif
/*++
  Purpose:

    SDL event filter function. A filter to process all events.

  Parameters:

    [IN]  lpEvent - pointer to the event.

  Return value:

    1 = the event will be added to the internal queue.
    0 = the event will be dropped from the queue.

--*/
{
   switch (lpEvent->type)
   {
   case SDL_QUIT:
      //
      // clicked on the close button of the window. Quit immediately.
      //
      PAL_Shutdown();
      exit(0);
   }

   //
   // All events are handled here; don't put anything to the internal queue
   //
   return 0;
}

VOID
PAL_ClearKeyState(
   VOID
)
/*++
  Purpose:

    Clear the record of pressed keys.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   g_InputState.dwKeyPress = 0;
}

VOID
PAL_InitInput(
   VOID
)
/*++
  Purpose:

    Initialize the input subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{ 
   memset(&g_InputState, 0, sizeof(g_InputState));
   g_InputState.dir = kDirUnknown;
   g_InputState.prevdir = kDirUnknown;
#if SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION <= 2
   SDL_SetEventFilter(PAL_EventFilter);
#else
   SDL_SetEventFilter(PAL_EventFilter, NULL);
#endif

   //
   // Setup input
   //
   sceCtrlSetSamplingCycle(0);
   sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
   pad.Buttons = 0;

	 //
	 // Start thread to read data 
	 //
	 if((pad_sem =  SDL_CreateSemaphore(1)) == NULL)
	 {
		 TerminateOnError("Can't create input semaphore\n");
		 return;
	 }
	 running = 1;
	 if((bthread = SDL_CreateThread(PSP_JoystickUpdate, NULL)) == NULL)
	 {
		 TerminateOnError("Can't create input thread\n");
		 return;
	 }
}

VOID
PAL_ShutdownInput(
   VOID
)
/*++
  Purpose:

    Shutdown the input subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
	//
	// Cleanup Threads and Semaphore. 
	//
	running = 0;
	SDL_WaitThread(bthread, NULL);
	SDL_DestroySemaphore(pad_sem);
}

VOID
PAL_ProcessEvent(
   VOID
)
/*++
  Purpose:

    Process all events.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   while (PAL_PollEvent(NULL));
   PAL_JoystickEventFilter();
}

#endif
