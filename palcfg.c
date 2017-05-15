/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2017, SDLPAL development team.
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

#define MAKE_VALUE(defv, minv, maxv) {(LPCSTR)(defv)}, {(LPCSTR)(minv)}, {(LPCSTR)(maxv)}

static const ConfigItem gConfigItems[PALCFG_ALL_MAX] = {
	{ PALCFG_FULLSCREEN,        PALCFG_BOOLEAN,  "FullScreen",        10, MAKE_VALUE(FALSE,                         FALSE,                 TRUE) },
	{ PALCFG_KEEPASPECTRATIO,   PALCFG_BOOLEAN,  "KeepAspectRatio",   15, MAKE_VALUE(TRUE,                          FALSE,                 TRUE) },
	{ PALCFG_LAUNCHSETTING,     PALCFG_BOOLEAN,  "LaunchSetting",     13, MAKE_VALUE(PAL_HAS_CONFIG_PAGE,           FALSE,                 TRUE) },
	{ PALCFG_STEREO,            PALCFG_BOOLEAN,  "Stereo",             6, MAKE_VALUE(TRUE,                          FALSE,                 TRUE) },                  // Default for stereo audio
	{ PALCFG_USEEMBEDDEDFONTS,  PALCFG_BOOLEAN,  "UseEmbeddedFonts",  16, MAKE_VALUE(TRUE,                          FALSE,                 TRUE) },                  // Default for using embedded fonts in DOS version
	{ PALCFG_USESURROUNDOPL,    PALCFG_BOOLEAN,  "UseSurroundOPL",    14, MAKE_VALUE(TRUE,                          FALSE,                 TRUE) },                  // Default for using surround opl
	{ PALCFG_USETOUCHOVERLAY,   PALCFG_BOOLEAN,  "UseTouchOverlay",   15, MAKE_VALUE(PAL_HAS_TOUCH,                 FALSE,                 TRUE) },

	{ PALCFG_SURROUNDOPLOFFSET, PALCFG_INTEGER,  "SurroundOPLOffset", 17, MAKE_VALUE(384,                           INT32_MIN,             INT32_MAX) },
	{ PALCFG_LOGLEVEL,          PALCFG_INTEGER,  "LogLevel",           8, MAKE_VALUE(PAL_DEFAULT_LOGLEVEL,          LOGLEVEL_MIN,          LOGLEVEL_MAX) },

	{ PALCFG_AUDIOBUFFERSIZE,   PALCFG_UNSIGNED, "AudioBufferSize",   15, MAKE_VALUE(PAL_AUDIO_DEFAULT_BUFFER_SIZE, 2,                     32768) },
	{ PALCFG_CODEPAGE,          PALCFG_UNSIGNED, "CodePage",           8, MAKE_VALUE(CP_BIG5,                       CP_MIN,                CP_MAX - 1) },            // Default for BIG5
	{ PALCFG_OPLSAMPLERATE,     PALCFG_UNSIGNED, "OPLSampleRate",     13, MAKE_VALUE(49716,                         0,                     UINT32_MAX) },
	{ PALCFG_RESAMPLEQUALITY,   PALCFG_UNSIGNED, "ResampleQuality",   15, MAKE_VALUE(RESAMPLER_QUALITY_MAX,         RESAMPLER_QUALITY_MIN, RESAMPLER_QUALITY_MAX) }, // Default for best quality
	{ PALCFG_SAMPLERATE,        PALCFG_UNSIGNED, "SampleRate",        10, MAKE_VALUE(44100,                         0,                     PAL_MAX_SAMPLERATE) },
	{ PALCFG_MUSICVOLUME,       PALCFG_UNSIGNED, "MusicVolume",       11, MAKE_VALUE(PAL_MAX_VOLUME,                0,                     PAL_MAX_VOLUME) },        // Default for maximum volume
	{ PALCFG_SOUNDVOLUME,       PALCFG_UNSIGNED, "SoundVolume",       11, MAKE_VALUE(PAL_MAX_VOLUME,                0,                     PAL_MAX_VOLUME) },        // Default for maximum volume
	{ PALCFG_WINDOWHEIGHT,      PALCFG_UNSIGNED, "WindowHeight",      12, MAKE_VALUE(PAL_DEFAULT_WINDOW_HEIGHT,     0,                     UINT32_MAX) },
	{ PALCFG_WINDOWWIDTH,       PALCFG_UNSIGNED, "WindowWidth",       11, MAKE_VALUE(PAL_DEFAULT_WINDOW_WIDTH,      0,                     UINT32_MAX) },

	{ PALCFG_CD,                PALCFG_STRING,   "CD",                 2, MAKE_VALUE("OGG",    NULL, NULL) },
	{ PALCFG_GAMEPATH,          PALCFG_STRING,   "GamePath",           8, MAKE_VALUE(NULL,     NULL, NULL) },
	{ PALCFG_SAVEPATH,          PALCFG_STRING,   "SavePath",           8, MAKE_VALUE(NULL,     NULL, NULL) },
	{ PALCFG_MESSAGEFILE,       PALCFG_STRING,   "MessageFileName",   15, MAKE_VALUE(NULL,     NULL, NULL) },
	{ PALCFG_BDFFILE,           PALCFG_STRING,   "BDFFileName",       11, MAKE_VALUE(NULL,     NULL, NULL) },
	{ PALCFG_MUSIC,             PALCFG_STRING,   "Music",              5, MAKE_VALUE("RIX",    NULL, NULL) },
	{ PALCFG_OPL,               PALCFG_STRING,   "OPL",                3, MAKE_VALUE("DOSBOX", NULL, NULL) },
	{ PALCFG_LOGFILE,           PALCFG_STRING,   "LogFile",            7, MAKE_VALUE(NULL,     NULL, NULL) },
	{ PALCFG_RIXEXTRAINIT,      PALCFG_STRING,   "RIXExtraInit",      12, MAKE_VALUE(NULL,     NULL, NULL) },
	{ PALCFG_CLIMIDIPLAYER,     PALCFG_STRING,   "CLIMIDIPlayer",     13, MAKE_VALUE(NULL,     NULL, NULL) },
};

