/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2016, Lou Yihua <louyihua@21cn.com>.
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

#ifndef CONFIG_H
#define CONFIG_H

# ifdef __cplusplus
extern "C"
{
# endif

#include "palcommon.h"

#define     PAL_MAX_SAMPLERATE           48000
#define     PAL_MAX_VOLUME               100

typedef enum tagPALCFG_ITEM
{
	PALCFG_ALL_MIN = 0,

	PALCFG_BOOLEAN_MIN = PALCFG_ALL_MIN,
	/* Booleans */
	PALCFG_FULLSCREEN = PALCFG_BOOLEAN_MIN,
	PALCFG_KEEPASPECTRATIO,
	PALCFG_LAUNCHSETTING,
	PALCFG_STEREO,
	PALCFG_USEEMBEDDEDFONTS,
	PALCFG_USESURROUNDOPL,
	PALCFG_USETOUCHOVERLAY,
	/* Booleans */
	PALCFG_BOOLEAN_MAX,

	PALCFG_INTEGER_MIN = PALCFG_BOOLEAN_MAX,
	/* Integers */
	PALCFG_SURROUNDOPLOFFSET = PALCFG_INTEGER_MIN,
	/* Integers */
	PALCFG_INTEGER_MAX,

	PALCFG_UNSIGNED_MIN = PALCFG_INTEGER_MAX,
	/* Unsigneds */
	PALCFG_AUDIOBUFFERSIZE = PALCFG_UNSIGNED_MIN,
	PALCFG_CODEPAGE,
	PALCFG_OPLSAMPLERATE,
	PALCFG_RESAMPLEQUALITY,
	PALCFG_SAMPLERATE,
	PALCFG_MUSICVOLUME,
	PALCFG_SOUNDVOLUME,
	PALCFG_WINDOWHEIGHT,
	PALCFG_WINDOWWIDTH,
	/* Unsigneds */
	PALCFG_UNSIGNED_MAX,

	PALCFG_STRING_MIN = PALCFG_UNSIGNED_MAX,
	/* Strings */
	PALCFG_CD = PALCFG_STRING_MIN,
	PALCFG_GAMEPATH,
	PALCFG_MESSAGEFILE,
	PALCFG_BDFFILE,
	PALCFG_MUSIC,
	PALCFG_OPL,
	PALCFG_RIXEXTRAINIT,
	/* Strings */
	PALCFG_STRING_MAX,

	PALCFG_ALL_MAX = PALCFG_STRING_MAX
} PALCFG_ITEM;

typedef enum tagPALCFG_TYPE
{
	PALCFG_STRING,
	PALCFG_BOOLEAN,
	PALCFG_INTEGER,
	PALCFG_UNSIGNED,
} PALCFG_TYPE;

typedef union tagConfigValue
{
	LPCSTR   sValue;
	DWORD    uValue;
	INT      iValue;
	BOOL     bValue;
} ConfigValue;

typedef struct tagConfigItem
{
	PALCFG_ITEM        Item;
	PALCFG_TYPE        Type;
	const char*        Name;
	int                NameLength;
	const ConfigValue  DefaultValue;
	const ConfigValue  MinValue;
	const ConfigValue  MaxValue;
} ConfigItem;

typedef struct tagSCREENLAYOUT
{
	PAL_POS          EquipImageBox;
	PAL_POS          EquipRoleListBox;
	PAL_POS          EquipItemName;
	PAL_POS          EquipItemAmount;
	PAL_POS          EquipLabels[MAX_PLAYER_EQUIPMENTS];
	PAL_POS          EquipNames[MAX_PLAYER_EQUIPMENTS];
	PAL_POS          EquipStatusLabels[5];
	PAL_POS          EquipStatusValues[5];

	PAL_POS          RoleName;
	PAL_POS          RoleImage;
	PAL_POS          RoleExpLabel;
	PAL_POS          RoleLevelLabel;
	PAL_POS          RoleHPLabel;
	PAL_POS          RoleMPLabel;
	PAL_POS          RoleStatusLabels[5];
	PAL_POS          RoleCurrExp;
	PAL_POS          RoleNextExp;
	PAL_POS          RoleExpSlash;
	PAL_POS          RoleLevel;
	PAL_POS          RoleCurHP;
	PAL_POS          RoleMaxHP;
	PAL_POS          RoleHPSlash;
	PAL_POS          RoleCurMP;
	PAL_POS          RoleMaxMP;
	PAL_POS          RoleMPSlash;
	PAL_POS          RoleStatusValues[5];
	PAL_POS          RoleEquipImageBoxes[MAX_PLAYER_EQUIPMENTS];
	PAL_POS          RoleEquipNames[MAX_PLAYER_EQUIPMENTS];
	PAL_POS          RolePoisonNames[MAX_POISONS];

	PAL_POS          ExtraItemDescLines;
	PAL_POS          ExtraMagicDescLines;
} SCREENLAYOUT;

typedef struct tagCONFIGURATION
{
	union {
		SCREENLAYOUT     ScreenLayout;
		PAL_POS          ScreenLayoutArray[sizeof(SCREENLAYOUT) / sizeof(PAL_POS)];
	};
	enum {
		USE_8x8_FONT = 1,
		DISABLE_SHADOW = 2,
	}                ScreenLayoutFlag[sizeof(SCREENLAYOUT) / sizeof(PAL_POS)];

	/* Configurable options */
	char            *pszGamePath;
	char            *pszMsgFile;
	char            *pszBdfFile;
	CODEPAGE         uCodePage;
	DWORD            dwWordLength;
	DWORD            dwScreenWidth;
	DWORD            dwScreenHeight;
	INT              iSurroundOPLOffset;
	INT              iAudioChannels;
	INT              iSampleRate;
	INT              iOPLSampleRate;
	INT              iResampleQuality;
	INT              iMusicVolume;
	INT              iSoundVolume;
	MUSICTYPE        eMusicType;
	MUSICTYPE        eCDType;
	OPLTYPE          eOPLType;
	WORD             wAudioBufferSize;
	BOOL             fIsWIN95;
	BOOL             fUseEmbeddedFonts;
	BOOL             fUseSurroundOPL;
#if SDL_VERSION_ATLEAST(2,0,0)
	BOOL             fKeepAspectRatio;
#endif
	BOOL             fFullScreen;
	BOOL             fEnableJoyStick;
	BOOL             fUseCustomScreenLayout;
	BOOL             fLaunchSetting;
	BOOL             fUseTouchOverlay;
#if USE_RIX_EXTRA_INIT
	uint32_t        *pExtraFMRegs;
	uint8_t         *pExtraFMVals;
	uint32_t         dwExtraLength;
#endif
} CONFIGURATION, *LPCONFIGURATION;

extern CONFIGURATION gConfig;

VOID
PAL_LoadConfig(
	BOOL fFromFile
);

BOOL
PAL_SaveConfig(
	VOID
);

BOOL
PAL_ParseConfigLine(
	const char * line,
	const ConfigItem ** pItem,
	ConfigValue * pValue
);

ConfigValue
PAL_DefaultConfig(
	PALCFG_ITEM item
);

const char *
PAL_ConfigName(
	PALCFG_ITEM item
);

BOOL
PAL_LimitConfig(
	PALCFG_ITEM item,
	ConfigValue * pValue
);

# ifdef __cplusplus
}
# endif

#endif
