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
// palcfg.c: Configuration definition.
//  @Author: Lou Yihua <louyihua@21cn.com>, 2016.
//

#include "global.h"
#include "palcfg.h"
#include "util.h"
#include "resampler.h"
#include <stdint.h>

#if !defined(PAL_HAS_TOUCH)
#define PAL_HAS_TOUCH     0
#endif

#define MAKE_BOOLEAN(defv, minv, maxv)  { .bValue = defv }, { .bValue = minv }, { .bValue = maxv }
#define MAKE_INTEGER(defv, minv, maxv)  { .iValue = defv }, { .iValue = minv }, { .iValue = maxv }
#define MAKE_UNSIGNED(defv, minv, maxv) { .uValue = defv }, { .uValue = minv }, { .uValue = maxv }
#define MAKE_STRING(defv)               { .sValue = defv }, { .sValue = NULL }, { .sValue = NULL }

static const ConfigItem gConfigItems[PALCFG_ALL_MAX] = {
	{ PALCFG_FULLSCREEN,        PALCFG_BOOLEAN,  "FullScreen",        10, MAKE_BOOLEAN(FALSE,                         FALSE,                 TRUE) },
	{ PALCFG_KEEPASPECTRATIO,   PALCFG_BOOLEAN,  "KeepAspectRatio",   15, MAKE_BOOLEAN(TRUE,                          FALSE,                 TRUE) },
	{ PALCFG_LAUNCHSETTING,     PALCFG_BOOLEAN,  "LaunchSetting",     13, MAKE_BOOLEAN(PAL_HAS_CONFIG_PAGE,           FALSE,                 TRUE) },
	{ PALCFG_STEREO,            PALCFG_BOOLEAN,  "Stereo",             6, MAKE_BOOLEAN(TRUE,                          FALSE,                 TRUE) },                  // Default for stereo audio
	{ PALCFG_USESURROUNDOPL,    PALCFG_BOOLEAN,  "UseSurroundOPL",    14, MAKE_BOOLEAN(TRUE,                          FALSE,                 TRUE) },                  // Default for using surround opl
	{ PALCFG_ENABLEKEYREPEAT,   PALCFG_BOOLEAN,  "EnableKeyRepeat",   15, MAKE_BOOLEAN(FALSE,                         FALSE,                 TRUE) },
	{ PALCFG_USETOUCHOVERLAY,   PALCFG_BOOLEAN,  "UseTouchOverlay",   15, MAKE_BOOLEAN(PAL_HAS_TOUCH,                 FALSE,                 TRUE) },
	{ PALCFG_ENABLEAVIPLAY,     PALCFG_BOOLEAN,  "EnableAviPlay",     13, MAKE_BOOLEAN(TRUE,                          FALSE,                 TRUE) },
	{ PALCFG_ENABLEGLSL,        PALCFG_BOOLEAN,  "EnableGLSL",        10, MAKE_BOOLEAN(FALSE,                         FALSE,                 TRUE) },
    { PALCFG_ENABLEHDR,         PALCFG_BOOLEAN,  "EnableHDR",          9, MAKE_BOOLEAN(FALSE,                         FALSE,                 TRUE) },

	{ PALCFG_SURROUNDOPLOFFSET, PALCFG_INTEGER,  "SurroundOPLOffset", 17, MAKE_INTEGER(384,                           INT32_MIN,             INT32_MAX) },
	{ PALCFG_LOGLEVEL,          PALCFG_INTEGER,  "LogLevel",           8, MAKE_INTEGER(PAL_DEFAULT_LOGLEVEL,          LOGLEVEL_MIN,          LOGLEVEL_MAX) },
	{ PALCFG_AUDIODEVICE,       PALCFG_INTEGER,  "AudioDevice",       11, MAKE_INTEGER(-1,                            INT32_MIN,             INT32_MAX) },

	{ PALCFG_AUDIOBUFFERSIZE,   PALCFG_UNSIGNED, "AudioBufferSize",   15, MAKE_UNSIGNED(PAL_AUDIO_DEFAULT_BUFFER_SIZE, 2,                     32768) },
	{ PALCFG_OPLSAMPLERATE,     PALCFG_UNSIGNED, "OPLSampleRate",     13, MAKE_UNSIGNED(49716,                         0,                     UINT32_MAX) },
	{ PALCFG_RESAMPLEQUALITY,   PALCFG_UNSIGNED, "ResampleQuality",   15, MAKE_UNSIGNED(RESAMPLER_QUALITY_MAX,         RESAMPLER_QUALITY_MIN, RESAMPLER_QUALITY_MAX) }, // Default for best quality
	{ PALCFG_SAMPLERATE,        PALCFG_UNSIGNED, "SampleRate",        10, MAKE_UNSIGNED(44100,                         0,                     PAL_MAX_SAMPLERATE) },
	{ PALCFG_MUSICVOLUME,       PALCFG_UNSIGNED, "MusicVolume",       11, MAKE_UNSIGNED(PAL_MAX_VOLUME,                0,                     PAL_MAX_VOLUME) },        // Default for maximum volume
	{ PALCFG_SOUNDVOLUME,       PALCFG_UNSIGNED, "SoundVolume",       11, MAKE_UNSIGNED(PAL_MAX_VOLUME,                0,                     PAL_MAX_VOLUME) },        // Default for maximum volume
	{ PALCFG_WINDOWHEIGHT,      PALCFG_UNSIGNED, "WindowHeight",      12, MAKE_UNSIGNED(PAL_DEFAULT_WINDOW_HEIGHT,     0,                     UINT32_MAX) },
	{ PALCFG_WINDOWWIDTH,       PALCFG_UNSIGNED, "WindowWidth",       11, MAKE_UNSIGNED(PAL_DEFAULT_WINDOW_WIDTH,      0,                     UINT32_MAX) },
    { PALCFG_TEXTUREHEIGHT,     PALCFG_UNSIGNED, "TextureHeight",     13, MAKE_UNSIGNED(PAL_DEFAULT_TEXTURE_HEIGHT,    0,                     UINT32_MAX) },
    { PALCFG_TEXTUREWIDTH,      PALCFG_UNSIGNED, "TextureWidth",      12, MAKE_UNSIGNED(PAL_DEFAULT_TEXTURE_WIDTH,     0,                     UINT32_MAX) },

	{ PALCFG_CD,                PALCFG_STRING,   "CD",                 2, MAKE_STRING("NONE") },
	{ PALCFG_GAMEPATH,          PALCFG_STRING,   "GamePath",           8, MAKE_STRING(NULL) },
    { PALCFG_SAVEPATH,          PALCFG_STRING,   "SavePath",           8, MAKE_STRING(NULL) },
    { PALCFG_SHADERPATH,        PALCFG_STRING,   "ShaderPath",        10, MAKE_STRING(NULL) },
	{ PALCFG_MESSAGEFILE,       PALCFG_STRING,   "MessageFileName",   15, MAKE_STRING(NULL) },
	{ PALCFG_FONTFILE,          PALCFG_STRING,   "FontFileName",      12, MAKE_STRING(NULL) },
	{ PALCFG_MUSIC,             PALCFG_STRING,   "Music",              5, MAKE_STRING("RIX") },
	{ PALCFG_OPL_CORE,          PALCFG_STRING,   "OPLCore",            7, MAKE_STRING("DBFLT") },
	{ PALCFG_OPL_CHIP,          PALCFG_STRING,   "OPLChip",            7, MAKE_STRING("OPL2") },
	{ PALCFG_LOGFILE,           PALCFG_STRING,   "LogFileName",       11, MAKE_STRING(NULL) },
	{ PALCFG_RIXEXTRAINIT,      PALCFG_STRING,   "RIXExtraInit",      12, MAKE_STRING(NULL) },
	{ PALCFG_MIDICLIENT,        PALCFG_STRING,   "MIDIClient",        10, MAKE_STRING(NULL) },
	{ PALCFG_SCALEQUALITY,      PALCFG_STRING,   "ScaleQuality",      12, MAKE_STRING("0") },
	{ PALCFG_SHADER,            PALCFG_STRING,   "Shader",             6, MAKE_STRING(NULL) },
};

