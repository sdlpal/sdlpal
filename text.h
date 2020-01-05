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

#ifndef _TEXT_H
#define _TEXT_H

#include "common.h"

typedef enum tagDIALOGPOSITION
{
   kDialogUpper       = 0,
   kDialogCenter,
   kDialogLower,
   kDialogCenterWindow
} DIALOGLOCATION;

typedef enum tagFONTFLAVOR
{
   kFontFlavorAuto     = 0,
   kFontFlavorUnifont,
   kFontFlavorSimpChin,
   kFontFlavorTradChin,
   kFontFlavorJapanese,
} FONTFLAVOR;

PAL_C_LINKAGE_BEGIN

typedef struct tagTEXTLIB
{
    LPWSTR         *lpWordBuf;
    LPWSTR         *lpMsgBuf;
    int           ***lpIndexBuf; 
	
	int            *indexMaxCounter;
	// The variable indexMaxCounter stores the value of (item->indexEnd - item->index), 
	// which means the span between eid and sid. 
		
    BOOL            fUseISOFont;
	int             iFontFlavor;

    int             nWords;
    int             nMsgs;
    int             nIndices;

    int             nCurrentDialogLine;
    BYTE            bCurrentFontColor;
    PAL_POS         posIcon;
    PAL_POS         posDialogTitle;
    PAL_POS         posDialogText;
    BYTE            bDialogPosition;
    BYTE            bIcon;
    int             iDelayTime;
    INT             iDialogShadow;
    BOOL            fUserSkip;
    BOOL            fPlayingRNG;

    BYTE            bufDialogIcons[282];
} TEXTLIB, *LPTEXTLIB;

extern TEXTLIB         g_TextLib;

extern LPWSTR g_rcCredits[12];

INT
PAL_InitText(
   VOID
);

VOID
PAL_FreeText(
   VOID
);

LPCWSTR
PAL_GetWord(
   int        iNumWord
);

LPCWSTR
PAL_GetMsg(
   int        iNumMsg
);

int
PAL_GetMsgNum(
   int        iIndex,
   int        iSpan,
   int        iOrder
);

LPWSTR
PAL_UnescapeText(
   LPCWSTR    lpszText
);

VOID
PAL_DrawText(
   LPCWSTR    lpszText,
   PAL_POS    pos,
   BYTE       bColor,
   BOOL       fShadow,
   BOOL       fUpdate,
   BOOL       fUse8x8Font
);

VOID
PAL_DrawTextUnescape(
   LPCWSTR    lpszText,
   PAL_POS    pos,
   BYTE       bColor,
   BOOL       fShadow,
   BOOL       fUpdate,
   BOOL       fUse8x8Font,
   BOOL       fUnescape
);

VOID
PAL_DialogSetDelayTime(
   INT          iDelayTime
);

VOID
PAL_StartDialog(
   BYTE         bDialogLocation,
   BYTE         bFontColor,
   INT          iNumCharFace,
   BOOL         fPlayingRNG
);

VOID
PAL_StartDialogWithOffset(
   BYTE         bDialogLocation,
   BYTE         bFontColor,
   INT          iNumCharFace,
   BOOL         fPlayingRNG,
   INT          xOff,
   INT          yOff
);

int
TEXT_DisplayText(
   LPCWSTR        lpszText,
   int            x,
   int            y,
   BOOL           isDialog
);

VOID
PAL_ShowDialogText(
   LPCWSTR    lpszText
);

VOID
PAL_ClearDialog(
   BOOL         fWaitForKey
);

VOID
PAL_EndDialog(
   VOID
);

BOOL
PAL_IsInDialog(
   VOID
);

BOOL
PAL_DialogIsPlayingRNG(
   VOID
);

INT
PAL_MultiByteToWideChar(
   LPCSTR        mbs,
   int           mbslength,
   LPWSTR        wcs,
   int           wcslength
);

INT
PAL_MultiByteToWideCharCP(
	CODEPAGE      cp,
	LPCSTR        mbs,
	int           mbslength,
	LPWSTR        wcs,
	int           wcslength
	);

WCHAR
PAL_GetInvalidChar(
   CODEPAGE      uCodePage
);

CODEPAGE
PAL_GetCodePage(
	void
);

void
PAL_SetCodePage(
	CODEPAGE    uCodePage
);

CODEPAGE
PAL_DetectCodePageForString(
	const char *   text,
	int            text_len,
	CODEPAGE       default_cp,
	int *          probability
);

INT
PAL_swprintf(
	LPWSTR buffer,
	size_t count,
	LPCWSTR format,
	...
);

PAL_C_LINKAGE_END

#endif
