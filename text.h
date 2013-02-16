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

#ifndef _TEXT_H
#define _TEXT_H

typedef enum tagDIALOGPOSITION
{
   kDialogUpper       = 0,
   kDialogCenter,
   kDialogLower,
   kDialogCenterWindow
} DIALOGLOCATION;

#define PAL_ADDITIONAL_WORD_FIRST           10000

INT
PAL_InitText(
   VOID
);

VOID
PAL_FreeText(
   VOID
);

LPCSTR
PAL_GetWord(
   WORD       wNumWord
);

LPCSTR
PAL_GetMsg(
   WORD       wNumMsg
);

VOID
PAL_DrawText(
   LPCSTR     lpszText,
   PAL_POS    pos,
   BYTE       bColor,
   BOOL       fShadow,
   BOOL       fUpdate
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
   LPCSTR       szText
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

#endif