static const char *music_types[] = { "MIDI", "RIX", "MP3", "OGG", "RAW" };
static const char *opl_types[] = { "DOSBOX", "MAME", "DOSBOXNEW" };


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

BOOL
PAL_ParseConfigLine(
	const char * line,
	const ConfigItem ** ppItem,
	ConfigValue * pValue
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
							sscanf(ptr, "%d", &pValue->bValue);
							pValue->bValue = pValue->bValue ? TRUE : FALSE;
							break;
						case PALCFG_STRING:
							//
							// Skip leading spaces
							//
							while (*ptr && isspace(*ptr)) ptr++;
							pValue->sValue = ptr;
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

ConfigValue
PAL_DefaultConfig(
	PALCFG_ITEM item
)
{
	return gConfigItems[item].DefaultValue;
}

const char *
PAL_ConfigName(
	PALCFG_ITEM item
)
{
	return gConfigItems[item].Name;
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
	free(gConfig.pszBdfFile);
	free(gConfig.pszGamePath);
	free(gConfig.pszSavePath);
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
	MUSICTYPE eCDType = MUSIC_OGG;
	OPLTYPE   eOPLType = OPL_DOSBOX;
	SCREENLAYOUT screen_layout = {
		// Equipment Screen
		PAL_XY(8, 8), PAL_XY(2, 95), PAL_XY(5, 70), PAL_XY(51, 57),
		{ PAL_XY(92, 11), PAL_XY(92, 33), PAL_XY(92, 55), PAL_XY(92, 77), PAL_XY(92, 99), PAL_XY(92, 121) },
		{ PAL_XY(130, 11), PAL_XY(130, 33), PAL_XY(130, 55), PAL_XY(130, 77), PAL_XY(130, 99), PAL_XY(130, 121) },
		{ PAL_XY(226, 10), PAL_XY(226, 32), PAL_XY(226, 54), PAL_XY(226, 76), PAL_XY(226, 98) },
		{ PAL_XY(260, 14), PAL_XY(260, 36), PAL_XY(260, 58), PAL_XY(260, 80), PAL_XY(260, 102) },

		// Status Screen
		PAL_XY(110, 8), PAL_XY(110, 30), PAL_XY(6, 6),  PAL_XY(6, 32),  PAL_XY(6, 54),  PAL_XY(6, 76),
		{ PAL_XY(6, 98),   PAL_XY(6, 118),  PAL_XY(6, 138),  PAL_XY(6, 158),  PAL_XY(6, 178) },
		PAL_XY(58, 6), PAL_XY(58, 15), PAL_XY(0, 0), PAL_XY(54, 35), PAL_XY(42, 56),
		PAL_XY(63, 61), PAL_XY(65, 58), PAL_XY(42, 78), PAL_XY(63, 83), PAL_XY(65, 80),
		{ PAL_XY(42, 102), PAL_XY(42, 122), PAL_XY(42, 142), PAL_XY(42, 162), PAL_XY(42, 182) },
		{ PAL_XY(189, -1), PAL_XY(247, 39), PAL_XY(251, 101), PAL_XY(201, 133), PAL_XY(141, 141), PAL_XY(81, 125) },
		{ PAL_XY(195, 38), PAL_XY(253, 78), PAL_XY(257, 140), PAL_XY(207, 172), PAL_XY(147, 180), PAL_XY(87, 164) },
		{ PAL_XY(185, 58), PAL_XY(185, 76), PAL_XY(185, 94), PAL_XY(185, 112), PAL_XY(185, 130), PAL_XY(185, 148), PAL_XY(185, 166), PAL_XY(185, 184), PAL_XY(185, 184), PAL_XY(185, 184) },

		// Extra Lines
		PAL_XY(0, 0), PAL_XY(0, 0)
	};

	for (PALCFG_ITEM i = PALCFG_ALL_MIN; i < PALCFG_ALL_MAX; i++) values[i] = PAL_DefaultConfig(i);

	if (fFromFile && (fp = fopen(va("%ssdlpal.cfg", PAL_CONFIG_PREFIX), "r")))
	{
		PAL_LARGE char buf[512];

		//
		// Load the configuration data
		//
		while (fgets(buf, 512, fp) != NULL)
		{
			ConfigValue value;
			const ConfigItem * item;
			if (PAL_ParseConfigLine(buf, &item, &value))
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
				case PALCFG_BDFFILE:
					gConfig.pszBdfFile = ParseStringValue(value.sValue, gConfig.pszBdfFile);
					break;
				case PALCFG_GAMEPATH:
					gConfig.pszGamePath = ParseStringValue(value.sValue, gConfig.pszGamePath);
					break;
				case PALCFG_SAVEPATH:
					gConfig.pszSavePath = ParseStringValue(value.sValue, gConfig.pszSavePath);
					break;
				case PALCFG_LOGFILE:
					gConfig.pszLogFile = ParseStringValue(value.sValue, gConfig.pszLogFile);
					break;
				case PALCFG_CD:
				{
					if (PAL_HAS_MP3 && SDL_strncasecmp(value.sValue, "MP3", 3) == 0)
						eCDType = MUSIC_MP3;
					else if (PAL_HAS_OGG && SDL_strncasecmp(value.sValue, "OGG", 3) == 0)
						eCDType = MUSIC_OGG;
					else if (PAL_HAS_SDLCD && SDL_strncasecmp(value.sValue, "RAW", 3) == 0)
						eCDType = MUSIC_SDLCD;
					break;
				}
				case PALCFG_MUSIC:
				{
					if (PAL_HAS_NATIVEMIDI && SDL_strncasecmp(value.sValue, "MIDI", 4) == 0)
						eMusicType = MUSIC_MIDI;
					else if (PAL_HAS_MP3 && SDL_strncasecmp(value.sValue, "MP3", 3) == 0)
						eMusicType = MUSIC_MP3;
					else if (PAL_HAS_OGG && SDL_strncasecmp(value.sValue, "OGG", 3) == 0)
						eMusicType = MUSIC_OGG;
					else if (SDL_strncasecmp(value.sValue, "RIX", 3) == 0)
						eMusicType = MUSIC_RIX;
					break;
				}
				case PALCFG_OPL:
				{
					if (SDL_strncasecmp(value.sValue, "DOSBOXNEW", 9) == 0)
						eOPLType = OPL_DOSBOX_NEW;
					else if (SDL_strncasecmp(value.sValue, "DOSBOX", 6) == 0)
						eOPLType = OPL_DOSBOX;
					else if (SDL_strncasecmp(value.sValue, "MAME", 4) == 0)
						eOPLType = OPL_MAME;
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
				case PALCFG_CLIMIDIPLAYER:
					gConfig.pszCLIMIDIPlayerPath = ParseStringValue(value.sValue, gConfig.pszCLIMIDIPlayerPath);
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
	gConfig.eMusicType = eMusicType;
	gConfig.eCDType = eCDType;
	gConfig.eOPLType = eOPLType;
	gConfig.dwWordLength = 10;	// This is the default value for Chinese version
	gConfig.ScreenLayout = screen_layout;

	gConfig.fIsWIN95 = FALSE;	// Default for DOS version
	gConfig.fUseEmbeddedFonts = values[PALCFG_USEEMBEDDEDFONTS].bValue;
	gConfig.fUseSurroundOPL = values[PALCFG_STEREO].bValue && values[PALCFG_USESURROUNDOPL].bValue;
	gConfig.fLaunchSetting = values[PALCFG_LAUNCHSETTING].bValue;
	gConfig.fUseTouchOverlay = values[PALCFG_USETOUCHOVERLAY].bValue;
	gConfig.fKeepAspectRatio = values[PALCFG_KEEPASPECTRATIO].bValue;
	gConfig.fFullScreen = values[PALCFG_FULLSCREEN].bValue;
	gConfig.iAudioChannels = values[PALCFG_STEREO].bValue ? 2 : 1;

	gConfig.iSurroundOPLOffset = values[PALCFG_SURROUNDOPLOFFSET].iValue;
	gConfig.iLogLevel = values[PALCFG_LOGLEVEL].iValue;

	gConfig.iSampleRate = values[PALCFG_SAMPLERATE].uValue;
	gConfig.iOPLSampleRate = values[PALCFG_OPLSAMPLERATE].uValue;
	gConfig.iResampleQuality = values[PALCFG_RESAMPLEQUALITY].uValue;
	gConfig.uCodePage = values[PALCFG_CODEPAGE].uValue;
	gConfig.wAudioBufferSize = (WORD)values[PALCFG_AUDIOBUFFERSIZE].uValue;
	gConfig.iMusicVolume = values[PALCFG_MUSICVOLUME].uValue;
	gConfig.iSoundVolume = values[PALCFG_SOUNDVOLUME].uValue;

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
}


BOOL
PAL_SaveConfig(
	void
)
{
	char buf[512];
	FILE *fp = fopen(va("%ssdlpal.cfg", PAL_CONFIG_PREFIX), "w");

	if (fp)
	{
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_KEEPASPECTRATIO), gConfig.fKeepAspectRatio); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_FULLSCREEN), gConfig.fFullScreen); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_LAUNCHSETTING), gConfig.fLaunchSetting); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_STEREO), gConfig.iAudioChannels == 2 ? TRUE : FALSE); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_USEEMBEDDEDFONTS), gConfig.fUseEmbeddedFonts); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_USESURROUNDOPL), gConfig.fUseSurroundOPL); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_USETOUCHOVERLAY), gConfig.fUseTouchOverlay); fputs(buf, fp);

		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_SURROUNDOPLOFFSET), gConfig.iSurroundOPLOffset); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_LOGLEVEL), gConfig.iLogLevel); fputs(buf, fp);

		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_AUDIOBUFFERSIZE), gConfig.wAudioBufferSize); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_CODEPAGE), gConfig.uCodePage); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_OPLSAMPLERATE), gConfig.iOPLSampleRate); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_RESAMPLEQUALITY), gConfig.iResampleQuality); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_SAMPLERATE), gConfig.iSampleRate); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_MUSICVOLUME), gConfig.iMusicVolume); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_SOUNDVOLUME), gConfig.iSoundVolume); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_WINDOWHEIGHT), gConfig.dwScreenHeight); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_WINDOWWIDTH), gConfig.dwScreenWidth); fputs(buf, fp);

		sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_CD), music_types[gConfig.eCDType]); fputs(buf, fp);
		sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_MUSIC), music_types[gConfig.eMusicType]); fputs(buf, fp);
		sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_OPL), opl_types[gConfig.eOPLType]); fputs(buf, fp);

		if (gConfig.pszGamePath && *gConfig.pszGamePath && strncmp(gConfig.pszGamePath, PAL_PREFIX, strnlen(gConfig.pszGamePath, PAL_MAX_PATH)) != 0) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_GAMEPATH), gConfig.pszGamePath); fputs(buf, fp); }
		if (gConfig.pszSavePath && *gConfig.pszSavePath && strncmp(gConfig.pszSavePath, PAL_SAVE_PREFIX, strnlen(gConfig.pszSavePath, PAL_MAX_PATH)) != 0) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_SAVEPATH), gConfig.pszSavePath); fputs(buf, fp); }
		if (gConfig.pszMsgFile && *gConfig.pszMsgFile) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_MESSAGEFILE), gConfig.pszMsgFile); fputs(buf, fp); }
		if (gConfig.pszBdfFile && *gConfig.pszBdfFile) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_BDFFILE), gConfig.pszBdfFile); fputs(buf, fp); }
		if (gConfig.pszLogFile && *gConfig.pszLogFile) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_LOGFILE), gConfig.pszLogFile); fputs(buf, fp); }
		if (gConfig.pszCLIMIDIPlayerPath && *gConfig.pszCLIMIDIPlayerPath) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_CLIMIDIPLAYER), gConfig.pszCLIMIDIPlayerPath); fputs(buf, fp); }

		fclose(fp);

		return TRUE;
	}
	else
		return FALSE;
}

