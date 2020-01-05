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

#ifndef GLOBAL_H
#define GLOBAL_H

#include "common.h"
#include "palcommon.h"
#include "map.h"
#include "ui.h"

//
// SOME NOTES ON "AUTO SCRIPT" AND "TRIGGER SCRIPT":
//
// Auto scripts are executed automatically in each frame.
//
// Trigger scripts are only executed when the event is triggered (player touched
// an event object, player triggered an event script by pressing Spacebar).
//

// status of characters
typedef enum tagSTATUS
{
   kStatusConfused = 0,  // attack friends randomly
#ifdef PAL_CLASSIC
   kStatusParalyzed,     // paralyzed
#else
   kStatusSlow,          // slower
#endif
   kStatusSleep,         // not allowed to move
   kStatusSilence,       // cannot use magic
   kStatusPuppet,        // for dead players only, continue attacking
   kStatusBravery,       // more power for physical attacks
   kStatusProtect,       // more defense value
   kStatusHaste,         // faster
   kStatusDualAttack,    // dual attack
   kStatusAll
} STATUS;

#ifndef PAL_CLASSIC
#define kStatusParalyzed kStatusSleep
#endif

// body parts of equipments
typedef enum tagBODYPART
{
   kBodyPartHead     = 0,
   kBodyPartBody,
   kBodyPartShoulder,
   kBodyPartHand,
   kBodyPartFeet,
   kBodyPartWear,
   kBodyPartExtra,
} BODYPART;

// state of event object, used by the sState field of the EVENTOBJECT struct
typedef enum tagOBJECTSTATE
{
   kObjStateHidden               = 0,
   kObjStateNormal               = 1,
   kObjStateBlocker              = 2
} OBJECTSTATE, *LPOBJECTSTATE;

typedef enum tagTRIGGERMODE
{
   kTriggerNone                  = 0,
   kTriggerSearchNear            = 1,
   kTriggerSearchNormal          = 2,
   kTriggerSearchFar             = 3,
   kTriggerTouchNear             = 4,
   kTriggerTouchNormal           = 5,
   kTriggerTouchFar              = 6,
   kTriggerTouchFarther          = 7,
   kTriggerTouchFarthest         = 8
} TRIGGERMODE;

typedef struct tagEVENTOBJECT
{
   SHORT        sVanishTime;         // vanish time (?)
   WORD         x;                   // X coordinate on the map
   WORD         y;                   // Y coordinate on the map
   SHORT        sLayer;              // layer value
   WORD         wTriggerScript;      // Trigger script entry
   WORD         wAutoScript;         // Auto script entry
   SHORT        sState;              // state of this object
   WORD         wTriggerMode;        // trigger mode
   WORD         wSpriteNum;          // number of the sprite
   USHORT       nSpriteFrames;       // total number of frames of the sprite
   WORD         wDirection;          // direction
   WORD         wCurrentFrameNum;    // current frame number
   USHORT       nScriptIdleFrame;    // count of idle frames, used by trigger script
   WORD         wSpritePtrOffset;    // FIXME: ???
   USHORT       nSpriteFramesAuto;   // total number of frames of the sprite, used by auto script
   WORD         wScriptIdleFrameCountAuto;     // count of idle frames, used by auto script
} EVENTOBJECT, *LPEVENTOBJECT;

typedef struct tagSCENE
{
   WORD         wMapNum;         // number of the map
   WORD         wScriptOnEnter;  // when entering this scene, execute script from here
   WORD         wScriptOnTeleport;  // when teleporting out of this scene, execute script from here
   WORD         wEventObjectIndex;  // event objects in this scene begins from number wEventObjectIndex + 1
} SCENE, *LPSCENE;

// object including system strings, players, items, magics, enemies and poison scripts.

// system strings and players
typedef struct tagOBJECT_PLAYER
{
   WORD         wReserved[2];    // always zero
   WORD         wScriptOnFriendDeath; // when friends in party dies, execute script from here
   WORD         wScriptOnDying;  // when dying, execute script from here
} OBJECT_PLAYER;

typedef enum tagITEMFLAG
{
   kItemFlagUsable          = (1 << 0),
   kItemFlagEquipable       = (1 << 1),
   kItemFlagThrowable       = (1 << 2),
   kItemFlagConsuming       = (1 << 3),
   kItemFlagApplyToAll      = (1 << 4),
   kItemFlagSellable        = (1 << 5),
   kItemFlagEquipableByPlayerRole_First  = (1 << 6)
} ITEMFLAG;