static const char *music_types[] = { "MIDI", "RIX", "MP3", "OGG", "OPUS", "RAW" };
static const char *cd_types[] = { "NONE", "MP3", "OGG", "OPUS", "RAW" };
static const char *opl_cores[] = { "MAME", "DBFLT", "DBINT", "NUKED" };
static const char *opl_chips[] = { "OPL2", "OPL3" };

static char * ParseStringValue(const char *sValue, char *original)
{
	int n = strlen(sValue);
	while (n > 0 && isspace(sValue[n - 1])) n--;
	if (n > 0)
	{
		char *newval = (char *)realloc(original, n + 1);
		memcpy(newval, sValue, n);
		newval[n] = '\0';
		return newval;
	}
	return original;
}

static BOOL
PAL_ParseConfigLine(
	const char * line,
	const ConfigItem ** ppItem,
	ConfigValue * pValue,
	int * sLength
)
{
	//
	// Skip leading spaces
	//
	while (*line && isspace(*line)) line++;

	//
	// Skip comments
	//
	if (*line && *line != '#')
	{
		const char *ptr;
		if ((ptr = strchr(line, '=')) != NULL)
		{
			const char *end = ptr++;

			//
			// Skip tailing spaces
			//
			while (end > line && isspace(end[-1])) end--;

			int len = end - line;

			for (int i = 0; i < sizeof(gConfigItems) / sizeof(ConfigItem); i++)
			{
				if (gConfigItems[i].NameLength == len &&
					SDL_strncasecmp(line, gConfigItems[i].Name, len) == 0)
				{
					if (ppItem) *ppItem = &gConfigItems[i];
					if (pValue)
					{
						switch (gConfigItems[i].Type)
						{
						case PALCFG_UNSIGNED:
							sscanf(ptr, "%u", &pValue->uValue);
							if (pValue->uValue < gConfigItems[i].MinValue.uValue)
								pValue->uValue = gConfigItems[i].MinValue.uValue;
							else if (pValue->uValue > gConfigItems[i].MaxValue.uValue)
								pValue->uValue = gConfigItems[i].MaxValue.uValue;
							break;
						case PALCFG_INTEGER:
							sscanf(ptr, "%d", &pValue->iValue);
							if (pValue->iValue < gConfigItems[i].MinValue.iValue)
								pValue->iValue = gConfigItems[i].MinValue.iValue;
							else if (pValue->iValue > gConfigItems[i].MaxValue.iValue)
								pValue->iValue = gConfigItems[i].MaxValue.iValue;
							break;
						case PALCFG_BOOLEAN:
							sscanf(ptr, "%d", &pValue->iValue);
							pValue->bValue = pValue->iValue ? TRUE : FALSE;
							break;
						case PALCFG_STRING:
							//
							// Skip leading spaces
							//
							while (*ptr && isspace(*ptr)) ptr++;
							pValue->sValue = ptr;
							//
							// Get line length
							//
							while (*ptr && *ptr != '\r' && *ptr != '\n') ptr++;
							if (sLength) *sLength = ptr - pValue->sValue;
							break;
						}
						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}

const char *
PAL_ConfigName(
	PALCFG_ITEM item
)
{
	return gConfigItems[item].Name;
}

PALCFG_ITEM
PAL_ConfigIndex(
	const char  *name
)
{
	for (int index = PALCFG_ALL_MIN; index < PALCFG_ALL_MAX; index++)
	{
		if (SDL_strcasecmp(gConfigItems[index].Name, name) == 0)
		{
			return index;
		}
	}
	return -1;
}

PALCFG_TYPE
PAL_ConfigType(
	PALCFG_ITEM item
)
{
	return gConfigItems[item].Type;
}

BOOL
PAL_LimitConfig(
	PALCFG_ITEM item,
	ConfigValue * pValue
)
{
	if (!pValue) return FALSE;

	switch (gConfigItems[item].Type)
	{
	case PALCFG_UNSIGNED:
		if (pValue->uValue < gConfigItems[item].MinValue.uValue)
		{
			pValue->uValue = gConfigItems[item].MinValue.uValue;
			return TRUE;
		}
		else if (pValue->uValue > gConfigItems[item].MaxValue.uValue)
		{
			pValue->uValue = gConfigItems[item].MaxValue.uValue;
			return TRUE;
		}
		else
			return FALSE;
	case PALCFG_INTEGER:
		if (pValue->iValue < gConfigItems[item].MinValue.iValue)
		{
			pValue->iValue = gConfigItems[item].MinValue.iValue;
			return TRUE;
		}
		else if (pValue->iValue > gConfigItems[item].MaxValue.iValue)
		{
			pValue->iValue = gConfigItems[item].MaxValue.iValue;
			return TRUE;
		}
		else
			return FALSE;
	case PALCFG_BOOLEAN:
		if (pValue->bValue != TRUE && pValue->bValue != FALSE)
		{
			pValue->bValue = pValue->bValue ? TRUE : FALSE;
			return TRUE;
		}
		else
			return FALSE;
	default:
		return FALSE;
	}
}


void
PAL_FreeConfig(
	void
)
{

#if USE_RIX_EXTRA_INIT
	free(gConfig.pExtraFMRegs);
	free(gConfig.pExtraFMVals);
	free(gConfig.dwExtraLength);
#endif
	free(gConfig.pszMsgFile);
	free(gConfig.pszFontFile);
	free(gConfig.pszGamePath);
    free(gConfig.pszSavePath);
    free(gConfig.pszShaderPath);
    free(gConfig.pszScaleQuality);
	free(gConfig.pszLogFile);

	memset(&gConfig, 0, sizeof(CONFIGURATION));
}

void
PAL_LoadConfig(
	BOOL fFromFile
)
{
	FILE     *fp;
	ConfigValue  values[PALCFG_ALL_MAX];
	MUSICTYPE eMusicType = MUSIC_RIX;
	CDTYPE eCDType = CD_NONE;
	OPLCORE_TYPE eOPLCore = OPLCORE_DBFLT;
	OPLCHIP_TYPE eOPLChip = OPLCHIP_OPL2;
	static const SCREENLAYOUT screen_layout = {
		// Equipment Screen
		.EquipImageBox     = PAL_XY(8, 8),
		.EquipRoleListBox  = PAL_XY(2, 95),
		.EquipItemName     = PAL_XY(5, 70),
		.EquipItemAmount   = PAL_XY(51, 57),
		.EquipLabels       = {
			PAL_XY(92, 11), PAL_XY(92, 33),
			PAL_XY(92, 55), PAL_XY(92, 77),
			PAL_XY(92, 99), PAL_XY(92, 121)
		},
		.EquipNames        = {
			PAL_XY(130, 11), PAL_XY(130, 33),
			PAL_XY(130, 55), PAL_XY(130, 77),
			PAL_XY(130, 99), PAL_XY(130, 121)
		},
		.EquipStatusLabels = {
			PAL_XY(226, 10), PAL_XY(226, 32),
			PAL_XY(226, 54), PAL_XY(226, 76),
			PAL_XY(226, 98)
		},
		.EquipStatusValues = {
			PAL_XY(260, 14), PAL_XY(260, 36),
			PAL_XY(260, 58), PAL_XY(260, 80),
			PAL_XY(260, 102)
		},

		// Status Screen
		.RoleName            = PAL_XY(110, 8),
		.RoleImage           = PAL_XY(110, 30),
		.RoleExpLabel        = PAL_XY(6, 6),
		.RoleLevelLabel      = PAL_XY(6, 32),
		.RoleHPLabel         = PAL_XY(6, 54),
		.RoleMPLabel         = PAL_XY(6, 76),
		.RoleStatusLabels    = {
			PAL_XY(6, 98),  PAL_XY(6, 118),
			PAL_XY(6, 138), PAL_XY(6, 158),
			PAL_XY(6, 178)
		},
		.RoleCurrExp         = PAL_XY(58, 6),
		.RoleNextExp         = PAL_XY(58, 15),
		.RoleExpSlash        = PAL_XY(0, 0),
		.RoleLevel           = PAL_XY(54, 35),
		.RoleCurHP           = PAL_XY(42, 56),
		.RoleMaxHP           = PAL_XY(63, 61),
		.RoleHPSlash         = PAL_XY(65, 58),
		.RoleCurMP           = PAL_XY(42, 78),
		.RoleMaxMP           = PAL_XY(63, 83),
		.RoleMPSlash         = PAL_XY(65, 80),
		.RoleStatusValues    = {
			PAL_XY(42, 102), PAL_XY(42, 122),
			PAL_XY(42, 142), PAL_XY(42, 162),
			PAL_XY(42, 182)
		},
		.RoleEquipImageBoxes = {
			PAL_XY(189, -1),  PAL_XY(247, 39),
			PAL_XY(251, 101), PAL_XY(201, 133),
			PAL_XY(141, 141), PAL_XY(81, 125)
		},
		.RoleEquipNames      = {
			PAL_XY(195, 38),  PAL_XY(253, 78),
			PAL_XY(257, 140), PAL_XY(207, 172),
			PAL_XY(147, 180), PAL_XY(87, 164)
		},
		.RolePoisonNames     = {
			PAL_XY(185, 58),  PAL_XY(185, 76),
			PAL_XY(185, 94),  PAL_XY(185, 112),
			PAL_XY(185, 130), PAL_XY(185, 148),
			PAL_XY(185, 166), PAL_XY(185, 184),
			PAL_XY(185, 184), PAL_XY(185, 184)
		},

		// Extra Lines
		.ExtraItemDescLines  = PAL_XY(0, 0),
		.ExtraMagicDescLines = PAL_XY(0, 0),
	};

	for (PALCFG_ITEM i = PALCFG_ALL_MIN; i < PALCFG_ALL_MAX; i++) values[i] = gConfigItems[i].DefaultValue;

	if (fFromFile && (fp = UTIL_OpenFileAtPathForMode(PAL_CONFIG_PREFIX, "sdlpal.cfg", "r")))
	{
		PAL_LARGE char buf[512];

		//
		// Load the configuration data
		//
		while (fgets(buf, 512, fp) != NULL)
		{
			ConfigValue value;
			const ConfigItem * item;
			int slen;
			if (PAL_ParseConfigLine(buf, &item, &value, &slen))
			{
				switch (item->Item)
				{
				case PALCFG_AUDIOBUFFERSIZE:
					if ((value.uValue & (value.uValue - 1)) != 0)
					{
						/* Make sure iAudioBufferSize is power of 2 */
						int n = 0;
						while (value.uValue) { value.uValue >>= 1; n++; }
						value.uValue = 1 << (n - 1);
					}
					values[item->Item] = value;
					break;
				case PALCFG_MESSAGEFILE:
					gConfig.pszMsgFile = ParseStringValue(value.sValue, gConfig.pszMsgFile);
					break;
				case PALCFG_FONTFILE:
					gConfig.pszFontFile = ParseStringValue(value.sValue, gConfig.pszFontFile);
					break;
				case PALCFG_GAMEPATH:
					gConfig.pszGamePath = ParseStringValue(value.sValue, gConfig.pszGamePath);
					break;
				case PALCFG_SAVEPATH:
					gConfig.pszSavePath = ParseStringValue(value.sValue, gConfig.pszSavePath);
					break;
                case PALCFG_SHADERPATH:
                    gConfig.pszShaderPath = ParseStringValue(value.sValue, gConfig.pszShaderPath);
                    break;
				case PALCFG_LOGFILE:
					gConfig.pszLogFile = ParseStringValue(value.sValue, gConfig.pszLogFile);
					break;
				case PALCFG_CD:
				{
					if (PAL_HAS_MP3 && SDL_strncasecmp(value.sValue, "MP3", slen) == 0)
						eCDType = CD_MP3;
					else if (PAL_HAS_OGG && SDL_strncasecmp(value.sValue, "OGG", slen) == 0)
						eCDType = CD_OGG;
					else if (PAL_HAS_OPUS && SDL_strncasecmp(value.sValue, "OPUS", slen) == 0)
						eCDType = CD_OPUS;
					else if (PAL_HAS_SDLCD && SDL_strncasecmp(value.sValue, "RAW", slen) == 0)
						eCDType = CD_SDLCD;
					break;
				}
				case PALCFG_MUSIC:
				{
					if (PAL_HAS_NATIVEMIDI && SDL_strncasecmp(value.sValue, "MIDI", slen) == 0)
						eMusicType = MUSIC_MIDI;
					else if (PAL_HAS_MP3 && SDL_strncasecmp(value.sValue, "MP3", slen) == 0)
						eMusicType = MUSIC_MP3;
					else if (PAL_HAS_OGG && SDL_strncasecmp(value.sValue, "OGG", slen) == 0)
						eMusicType = MUSIC_OGG;
					else if (PAL_HAS_OPUS && SDL_strncasecmp(value.sValue, "OPUS", slen) == 0)
						eMusicType = MUSIC_OPUS;
					else if (SDL_strncasecmp(value.sValue, "RIX", slen) == 0)
						eMusicType = MUSIC_RIX;
					break;
				}
				case PALCFG_OPL_CORE:
				{
					if (SDL_strncasecmp(value.sValue, "DBINT", slen) == 0)
						eOPLCore = OPLCORE_DBINT;
					else if (SDL_strncasecmp(value.sValue, "DBFLT", slen) == 0)
						eOPLCore = OPLCORE_DBFLT;
					else if (SDL_strncasecmp(value.sValue, "MAME", slen) == 0)
						eOPLCore = OPLCORE_MAME;
					else if (SDL_strncasecmp(value.sValue, "NUKED", slen) == 0)
						eOPLCore = OPLCORE_NUKED;
					break;
				}
				case PALCFG_OPL_CHIP:
				{
					if (SDL_strncasecmp(value.sValue, "OPL2", slen) == 0)
						eOPLChip = OPLCHIP_OPL2;
					else if (SDL_strncasecmp(value.sValue, "OPL3", slen) == 0)
						eOPLChip = OPLCHIP_OPL3;
					break;
				}
				case PALCFG_RIXEXTRAINIT:
				{
#if USE_RIX_EXTRA_INIT
					int n = 1;
					char *p;
					for (p = ptr; *p < *end; p++)
					{
						if (*p == ',')
							n++;
					}
					n &= ~0x1;

					if (n > 0)
					{
						uint32_t *regs = malloc(sizeof(uint32_t) * (n >> 1));
						uint8_t *vals = malloc(sizeof(uint8_t) * (n >> 1));
						uint32_t d, i, v = 1;
						if (regs && vals)
						{
							for (p = ptr, i = 0; *p < *end; p++, i++)
							{
								if (sscanf(p, "%u", &regs[i]) == 0) { v = 0; break; }
								while (*p < *end && *p != ',') p++; p++;
								if (sscanf(p, "%u", &d) == 0) { v = 0; break; }
								while (*p < *end && *p != ',') p++;
								vals[i] = (uint8_t)d;
							}
							if (v)
							{
								gConfig.pExtraFMRegs = regs;
								gConfig.pExtraFMVals = vals;
								gConfig.dwExtraLength = n >> 1;
							}
							else
							{
								free(regs);
								free(vals);
							}
						}
					}
#endif
					break;
				}
				case PALCFG_MIDICLIENT:
					gConfig.pszMIDIClient = ParseStringValue(value.sValue, gConfig.pszMIDIClient);
					break;
				case PALCFG_SCALEQUALITY:
					gConfig.pszScaleQuality = ParseStringValue(value.sValue, gConfig.pszScaleQuality);
					break;
				case PALCFG_SHADER:
					gConfig.pszShader = ParseStringValue(value.sValue, gConfig.pszShader);
					break;
				default:
					values[item->Item] = value;
					break;
				}
			}
		}

		UTIL_CloseFile(fp);
	}

	//
	// Set configurable global options
	//
	if (!gConfig.pszSavePath) gConfig.pszSavePath = gConfig.pszGamePath ? strdup(gConfig.pszGamePath) : strdup(PAL_SAVE_PREFIX);
	if (!gConfig.pszGamePath) gConfig.pszGamePath = strdup(PAL_PREFIX);
    if (!gConfig.pszShaderPath) gConfig.pszShaderPath = strdup(gConfig.pszGamePath);
	gConfig.eMusicType = eMusicType;
	gConfig.eCDType = eCDType;
	gConfig.eOPLCore = eOPLCore;
	gConfig.eOPLChip = (eOPLCore == OPLCORE_NUKED ? OPLCHIP_OPL3 : eOPLChip);
	gConfig.dwWordLength = 10;	// This is the default value for Chinese version
	gConfig.ScreenLayout = screen_layout;

	gConfig.fIsWIN95 = FALSE;	// Default for DOS version
	gConfig.fUseSurroundOPL = values[PALCFG_STEREO].bValue && values[PALCFG_USESURROUNDOPL].bValue;
	gConfig.fLaunchSetting = values[PALCFG_LAUNCHSETTING].bValue;
	gConfig.fEnableKeyRepeat = values[PALCFG_ENABLEKEYREPEAT].bValue;
	gConfig.fUseTouchOverlay = values[PALCFG_USETOUCHOVERLAY].bValue;
	gConfig.fKeepAspectRatio = values[PALCFG_KEEPASPECTRATIO].bValue;
	gConfig.fFullScreen = values[PALCFG_FULLSCREEN].bValue;
	gConfig.fEnableAviPlay = values[PALCFG_ENABLEAVIPLAY].bValue;
	gConfig.fEnableGLSL = values[PALCFG_ENABLEGLSL].bValue;
    gConfig.fEnableHDR = values[PALCFG_ENABLEHDR].bValue;
	gConfig.iAudioChannels = values[PALCFG_STEREO].bValue ? 2 : 1;

	gConfig.iSurroundOPLOffset = values[PALCFG_SURROUNDOPLOFFSET].iValue;
	gConfig.iLogLevel = values[PALCFG_LOGLEVEL].iValue;
	gConfig.iAudioDevice = values[PALCFG_AUDIODEVICE].iValue;

	gConfig.iSampleRate = values[PALCFG_SAMPLERATE].uValue;
	gConfig.iOPLSampleRate = values[PALCFG_OPLSAMPLERATE].uValue;
	gConfig.iResampleQuality = values[PALCFG_RESAMPLEQUALITY].uValue;
	gConfig.wAudioBufferSize = (WORD)values[PALCFG_AUDIOBUFFERSIZE].uValue;
	gConfig.iMusicVolume = values[PALCFG_MUSICVOLUME].uValue;
	gConfig.iSoundVolume = values[PALCFG_SOUNDVOLUME].uValue;

	gConfig.dwTextureWidth  = values[PALCFG_TEXTUREWIDTH].uValue;
	gConfig.dwTextureHeight = values[PALCFG_TEXTUREHEIGHT].uValue;

	if (UTIL_GetScreenSize(&values[PALCFG_WINDOWWIDTH].uValue, &values[PALCFG_WINDOWHEIGHT].uValue))
	{
		gConfig.dwScreenWidth = values[PALCFG_WINDOWWIDTH].uValue;
		gConfig.dwScreenHeight = values[PALCFG_WINDOWHEIGHT].uValue;
	}
	else
	{
		gConfig.dwScreenWidth = PAL_DEFAULT_WINDOW_WIDTH;
		gConfig.dwScreenHeight = PAL_DEFAULT_WINDOW_HEIGHT;
	}
    
    if( gConfig.dwTextureWidth == 0 && gConfig.dwTextureHeight == 0 ) {
        gConfig.dwTextureWidth = PAL_DEFAULT_TEXTURE_WIDTH;
        gConfig.dwTextureHeight = PAL_DEFAULT_TEXTURE_HEIGHT;
    }
    
    if(gConfig.fEnableGLSL && !gConfig.pszShader) {
        UTIL_LogOutput(LOGLEVEL_ERROR, "Filter backend GLSL enabled but no valid effect file specified");
        gConfig.fEnableGLSL = FALSE;
    }
}


BOOL
PAL_SaveConfig(
	void
)
{
	char buf[512];
	FILE *fp = UTIL_OpenFileAtPathForMode(PAL_CONFIG_PREFIX, "sdlpal.cfg", "w");

	if (fp)
	{
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_KEEPASPECTRATIO), gConfig.fKeepAspectRatio); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_FULLSCREEN), gConfig.fFullScreen); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_LAUNCHSETTING), gConfig.fLaunchSetting); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_STEREO), gConfig.iAudioChannels == 2 ? TRUE : FALSE); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_USESURROUNDOPL), gConfig.fUseSurroundOPL); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_ENABLEKEYREPEAT), gConfig.fEnableKeyRepeat); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_USETOUCHOVERLAY), gConfig.fUseTouchOverlay); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_ENABLEAVIPLAY), gConfig.fEnableAviPlay); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_ENABLEGLSL), gConfig.fEnableGLSL); fputs(buf, fp);
        sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_ENABLEHDR), gConfig.fEnableHDR); fputs(buf, fp);

		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_SURROUNDOPLOFFSET), gConfig.iSurroundOPLOffset); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_LOGLEVEL), gConfig.iLogLevel); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_AUDIODEVICE), gConfig.iAudioDevice); fputs(buf, fp);

		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_AUDIOBUFFERSIZE), gConfig.wAudioBufferSize); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_OPLSAMPLERATE), gConfig.iOPLSampleRate); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_RESAMPLEQUALITY), gConfig.iResampleQuality); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_SAMPLERATE), gConfig.iSampleRate); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_MUSICVOLUME), gConfig.iMusicVolume); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_SOUNDVOLUME), gConfig.iSoundVolume); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_WINDOWHEIGHT), gConfig.dwScreenHeight); fputs(buf, fp);
        sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_WINDOWWIDTH), gConfig.dwScreenWidth); fputs(buf, fp);
        sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_TEXTUREHEIGHT), gConfig.dwTextureHeight); fputs(buf, fp);
        sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_TEXTUREWIDTH), gConfig.dwTextureWidth); fputs(buf, fp);

		sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_CD), cd_types[gConfig.eCDType]); fputs(buf, fp);
		sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_MUSIC), music_types[gConfig.eMusicType]); fputs(buf, fp);
		sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_OPL_CORE), opl_cores[gConfig.eOPLCore]); fputs(buf, fp);
		sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_OPL_CHIP), opl_chips[gConfig.eOPLChip]); fputs(buf, fp);

		if (gConfig.pszGamePath && *gConfig.pszGamePath && strcmp(gConfig.pszGamePath, PAL_PREFIX) != 0) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_GAMEPATH), gConfig.pszGamePath); fputs(buf, fp); }
		if (gConfig.pszSavePath && *gConfig.pszSavePath && strcmp(gConfig.pszSavePath, PAL_SAVE_PREFIX) != 0) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_SAVEPATH), gConfig.pszSavePath); fputs(buf, fp); }
        if (gConfig.pszShaderPath && *gConfig.pszShaderPath && gConfig.pszGamePath && strcmp(gConfig.pszShaderPath, gConfig.pszGamePath) != 0) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_SHADERPATH), gConfig.pszShaderPath); fputs(buf, fp); }
		if (gConfig.pszMsgFile && *gConfig.pszMsgFile) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_MESSAGEFILE), gConfig.pszMsgFile); fputs(buf, fp); }
		if (gConfig.pszFontFile && *gConfig.pszFontFile) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_FONTFILE), gConfig.pszFontFile); fputs(buf, fp); }
		if (gConfig.pszLogFile && *gConfig.pszLogFile) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_LOGFILE), gConfig.pszLogFile); fputs(buf, fp); }
		if (gConfig.pszMIDIClient && *gConfig.pszMIDIClient) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_MIDICLIENT), gConfig.pszMIDIClient); fputs(buf, fp); }
		if (gConfig.pszScaleQuality && *gConfig.pszScaleQuality) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_SCALEQUALITY), gConfig.pszScaleQuality); fputs(buf, fp); }
		if (gConfig.pszShader && *gConfig.pszShader) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_SHADER), gConfig.pszShader); fputs(buf, fp); }

		fclose(fp);

		return TRUE;
	}
	else
		return FALSE;
}


