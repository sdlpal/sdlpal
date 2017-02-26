/* -*- mode: c; tab-width: 4; c-basic-offset: 3; c-file-style: "linux" -*- */
//
// Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Portions Copyright (c) 2009, netwan.
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
#include <math.h>

volatile PALINPUTSTATE   g_InputState;
#if PAL_HAS_JOYSTICKS
static SDL_Joystick     *g_pJoy = NULL;
#endif
#if SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION <= 2
#define SDL_JoystickNameForIndex SDL_JoystickName
#endif
BOOL                     g_fUseJoystick = TRUE;

static void _default_init_filter() {}
static int _default_input_event_filter(const SDL_Event *event, volatile PALINPUTSTATE *state) { return 0; }
static void _default_input_shutdown_filter() {}

static void (*input_init_filter)() = _default_init_filter;
static int (*input_event_filter)(const SDL_Event *, volatile PALINPUTSTATE *) = _default_input_event_filter;
static void (*input_shutdown_filter)() = _default_input_shutdown_filter;


static VOID
PAL_KeyboardEventFilter(
   const SDL_Event       *lpEvent
)
/*++
  Purpose:

    Handle keyboard events.

  Parameters:

    [IN]  lpEvent - pointer to the event.

  Return value:

    None.

--*/
{
   switch (lpEvent->type)
   {
   case SDL_KEYDOWN:
      //
      // Pressed a key
      //
      if (lpEvent->key.keysym.mod & KMOD_ALT)
      {
         if (lpEvent->key.keysym.sym == SDLK_RETURN)
         {
            //
            // Pressed Alt+Enter (toggle fullscreen)...
            //
            VIDEO_ToggleFullscreen();
            return;
         }
         else if (lpEvent->key.keysym.sym == SDLK_F4)
         {
            //
            // Pressed Alt+F4 (Exit program)...
            //
            PAL_Shutdown(0);
         }
      }

      switch (lpEvent->key.keysym.sym)
      {
      case SDLK_UP:
      case SDLK_KP8:
         if (gpGlobals->fInBattle || g_InputState.dir != kDirNorth)
         {
            g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
            g_InputState.dir = kDirNorth;
            g_InputState.dwKeyPress |= kKeyUp;
         }
         break;

      case SDLK_DOWN:
      case SDLK_KP2:
         if (gpGlobals->fInBattle || g_InputState.dir != kDirSouth)
         {
            g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
            g_InputState.dir = kDirSouth;
            g_InputState.dwKeyPress |= kKeyDown;
         }
         break;

      case SDLK_LEFT:
      case SDLK_KP4:
         if (gpGlobals->fInBattle || g_InputState.dir != kDirWest)
         {
            g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
            g_InputState.dir = kDirWest;
            g_InputState.dwKeyPress |= kKeyLeft;
         }
         break;

     case SDLK_RIGHT:
     case SDLK_KP6:
         if (gpGlobals->fInBattle || g_InputState.dir != kDirEast)
         {
            g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
            g_InputState.dir = kDirEast;
            g_InputState.dwKeyPress |= kKeyRight;
         }
         break;

      case SDLK_ESCAPE:
      case SDLK_INSERT:
      case SDLK_LALT:
      case SDLK_RALT:
      case SDLK_KP0:
         g_InputState.dwKeyPress |= kKeyMenu;
         break;

      case SDLK_RETURN:
      case SDLK_SPACE:
      case SDLK_KP_ENTER:
      case SDLK_LCTRL:
         g_InputState.dwKeyPress |= kKeySearch;
         break;

      case SDLK_PAGEUP:
      case SDLK_KP9:
         g_InputState.dwKeyPress |= kKeyPgUp;
         break;

      case SDLK_PAGEDOWN:
      case SDLK_KP3:
         g_InputState.dwKeyPress |= kKeyPgDn;
         break;

      case SDLK_HOME:
         g_InputState.dwKeyPress |= kKeyHome;
         break;

      case SDLK_END:
         g_InputState.dwKeyPress |= kKeyEnd;
         break;

      case SDLK_7: //7 for mobile device
      case SDLK_r:
         g_InputState.dwKeyPress |= kKeyRepeat;
         break;

      case SDLK_2: //2 for mobile device
      case SDLK_a:
         g_InputState.dwKeyPress |= kKeyAuto;
         break;

      case SDLK_d:
         g_InputState.dwKeyPress |= kKeyDefend;
         break;

      case SDLK_e:
         g_InputState.dwKeyPress |= kKeyUseItem;
         break;

      case SDLK_w:
         g_InputState.dwKeyPress |= kKeyThrowItem;
         break;

      case SDLK_q:
         g_InputState.dwKeyPress |= kKeyFlee;
         break;

      case SDLK_s:
         g_InputState.dwKeyPress |= kKeyStatus;
         break;

      case SDLK_f:
      case SDLK_5: // 5 for mobile device
         g_InputState.dwKeyPress |= kKeyForce;
         break;

      case SDLK_HASH: //# for mobile device
      case SDLK_p:
         VIDEO_SaveScreenshot();
         break;

      default:
         break;
      }
      break;

   case SDL_KEYUP:
      //
      // Released a key
      //
      switch (lpEvent->key.keysym.sym)
      {
      case SDLK_UP:
      case SDLK_KP8:
         if (g_InputState.dir == kDirNorth)
         {
            g_InputState.dir = g_InputState.prevdir;
         }
         g_InputState.prevdir = kDirUnknown;
         break;

      case SDLK_DOWN:
      case SDLK_KP2:
         if (g_InputState.dir == kDirSouth)
         {
            g_InputState.dir = g_InputState.prevdir;
         }
         g_InputState.prevdir = kDirUnknown;
         break;

      case SDLK_LEFT:
      case SDLK_KP4:
         if (g_InputState.dir == kDirWest)
         {
            g_InputState.dir = g_InputState.prevdir;
         }
         g_InputState.prevdir = kDirUnknown;
         break;

      case SDLK_RIGHT:
      case SDLK_KP6:
         if (g_InputState.dir == kDirEast)
         {
            g_InputState.dir = g_InputState.prevdir;
         }
         g_InputState.prevdir = kDirUnknown;
         break;

      default:
         break;
      }
      break;
   }
}

