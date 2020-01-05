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
#include <math.h>

volatile PALINPUTSTATE   g_InputState;
#if PAL_HAS_JOYSTICKS
static SDL_Joystick     *g_pJoy = NULL;
#endif

#if !SDL_VERSION_ATLEAST(2,0,0)
# define SDLK_KP_1     SDLK_KP1
# define SDLK_KP_2     SDLK_KP2
# define SDLK_KP_3     SDLK_KP3
# define SDLK_KP_4     SDLK_KP4
# define SDLK_KP_5     SDLK_KP5
# define SDLK_KP_6     SDLK_KP6
# define SDLK_KP_7     SDLK_KP7
# define SDLK_KP_8     SDLK_KP8
# define SDLK_KP_9     SDLK_KP9
# define SDLK_KP_0     SDLK_KP0

# define SDL_JoystickNameForIndex    SDL_JoystickName
# define SDL_GetKeyboardState        SDL_GetKeyState
# define SDL_GetScancodeFromKey(x)   (x)
#endif

BOOL                     g_fUseJoystick = TRUE;

static void _default_init_filter() {}
static int _default_input_event_filter(const SDL_Event *event, volatile PALINPUTSTATE *state) { return 0; }
static void _default_input_shutdown_filter() {}

static void (*input_init_filter)() = _default_init_filter;
static int (*input_event_filter)(const SDL_Event *, volatile PALINPUTSTATE *) = _default_input_event_filter;
static void (*input_shutdown_filter)() = _default_input_shutdown_filter;

static const int g_KeyMap[][2] = {
   { SDLK_UP,        kKeyUp },
   { SDLK_KP_8,      kKeyUp },
   { SDLK_DOWN,      kKeyDown },
   { SDLK_KP_2,      kKeyDown },
   { SDLK_LEFT,      kKeyLeft },
   { SDLK_KP_4,      kKeyLeft },
   { SDLK_RIGHT,     kKeyRight },
   { SDLK_KP_6,      kKeyRight },
   { SDLK_ESCAPE,    kKeyMenu },
   { SDLK_INSERT,    kKeyMenu },
   { SDLK_LALT,      kKeyMenu },
   { SDLK_RALT,      kKeyMenu },
   { SDLK_KP_0,      kKeyMenu },
   { SDLK_RETURN,    kKeySearch },
   { SDLK_SPACE,     kKeySearch },
   { SDLK_KP_ENTER,  kKeySearch },
   { SDLK_LCTRL,     kKeySearch },
   { SDLK_PAGEUP,    kKeyPgUp },
   { SDLK_KP_9,      kKeyPgUp },
   { SDLK_PAGEDOWN,  kKeyPgDn },
   { SDLK_KP_3,      kKeyPgDn },
   { SDLK_HOME,      kKeyHome },
   { SDLK_KP_7,      kKeyHome },
   { SDLK_END,       kKeyEnd },
   { SDLK_KP_1,      kKeyEnd },
   { SDLK_r,         kKeyRepeat },
   { SDLK_a,         kKeyAuto },
   { SDLK_d,         kKeyDefend },
   { SDLK_e,         kKeyUseItem },
   { SDLK_w,         kKeyThrowItem },
   { SDLK_q,         kKeyFlee },
   { SDLK_f,         kKeyForce },
   { SDLK_s,         kKeyStatus }
};

static VOID
PAL_KeyDown(
   INT         key,
   BOOL        fRepeat
)
/*++
  Purpose:

    Called when user pressed a key.

  Parameters:

    [IN]  key - keycode of the pressed key.

  Return value:

    None.

--*/
{
   switch (key)
   {
   case kKeyUp:
      if (g_InputState.dir != kDirNorth && !fRepeat)
      {
         g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
         g_InputState.dir = kDirNorth;
      }
      g_InputState.dwKeyPress |= kKeyUp;
      break;

   case kKeyDown:
      if (g_InputState.dir != kDirSouth && !fRepeat)
      {
         g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
         g_InputState.dir = kDirSouth;
      }
      g_InputState.dwKeyPress |= kKeyDown;
      break;

   case kKeyLeft:
      if (g_InputState.dir != kDirWest && !fRepeat)
      {
         g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
         g_InputState.dir = kDirWest;
      }
      g_InputState.dwKeyPress |= kKeyLeft;
      break;

   case kKeyRight:
      if (g_InputState.dir != kDirEast && !fRepeat)
      {
         g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
         g_InputState.dir = kDirEast;
      }
      g_InputState.dwKeyPress |= kKeyRight;
      break;

   default:
      g_InputState.dwKeyPress |= key;
      break;
   }
}

