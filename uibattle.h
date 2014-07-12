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

#ifndef UIBATTLE_H
#define UIBATTLE_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "ui.h"

typedef enum tagBATTLEUISTATE
{
   kBattleUIWait,
   kBattleUISelectMove,
   kBattleUISelectTargetEnemy,
   kBattleUISelectTargetPlayer,
   kBattleUISelectTargetEnemyAll,
   kBattleUISelectTargetPlayerAll,
} BATTLEUISTATE;

typedef enum tagBATTLEMENUSTATE
{
   kBattleMenuMain,
   kBattleMenuMagicSelect,
   kBattleMenuUseItemSelect,
   kBattleMenuThrowItemSelect,
   kBattleMenuMisc,
   kBattleMenuMiscItemSubMenu,
} BATTLEMENUSTATE;

typedef enum tagBATTLEUIACTION
{
   kBattleUIActionAttack,
   kBattleUIActionMagic,
   kBattleUIActionCoopMagic,
   kBattleUIActionMisc,
} BATTLEUIACTION;

#define SPRITENUM_BATTLEICON_ATTACK      40
#define SPRITENUM_BATTLEICON_MAGIC       41
#define SPRITENUM_BATTLEICON_COOPMAGIC   42
#define SPRITENUM_BATTLEICON_MISCMENU    43

#define SPRITENUM_BATTLE_ARROW_CURRENTPLAYER           69
#define SPRITENUM_BATTLE_ARROW_CURRENTPLAYER_RED       68

#define SPRITENUM_BATTLE_ARROW_SELECTEDPLAYER          67
#define SPRITENUM_BATTLE_ARROW_SELECTEDPLAYER_RED      66

#define BATTLEUI_LABEL_ITEM              5
#define BATTLEUI_LABEL_DEFEND            58
#define BATTLEUI_LABEL_AUTO              56
#define BATTLEUI_LABEL_INVENTORY         57
#define BATTLEUI_LABEL_FLEE              59
#define BATTLEUI_LABEL_STATUS            60

#define BATTLEUI_LABEL_USEITEM           23
#define BATTLEUI_LABEL_THROWITEM         24

#define TIMEMETER_COLOR_DEFAULT          0x1B
#define TIMEMETER_COLOR_SLOW             0x5B
#define TIMEMETER_COLOR_HASTE            0x2A

#define BATTLEUI_MAX_SHOWNUM             16

typedef struct tagSHOWNUM
{
   WORD             wNum;
   PAL_POS          pos;
   DWORD            dwTime;
   NUMCOLOR         color;
} SHOWNUM;

typedef struct tagBATTLEUI
{
   BATTLEUISTATE    state;
   BATTLEMENUSTATE  MenuState;

   CHAR             szMsg[256];           // message to be shown on the screen
   CHAR             szNextMsg[256];       // next message to be shown on the screen
   DWORD            dwMsgShowTime;        // the end time of showing the message
   WORD             wNextMsgDuration;     // duration of the next message

   WORD             wCurPlayerIndex;      // index of the current player
   WORD             wSelectedAction;      // current selected action
   WORD             wSelectedIndex;       // current selected index of player or enemy
   WORD             wPrevEnemyTarget;     // previous enemy target

   WORD             wActionType;          // type of action to be performed
   WORD             wObjectID;            // object ID of the item or magic to use

   BOOL             fAutoAttack;          // TRUE if auto attack

   SHOWNUM          rgShowNum[BATTLEUI_MAX_SHOWNUM];
} BATTLEUI;

VOID
PAL_PlayerInfoBox(
   PAL_POS         pos,
   WORD            wPlayerRole,
   INT             iTimeMeter,
   BYTE            bTimeMeterColor,
   BOOL            fUpdate
);

VOID
PAL_BattleUIShowText(
   LPCSTR        lpszText,
   WORD          wDuration
);

VOID
PAL_BattleUIPlayerReady(
   WORD          wPlayerIndex
);

VOID
PAL_BattleUIUpdate(
   VOID
);

VOID
PAL_BattleUIShowNum(
   WORD           wNum,
   PAL_POS        pos,
   NUMCOLOR       color
);

#ifdef __cplusplus
}
#endif
#endif