static VOID
PAL_MouseEventFilter(
   const SDL_Event *lpEvent
)
/*++
  Purpose:

    Handle mouse events.

  Parameters:

    [IN]  lpEvent - pointer to the event.

  Return value:

    None.

--*/
{
#if PAL_HAS_MOUSE
   static short hitTest = 0; // Double click detect;   
   const SDL_VideoInfo *vi;

   double       screenWidth, gridWidth;
   double       screenHeight, gridHeight;
   double       mx, my;
   double       thumbx;
   double       thumby;
   INT          gridIndex;
   BOOL         isLeftMouseDBClick = FALSE;
   BOOL         isLeftMouseClick = FALSE;
   BOOL         isRightMouseClick = FALSE;
   static INT   lastReleaseButtonTime, lastPressButtonTime, betweenTime;
   static INT   lastPressx = 0;
   static INT   lastPressy = 0;
   static INT   lastReleasex = 0;
   static INT   lastReleasey = 0;

   if (lpEvent->type!= SDL_MOUSEBUTTONDOWN && lpEvent->type != SDL_MOUSEBUTTONUP)
      return;

   vi = SDL_GetVideoInfo();
   screenWidth = vi->current_w;
   screenHeight = vi->current_h;
   gridWidth = screenWidth / 3;
   gridHeight = screenHeight / 3;
   mx = lpEvent->button.x;
   my = lpEvent->button.y;
   thumbx = ceil(mx / gridWidth);
   thumby = floor(my / gridHeight);
   gridIndex = thumbx + thumby * 3 - 1;
   
   switch (lpEvent->type)
   {
   case SDL_MOUSEBUTTONDOWN:
      lastPressButtonTime = SDL_GetTicks();
      lastPressx = lpEvent->button.x;
      lastPressy = lpEvent->button.y;
      switch (gridIndex)
      {
      case 2:
         g_InputState.prevdir = g_InputState.dir;
         g_InputState.dir = kDirNorth;
         break;
      case 6:
         g_InputState.prevdir = g_InputState.dir;
         g_InputState.dir = kDirSouth;
         break;
      case 0:
         g_InputState.prevdir = g_InputState.dir;
         g_InputState.dir = kDirWest;
         break;
      case 8:
         g_InputState.prevdir = g_InputState.dir;
         g_InputState.dir = kDirEast;
         break;
      case 1:
        //g_InputState.prevdir = g_InputState.dir;
        //g_InputState.dir = kDirNorth;
         g_InputState.dwKeyPress |= kKeyUp;
         break;
      case 7:
        //g_InputState.prevdir = g_InputState.dir;
        //g_InputState.dir = kDirSouth; 
         g_InputState.dwKeyPress |= kKeyDown;
         break;
      case 3:
        //g_InputState.prevdir = g_InputState.dir;
        //g_InputState.dir = kDirWest;
        g_InputState.dwKeyPress |= kKeyLeft;
         break;
      case 5:
         //g_InputState.prevdir = g_InputState.dir;
         //g_InputState.dir = kDirEast;
         g_InputState.dwKeyPress |= kKeyRight;
         break;
      }
      break;
   case SDL_MOUSEBUTTONUP:
      lastReleaseButtonTime = SDL_GetTicks();
      lastReleasex = lpEvent->button.x;
      lastReleasey = lpEvent->button.y;
      hitTest ++;
      if (abs(lastPressx - lastReleasex) < 25 &&
                     abs(lastPressy - lastReleasey) < 25)
      {
        betweenTime = lastReleaseButtonTime - lastPressButtonTime;
        if (betweenTime >500)
        {
           isRightMouseClick = TRUE;
        }
        else if (betweenTime >=0)
        {
           if((betweenTime < 100) && (hitTest >= 2))
           {
              isLeftMouseClick = TRUE;
                hitTest = 0;  
           }
           else
           {  
              isLeftMouseClick = TRUE;
              if(betweenTime > 100)
              {
                 hitTest = 0;
              }
              
           }
        }
      }
      switch (gridIndex)
      {
      case 2:
        if( isLeftMouseDBClick )
       {
          AUDIO_IncreaseVolume();
          break;
       }
      case 6:
      case 0:
        if( isLeftMouseDBClick )
       {
          AUDIO_DecreaseVolume();
          break;
       }
      case 7:
         if (isRightMouseClick) //repeat attack
         {
            g_InputState.dwKeyPress |= kKeyRepeat;
            break;
         }
      case 8:
         g_InputState.dir = kDirUnknown;
         g_InputState.prevdir = kDirUnknown;
         break;
      case 1:
        if( isRightMouseClick )
       {
          g_InputState.dwKeyPress |= kKeyForce;
       }
        break;
      case 3:
        if( isRightMouseClick )
       {
          g_InputState.dwKeyPress |= kKeyAuto;
       }
        break;
      case 5:
        if( isRightMouseClick )
       {
          g_InputState.dwKeyPress |= kKeyDefend;
       }
       break;
      case 4:
      if (isRightMouseClick) // menu
      {
         g_InputState.dwKeyPress |= kKeyMenu;
      }
      else if (isLeftMouseClick) // search
      {
         g_InputState.dwKeyPress |= kKeySearch;
      }
      
        break;
      }
      break;
   }
#endif
}