// items
typedef struct tagOBJECT_ITEM_DOS
{
   WORD         wBitmap;         // bitmap number in BALL.MKF
   WORD         wPrice;          // price
   WORD         wScriptOnUse;    // script executed when using this item
   WORD         wScriptOnEquip;  // script executed when equipping this item
   WORD         wScriptOnThrow;  // script executed when throwing this item to enemy
   WORD         wFlags;          // flags
} OBJECT_ITEM_DOS;

// items
typedef struct tagOBJECT_ITEM
{
	WORD         wBitmap;         // bitmap number in BALL.MKF
	WORD         wPrice;          // price
	WORD         wScriptOnUse;    // script executed when using this item
	WORD         wScriptOnEquip;  // script executed when equipping this item
	WORD         wScriptOnThrow;  // script executed when throwing this item to enemy
	WORD         wScriptDesc;     // description script
	WORD         wFlags;          // flags
} OBJECT_ITEM;

typedef enum tagMAGICFLAG
{
   kMagicFlagUsableOutsideBattle        = (1 << 0),
   kMagicFlagUsableInBattle             = (1 << 1),
   kMagicFlagUsableToEnemy              = (1 << 3),
   kMagicFlagApplyToAll                 = (1 << 4),
} MAGICFLAG;

// magics
typedef struct tagOBJECT_MAGIC_DOS
{
   WORD         wMagicNumber;      // magic number, according to DATA.MKF #3
   WORD         wReserved1;        // always zero
   WORD         wScriptOnSuccess;  // when magic succeed, execute script from here
   WORD         wScriptOnUse;      // when use this magic, execute script from here
   WORD         wReserved2;        // always zero
   WORD         wFlags;            // flags
} OBJECT_MAGIC_DOS;

// magics
typedef struct tagOBJECT_MAGIC
{
	WORD         wMagicNumber;      // magic number, according to DATA.MKF #3
	WORD         wReserved1;        // always zero
	WORD         wScriptOnSuccess;  // when magic succeed, execute script from here
	WORD         wScriptOnUse;      // when use this magic, execute script from here
	WORD         wScriptDesc;       // description script
	WORD         wReserved2;        // always zero
	WORD         wFlags;            // flags
} OBJECT_MAGIC;

// enemies
typedef struct tagOBJECT_ENEMY
{
   WORD         wEnemyID;        // ID of the enemy, according to DATA.MKF #1.
                                 // Also indicates the bitmap number in ABC.MKF.
   WORD         wResistanceToSorcery;  // resistance to sorcery and poison (0 min, 10 max)
   WORD         wScriptOnTurnStart;    // script executed when turn starts
   WORD         wScriptOnBattleEnd;    // script executed when battle ends
   WORD         wScriptOnReady;        // script executed when the enemy is ready
} OBJECT_ENEMY;

// poisons (scripts executed in each round)
typedef struct tagOBJECT_POISON
{
   WORD         wPoisonLevel;    // level of the poison
   WORD         wColor;          // color of avatars
   WORD         wPlayerScript;   // script executed when player has this poison (per round)
   WORD         wReserved;       // always zero
   WORD         wEnemyScript;    // script executed when enemy has this poison (per round)
} OBJECT_POISON;

typedef union tagOBJECT_DOS
{
	WORD              rgwData[6];
	OBJECT_PLAYER     player;
	OBJECT_ITEM_DOS   item;
	OBJECT_MAGIC_DOS  magic;
	OBJECT_ENEMY      enemy;
	OBJECT_POISON     poison;
} OBJECT_DOS, *LPOBJECT_DOS;

typedef union tagOBJECT
{
	WORD              rgwData[7];
	OBJECT_PLAYER     player;
	OBJECT_ITEM       item;
	OBJECT_MAGIC      magic;
	OBJECT_ENEMY      enemy;
	OBJECT_POISON     poison;
} OBJECT, *LPOBJECT;

typedef struct tagSCRIPTENTRY
{
   WORD          wOperation;     // operation code
   WORD          rgwOperand[3];  // operands
} SCRIPTENTRY, *LPSCRIPTENTRY;

typedef struct tagINVENTORY
{
   WORD          wItem;             // item object code
   USHORT        nAmount;           // amount of this item
   USHORT        nAmountInUse;      // in-use amount of this item
} INVENTORY, *LPINVENTORY;