ConfigValue
PAL_GetConfigItem(
	PALCFG_ITEM   item,
	BOOL default_value
)
{
	ConfigValue value = gConfigItems[item].DefaultValue;
	if (!default_value)
	{
		switch (item)
		{
		case PALCFG_FULLSCREEN:        value.bValue = gConfig.fFullScreen; break;
		case PALCFG_KEEPASPECTRATIO:   value.bValue = gConfig.fKeepAspectRatio; break;
		case PALCFG_LAUNCHSETTING:     value.bValue = gConfig.fLaunchSetting; break;
		case PALCFG_STEREO:            value.bValue = (gConfig.iAudioChannels == 2); break;
		case PALCFG_USESURROUNDOPL:    value.bValue = gConfig.fUseSurroundOPL; break;
		case PALCFG_ENABLEKEYREPEAT:   value.bValue = gConfig.fEnableKeyRepeat; break;
		case PALCFG_USETOUCHOVERLAY:   value.bValue = gConfig.fUseTouchOverlay; break;
		case PALCFG_ENABLEAVIPLAY:     value.bValue = gConfig.fEnableAviPlay; break;
		case PALCFG_ENABLEGLSL:        value.bValue = gConfig.fEnableGLSL; break;
		case PALCFG_ENABLEHDR:        value.bValue = gConfig.fEnableHDR; break;
		case PALCFG_SURROUNDOPLOFFSET: value.iValue = gConfig.iSurroundOPLOffset; break;
		case PALCFG_LOGLEVEL:          value.iValue = gConfig.iLogLevel; break;
		case PALCFG_AUDIODEVICE:       value.iValue = gConfig.iAudioDevice; break;

		case PALCFG_AUDIOBUFFERSIZE:   value.uValue = gConfig.wAudioBufferSize; break;
		case PALCFG_OPLSAMPLERATE:     value.uValue = gConfig.iOPLSampleRate; break;
		case PALCFG_RESAMPLEQUALITY:   value.uValue = gConfig.iResampleQuality; break;
		case PALCFG_SAMPLERATE:        value.uValue = gConfig.iSampleRate; break;
		case PALCFG_MUSICVOLUME:       value.uValue = gConfig.iMusicVolume; break;
		case PALCFG_SOUNDVOLUME:       value.uValue = gConfig.iSoundVolume; break;
		case PALCFG_WINDOWHEIGHT:      value.uValue = gConfig.dwScreenHeight; break;
		case PALCFG_WINDOWWIDTH:       value.uValue = gConfig.dwScreenWidth; break;

		case PALCFG_CD:                value.sValue = cd_types[gConfig.eCDType]; break;
		case PALCFG_GAMEPATH:          value.sValue = gConfig.pszGamePath; break;
		case PALCFG_SAVEPATH:          value.sValue = gConfig.pszSavePath; break;
        case PALCFG_SHADERPATH:        value.sValue = gConfig.pszShaderPath; break;
		case PALCFG_MESSAGEFILE:       value.sValue = gConfig.pszMsgFile; break;
		case PALCFG_FONTFILE:          value.sValue = gConfig.pszFontFile; break;
		case PALCFG_LOGFILE:           value.sValue = gConfig.pszLogFile; break;
		case PALCFG_MIDICLIENT:        value.sValue = gConfig.pszMIDIClient; break;
		case PALCFG_SCALEQUALITY:      value.sValue = gConfig.pszScaleQuality; break;
		case PALCFG_SHADER:            value.sValue = gConfig.pszShader; break;
		case PALCFG_MUSIC:             value.sValue = music_types[gConfig.eMusicType]; break;
		case PALCFG_OPL_CORE:          value.sValue = opl_cores[gConfig.eOPLCore]; break;
		case PALCFG_OPL_CHIP:          value.sValue = opl_chips[gConfig.eOPLChip]; break;
		case PALCFG_TEXTUREHEIGHT:     value.uValue = gConfig.dwTextureHeight; break;
		case PALCFG_TEXTUREWIDTH:      value.uValue = gConfig.dwTextureWidth; break;
		default:                       break;
		}
	}
	return value;
}