static VOID
PAL_KeyUp(
   INT         key
)
/*++
  Purpose:

    Called when user released a key.

  Parameters:

    [IN]  key - keycode of the released key.

  Return value:

    None.

--*/
{
   switch (key)
   {
   case kKeyUp:
      if (g_InputState.dir == kDirNorth)
      {
         g_InputState.dir = g_InputState.prevdir;
      }
      g_InputState.prevdir = kDirUnknown;
      break;

   case kKeyDown:
      if (g_InputState.dir == kDirSouth)
      {
         g_InputState.dir = g_InputState.prevdir;
      }
      g_InputState.prevdir = kDirUnknown;
      break;

   case kKeyLeft:
      if (g_InputState.dir == kDirWest)
      {
         g_InputState.dir = g_InputState.prevdir;
      }
      g_InputState.prevdir = kDirUnknown;
      break;

   case kKeyRight:
      if (g_InputState.dir == kDirEast)
      {
         g_InputState.dir = g_InputState.prevdir;
      }
      g_InputState.prevdir = kDirUnknown;
      break;

   default:
      break;
   }
}

static VOID
PAL_UpdateKeyboardState(
   VOID
)
/*++
  Purpose:

    Poll & update keyboard state.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   static DWORD   rgdwKeyLastTime[sizeof(g_KeyMap) / sizeof(g_KeyMap[0])] = {0};
   LPCBYTE        keyState = (LPCBYTE)SDL_GetKeyboardState(NULL);
   int            i;
   DWORD          dwCurrentTime = SDL_GetTicks();

   for (i = 0; i < sizeof(g_KeyMap) / sizeof(g_KeyMap[0]); i++)
   {
      if (keyState[SDL_GetScancodeFromKey(g_KeyMap[i][0])])
      {
         if (dwCurrentTime > rgdwKeyLastTime[i])
         {
            PAL_KeyDown(g_KeyMap[i][1], (rgdwKeyLastTime[i] != 0));
            if (gConfig.fEnableKeyRepeat)
            {
               rgdwKeyLastTime[i] = dwCurrentTime + (rgdwKeyLastTime[i] == 0 ? 200 : 75);
            }
            else
            {
               rgdwKeyLastTime[i] = 0xFFFFFFFF;
            }
         }
      }
      else
      {
         if (rgdwKeyLastTime[i] != 0)
         {
            PAL_KeyUp(g_KeyMap[i][1]);
            rgdwKeyLastTime[i] = 0;
         }
      }
   }
}

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
   if (lpEvent->type == SDL_KEYDOWN)
   {
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
      else if (lpEvent->key.keysym.sym == SDLK_p)
      {
         VIDEO_SaveScreenshot();
      }
#if PAL_HAS_GLSL
      else if (lpEvent->key.keysym.sym == SDLK_z)
      {
         Filter_StepParamSlot(1);
      }
      else if (lpEvent->key.keysym.sym == SDLK_x)
      {
         Filter_StepParamSlot(-1);
      }
      else if (lpEvent->key.keysym.sym == SDLK_COMMA)
      {
         Filter_StepCurrentParam(1);
      }
      else if (lpEvent->key.keysym.sym == SDLK_PERIOD)
      {
         Filter_StepCurrentParam(-1);
      }
#endif
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
      g_InputState.joystickNeedUpdate = TRUE;
      //
      // Moved an axis on joystick
      //
      switch (lpEvent->jaxis.axis)
      {
      case 0:
         //
         // X axis
         //
         if (lpEvent->jaxis.value > 3200)
         {
            g_InputState.axisX = 1;
         }
         else if (lpEvent->jaxis.value < -3200)
         {
            g_InputState.axisX = -1;
         }
         else
         {
            g_InputState.axisX = 0;
         }
         break;

      case 1:
         //
         // Y axis
         //
         if (lpEvent->jaxis.value > 3200)
         {
            g_InputState.axisY = 1;
         }
         else if (lpEvent->jaxis.value < -3200)
         {
            g_InputState.axisY = -1;
         }
         else
         {
            g_InputState.axisY = 0;
         }
         break;
      }
      break;

   case SDL_JOYHATMOTION:
      //
      // Pressed the joystick hat button
      //
      switch (lpEvent->jhat.value)
      {
         case SDL_HAT_LEFT:
         case SDL_HAT_LEFTUP:
            g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
            g_InputState.dir = kDirWest;
            g_InputState.dwKeyPress = kKeyLeft;
            break;

         case SDL_HAT_RIGHT:
         case SDL_HAT_RIGHTDOWN:
            g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
            g_InputState.dir = kDirEast;
            g_InputState.dwKeyPress = kKeyRight;
            break;

         case SDL_HAT_UP:
         case SDL_HAT_RIGHTUP:
            g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
            g_InputState.dir = kDirNorth;
            g_InputState.dwKeyPress = kKeyUp;
            break;

         case SDL_HAT_DOWN:
         case SDL_HAT_LEFTDOWN:
            g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
            g_InputState.dir = kDirSouth;
            g_InputState.dwKeyPress = kKeyDown;
            break;

         case SDL_HAT_CENTERED:
            g_InputState.prevdir = (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
            g_InputState.dir = kDirUnknown;
            g_InputState.dwKeyPress = kKeyNone;
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

#if PAL_HAS_JOYSTICKS

static VOID
PAL_UpdateJoyStickState(
VOID
)
/*++
 Purpose:
 
 Poll & update joystick state.
 
 Parameters:
 
 None.
 
 Return value:
 
 None.
 
 --*/
{
   if( g_InputState.axisX == 1 && g_InputState.axisY >= 0 )
   {
      g_InputState.prevdir = g_InputState.dir;
      g_InputState.dir = kDirEast;
      g_InputState.dwKeyPress |= kKeyRight;
   }
   else if( g_InputState.axisX == -1 && g_InputState.axisY <= 0 )
   {
      g_InputState.prevdir = g_InputState.dir;
      g_InputState.dir = kDirWest;
      g_InputState.dwKeyPress |= kKeyLeft;
   }
   else if( g_InputState.axisY == 1 && g_InputState.axisX <= 0 )
   {
      g_InputState.prevdir = g_InputState.dir;
      g_InputState.dir = kDirSouth;
      g_InputState.dwKeyPress |= kKeyDown;
   }
   else if( g_InputState.axisY == -1 && g_InputState.axisX >= 0 )
   {
      g_InputState.prevdir = g_InputState.dir;
      g_InputState.dir = kDirNorth;
      g_InputState.dwKeyPress |= kKeyUp;
   }
   else
   {
      g_InputState.prevdir = g_InputState.dir;
      g_InputState.dir = kDirUnknown;
      if(!input_event_filter)
         g_InputState.dwKeyPress = kKeyNone;
   }
}