typedef struct tagSTORE
{
   WORD          rgwItems[MAX_STORE_ITEM];
} STORE, *LPSTORE;

typedef struct tagENEMY
{
   WORD        wIdleFrames;         // total number of frames when idle
   WORD        wMagicFrames;        // total number of frames when using magics
   WORD        wAttackFrames;       // total number of frames when doing normal attack
   WORD        wIdleAnimSpeed;      // speed of the animation when idle
   WORD        wActWaitFrames;      // FIXME: ???
   WORD        wYPosOffset;
   SHORT       wAttackSound;        // sound played when this enemy uses normal attack
   SHORT       wActionSound;        // FIXME: ???
   SHORT       wMagicSound;         // sound played when this enemy uses magic
   SHORT       wDeathSound;         // sound played when this enemy dies
   SHORT       wCallSound;          // sound played when entering the battle
   WORD        wHealth;             // total HP of the enemy
   WORD        wExp;                // How many EXPs we'll get for beating this enemy
   WORD        wCash;               // how many cashes we'll get for beating this enemy
   WORD        wLevel;              // this enemy's level
   WORD        wMagic;              // this enemy's magic number
   WORD        wMagicRate;          // chance for this enemy to use magic
   WORD        wAttackEquivItem;    // equivalence item of this enemy's normal attack
   WORD        wAttackEquivItemRate;// chance for equivalence item
   WORD        wStealItem;          // which item we'll get when stealing from this enemy
   WORD        nStealItem;          // total amount of the items which can be stolen
   WORD        wAttackStrength;     // normal attack strength
   WORD        wMagicStrength;      // magical attack strength
   WORD        wDefense;            // resistance to all kinds of attacking
   WORD        wDexterity;          // dexterity
   WORD        wFleeRate;           // chance for successful fleeing
   WORD        wPoisonResistance;   // resistance to poison
   WORD        wElemResistance[NUM_MAGIC_ELEMENTAL]; // resistance to elemental magics
   WORD        wPhysicalResistance; // resistance to physical attack
   WORD        wDualMove;           // whether this enemy can do dual move or not
   WORD        wCollectValue;       // value for collecting this enemy for items
} ENEMY, *LPENEMY;

typedef struct tagENEMYTEAM
{
   WORD        rgwEnemy[MAX_ENEMIES_IN_TEAM];
} ENEMYTEAM, *LPENEMYTEAM;

typedef WORD PLAYERS[MAX_PLAYER_ROLES];

typedef struct tagPLAYERROLES
{
   PLAYERS            rgwAvatar;             // avatar (shown in status view)
   PLAYERS            rgwSpriteNumInBattle;  // sprite displayed in battle (in F.MKF)
   PLAYERS            rgwSpriteNum;          // sprite displayed in normal scene (in MGO.MKF)
   PLAYERS            rgwName;               // name of player class (in WORD.DAT)
   PLAYERS            rgwAttackAll;          // whether player can attack everyone in a bulk or not
   PLAYERS            rgwUnknown1;           // FIXME: ???
   PLAYERS            rgwLevel;              // level
   PLAYERS            rgwMaxHP;              // maximum HP
   PLAYERS            rgwMaxMP;              // maximum MP
   PLAYERS            rgwHP;                 // current HP
   PLAYERS            rgwMP;                 // current MP
   WORD               rgwEquipment[MAX_PLAYER_EQUIPMENTS][MAX_PLAYER_ROLES]; // equipments
   PLAYERS            rgwAttackStrength;     // normal attack strength
   PLAYERS            rgwMagicStrength;      // magical attack strength
   PLAYERS            rgwDefense;            // resistance to all kinds of attacking
   PLAYERS            rgwDexterity;          // dexterity
   PLAYERS            rgwFleeRate;           // chance of successful fleeing
   PLAYERS            rgwPoisonResistance;   // resistance to poison
   WORD               rgwElementalResistance[NUM_MAGIC_ELEMENTAL][MAX_PLAYER_ROLES]; // resistance to elemental magics
   PLAYERS            rgwUnknown2;           // FIXME: ???
   PLAYERS            rgwUnknown3;           // FIXME: ???
   PLAYERS            rgwUnknown4;           // FIXME: ???
   PLAYERS            rgwCoveredBy;          // who will cover me when I am low of HP or not sane
   WORD               rgwMagic[MAX_PLAYER_MAGICS][MAX_PLAYER_ROLES]; // magics
   PLAYERS            rgwWalkFrames;         // walk frame (???)
   PLAYERS            rgwCooperativeMagic;   // cooperative magic
   PLAYERS            rgwUnknown5;           // FIXME: ???
   PLAYERS            rgwUnknown6;           // FIXME: ???
   PLAYERS            rgwDeathSound;         // sound played when player dies
   PLAYERS            rgwAttackSound;        // sound played when player attacks
   PLAYERS            rgwWeaponSound;        // weapon sound (???)
   PLAYERS            rgwCriticalSound;      // sound played when player make critical hits
   PLAYERS            rgwMagicSound;         // sound played when player is casting a magic
   PLAYERS            rgwCoverSound;         // sound played when player cover others
   PLAYERS            rgwDyingSound;         // sound played when player is dying
} PLAYERROLES, *LPPLAYERROLES;