void
PAL_SetConfigItem(
	PALCFG_ITEM       item,
	const ConfigValue value
)
{
	switch (item)
	{
	case PALCFG_FULLSCREEN:        gConfig.fFullScreen = value.bValue; break;
	case PALCFG_KEEPASPECTRATIO:   gConfig.fKeepAspectRatio = value.bValue; break;
	case PALCFG_LAUNCHSETTING:     gConfig.fLaunchSetting = value.bValue; break;
	case PALCFG_STEREO:            gConfig.iAudioChannels = value.bValue ? 2 : 1; break;
	case PALCFG_USESURROUNDOPL:    gConfig.fUseSurroundOPL = value.bValue; break;
	case PALCFG_ENABLEKEYREPEAT:   gConfig.fEnableKeyRepeat = value.bValue; break;
	case PALCFG_USETOUCHOVERLAY:   gConfig.fUseTouchOverlay = value.bValue; break;
	case PALCFG_ENABLEAVIPLAY:     gConfig.fEnableAviPlay = value.bValue; break;
	case PALCFG_ENABLEGLSL:        gConfig.fEnableGLSL = value.bValue; break;
	case PALCFG_ENABLEHDR:         gConfig.fEnableHDR = value.bValue; break;
	case PALCFG_SURROUNDOPLOFFSET: gConfig.iSurroundOPLOffset = value.iValue; break;
	case PALCFG_LOGLEVEL:          gConfig.iLogLevel = value.iValue; break;
	case PALCFG_AUDIODEVICE:       gConfig.iAudioDevice = value.iValue; break;

	case PALCFG_AUDIOBUFFERSIZE:   gConfig.wAudioBufferSize = value.uValue; break;
	case PALCFG_OPLSAMPLERATE:     gConfig.iOPLSampleRate = value.uValue; break;
	case PALCFG_RESAMPLEQUALITY:   gConfig.iResampleQuality = value.uValue; break;
	case PALCFG_SAMPLERATE:        gConfig.iSampleRate = value.uValue; break;
	case PALCFG_MUSICVOLUME:       gConfig.iMusicVolume = value.uValue; break;
	case PALCFG_SOUNDVOLUME:       gConfig.iSoundVolume = value.uValue; break;
	case PALCFG_WINDOWHEIGHT:      gConfig.dwScreenHeight = value.uValue; break;
	case PALCFG_WINDOWWIDTH:       gConfig.dwScreenWidth = value.uValue; break;

	case PALCFG_TEXTUREHEIGHT:     gConfig.dwTextureHeight = value.uValue; break;
	case PALCFG_TEXTUREWIDTH:      gConfig.dwTextureWidth = value.uValue; break;
	case PALCFG_GAMEPATH:
		if (gConfig.pszGamePath) free(gConfig.pszGamePath);
		gConfig.pszGamePath = value.sValue && value.sValue[0] ? strdup(value.sValue) : strdup(PAL_PREFIX);
		break;
	case PALCFG_SAVEPATH:
		if (gConfig.pszSavePath) free(gConfig.pszSavePath);
		gConfig.pszSavePath = value.sValue && value.sValue[0] ? strdup(value.sValue) : (gConfig.pszGamePath ? strdup(gConfig.pszGamePath) : strdup(PAL_SAVE_PREFIX));
		break;
    case PALCFG_SHADERPATH:
        if (gConfig.pszShaderPath) free(gConfig.pszShaderPath);
        gConfig.pszShaderPath = value.sValue && value.sValue[0] ? strdup(value.sValue) : strdup(PAL_PREFIX);
        break;
	case PALCFG_MESSAGEFILE:
		if (gConfig.pszMsgFile) free(gConfig.pszMsgFile);
		gConfig.pszMsgFile = value.sValue && value.sValue[0] ? strdup(value.sValue) : NULL;
		break;
	case PALCFG_FONTFILE:
		if (gConfig.pszFontFile) free(gConfig.pszFontFile);
		gConfig.pszFontFile = value.sValue && value.sValue[0] ? strdup(value.sValue) : NULL;
		break;
	case PALCFG_LOGFILE:
		if (gConfig.pszLogFile) free(gConfig.pszLogFile);
		gConfig.pszLogFile = value.sValue && value.sValue[0] ? strdup(value.sValue) : NULL;
		break;
	case PALCFG_MIDICLIENT:
		if (gConfig.pszMIDIClient) free(gConfig.pszMIDIClient);
		gConfig.pszMIDIClient = value.sValue && value.sValue[0] ? strdup(value.sValue) : NULL;
		break;
	case PALCFG_SCALEQUALITY:
		if (gConfig.pszScaleQuality) free(gConfig.pszScaleQuality);
		gConfig.pszScaleQuality = value.sValue && value.sValue[0] ? strdup(value.sValue) : NULL;
		break;
	case PALCFG_SHADER:
		if (gConfig.pszShader) free(gConfig.pszShader);
		gConfig.pszShader = value.sValue && value.sValue[0] ? strdup(value.sValue) : NULL;
		break;
	case PALCFG_CD:
		for (int i = 0; i < sizeof(cd_types) / sizeof(cd_types[0]); i++)
		{
			if (SDL_strcasecmp(value.sValue, cd_types[i]) == 0)
			{
				gConfig.eCDType = (CDTYPE)i;
				return;
			}
		}
		break;
	case PALCFG_MUSIC:
		for (int i = 0; i < sizeof(music_types) / sizeof(music_types[0]); i++)
		{
			if (SDL_strcasecmp(value.sValue, music_types[i]) == 0)
			{
				gConfig.eMusicType = (MUSICTYPE)i;
				return;
			}
		}
		break;
	case PALCFG_OPL_CORE:
		for (int i = 0; i < sizeof(opl_cores) / sizeof(opl_cores[0]); i++)
		{
			if (SDL_strcasecmp(value.sValue, opl_cores[i]) == 0)
			{
				gConfig.eOPLCore = (OPLCORE_TYPE)i;
				return;
			}
		}
		break;
	case PALCFG_OPL_CHIP:
		for (int i = 0; i < sizeof(opl_chips) / sizeof(opl_chips[0]); i++)
		{
			if (SDL_strcasecmp(value.sValue, opl_chips[i]) == 0)
			{
				gConfig.eOPLChip = (OPLCHIP_TYPE)i;
				return;
			}
		}
		break;
	default:
		break;
	}
}

