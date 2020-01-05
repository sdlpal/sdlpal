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

#ifndef VIDEO_H
#define VIDEO_H

#include "common.h"

#define VIDEO_CopySurface(s, sr, t, tr) SDL_BlitSurface((s), (sr), (t), (tr))
#define VIDEO_CopyEntireSurface(s, t)   SDL_BlitSurface((s), NULL, (t), NULL)
#define VIDEO_BackupScreen(s)           SDL_BlitSurface((s), NULL, gpScreenBak, NULL)
#define VIDEO_RestoreScreen(t)          SDL_BlitSurface(gpScreenBak, NULL, (t), NULL)
#define VIDEO_FreeSurface(s)            SDL_FreeSurface(s)

PAL_C_LINKAGE_BEGIN

extern SDL_Surface *gpScreen;
extern SDL_Surface *gpScreenBak;
extern volatile BOOL g_bRenderPaused;

#if PAL_HAS_GLSL
void Filter_StepParamSlot(int step);
void Filter_StepCurrentParam(int step);
#endif

INT
VIDEO_Startup(
   VOID
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
VIDEO_ChangeDepth(
   INT             bpp
);

VOID
VIDEO_SaveScreenshot(
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

void
VIDEO_SetWindowTitle(
	const char*   pszTitle
);

SDL_Surface *
VIDEO_DuplicateSurface(
	SDL_Surface    *pSource,
	const SDL_Rect *pRect
);

SDL_Surface *
VIDEO_CreateCompatibleSurface(
	SDL_Surface    *pSource
);

SDL_Surface *
VIDEO_CreateCompatibleSizedSurface(
	SDL_Surface    *pSource,
	const SDL_Rect *pSize
);

void
VIDEO_UpdateSurfacePalette(
	SDL_Surface    *pSurface
);

VOID
VIDEO_DrawSurfaceToScreen(
    SDL_Surface    *pSurface
);

VOID
VIDEO_RenderCopy(
    VOID
);

VOID
VIDEO_SetupTouchArea(
    int window_w,
    int window_h,
    int draw_w,
    int draw_h
);

PAL_C_LINKAGE_END

#endif