static VOID
PAL_JoystickEventFilter(
   const SDL_Event       *lpEvent
)
/*++
  Purpose:

    Handle joystick events.

  Parameters:

    [IN]  lpEvent - pointer to the event.

  Return value:

    None.

--*/
{
#if PAL_HAS_JOYSTICKS
   switch (lpEvent->type)
   {
   case SDL_JOYAXISMOTION:
      //
      // Moved an axis on joystick
      //
      switch (lpEvent->jaxis.axis)
      {
      case 0:
         //
         // X axis
         //
         if (lpEvent->jaxis.value > 20000)
         {
            if (g_InputState.dir != kDirEast)
            {
               g_InputState.dwKeyPress |= kKeyRight;
            }
            g_InputState.prevdir = g_InputState.dir;
            g_InputState.dir = kDirEast;
         }
         else if (lpEvent->jaxis.value < -20000)
         {
            if (g_InputState.dir != kDirWest)
            {
               g_InputState.dwKeyPress |= kKeyLeft;
            }
            g_InputState.prevdir = g_InputState.dir;
            g_InputState.dir = kDirWest;
         }
         else
         {
            if (g_InputState.prevdir != kDirEast &&
               g_InputState.prevdir != kDirWest)
            {
               g_InputState.dir = g_InputState.prevdir;
            }
            g_InputState.prevdir = kDirUnknown;
         }
         break;

      case 1:
         //
         // Y axis
         //
         if (lpEvent->jaxis.value > 20000)
         {
            if (g_InputState.dir != kDirSouth)
            {
               g_InputState.dwKeyPress |= kKeyDown;
            }
            g_InputState.prevdir = g_InputState.dir;
            g_InputState.dir = kDirSouth;
         }
         else if (lpEvent->jaxis.value < -20000)
         {
            if (g_InputState.dir != kDirNorth)
            {
               g_InputState.dwKeyPress |= kKeyUp;
            }
            g_InputState.prevdir = g_InputState.dir;
            g_InputState.dir = kDirNorth;
         }
         else
         {
            if (g_InputState.prevdir != kDirNorth &&
               g_InputState.prevdir != kDirSouth)
            {
               g_InputState.dir = g_InputState.prevdir;
            }
            g_InputState.prevdir = kDirUnknown;
         }
         break;
      }
      break;

   case SDL_JOYBUTTONDOWN:
      //
      // Pressed the joystick button
      //
      switch (lpEvent->jbutton.button & 1)
      {
      case 0:
         g_InputState.dwKeyPress |= kKeyMenu;
         break;

      case 1:
         g_InputState.dwKeyPress |= kKeySearch;
         break;
      }
      break;
   }
#endif
}