BOOL
PAL_GetConfigBoolean(
	PALCFG_ITEM item,
	BOOL        default_value
)
{
	return gConfigItems[item].Type == PALCFG_BOOLEAN ? PAL_GetConfigItem(item, default_value).bValue : FALSE;
}

long
PAL_GetConfigNumber(
	PALCFG_ITEM item,
	BOOL        default_value
)
{
	switch (gConfigItems[item].Type)
	{
	case PALCFG_INTEGER:  return (long)PAL_GetConfigItem(item, default_value).iValue;
	case PALCFG_UNSIGNED: return (long)PAL_GetConfigItem(item, default_value).uValue;
	default:              return 0;
	}
}

int
PAL_GetConfigInteger(
	PALCFG_ITEM item,
	BOOL        default_value
)
{
	return gConfigItems[item].Type == PALCFG_INTEGER ? PAL_GetConfigItem(item, default_value).iValue : 0;
}

unsigned int
PAL_GetConfigUnsigned(
	PALCFG_ITEM item,
	BOOL        default_value
)
{
	return gConfigItems[item].Type == PALCFG_UNSIGNED ? PAL_GetConfigItem(item, default_value).uValue : 0;
}

const char *
PAL_GetConfigString(
	PALCFG_ITEM item,
	BOOL        default_value
)
{
	return gConfigItems[item].Type == PALCFG_STRING ? PAL_GetConfigItem(item, default_value).sValue : NULL;
}

