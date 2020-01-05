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

#ifndef BATTLE_H
#define BATTLE_H

#include "global.h"
#include "uibattle.h"

#define       BATTLE_FPS               25
#define       BATTLE_FRAME_TIME        (1000 / BATTLE_FPS)

typedef enum tagBATTLERESULT
{
   kBattleResultWon        = 3,      // player won the battle
   kBattleResultLost       = 1,      // player lost the battle
   kBattleResultFleed      = 0xFFFF, // player fleed from the battle
   kBattleResultTerminated = 0,      // battle terminated with scripts
   kBattleResultOnGoing    = 1000,   // the battle is ongoing
   kBattleResultPreBattle  = 1001,   // running pre-battle scripts
   kBattleResultPause      = 1002,   // battle pause
} BATTLERESULT;

typedef enum tagFIGHTERSTATE
{
   kFighterWait,  // waiting time
   kFighterCom,   // accepting command
   kFighterAct,   // doing the actual move
} FIGHTERSTATE;

typedef enum tagBATTLEACTIONTYPE
{
   kBattleActionPass,          // do nothing
   kBattleActionDefend,        // defend
   kBattleActionAttack,        // physical attack
   kBattleActionMagic,         // use magic
   kBattleActionCoopMagic,     // use cooperative magic
   kBattleActionFlee,          // flee from the battle
   kBattleActionThrowItem,     // throw item onto enemy
   kBattleActionUseItem,       // use item
   kBattleActionAttackMate,    // attack teammate (confused only)
} BATTLEACTIONTYPE;

typedef struct tagBATTLEACTION
{
   BATTLEACTIONTYPE   ActionType;
   WORD               wActionID;   // item/magic to use
   SHORT              sTarget;     // -1 for everyone
   FLOAT              flRemainingTime;  // remaining waiting time before the action start
} BATTLEACTION;

typedef struct tagBATTLEENEMY
{
   WORD               wObjectID;              // Object ID of this enemy
   ENEMY              e;                      // detailed data of this enemy
   WORD               rgwStatus[kStatusAll];  // status effects
   FLOAT              flTimeMeter;            // time-charging meter (0 = empty, 100 = full).
   POISONSTATUS       rgPoisons[MAX_POISONS]; // poisons
   LPSPRITE           lpSprite;
   PAL_POS            pos;                    // current position on the screen
   PAL_POS            posOriginal;            // original position on the screen
   WORD               wCurrentFrame;          // current frame number
   FIGHTERSTATE       state;                  // state of this enemy

#ifndef PAL_CLASSIC
   BOOL               fTurnStart;
   BOOL               fFirstMoveDone;
   BOOL               fDualMove;
#endif

   WORD               wScriptOnTurnStart;
   WORD               wScriptOnBattleEnd;
   WORD               wScriptOnReady;

   WORD               wPrevHP;              // HP value prior to action

   INT                iColorShift;
} BATTLEENEMY;

// We only put some data used in battle here; other data can be accessed in the global data.
typedef struct tagBATTLEPLAYER
{
   INT                iColorShift;
   FLOAT              flTimeMeter;          // time-charging meter (0 = empty, 100 = full).
   FLOAT              flTimeSpeedModifier;
   WORD               wHidingTime;          // remaining hiding time
   LPSPRITE           lpSprite;
   PAL_POS            pos;                  // current position on the screen
   PAL_POS            posOriginal;          // original position on the screen
   WORD               wCurrentFrame;        // current frame number
   FIGHTERSTATE       state;                // state of this player
   BATTLEACTION       action;               // action to perform
   BATTLEACTION       prevAction;           // action of the previous turn
   BOOL               fDefending;           // TRUE if player is defending
   WORD               wPrevHP;              // HP value prior to action
   WORD               wPrevMP;              // MP value prior to action
#ifndef PAL_CLASSIC
   SHORT              sTurnOrder;           // turn order
#endif
} BATTLEPLAYER;

typedef struct tagSUMMON
{
   LPSPRITE           lpSprite;
   WORD               wCurrentFrame;
} SUMMON;

#define MAX_BATTLE_ACTIONS    256
#define MAX_KILLED_ENEMIES    256

#ifdef PAL_CLASSIC

typedef enum tabBATTLEPHASE
{
   kBattlePhaseSelectAction,
   kBattlePhasePerformAction
} BATTLEPHASE;

typedef struct tagACTIONQUEUE
{
   BOOL       fIsEnemy;
   WORD       wDexterity;
   WORD       wIndex;
} ACTIONQUEUE;

#define MAX_ACTIONQUEUE_ITEMS (MAX_PLAYERS_IN_PARTY + MAX_ENEMIES_IN_TEAM * 2)

#endif

typedef struct tagBATTLE
{
   BATTLEPLAYER     rgPlayer[MAX_PLAYERS_IN_PARTY];
   BATTLEENEMY      rgEnemy[MAX_ENEMIES_IN_TEAM];

   WORD             wMaxEnemyIndex;

   SDL_Surface     *lpSceneBuf;
   SDL_Surface     *lpBackground;

   SHORT            sBackgroundColorShift;

   LPSPRITE         lpSummonSprite;       // sprite of summoned god
   PAL_POS          posSummon;
   INT              iSummonFrame;         // current frame of the summoned god

   INT              iExpGained;           // total experience value gained
   INT              iCashGained;          // total cash gained

   BOOL             fIsBoss;              // TRUE if boss fight
   BOOL             fEnemyCleared;        // TRUE if enemies are cleared
   BATTLERESULT     BattleResult;

   FLOAT            flTimeChargingUnit;   // the base waiting time unit

   BATTLEUI         UI;

   LPBYTE           lpEffectSprite;

   BOOL             fEnemyMoving;         // TRUE if enemy is moving

   INT              iHidingTime;          // Time of hiding

   WORD             wMovingPlayerIndex;   // current moving player index

   int              iBlow;

#ifdef PAL_CLASSIC
   BATTLEPHASE      Phase;
   ACTIONQUEUE      ActionQueue[MAX_ACTIONQUEUE_ITEMS];
   int              iCurAction;
   BOOL             fRepeat;              // TRUE if player pressed Repeat
   BOOL             fForce;               // TRUE if player pressed Force
   BOOL             fFlee;                // TRUE if player pressed Flee
   BOOL             fPrevAutoAtk;         // TRUE if auto-attack was used in the previous turn
   BOOL             fPrevPlayerAutoAtk;   // TRUE if auto-attack was used by previous player in the same turn

   WORD             coopContributors[MAX_PLAYERS_IN_PARTY];
   BOOL             fThisTurnCoop;
#endif
} BATTLE;

PAL_C_LINKAGE_BEGIN

extern BATTLE g_Battle;

VOID
PAL_LoadBattleSprites(
   VOID
);

VOID
PAL_BattleMakeScene(
   VOID
);

VOID
PAL_BattleFadeScene(
   VOID
);

VOID
PAL_BattleEnemyEscape(
   VOID
);

VOID
PAL_BattlePlayerEscape(
   VOID
);

BATTLERESULT
PAL_StartBattle(
   WORD        wEnemyTeam,
   BOOL        fIsBoss
);

PAL_C_LINKAGE_END

#endif