BOOL
PAL_GetConfigItem(
	const char  *szName,
	ConfigValue *value,
	BOOL isDefault
)
{
	for (int index = PALCFG_ALL_MIN; index < PALCFG_ALL_MAX; index++)
	{
		if (SDL_strcasecmp(gConfigItems[index].Name, szName) == 0)
		{
			if (isDefault)
			{
				*value = gConfigItems[index].DefaultValue;
				return TRUE;
			}
			switch (index)
			{
			case PALCFG_FULLSCREEN:        value->bValue = gConfig.fFullScreen; return TRUE;
			case PALCFG_KEEPASPECTRATIO:   value->bValue = gConfig.fKeepAspectRatio; return TRUE;
			case PALCFG_LAUNCHSETTING:     value->bValue = gConfig.fLaunchSetting; return TRUE;
			case PALCFG_STEREO:            value->bValue = (gConfig.iAudioChannels == 2); return TRUE;
			case PALCFG_USEEMBEDDEDFONTS:  value->bValue = gConfig.fUseEmbeddedFonts; return TRUE;
			case PALCFG_USESURROUNDOPL:    value->bValue = gConfig.fUseSurroundOPL; return TRUE;
			case PALCFG_USETOUCHOVERLAY:   value->bValue = gConfig.fUseTouchOverlay; return TRUE;
			case PALCFG_SURROUNDOPLOFFSET: value->iValue = gConfig.iSurroundOPLOffset; return TRUE;
			case PALCFG_AUDIOBUFFERSIZE:   value->uValue = gConfig.wAudioBufferSize; return TRUE;
			case PALCFG_CODEPAGE:          value->uValue = gConfig.uCodePage; return TRUE;
			case PALCFG_OPLSAMPLERATE:     value->uValue = gConfig.iOPLSampleRate; return TRUE;
			case PALCFG_RESAMPLEQUALITY:   value->uValue = gConfig.iResampleQuality; return TRUE;
			case PALCFG_SAMPLERATE:        value->uValue = gConfig.iSampleRate; return TRUE;
			case PALCFG_MUSICVOLUME:       value->uValue = gConfig.iMusicVolume; return TRUE;
			case PALCFG_SOUNDVOLUME:       value->uValue = gConfig.iSoundVolume; return TRUE;
			case PALCFG_WINDOWHEIGHT:      value->uValue = gConfig.dwScreenHeight; return TRUE;
			case PALCFG_WINDOWWIDTH:       value->uValue = gConfig.dwScreenWidth; return TRUE;
			case PALCFG_CD:                value->sValue = music_types[gConfig.eCDType]; return TRUE;
			case PALCFG_GAMEPATH:          value->sValue = gConfig.pszGamePath; return TRUE;
			case PALCFG_SAVEPATH:          value->sValue = gConfig.pszSavePath; return TRUE;
			case PALCFG_MESSAGEFILE:       value->sValue = gConfig.pszMsgFile; return TRUE;
			case PALCFG_BDFFILE:           value->sValue = gConfig.pszBdfFile; return TRUE;
			case PALCFG_LOGFILE:           value->sValue = gConfig.pszLogFile; return TRUE;
			case PALCFG_CLIMIDIPLAYER:     value->sValue = gConfig.pszCLIMIDIPlayerPath; return TRUE;
			case PALCFG_MUSIC:             value->sValue = music_types[gConfig.eMusicType]; return TRUE;
			case PALCFG_OPL:               value->sValue = opl_types[gConfig.eOPLType]; return TRUE;
			}
			break;
		}
	}
	return FALSE;
}