BOOL
PAL_SetConfigBoolean(
	PALCFG_ITEM item,
	BOOL        value
)
{
	if (gConfigItems[item].Type == PALCFG_BOOLEAN)
	{
		ConfigValue val = { .bValue = value };
		PAL_SetConfigItem(item, val);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL
PAL_SetConfigNumber(
	PALCFG_ITEM item,
	long        value
)
{
	switch (gConfigItems[item].Type)
	{
	case PALCFG_UNSIGNED: return PAL_SetConfigUnsigned(item, (unsigned int)value);
	case PALCFG_INTEGER:  return PAL_SetConfigInteger(item, (int)value);
	default:              return FALSE;
	}
}

BOOL
PAL_SetConfigInteger(
	PALCFG_ITEM item,
	int         value
)
{
	if (gConfigItems[item].Type == PALCFG_INTEGER)
	{
		ConfigValue val = { .iValue = value };
		PAL_SetConfigItem(item, val);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL
PAL_SetConfigUnsigned(
	PALCFG_ITEM  item,
	unsigned int value
)
{
	if (gConfigItems[item].Type == PALCFG_UNSIGNED)
	{
		ConfigValue val = { .uValue = value };
		PAL_SetConfigItem(item, val);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL
PAL_SetConfigString(
	PALCFG_ITEM item,
	const char *value
)
{
	if (gConfigItems[item].Type == PALCFG_STRING)
	{
		ConfigValue val = { .sValue = value };
		PAL_SetConfigItem(item, val);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}
