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

#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "common.h"

#define CHUNKNUM_SPRITEUI                  9

#define MENUITEM_COLOR                     0x4F
#define MENUITEM_COLOR_INACTIVE            0x1C
#define MENUITEM_COLOR_CONFIRMED           0x2C
#define MENUITEM_COLOR_SELECTED_INACTIVE   0x1F
#define MENUITEM_COLOR_SELECTED_FIRST      0xF9
#define MENUITEM_COLOR_SELECTED_TOTALNUM   6

#define MENUITEM_COLOR_SELECTED                                    \
   (MENUITEM_COLOR_SELECTED_FIRST +                                \
      SDL_GetTicks() / (600 / MENUITEM_COLOR_SELECTED_TOTALNUM)    \
      % MENUITEM_COLOR_SELECTED_TOTALNUM)

#define MENUITEM_COLOR_EQUIPPEDITEM        0xC8

#define DESCTEXT_COLOR                     0x2E

#ifdef PAL_WIN95
#define MAINMENU_BACKGROUND_FBPNUM         2
#else
#define MAINMENU_BACKGROUND_FBPNUM         60
#endif
#define RIX_NUM_OPENINGMENU                4
#define MAINMENU_LABEL_NEWGAME             7
#define MAINMENU_LABEL_LOADGAME            8

#define LOADMENU_LABEL_SLOT_FIRST          43

#define CONFIRMMENU_LABEL_NO               19
#define CONFIRMMENU_LABEL_YES              20

#define CASH_LABEL                         21

#define SWITCHMENU_LABEL_DISABLE           17
#define SWITCHMENU_LABEL_ENABLE            18

#define GAMEMENU_LABEL_STATUS              3
#define GAMEMENU_LABEL_MAGIC               4
#define GAMEMENU_LABEL_INVENTORY           5
#define GAMEMENU_LABEL_SYSTEM              6

#define SYSMENU_LABEL_SAVE                 11
#define SYSMENU_LABEL_LOAD                 12
#define SYSMENU_LABEL_MUSIC                13
#define SYSMENU_LABEL_SOUND                14
#define SYSMENU_LABEL_QUIT                 15
#define SYSMENU_LABEL_BATTLEMODE           (PAL_ADDITIONAL_WORD_FIRST)

#define BATTLESPEEDMENU_LABEL_1            (PAL_ADDITIONAL_WORD_FIRST + 1)
#define BATTLESPEEDMENU_LABEL_2            (PAL_ADDITIONAL_WORD_FIRST + 2)
#define BATTLESPEEDMENU_LABEL_3            (PAL_ADDITIONAL_WORD_FIRST + 3)
#define BATTLESPEEDMENU_LABEL_4            (PAL_ADDITIONAL_WORD_FIRST + 4)
#define BATTLESPEEDMENU_LABEL_5            (PAL_ADDITIONAL_WORD_FIRST + 5)

#define INVMENU_LABEL_USE                  23
#define INVMENU_LABEL_EQUIP                22

#define STATUS_BACKGROUND_FBPNUM           0
#define STATUS_LABEL_EXP                   2
#define STATUS_LABEL_LEVEL                 48
#define STATUS_LABEL_HP                    49
#define STATUS_LABEL_MP                    50
#define STATUS_LABEL_ATTACKPOWER           51
#define STATUS_LABEL_MAGICPOWER            52
#define STATUS_LABEL_RESISTANCE            53
#define STATUS_LABEL_DEXTERITY             54
#define STATUS_LABEL_FLEERATE              55
#define STATUS_COLOR_EQUIPMENT             0xBE

#define BUYMENU_LABEL_CURRENT              35
#define SELLMENU_LABEL_PRICE               25

#define SPRITENUM_SLASH                    39
#define SPRITENUM_ITEMBOX                  70
#define SPRITENUM_CURSOR_YELLOW            68
#define SPRITENUM_CURSOR                   69
#define SPRITENUM_PLAYERINFOBOX            18
#define SPRITENUM_PLAYERFACE_FIRST         48

#define EQUIPMENU_BACKGROUND_FBPNUM        1

#define ITEMUSEMENU_COLOR_STATLABEL        0xBB

#define BATTLEWIN_GETEXP_LABEL             30
#define BATTLEWIN_BEATENEMY_LABEL          9
#define BATTLEWIN_DOLLAR_LABEL             10
#define BATTLEWIN_LEVELUP_LABEL            32
#define BATTLEWIN_ADDMAGIC_LABEL           33
#define BATTLEWIN_LEVELUP_LABEL_COLOR      0x39
#define SPRITENUM_ARROW                    47

#define BATTLE_LABEL_ESCAPEFAIL            31

typedef struct tagBOX
{
   PAL_POS        pos;
   WORD           wWidth, wHeight;
   SDL_Surface   *lpSavedArea;
} BOX, *LPBOX;

typedef struct tagMENUITEM
{
   WORD          wValue;
   WORD          wNumWord;
   BOOL          fEnabled;
   PAL_POS       pos;
} MENUITEM, *LPMENUITEM;

typedef struct tagOBJECTDESC
{
   WORD                        wObjectID;
   LPSTR                       lpDesc;
   struct tagOBJECTDESC       *next;
} OBJECTDESC, *LPOBJECTDESC;

typedef VOID (*LPITEMCHANGED_CALLBACK)(WORD);

#define MENUITEM_VALUE_CANCELLED      0xFFFF

typedef enum tagNUMCOLOR
{
   kNumColorYellow,
   kNumColorBlue,
   kNumColorCyan
} NUMCOLOR;

typedef enum tagNUMALIGN
{
   kNumAlignLeft,
   kNumAlignMid,
   kNumAlignRight
} NUMALIGN;

INT
PAL_InitUI(
   VOID
);

VOID
PAL_FreeUI(
   VOID
);

LPBOX
PAL_CreateBox(
   PAL_POS        pos,
   INT            nRows,
   INT            nColumns,
   INT            iStyle,
   BOOL           fSaveScreen
);

LPBOX
PAL_CreateSingleLineBox(
   PAL_POS        pos,
   INT            nLen,
   BOOL           fSaveScreen
);

VOID
PAL_DeleteBox(
   LPBOX          lpBox
);

WORD
PAL_ReadMenu(
   LPITEMCHANGED_CALLBACK    lpfnMenuItemChanged,
   LPMENUITEM                rgMenuItem,
   INT                       nMenuItem,
   WORD                      wDefaultItem,
   BYTE                      bLabelColor
);

VOID
PAL_DrawNumber(
   UINT            iNum,
   UINT            nLength,
   PAL_POS         pos,
   NUMCOLOR        color,
   NUMALIGN        align
);

LPOBJECTDESC
PAL_LoadObjectDesc(
   LPCSTR          lpszFileName
);

VOID
PAL_FreeObjectDesc(
   LPOBJECTDESC    lpObjectDesc
);

LPCSTR
PAL_GetObjectDesc(
   LPOBJECTDESC   lpObjectDesc,
   WORD           wObjectID
);

extern LPSPRITE gpSpriteUI;

#ifdef __cplusplus
}
#endif

#endif