#if PAL_HAS_TOUCH

#define  TOUCH_NONE     0
#define    TOUCH_UP      1
#define    TOUCH_DOWN      2
#define    TOUCH_LEFT      3
#define    TOUCH_RIGHT   4
#define    TOUCH_BUTTON1   5
#define    TOUCH_BUTTON2   6
#define  TOUCH_BUTTON3  7
#define  TOUCH_BUTTON4  8

static float gfTouchXMin = 0.0f;
static float gfTouchXMax = 1.0f;
static float gfTouchYMin = 0.0f;
static float gfTouchYMax = 1.0f;

VOID
PAL_SetTouchBounds(
   DWORD dwScreenWidth,
   DWORD dwScreenHeight,
   SDL_Rect renderRect
)
{
   gfTouchXMin = (float)renderRect.x / dwScreenWidth;
   gfTouchXMax = (float)(renderRect.x + renderRect.w) / dwScreenWidth;
   gfTouchYMin = (float)renderRect.y / dwScreenHeight;
   gfTouchYMax = (float)(renderRect.y + renderRect.h) / dwScreenHeight;
}

static int
PAL_GetTouchArea(
   float X,
   float Y
)
{
   if (X < gfTouchXMin || X > gfTouchXMax || Y < 0.5f || Y > gfTouchYMax)
   {
      //
      // Upper area or cropped area
      //
      return TOUCH_NONE;
   }
   else
   {
      X = (X - gfTouchXMin) / (gfTouchXMax - gfTouchXMin);
	  Y = (Y - gfTouchYMin) / (gfTouchYMax - gfTouchYMin);
   }

   if (X < 1.0f / 3)
   {
      if (Y - 0.5f < (1.0f / 6 - fabsf(X - 1.0f / 3 / 2)) * (0.5f / (1.0f / 3)))
      {
         return TOUCH_UP;
      }
      else if (Y - 0.75f > fabsf(X - 1.0f / 3 / 2) * (0.5f / (1.0f / 3)))
      {
         return TOUCH_DOWN;
      }
      else if (X < 1.0f / 3 / 2 && fabsf(Y - 0.75f) < 0.25f - X * (0.5f / (1.0f / 3)))
      {
         return TOUCH_LEFT;
      }
      else
      {
         return TOUCH_RIGHT;
      }
   }
   else if (X > 1.0f - 1.0f / 3)
   {
      if (X < 1.0f - (1.0f / 3 / 2))
      {
         if (Y < 0.75f)
         {
            return TOUCH_BUTTON1;
         }
         else
         {
            return TOUCH_BUTTON3;
         }
      }
      else
      {
         if (Y < 0.75f)
         {
            return TOUCH_BUTTON2;
         }
         else
         {
            return TOUCH_BUTTON4;
         }
      }
   }
   else 
   {
      return TOUCH_NONE;
   }
}

