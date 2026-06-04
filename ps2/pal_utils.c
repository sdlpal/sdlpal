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

#include "../main.h"
#include <kernel.h>

#include <iopcontrol.h>
#include <kernel.h>
#include <loadfile.h>
#include <malloc.h>
#include <sbv_patches.h>
#include <sifrpc.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <tamtypes.h>

#include <debug.h>
#include <unistd.h>

#include <ps2_all_drivers.h>

#include "libmtap.h"
#include "libpad.h"
#include <ps2_joystick_driver.h>

static char *padBuf[2][4];
static u32 padConnected[2][4]; // 2 ports, 4 slots
static u32 padOpen[2][4];
static u32 mtapConnected[2];
static u32 maxslot[2];

u32 i;

struct padButtonStatus buttons;
u32 paddata;
u32 old_pad[2][4];
u32 new_pad[2][4];
s32 ret;

void find_controllers() {
  u32 port, slot;
  u32 mtapcon;

  // Look for multitaps and controllers on both ports
  for (port = 0; port < 2; port++) {

    mtapcon = mtapGetConnection(port);

    if ((mtapcon == 1) && (mtapConnected[port] == 0)) {
      printf("Multitap (%i) connected\n", (int)port);
    }

    if ((mtapcon == 0) && (mtapConnected[port] == 1)) {
      printf("Multitap (%i) disconnected(int argc, char **argv)\n", (int)port);
    }

    mtapConnected[port] = mtapcon;

    // Check for multitap
    if (mtapConnected[port] == 1)
      maxslot[port] = 4;
    else
      maxslot[port] = 1;

    // Find any connected controllers
    for (slot = 0; slot < maxslot[port]; slot++) {
      if (padOpen[port][slot] == 0) {
        padOpen[port][slot] = padPortOpen(port, slot, padBuf[port][slot]);
      }

      if (padOpen[port][slot] == 1) {

        if (padGetState(port, slot) == PAD_STATE_STABLE) {
          if (padConnected[port][slot] == 0) {
            printf("Controller (%i,%i) connected\n", (int)port, (int)slot);
          }

          padConnected[port][slot] = 1;
        } else {
          if ((padGetState(port, slot) == PAD_STATE_DISCONN) &&
              (padConnected[port][slot] == 1)) {
            printf("Controller (%i,%i) disconnected\n", (int)port, (int)slot);
            padConnected[port][slot] = 0;
          }
        }
      }
    }

    // Close controllers when multitap is disconnected

    if (mtapConnected[port] == 0) {
      for (slot = 1; slot < 4; slot++) {
        if (padOpen[port][slot] == 1) {
          padPortClose(port, slot);
          padOpen[port][slot] = 0;
        }
      }
    }
  }
}

void init_filter() {}

BOOL UTIL_GetScreenSize(DWORD *pdwScreenWidth, DWORD *pdwScreenHeight) {
  return FALSE;
}

BOOL UTIL_IsAbsolutePath(LPCSTR lpszFileName) { return FALSE; }

void UTIL_LogToScreen(LOGLEVEL _, const char *string, const char *__) {
  printf(string);
}

static int input_event_filter(const SDL_Event *lpEvent,
                              volatile PALINPUTSTATE *state) {
  input_ps2_filter();
  return 1;
}

static int isDir;
static int button;
static int old_button;

int input_ps2_filter() {

  u32 port = 0, slot = 0;

  find_controllers();

  for (port = 0; port < 2; port++) {
    for (slot = 0; slot < maxslot[port]; slot++) {
      if (padOpen[port][slot] && padConnected[port][slot]) {
        ret = padRead(port, slot, &buttons);

        if (ret != 0) {
          button = buttons.btns;
          int changed = (button != old_button);
          old_button = button;

          paddata = 0xffff ^ buttons.btns;

          new_pad[port][slot] = paddata & ~old_pad[port][slot];
          old_pad[port][slot] = paddata;

          if (changed) {
            if (new_pad[port][slot] & PAD_LEFT) {
              g_InputState.prevdir =
                  (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
              g_InputState.dir = kDirWest;
              g_InputState.dwKeyPress = kKeyLeft;
              isDir = 1;
              return 1;
            }
            if (new_pad[port][slot] & PAD_RIGHT) {
              g_InputState.prevdir =
                  (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
              g_InputState.dir = kDirEast;
              g_InputState.dwKeyPress = kKeyRight;
              isDir = 1;
              return 1;
            }
            if (new_pad[port][slot] & PAD_UP) {
              g_InputState.prevdir =
                  (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
              g_InputState.dir = kDirNorth;
              g_InputState.dwKeyPress = kKeyUp;
              isDir = 1;
              return 1;
            }
            if (new_pad[port][slot] & PAD_DOWN) {
              g_InputState.prevdir =
                  (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
              g_InputState.dir = kDirSouth;
              g_InputState.dwKeyPress = kKeyDown;
              isDir = 1;
              return 1;
            }
            isDir = 0;
            if (new_pad[port][slot] & PAD_START) {
              return 1;
            }
            if (new_pad[port][slot] & PAD_SELECT) {
              return 1;
            }
            if (new_pad[port][slot] & PAD_SQUARE) {
              return 1;
            }
            if (new_pad[port][slot] & PAD_TRIANGLE) {
              return 1;
            }
            if (new_pad[port][slot] & PAD_CIRCLE) {
              g_InputState.dwKeyPress = kKeySearch;
              return 1;
            }
            if (new_pad[port][slot] & PAD_CROSS) {
              g_InputState.dwKeyPress = kKeyMenu;
              return 1;
            }

            g_InputState.prevdir =
                (gpGlobals->fInBattle ? kDirUnknown : g_InputState.dir);
            g_InputState.dir = kDirUnknown;
            return 1;
          }
        }
      }
    }
  }
}

INT UTIL_Platform_Init(int argc, char *argv[]) {
  UTIL_LogAddOutputCallback(UTIL_LogToScreen, gConfig.iLogLevel);

  PAL_RegisterInputFilter(init_filter, input_event_filter, NULL);
  gConfig.fEnableAviPlay = 0; // TODO: Fix audio stutering
  gConfig.fLaunchSetting = FALSE;
  gConfig.eOPLCore = OPLCORE_DBINT;
  gConfig.fFullScreen = TRUE;
  gConfig.fKeepAspectRatio = FALSE;
  gConfig.eMIDISynth = SYNTH_TIMIDITY;

  mtapConnected[0] = 0;
  mtapConnected[1] = 0;

  mtapPortOpen(0);
  mtapPortOpen(1);

  for (i = 0; i < 4; i++) {
    padConnected[0][i] = 0;
    padConnected[1][i] = 0;
    padOpen[0][i] = 0;
    padOpen[1][i] = 0;
    old_pad[0][i] = 0;
    old_pad[1][i] = 0;
    new_pad[0][i] = 0;
    new_pad[1][i] = 0;

    padBuf[0][i] = memalign(64, 256);
    padBuf[1][i] = memalign(64, 256);
  }

  return 0;
}

INT UTIL_Platform_Startup(int argc, char *argv[]) 
{ 
    return 0; 
}

VOID UTIL_Platform_Quit(VOID) 
{

}