typedef enum tagMAGIC_TYPE
{
   kMagicTypeNormal           = 0,
   kMagicTypeAttackAll        = 1,  // draw the effect on each of the enemies
   kMagicTypeAttackWhole      = 2,  // draw the effect on the whole enemy team
   kMagicTypeAttackField      = 3,  // draw the effect on the battle field
   kMagicTypeApplyToPlayer    = 4,  // the magic is used on one player
   kMagicTypeApplyToParty     = 5,  // the magic is used on the whole party
   kMagicTypeTrance           = 8,  // trance the player
   kMagicTypeSummon           = 9,  // summon
} MAGIC_TYPE;

typedef struct tagMAGIC
{
   WORD               wEffect;               // effect sprite
   WORD               wType;                 // type of this magic
   WORD               wXOffset;
   WORD               wYOffset;
   WORD               wSummonEffect;         // summon effect sprite (in F.MKF)
   SHORT              wSpeed;                // speed of the effect
   WORD               wKeepEffect;           // FIXME: ???
   WORD               wFireDelay;            // start frame of the magic fire stage
   WORD               wEffectTimes;          // total times of effect
   WORD               wShake;                // shake screen
   WORD               wWave;                 // wave screen
   WORD               wUnknown;              // FIXME: ???
   WORD               wCostMP;               // MP cost
   WORD               wBaseDamage;           // base damage
   WORD               wElemental;            // elemental (0 = No Elemental, last = poison)
   SHORT              wSound;                // sound played when using this magic
} MAGIC, *LPMAGIC;

typedef struct tagBATTLEFIELD
{
   WORD               wScreenWave;                      // level of screen waving
   SHORT              rgsMagicEffect[NUM_MAGIC_ELEMENTAL]; // effect of attributed magics
} BATTLEFIELD, *LPBATTLEFIELD;

// magics learned when level up
typedef struct tagLEVELUPMAGIC
{
   WORD               wLevel;    // level reached
   WORD               wMagic;    // magic learned
} LEVELUPMAGIC, *LPLEVELUPMAGIC;

typedef struct tagLEVELUPMAGIC_ALL
{
   LEVELUPMAGIC       m[MAX_PLAYABLE_PLAYER_ROLES];
} LEVELUPMAGIC_ALL, *LPLEVELUPMAGIC_ALL;

typedef struct tagPALPOS
{
	WORD      x;
	WORD      y;
} PALPOS;

typedef struct tagENEMYPOS
{
	PALPOS pos[MAX_ENEMIES_IN_TEAM][MAX_ENEMIES_IN_TEAM];
} ENEMYPOS, *LPENEMYPOS;

// Exp. points needed for the next level
typedef WORD LEVELUPEXP, *LPLEVELUPEXP;

// game data which is available in data files.
typedef struct tagGAMEDATA
{
   LPEVENTOBJECT           lprgEventObject;
   int                     nEventObject;

   SCENE                   rgScene[MAX_SCENES];
   OBJECT                  rgObject[MAX_OBJECTS];

   LPSCRIPTENTRY           lprgScriptEntry;
   int                     nScriptEntry;

   LPSTORE                 lprgStore;
   int                     nStore;

   LPENEMY                 lprgEnemy;
   int                     nEnemy;

   LPENEMYTEAM             lprgEnemyTeam;
   int                     nEnemyTeam;

   PLAYERROLES             PlayerRoles;

   LPMAGIC                 lprgMagic;
   int                     nMagic;

   LPBATTLEFIELD           lprgBattleField;
   int                     nBattleField;

   LPLEVELUPMAGIC_ALL      lprgLevelUpMagic;
   int                     nLevelUpMagic;

   ENEMYPOS                EnemyPos;
   LEVELUPEXP              rgLevelUpExp[MAX_LEVELS + 1];

   WORD                    rgwBattleEffectIndex[10][2];
} GAMEDATA, *LPGAMEDATA;