static VOID
PAL_SetTouchAction(
  int area
)
{
   switch (area)
   {
   case TOUCH_UP:
      g_InputState.dir = kDirNorth;
      g_InputState.dwKeyPress |= kKeyUp;
      break;

   case TOUCH_DOWN:
      g_InputState.dir = kDirSouth;
      g_InputState.dwKeyPress |= kKeyDown;
      break;

   case TOUCH_LEFT:
      g_InputState.dir = kDirWest;
      g_InputState.dwKeyPress |= kKeyLeft;
      break;

   case TOUCH_RIGHT:
      g_InputState.dir = kDirEast;
      g_InputState.dwKeyPress |= kKeyRight;
      break;

   case TOUCH_BUTTON1:
      if (gpGlobals->fInBattle)
      {
         g_InputState.dwKeyPress |= kKeyRepeat;
      }
      else
      {
         g_InputState.dwKeyPress |= kKeyForce;
      }
      break;

   case TOUCH_BUTTON2:
      g_InputState.dwKeyPress |= kKeyMenu;
      break;

   case TOUCH_BUTTON3:
      g_InputState.dwKeyPress |= kKeyUseItem;
      break;

   case TOUCH_BUTTON4:
      g_InputState.dwKeyPress |= kKeySearch;
      break;
   }
}

static VOID
PAL_UnsetTouchAction(
  int area
)
{
   switch (area)
   {
   case TOUCH_UP:
   case TOUCH_DOWN:
   case TOUCH_LEFT:
   case TOUCH_RIGHT:
      g_InputState.dir = kDirUnknown;
      break;
   }
}

#endif

static VOID
PAL_TouchEventFilter(
   const SDL_Event *lpEvent
)
/*++
  Purpose:

    Handle touch events.

  Parameters:

    [IN]  lpEvent - pointer to the event.

  Return value:

    None.

--*/
{
#if PAL_HAS_TOUCH
   static SDL_TouchID finger1 = -1, finger2 = -1;
   static int prev_touch1 = TOUCH_NONE;
   static int prev_touch2 = TOUCH_NONE;

   switch (lpEvent->type)
   {
   case SDL_FINGERDOWN:
     if (finger1 == -1)
     {
        int area = PAL_GetTouchArea(lpEvent->tfinger.x, lpEvent->tfinger.y);

        finger1 = lpEvent->tfinger.fingerId;
        prev_touch1 = area;
        PAL_SetTouchAction(area);
     }
     else if (finger2 == -1)
     {
        int area = PAL_GetTouchArea(lpEvent->tfinger.x, lpEvent->tfinger.y);

        finger2 = lpEvent->tfinger.fingerId;
        prev_touch2 = area;
        PAL_SetTouchAction(area);
     }
     break;

   case SDL_FINGERUP:
     if (lpEvent->tfinger.fingerId == finger1)
     {
        PAL_UnsetTouchAction(prev_touch1);
        finger1 = -1;
        prev_touch1 = TOUCH_NONE;
     }
     else if (lpEvent->tfinger.fingerId == finger2)
     {
        PAL_UnsetTouchAction(prev_touch2);
        finger2 = -1;
        prev_touch2 = TOUCH_NONE;
     }
     break;

   case SDL_FINGERMOTION:
      if (lpEvent->tfinger.fingerId == finger1)
      {
         int area = PAL_GetTouchArea(lpEvent->tfinger.x, lpEvent->tfinger.y);
         if (prev_touch1 != area && area != TOUCH_NONE)
         {
            PAL_UnsetTouchAction(prev_touch1);
            prev_touch1 = area;
            PAL_SetTouchAction(area);
         }
      }
      else if (lpEvent->tfinger.fingerId == finger2)
      {
         int area = PAL_GetTouchArea(lpEvent->tfinger.x, lpEvent->tfinger.y);
         if (prev_touch2 != area && area != TOUCH_NONE)
         {
            PAL_UnsetTouchAction(prev_touch2);
            prev_touch2 = area;
            PAL_SetTouchAction(area);
         }
      }
      break;
   }
#endif
}

