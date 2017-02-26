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

#ifndef _PALUTILS_H
#define _PALUTILS_H

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef LPBYTE      LPSPRITE, LPBITMAPRLE;
typedef LPCBYTE     LPCSPRITE, LPCBITMAPRLE;

#ifndef PAL_POS_DEFINED
#define PAL_POS_DEFINED
typedef DWORD           PAL_POS;
#endif

#define PAL_XY(x, y)    (PAL_POS)(((((WORD)(y)) << 16) & 0xFFFF0000) | (((WORD)(x)) & 0xFFFF))
#define PAL_X(xy)       (SHORT)((xy) & 0xFFFF)
#define PAL_Y(xy)       (SHORT)(((xy) >> 16) & 0xFFFF)
#define PAL_XY_OFFSET(xy, x, y)    (PAL_POS)(((((INT)(y) << 16) & 0xFFFF0000) + ((xy) & 0xFFFF0000)) | (((INT)(x) & 0xFFFF) + ((xy) & 0xFFFF)))

typedef enum tagPALDIRECTION
{
   kDirSouth = 0,
   kDirWest,
   kDirNorth,
   kDirEast,
   kDirUnknown
} PALDIRECTION, *LPPALDIRECTION;
    
INT
PAL_RLEBlitToSurface(
   LPCBITMAPRLE      lpBitmapRLE,
   SDL_Surface      *lpDstSurface,
   PAL_POS           pos
);

INT
PAL_RLEBlitToSurfaceWithShadow(
   LPCBITMAPRLE      lpBitmapRLE,
   SDL_Surface      *lpDstSurface,
   PAL_POS           pos,
   BOOL              bShadow
);

INT
PAL_RLEBlitWithColorShift(
   LPCBITMAPRLE      lpBitmapRLE,
   SDL_Surface      *lpDstSurface,
   PAL_POS           pos,
   INT               iColorShift
);

INT
PAL_RLEBlitMonoColor(
   LPCBITMAPRLE      lpBitmapRLE,
   SDL_Surface      *lpDstSurface,
   PAL_POS           pos,
   BYTE              bColor,
   INT               iColorShift
);

INT
PAL_FBPBlitToSurface(
   LPBYTE            lpBitmapFBP,
   SDL_Surface      *lpDstSurface
);

INT
PAL_RLEGetWidth(
   LPCBITMAPRLE      lpBitmapRLE
);

INT
PAL_RLEGetHeight(
   LPCBITMAPRLE      lpBitmapRLE
);

WORD
PAL_SpriteGetNumFrames(
   LPCSPRITE       lpSprite
);

LPCBITMAPRLE
PAL_SpriteGetFrame(
   LPCSPRITE       lpSprite,
   INT             iFrameNum
);

INT
PAL_MKFGetChunkCount(
   FILE *fp
);

INT
PAL_MKFGetChunkSize(
   UINT    uiChunkNum,
   FILE   *fp
);

INT
PAL_MKFReadChunk(
   LPBYTE          lpBuffer,
   UINT            uiBufferSize,
   UINT            uiChunkNum,
   FILE           *fp
);

INT
PAL_MKFGetDecompressedSize(
   UINT    uiChunkNum,
   FILE   *fp
);

INT
PAL_MKFDecompressChunk(
   LPBYTE          lpBuffer,
   UINT            uiBufferSize,
   UINT            uiChunkNum,
   FILE           *fp
);

// From yj1.c:
extern INT
(*Decompress)(
   LPCVOID      Source,
   LPVOID       Destination,
   INT          DestSize
);

INT
YJ1_Decompress(
   LPCVOID      Source,
   LPVOID       Destination,
   INT          DestSize
);

INT
YJ2_Decompress(
   LPCVOID      Source,
   LPVOID       Destination,
   INT          DestSize
);

#define PAL_DelayUntil(t) \
   PAL_ProcessEvent(); \
   while (!SDL_TICKS_PASSED(SDL_GetTicks(), (t))) \
   { \
      PAL_ProcessEvent(); \
      SDL_Delay(1); \
   }

#ifdef __cplusplus
}
#endif

#endif // _PALUTILS_H
