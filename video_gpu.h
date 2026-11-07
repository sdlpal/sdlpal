/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
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

#ifndef VIDEO_GPU_H
#define VIDEO_GPU_H

#include "common.h"

// How VIDEO_GPU_DrawRLE() and VIDEO_GPU_DrawGlyph() put pixels on the surface.
// These values are shared with shaders/gpu/sprite.frag.
enum
{
   kGPUDrawNormal = 0,
   kGPUDrawColorShift = 1,
   kGPUDrawMonoColor = 2,
   kGPUDrawGlyph = 3,
   kGPUDrawShadow = 4,
   kGPUDrawFill = 5
};

#if PAL_HAS_SDL_GPU

PAL_C_LINKAGE_BEGIN

BOOL
VIDEO_GPU_Init(
   VOID
);

BOOL
VIDEO_GPU_ClaimWindow(
   SDL_Window     *pWindow
);

VOID
VIDEO_GPU_Shutdown(
   VOID
);

VOID
VIDEO_GPU_SetOverlay(
   SDL_Surface    *pOverlay,
   BYTE            bAlpha
);

//
// Surfaces. An 8-bit surface lives only on the GPU, and its pixels member is NULL unless
// every operation is being checked against its software version (see the comment at the
// top of video_gpu.c). The CPU exchanges images with it through these functions.
//
SDL_Surface *
VIDEO_GPU_CreateSurface(
   int             iWidth,
   int             iHeight
);

VOID
VIDEO_GPU_UnregisterSurface(
   SDL_Surface    *pSurface
);

BOOL
VIDEO_GPU_IsTarget(
   SDL_Surface    *pSurface
);

BOOL
VIDEO_GPU_ReadSurface(
   SDL_Surface    *pSurface,
   BYTE           *pDst,
   int             iPitch
);

BOOL
VIDEO_GPU_WriteImage(
   SDL_Surface    *pSurface,
   const BYTE     *pIndices
);

BYTE *
VIDEO_GPU_BeginPartialImage(
   SDL_Surface    *pSurface
);

VOID
VIDEO_GPU_EndPartialImage(
   SDL_Surface    *pSurface
);

//
// Drawing. These all return TRUE if the work is done, and FALSE if the caller still has
// to do it on the CPU: the surface is not on the GPU, or every operation is being checked
// against its software version (see the comment at the top of video_gpu.c).
//
BOOL
VIDEO_GPU_DrawRLE(
   LPCBITMAPRLE    lpBitmapRLE,
   SDL_Surface    *pDst,
   PAL_POS         pos,
   INT             iMode,
   INT             iColorShift,
   BYTE            bColor
);

BOOL
VIDEO_GPU_IsGlyphCached(
   DWORD           dwKey,
   INT             iWidth,
   INT             iHeight
);

BOOL
VIDEO_GPU_DrawGlyph(
   SDL_Surface    *pDst,
   DWORD           dwKey,
   const BYTE     *pMask,
   INT             iWidth,
   INT             iHeight,
   INT             x,
   INT             y,
   INT             iSrcX,
   INT             iSrcWidth,
   BYTE            bColor
);

BOOL
VIDEO_GPU_Fill(
   SDL_Surface    *pDst,
   const SDL_Rect *pRect,
   BYTE            bColor
);

BOOL
VIDEO_GPU_Blit(
   SDL_Surface    *pSrc,
   const SDL_Rect *pSrcRect,
   SDL_Surface    *pDst,
   const SDL_Rect *pDstRect
);

BOOL
VIDEO_GPU_ShiftBackground(
   SDL_Surface    *pSrc,
   SDL_Surface    *pDst,
   INT             iShift
);

BOOL
VIDEO_GPU_ApplyWave(
   SDL_Surface    *pSurface,
   const int       rgWave[32],
   INT             iIndex
);

BOOL
VIDEO_GPU_CrossFadeStep(
   SDL_Surface    *pFrom,
   SDL_Surface    *pDst,
   INT             iPhase,
   BOOL            fStep
);

BOOL
VIDEO_GPU_SwitchStep(
   SDL_Surface    *pFrom,
   SDL_Surface    *pDst,
   INT             iPhase
);

BOOL
VIDEO_GPU_ReplaceIndex(
   SDL_Surface    *pSurface,
   BYTE            bFrom,
   BYTE            bTo
);

//
// Palette.
//
BOOL
VIDEO_GPU_OnSetPalette(
   const SDL_Color *rgPalette
);

BOOL
VIDEO_GPU_SetPaletteFade(
   INT              iMode,
   const SDL_Color *rgPalette,
   const SDL_Color *rgPaletteTo,
   INT              iStep,
   BYTE             bColor
);

VOID
VIDEO_GPU_ResolvePalette(
   SDL_Color       *rgPalette
);

//
// Presentation.
//
VOID
VIDEO_GPU_WriteScreen(
   SDL_Surface    *pSource,
   const SDL_Rect *pSourceRect,
   const SDL_Rect *pDestRect
);

VOID
VIDEO_GPU_SetBlackBand(
   const SDL_Rect *pRect
);

VOID
VIDEO_GPU_Present(
   VOID
);

VOID
VIDEO_GPU_PresentSurface(
   SDL_Surface    *pSurface
);

SDL_Rect
VIDEO_GPU_CalcViewport(
   int             iWidth,
   int             iHeight
);

PAL_C_LINKAGE_END

#else

//
// Without SDL_GPU nothing is ever on the GPU, so callers always use their CPU path.
//
#define VIDEO_GPU_CreateSurface(w, h)         NULL
#define VIDEO_GPU_IsTarget(s)                 FALSE
#define VIDEO_GPU_WriteImage(s, p)            FALSE
#define VIDEO_GPU_BeginPartialImage(s)        ((BYTE *)NULL)
#define VIDEO_GPU_EndPartialImage(s)          ((void)0)
#define VIDEO_GPU_DrawRLE(r, s, p, m, c, b)   FALSE
#define VIDEO_GPU_IsGlyphCached(k, w, h)      FALSE
#define VIDEO_GPU_DrawGlyph(s, k, m, w, h, x, y, sx, sw, c) FALSE
#define VIDEO_GPU_Fill(s, r, c)               FALSE
#define VIDEO_GPU_Blit(s, sr, d, dr)          FALSE
#define VIDEO_GPU_ShiftBackground(s, d, n)    FALSE
#define VIDEO_GPU_ApplyWave(s, w, i)          FALSE
#define VIDEO_GPU_CrossFadeStep(f, d, p, s)   FALSE
#define VIDEO_GPU_SwitchStep(f, d, p)         FALSE
#define VIDEO_GPU_ReplaceIndex(s, f, t)       FALSE

#endif

#endif
