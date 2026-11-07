/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2026, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
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

#define TOUCHOVERLAY_ALPHAMOD           120

#define VIDEO_CopySurface(s, sr, t, tr) VIDEO_Blit((s), (sr), (t), (tr))
#define VIDEO_CopyEntireSurface(s, t)   VIDEO_Blit((s), NULL, (t), NULL)
#define VIDEO_BackupScreen(s)           VIDEO_Blit((s), NULL, gpScreenBak, NULL)
#define VIDEO_RestoreScreen(t)          VIDEO_Blit(gpScreenBak, NULL, (t), NULL)

// The palette fades of palette.c. These values are shared with shaders/gpu/palette.frag.
enum
{
   kPaletteFadeNone = 0,
   kPaletteFadeBrightness = 1,
   kPaletteFadeBlend = 2,
   kPaletteFadeFromColor = 3,
   kPaletteFadeToColor = 4,
   kPaletteFadeToRed = 5
};

PAL_C_LINKAGE_BEGIN

extern SDL_Surface *gpScreen;
extern SDL_Surface *gpScreenBak;
extern volatile BOOL g_bRenderPaused;
extern volatile BOOL g_bAppInBackground;

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

BOOL
VIDEO_SetPaletteFade(
   INT              iMode,
   const SDL_Color *rgPalette,
   const SDL_Color *rgPaletteTo,
   INT              iStep,
   BYTE             bColor
);

VOID
VIDEO_Blit(
   SDL_Surface    *lpSrc,
   const SDL_Rect *lpSrcRect,
   SDL_Surface    *lpDst,
   const SDL_Rect *lpDstRect
);

VOID
VIDEO_FillRect(
   SDL_Surface    *lpDst,
   const SDL_Rect *lpRect,
   BYTE            bColor
);

VOID
VIDEO_CrossFadeStep(
   SDL_Surface    *lpFrom,
   SDL_Surface    *lpTo,
   INT             iPhase,
   BOOL            fStep
);

VOID
VIDEO_FreeSurface(
   SDL_Surface    *lpSurface
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

#if SDL_VERSION_ATLEAST(3,0,0)
SDL_ScaleMode
VIDEO_GetScaleMode(
	VOID
);
#endif

PAL_C_LINKAGE_END

#endif
