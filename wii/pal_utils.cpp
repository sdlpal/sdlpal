/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2018, SDLPAL development team.
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
#include <fat.h>
#include <ogcsys.h>
#include <wiiuse/wpad.h>
#include <gccore.h>
#include <fcntl.h>
#include <network.h>
#include <debug.h>
#include <errno.h>
#include <unistd.h>
#ifdef DEBUG
extern "C"{
# include <net_print.h>
}
#endif

static BOOL isNunChuckConnected(int joystick)
{
	u32 exp_type;

	if (WPAD_Probe(joystick, &exp_type) != 0)
		exp_type = WPAD_EXP_NONE;

	return exp_type == WPAD_EXP_NUNCHUK;
}


static int input_event_filter(const SDL_Event *lpEvent, volatile PALINPUTSTATE *state)
{
    int button, which;
	switch (lpEvent->type)
	{
	case SDL_JOYHATMOTION:
		switch (lpEvent->jhat.value)
		{
		case SDL_HAT_LEFT:
		case SDL_HAT_LEFTUP:
			state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
			state->dir = kDirWest;
			state->dwKeyPress = kKeyLeft;
			break;

		case SDL_HAT_RIGHT:
		case SDL_HAT_RIGHTDOWN:
			state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
			state->dir = kDirEast;
			state->dwKeyPress = kKeyRight;
			break;

		case SDL_HAT_UP:
		case SDL_HAT_RIGHTUP:
			state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
			state->dir = kDirNorth;
			state->dwKeyPress = kKeyUp;
			break;

		case SDL_HAT_DOWN:
		case SDL_HAT_LEFTDOWN:
			state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
			state->dir = kDirSouth;
			state->dwKeyPress = kKeyDown;
			break;
			
		case SDL_HAT_CENTERED:
			state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
			state->dir = kDirUnknown;
			state->dwKeyPress = kKeyNone;
			break;
		}
		return 1;

	case SDL_JOYBUTTONDOWN:
		button = lpEvent->jbutton.button;
		which = lpEvent->jbutton.which;
reswitchbutton:
		switch (button)
		{
		case 1: //wiimote B
			if( !isNunChuckConnected(which) ) { button = 14; goto reswitchbutton; }
		case 9:  //wii classic A
			state->dwKeyPress |= kKeyMenu;
			return 1;

		case 0: //wiimote A
			if( !isNunChuckConnected(which) ) { button = 13; goto reswitchbutton; }
		case 10: //wii classic B
			state->dwKeyPress |= kKeySearch;
			return 1;

		case 4: //wiimote -
			if( isNunChuckConnected(which) ) { button = 14; goto reswitchbutton; }
		case 7: //nunchuk z
		case 11: //wii classic X
			state->dwKeyPress |= kKeyRepeat;
			return 1;

		case 5: //wiimote +
			if( isNunChuckConnected(which) ) { button = 13; goto reswitchbutton; }
		case 8: //nunchuk c
		case 12: //wii classic Y
			state->dwKeyPress |= kKeyForce;
			return 1;

		case 13: //wii classic L1
			state->dwKeyPress |= kKeyAuto;
			return 1;

		case 14: //wii classic R1
			state->dwKeyPress |= kKeyDefend;
			return 1;

		case 15: //wii classic L2
			state->dwKeyPress |= kKeyPgUp;
			return 1;

		case 16: //wii classic R2
			state->dwKeyPress |= kKeyPgDn;
			return 1;

		case 2: //wiimote 1
			if( !isNunChuckConnected(which) ) { button = 9; goto reswitchbutton; }
		case 17: //wii classic select
			state->dwKeyPress |= kKeyUseItem;
			return 1;

		case 3: //wiimote 2
			if( !isNunChuckConnected(which) ) { button = 10; goto reswitchbutton; }
		case 18: //wii classic start
			state->dwKeyPress |= kKeyThrowItem;
			return 1;

		case 6: //wiimote home
		case 19: //wii classic home
			state->dwKeyPress |= kKeyFlee;
			return 1;
		}
	}
	return 0;
}

BOOL
UTIL_GetScreenSize(
	DWORD *pdwScreenWidth,
	DWORD *pdwScreenHeight
)
{
    return (pdwScreenWidth && pdwScreenHeight && *pdwScreenWidth && *pdwScreenHeight);
}

BOOL
UTIL_IsAbsolutePath(
	LPCSTR  lpszFileName
)
{
	return FALSE;
}

int UTIL_Platform_Startup(
	int argc,
	char* argv[]
)
{
#if defined(DEBUG)
   VIDEO_SetBlack(FALSE);
   VIDEO_Flush();
   VIDEO_WaitVSync();
   char localip[16] = {0};
   char gateway[16] = {0};
   char netmask[16] = {0};
   int ret = if_config ( localip, netmask, gateway, TRUE);
   if (ret>=0) {
      printf ("\n network configured, ip: %s, gw: %s, mask %s\n", localip, gateway, netmask);
      net_print_init(NULL,0);
      
      printf("net_print_init() called.\n");
      net_print_string(__FILE__,__LINE__, "initial net_print from %s...\n",localip);

      DEBUG_Init(100,5656);
      printf("after DEBUG_Init()...\n");
      
      printf("Before _break() is called.\n");
      _break();
      printf("After _break() is called.\n");
   }
#endif
	fatInitDefault();
	return 0;
}

INT
UTIL_Platform_Init(
	int argc,
	char* argv[]
)
{
#ifdef DEBUG
	UTIL_LogAddOutputCallback([](LOGLEVEL, const char* str, const char*)->void {
      net_print_string(__FILE__,__LINE__, "%s\n",str);
	}, PAL_DEFAULT_LOGLEVEL);
#endif
	PAL_RegisterInputFilter(NULL, input_event_filter, NULL);
	gConfig.fLaunchSetting = FALSE;
	return 0;
}

VOID
UTIL_Platform_Quit(
	VOID
)
{
}
