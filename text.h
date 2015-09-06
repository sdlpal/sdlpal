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
// Modified by Lou Yihua <louyihua@21cn.com> with Unicode support, 2015
//

#ifndef _TEXT_H
#define _TEXT_H

typedef enum tagDIALOGPOSITION
{
   kDialogUpper       = 0,
   kDialogCenter,
   kDialogLower,
   kDialogCenterWindow
} DIALOGLOCATION;

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
   int        iOrder
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
   CODEPAGE      iCodePage
);

#endif