typedef struct tagFILES
{
   FILE            *fpFBP;      // battlefield background images
   FILE            *fpMGO;      // sprites in scenes
   FILE            *fpBALL;     // item bitmaps
   FILE            *fpDATA;     // misc data
   FILE            *fpF;        // player sprites during battle
   FILE            *fpFIRE;     // fire effect sprites
   FILE            *fpRGM;      // character face bitmaps
   FILE            *fpSSS;      // script data
} FILES, *LPFILES;

// player party
typedef struct tagPARTY
{
   WORD             wPlayerRole;         // player role
   SHORT            x, y;                // position
   WORD             wFrame;              // current frame number
   WORD             wImageOffset;        // FIXME: ???
} PARTY, *LPPARTY;

// player trail, used for other party members to follow the main party member
typedef struct tagTRAIL
{
   WORD             x, y;          // position
   WORD             wDirection;    // direction
} TRAIL, *LPTRAIL;

typedef struct tagEXPERIENCE
{
   WORD         wExp;                // current experience points
   WORD         wReserved;
   WORD         wLevel;              // current level
   WORD         wCount;
} EXPERIENCE, *LPEXPERIENCE;

typedef struct tagALLEXPERIENCE
{
   EXPERIENCE        rgPrimaryExp[MAX_PLAYER_ROLES];
   EXPERIENCE        rgHealthExp[MAX_PLAYER_ROLES];
   EXPERIENCE        rgMagicExp[MAX_PLAYER_ROLES];
   EXPERIENCE        rgAttackExp[MAX_PLAYER_ROLES];
   EXPERIENCE        rgMagicPowerExp[MAX_PLAYER_ROLES];
   EXPERIENCE        rgDefenseExp[MAX_PLAYER_ROLES];
   EXPERIENCE        rgDexterityExp[MAX_PLAYER_ROLES];
   EXPERIENCE        rgFleeExp[MAX_PLAYER_ROLES];
} ALLEXPERIENCE, *LPALLEXPERIENCE;

typedef struct tagPOISONSTATUS
{
   WORD              wPoisonID;       // kind of the poison
   WORD              wPoisonScript;   // script entry
} POISONSTATUS, *LPPOISONSTATUS;

typedef struct tagGLOBALVARS
{
   FILES            f;
   GAMEDATA         g;

   int              iCurMainMenuItem;    // current main menu item number
   int              iCurSystemMenuItem;  // current system menu item number
   int              iCurInvMenuItem;     // current inventory menu item number
   int              iCurPlayingRNG;      // current playing RNG animation
   BYTE             bCurrentSaveSlot;    // current save slot (1-5)
   BOOL             fInMainGame;         // TRUE if in main game
   BOOL             fGameStart;          // TRUE if the has just started
   BOOL             fEnteringScene;      // TRUE if entering a new scene
   BOOL             fNeedToFadeIn;       // TRUE if need to fade in when drawing scene
   BOOL             fInBattle;           // TRUE if in battle
   BOOL             fAutoBattle;         // TRUE if auto-battle
#ifndef PAL_CLASSIC
   BYTE             bBattleSpeed;        // Battle Speed (1 = Fastest, 5 = Slowest)
#endif
   WORD             wLastUnequippedItem; // last unequipped item

   PLAYERROLES      rgEquipmentEffect[MAX_PLAYER_EQUIPMENTS + 1]; // equipment effects
   WORD             rgPlayerStatus[MAX_PLAYER_ROLES][kStatusAll]; // player status

   PAL_POS          viewport;            // viewport coordination
   PAL_POS          partyoffset;
   WORD             wLayer;
   WORD             wMaxPartyMemberIndex;// max index of members in party (0 to MAX_PLAYERS_IN_PARTY - 1)
   PARTY            rgParty[MAX_PLAYABLE_PLAYER_ROLES]; // player party
   TRAIL            rgTrail[MAX_PLAYABLE_PLAYER_ROLES]; // player trail
   WORD             wPartyDirection;     // direction of the party
   WORD             wNumScene;           // current scene number
   WORD             wNumPalette;         // current palette number
   BOOL             fNightPalette;       // TRUE if use the darker night palette
   WORD             wNumMusic;           // current music number
   WORD             wNumBattleMusic;     // current music number in battle
   WORD             wNumBattleField;     // current battle field number
   WORD             wCollectValue;       // value of "collected" items
   WORD             wScreenWave;         // level of screen waving
   SHORT            sWaveProgression;
   WORD             wChaseRange;
   WORD             wChasespeedChangeCycles;
   USHORT           nFollower;

   DWORD            dwCash;              // amount of cash

   ALLEXPERIENCE    Exp;                 // experience status
   POISONSTATUS     rgPoisonStatus[MAX_POISONS][MAX_PLAYABLE_PLAYER_ROLES]; // poison status
   INVENTORY        rgInventory[MAX_INVENTORY];  // inventory status
   LPOBJECTDESC     lpObjectDesc;
   DWORD            dwFrameNum;
} GLOBALVARS, *LPGLOBALVARS;