BOOL
PAL_SetConfigItem(
	const char  *szName,
	const ConfigValue *value
)
{
	for (int index = PALCFG_ALL_MIN; index < PALCFG_ALL_MAX; index++)
	{
		if (SDL_strcasecmp(gConfigItems[index].Name, szName) == 0)
		{
			switch (index)
			{
			case PALCFG_FULLSCREEN:        gConfig.fFullScreen = value->bValue; return TRUE;
			case PALCFG_KEEPASPECTRATIO:   gConfig.fKeepAspectRatio = value->bValue; return TRUE;
			case PALCFG_LAUNCHSETTING:     gConfig.fLaunchSetting = value->bValue; return TRUE;
			case PALCFG_STEREO:            gConfig.iAudioChannels = value->bValue ? 2 : 1; return TRUE;
			case PALCFG_USEEMBEDDEDFONTS:  gConfig.fUseEmbeddedFonts = value->bValue; return TRUE;
			case PALCFG_USESURROUNDOPL:    gConfig.fUseSurroundOPL = value->bValue; return TRUE;
			case PALCFG_USETOUCHOVERLAY:   gConfig.fUseTouchOverlay = value->bValue; return TRUE;
			case PALCFG_SURROUNDOPLOFFSET: gConfig.iSurroundOPLOffset = value->iValue; return TRUE;
			case PALCFG_AUDIOBUFFERSIZE:   gConfig.wAudioBufferSize = value->uValue; return TRUE;
			case PALCFG_CODEPAGE:          gConfig.uCodePage = value->uValue; return TRUE;
			case PALCFG_OPLSAMPLERATE:     gConfig.iOPLSampleRate = value->uValue; return TRUE;
			case PALCFG_RESAMPLEQUALITY:   gConfig.iResampleQuality = value->uValue; return TRUE;
			case PALCFG_SAMPLERATE:        gConfig.iSampleRate = value->uValue; return TRUE;
			case PALCFG_MUSICVOLUME:       gConfig.iMusicVolume = value->uValue; return TRUE;
			case PALCFG_SOUNDVOLUME:       gConfig.iSoundVolume = value->uValue; return TRUE;
			case PALCFG_WINDOWHEIGHT:      gConfig.dwScreenHeight = value->uValue; return TRUE;
			case PALCFG_WINDOWWIDTH:       gConfig.dwScreenWidth = value->uValue; return TRUE;
			case PALCFG_GAMEPATH:
				if (gConfig.pszGamePath) free(gConfig.pszGamePath);
				gConfig.pszGamePath = value->sValue && value->sValue[0] ? strdup(value->sValue) : strdup(PAL_SAVE_PREFIX);
				return TRUE;
			case PALCFG_SAVEPATH:
				if (gConfig.pszSavePath) free(gConfig.pszSavePath);
				gConfig.pszSavePath = value->sValue && value->sValue[0] ? strdup(value->sValue) : (gConfig.pszGamePath ? strdup(gConfig.pszGamePath) : strdup(PAL_SAVE_PREFIX));
				return TRUE;
			case PALCFG_MESSAGEFILE:
				if (gConfig.pszMsgFile) free(gConfig.pszMsgFile);
				gConfig.pszMsgFile = value->sValue && value->sValue[0] ? strdup(value->sValue) : NULL;
				return TRUE;
			case PALCFG_BDFFILE:
				if (gConfig.pszBdfFile) free(gConfig.pszBdfFile);
				gConfig.pszBdfFile = value->sValue && value->sValue[0] ? strdup(value->sValue) : NULL;
				return TRUE;
			case PALCFG_LOGFILE:
				if (gConfig.pszLogFile) free(gConfig.pszLogFile);
				gConfig.pszLogFile = value->sValue && value->sValue[0] ? strdup(value->sValue) : NULL;
				return TRUE;
			case PALCFG_CLIMIDIPLAYER:
				if (gConfig.pszCLIMIDIPlayerPath) free(gConfig.pszCLIMIDIPlayerPath);
				gConfig.pszCLIMIDIPlayerPath = value->sValue && value->sValue[0] ? strdup(value->sValue) : NULL;
				return TRUE;
			case PALCFG_CD:
				for (int i = 0; i < sizeof(music_types) / sizeof(music_types[0]); i++)
				{
					if (SDL_strcasecmp(value->sValue, music_types[i]) == 0)
					{
						gConfig.eCDType = (MUSICTYPE)i;
						return TRUE;
					}
				}
				break;
			case PALCFG_MUSIC:
				for (int i = 0; i < sizeof(music_types) / sizeof(music_types[0]); i++)
				{
					if (SDL_strcasecmp(value->sValue, music_types[i]) == 0)
					{
						gConfig.eMusicType = (MUSICTYPE)i;
						return TRUE;
					}
				}
				break;
			case PALCFG_OPL:
				for (int i = 0; i < sizeof(opl_types) / sizeof(opl_types[0]); i++)
				{
					if (SDL_strcasecmp(value->sValue, opl_types[i]) == 0)
					{
						gConfig.eOPLType = (OPLTYPE)i;
						return TRUE;
					}
				}
				break;
			}
		}
	}
	return FALSE;
}