#endif

#if PAL_HAS_TOUCH

#define  TOUCH_NONE    0
#define  TOUCH_UP      1
#define  TOUCH_DOWN    2
#define  TOUCH_LEFT    3
#define  TOUCH_RIGHT   4
#define  TOUCH_BUTTON1 5
#define  TOUCH_BUTTON2 6
#define  TOUCH_BUTTON3 7
#define  TOUCH_BUTTON4 8

static float gfTouchXMin = 0.0f;
static float gfTouchXMax = 1.0f;
static float gfTouchYMin = 0.0f;
static float gfTouchYMax = 1.0f;

static SDL_TouchID gFinger1 = -1, gFinger2 = -1;
static DWORD g_dwFinger1Time = 0, g_dwFinger2Time = 0;
static int g_iPrevTouch1 = TOUCH_NONE;
static int g_iPrevTouch2 = TOUCH_NONE;

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
      g_InputState.dwKeyPress |= kKeyForce;
      break;

   case TOUCH_BUTTON2:
      g_InputState.dwKeyPress |= kKeyMenu;
      break;

   case TOUCH_BUTTON3:
      if (gpGlobals->fInBattle)
      {
         g_InputState.dwKeyPress |= kKeyRepeat;
      }
      else
      {
         g_InputState.dwKeyPress |= kKeyUseItem;
      }
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

