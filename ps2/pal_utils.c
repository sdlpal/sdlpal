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

#include <stdio.h>

BOOL UTIL_GetScreenSize(DWORD *pdwScreenWidth, DWORD *pdwScreenHeight) {
  return FALSE;
}

BOOL UTIL_IsAbsolutePath(LPCSTR lpszFileName) { return FALSE; }

void UTIL_LogToScreen(LOGLEVEL _, const char *string, const char *__) {
  printf(string);
}

INT UTIL_Platform_Init(int argc, char *argv[]) {
  UTIL_LogAddOutputCallback(UTIL_LogToScreen, gConfig.iLogLevel);

  gConfig.fLaunchSetting = FALSE;
  gConfig.iResampleQuality = 2;
  gConfig.eOPLCore = OPLCORE_DBINT;
  gConfig.fFullScreen = TRUE;
  gConfig.fEnableJoyStick = TRUE;
  gConfig.eMIDISynth = SYNTH_TIMIDITY;
  gConfig.wAudioBufferSize = 512;

  return 0;
}

INT UTIL_Platform_Startup(int argc, char *argv[]) 
{ 
    return 0; 
}

VOID UTIL_Platform_Quit(VOID) 
{

}