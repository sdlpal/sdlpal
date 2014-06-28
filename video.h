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

#ifndef VIDEO_H
#define VIDEO_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "common.h"

extern SDL_Surface *gpScreen;
extern SDL_Surface *gpScreenBak;
extern volatile BOOL g_bRenderPaused;

INT
#ifdef GEKKO // Rikku2000: Crash on compile, allready define on WIISDK
VIDEO_Init_GEKKO(
#else
VIDEO_Init(
#endif
   WORD             wScreenWidth,
   WORD             wScreenHeight,
   BOOL             fFullScreen
);

VOID
VIDEO_Shutdown(
   VOID
);

VOID
VIDEO_UpdateScreen(
   const SDL_Rect  *lpRect
);

VOID
VIDEO_SetPalette(
   SDL_Color        rgPalette[256]
);

VOID
VIDEO_Resize(
   INT             w,
   INT             h
);

SDL_Color *
VIDEO_GetPalette(
   VOID
);

VOID
VIDEO_ToggleFullscreen(
   VOID
);

VOID
VIDEO_SaveScreenshot(
   VOID
);

VOID
VIDEO_BackupScreen(
   VOID
);

VOID
VIDEO_RestoreScreen(
   VOID
);

VOID
VIDEO_ShakeScreen(
   WORD           wShakeTime,
   WORD           wShakeLevel
);

VOID
VIDEO_SwitchScreen(
   WORD           wSpeed
);

VOID
VIDEO_FadeScreen(
   WORD           wSpeed
);

#if SDL_VERSION_ATLEAST(2,0,0)
//
// For compatibility with SDL2.
//
VOID
SDL_WM_SetCaption(
   LPCSTR         lpszCaption,
   LPVOID         lpReserved
);
#endif

#ifdef __cplusplus
}
#endif

#endif