static VOID
PAL_TouchRepeatCheck(
   VOID
)
{
   if (!gConfig.fEnableKeyRepeat)
   {
      return;
   }
   if (gFinger1 != -1 && SDL_GetTicks() > g_dwFinger1Time)
   {
      PAL_UnsetTouchAction(g_iPrevTouch1);
      PAL_SetTouchAction(g_iPrevTouch1);
      g_dwFinger1Time = SDL_GetTicks() + 120;
   }

   if (gFinger2 != -1 && SDL_GetTicks() > g_dwFinger2Time)
   {
      PAL_UnsetTouchAction(g_iPrevTouch2);
      PAL_SetTouchAction(g_iPrevTouch2);
      g_dwFinger2Time = SDL_GetTicks() + 120;
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
   switch (lpEvent->type)
   {
   case SDL_FINGERDOWN:
     if (gFinger1 == -1)
     {
        int area = PAL_GetTouchArea(lpEvent->tfinger.x, lpEvent->tfinger.y);
        gFinger1 = lpEvent->tfinger.fingerId;
        g_iPrevTouch1 = area;
        PAL_SetTouchAction(area);
        g_dwFinger1Time = SDL_GetTicks() + 500;
     }
     else if (gFinger2 == -1)
     {
        int area = PAL_GetTouchArea(lpEvent->tfinger.x, lpEvent->tfinger.y);
        gFinger2 = lpEvent->tfinger.fingerId;
        g_iPrevTouch2 = area;
        PAL_SetTouchAction(area);
        g_dwFinger2Time = SDL_GetTicks() + 500;
     }
     break;

   case SDL_FINGERUP:
     if (lpEvent->tfinger.fingerId == gFinger1)
     {
        PAL_UnsetTouchAction(g_iPrevTouch1);
        gFinger1 = -1;
        g_iPrevTouch1 = TOUCH_NONE;
     }
     else if (lpEvent->tfinger.fingerId == gFinger2)
     {
        PAL_UnsetTouchAction(g_iPrevTouch2);
        gFinger2 = -1;
        g_iPrevTouch2 = TOUCH_NONE;
     }
     break;

   case SDL_FINGERMOTION:
      if (lpEvent->tfinger.fingerId == gFinger1)
      {
         int area = PAL_GetTouchArea(lpEvent->tfinger.x, lpEvent->tfinger.y);
         if (g_iPrevTouch1 != area && area != TOUCH_NONE)
         {
            PAL_UnsetTouchAction(g_iPrevTouch1);
            g_iPrevTouch1 = area;
            PAL_SetTouchAction(area);
            g_dwFinger1Time = SDL_GetTicks() + 500;
         }
      }
      else if (lpEvent->tfinger.fingerId == gFinger2)
      {
         int area = PAL_GetTouchArea(lpEvent->tfinger.x, lpEvent->tfinger.y);
         if (g_iPrevTouch2 != area && area != TOUCH_NONE)
         {
            PAL_UnsetTouchAction(g_iPrevTouch2);
            g_iPrevTouch2 = area;
            PAL_SetTouchAction(area);
            g_dwFinger2Time = SDL_GetTicks() + 500;
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

   case SDL_APP_WILLENTERBACKGROUND:
      g_bRenderPaused = TRUE;
      break;

   case SDL_APP_DIDENTERFOREGROUND:
      g_bRenderPaused = FALSE;
      VIDEO_UpdateScreen(NULL);
      break;
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
         if (PAL_IS_VALID_JOYSTICK(SDL_JoystickNameForIndex(i)))
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
   if (g_pJoy != NULL)
   {
      SDL_JoystickClose(g_pJoy);
      g_pJoy = NULL;
   }
#endif
   input_shutdown_filter();
}

static int
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
#if PAL_HAS_JOYSTICKS
   g_InputState.joystickNeedUpdate = FALSE;
#endif
   while (PAL_PollEvent(NULL));

   PAL_UpdateKeyboardState();
#if PAL_HAS_JOYSTICKS
   if(g_InputState.joystickNeedUpdate)
      PAL_UpdateJoyStickState();
#endif
#if PAL_HAS_TOUCH
   PAL_TouchRepeatCheck();
#endif
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