static int SDLCALL
PAL_EventFilter(
   const SDL_Event       *lpEvent
)

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
#if SDL_VERSION_ATLEAST(2,0,0)
   case SDL_WINDOWEVENT:
      if (lpEvent->window.event == SDL_WINDOWEVENT_RESIZED)
      {
         //
         // resized the window
         //
         VIDEO_Resize(lpEvent->window.data1, lpEvent->window.data2);
      }
      break;

#ifdef __IOS__
   case SDL_APP_WILLENTERBACKGROUND:
      g_bRenderPaused = TRUE;
      break;

   case SDL_APP_DIDENTERFOREGROUND:
      g_bRenderPaused = FALSE;
      VIDEO_UpdateScreen(NULL);
      break;
#endif

#else
           
   case SDL_VIDEORESIZE:
      //
      // resized the window
      //
      VIDEO_Resize(lpEvent->resize.w, lpEvent->resize.h);
      break;
#endif

   case SDL_QUIT:
      //
      // clicked on the close button of the window. Quit immediately.
      //
      PAL_Shutdown(0);
   }

   PAL_KeyboardEventFilter(lpEvent);
   PAL_MouseEventFilter(lpEvent);
   PAL_JoystickEventFilter(lpEvent);
   PAL_TouchEventFilter(lpEvent);

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
   memset((void *)&g_InputState, 0, sizeof(g_InputState));
   g_InputState.dir = kDirUnknown;
   g_InputState.prevdir = kDirUnknown;

   //
   // Check for joystick
   //
#if PAL_HAS_JOYSTICKS
   if (SDL_NumJoysticks() > 0 && g_fUseJoystick)
   {
      int i;
	  for (i = 0; i < SDL_NumJoysticks(); i++)
      {
         //
         // HACKHACK: applesmc and Android Accelerometer shouldn't be considered as real joysticks
         //
         if (strcmp(SDL_JoystickNameForIndex(i), "applesmc") != 0 && strcmp(SDL_JoystickNameForIndex(i), "Android Accelerometer") != 0)
         {
            g_pJoy = SDL_JoystickOpen(i);
            break;
         }
      }

      if (g_pJoy != NULL)
      {
         SDL_JoystickEventState(SDL_ENABLE);
      }
   }
#endif

#ifdef PAL_ALLOW_KEYREPEAT
   SDL_EnableKeyRepeat(0, 0);
#endif

   input_init_filter();
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
#if PAL_HAS_JOYSTICKS
# if SDL_VERSION_ATLEAST(2,0,0)
   if (g_pJoy != NULL)
   {
      SDL_JoystickClose(g_pJoy);
      g_pJoy = NULL;
   }
# else
   if (SDL_JoystickOpened(0))
   {
      assert(g_pJoy != NULL);
      SDL_JoystickClose(g_pJoy);
      g_pJoy = NULL;
   }
# endif
#endif
   input_shutdown_filter();
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
#if PAL_HAS_NATIVEMIDI
   MIDI_CheckLoop();
#endif
   while (PAL_PollEvent(NULL));
}

int
PAL_PollEvent(
   SDL_Event *event
)
/*++
  Purpose:

    Poll and process one event.

  Parameters:

    [OUT] event - Events polled from SDL.

  Return value:

    Return value of PAL_PollEvent.

--*/
{
   SDL_Event evt;

   int ret = SDL_PollEvent(&evt);
   if (ret != 0 && !input_event_filter(&evt, &g_InputState))
   {
      PAL_EventFilter(&evt);
   }

   if (event != NULL)
   {
      *event = evt;
   }

   return ret;
}

VOID
PAL_RegisterInputFilter(
   void (*init_filter)(),
   int (*event_filter)(const SDL_Event *, volatile PALINPUTSTATE *),
   void (*shutdown_filter)()
)
/*++
  Purpose:

    Register caller-defined input event filter.

  Parameters:

    [IN] init_filter - Filter that will be called inside PAL_InitInput
	[IN] event_filter - Filter that will be called inside PAL_PollEvent, 
	                    return non-zero value from this filter disables
						further internal event processing.
	[IN] shutdown_filter - Filter that will be called inside PAL_ShutdownInput

	Passing NULL to either parameter means the caller does not provide such filter.

  Return value:

    None.

--*/
{
	if (init_filter)
		input_init_filter = init_filter;
	if (event_filter)
		input_event_filter = event_filter;
	if (shutdown_filter)
		input_shutdown_filter = shutdown_filter;
}