PAL_C_LINKAGE_BEGIN

extern GLOBALVARS * const gpGlobals;

BOOL
PAL_IsWINVersion(
   BOOL *pfIsWIN95
);

CODEPAGE
PAL_DetectCodePage(
	const char *   filename
);

INT
PAL_InitGlobals(
   VOID
);

VOID
PAL_FreeGlobals(
   VOID
);

VOID
PAL_SaveGame(
   int           iSaveSlot,
   WORD          wSavedTimes
);

VOID
PAL_InitGameData(
   INT           iSaveSlot
);

INT
PAL_CountItem(
   WORD          wObjectID
);

BOOL
PAL_AddItemToInventory(
   WORD          wObjectID,
   INT           iNum
);

BOOL
PAL_IncreaseHPMP(
   WORD          wPlayerRole,
   SHORT         sHP,
   SHORT         sMP
);

INT
PAL_GetItemAmount(
   WORD        wItem
);

VOID
PAL_UpdateEquipments(
   VOID
);

VOID
PAL_CompressInventory(
   VOID
);

VOID
PAL_RemoveEquipmentEffect(
   WORD         wPlayerRole,
   WORD         wEquipPart
);

VOID
PAL_AddPoisonForPlayer(
   WORD           wPlayerRole,
   WORD           wPoisonID
);

VOID
PAL_CurePoisonByKind(
   WORD           wPlayerRole,
   WORD           wPoisonID
);

VOID
PAL_CurePoisonByLevel(
   WORD           wPlayerRole,
   WORD           wMaxLevel
);

BOOL
PAL_IsPlayerPoisonedByLevel(
   WORD           wPlayerRole,
   WORD           wMinLevel
);

BOOL
PAL_IsPlayerPoisonedByKind(
   WORD           wPlayerRole,
   WORD           wPoisonID
);

WORD
PAL_GetPlayerAttackStrength(
   WORD           wPlayerRole
);

WORD
PAL_GetPlayerMagicStrength(
   WORD           wPlayerRole
);

WORD
PAL_GetPlayerDefense(
   WORD           wPlayerRole
);

WORD
PAL_GetPlayerDexterity(
   WORD           wPlayerRole
);

WORD
PAL_GetPlayerFleeRate(
   WORD           wPlayerRole
);

WORD
PAL_GetPlayerPoisonResistance(
   WORD           wPlayerRole
);

WORD
PAL_GetPlayerElementalResistance(
   WORD           wPlayerRole,
   INT            iAttrib
);

WORD
PAL_GetPlayerBattleSprite(
   WORD           wPlayerRole
);

WORD
PAL_GetPlayerCooperativeMagic(
   WORD           wPlayerRole
);

BOOL
PAL_PlayerCanAttackAll(
   WORD           wPlayerRole
);

BOOL
PAL_AddMagic(
   WORD           wPlayerRole,
   WORD           wMagic
);

VOID
PAL_RemoveMagic(
   WORD           wPlayerRole,
   WORD           wMagic
);

VOID
PAL_SetPlayerStatus(
   WORD         wPlayerRole,
   WORD         wStatusID,
   WORD         wNumRound
);

VOID
PAL_RemovePlayerStatus(
   WORD         wPlayerRole,
   WORD         wStatusID
);

VOID
PAL_ClearAllPlayerStatus(
   VOID
);

VOID
PAL_PlayerLevelUp(
   WORD          wPlayerRole,
   WORD          wNumLevel
);

PAL_C_LINKAGE_END

#endif
