
#include "global.h"
#include "palcfg.h"
#include "util.h"
#include "resampler.h"
#include <stdint.h>

#if !defined(PAL_HAS_TOUCH)
#define PAL_HAS_TOUCH     0
#endif

static const ConfigItem gConfigItems[PALCFG_ALL_MAX] = {
	{ PALCFG_FULLSCREEN,        PALCFG_BOOLEAN,  "FULLSCREEN",        10, FALSE, FALSE, TRUE },
	{ PALCFG_KEEPASPECTRATIO,   PALCFG_BOOLEAN,  "KEEPASPECTRATIO",   15, TRUE,  FALSE, TRUE },
	{ PALCFG_LAUNCHSETTING,     PALCFG_BOOLEAN,  "LAUNCHSETTING",     13, TRUE,  FALSE, TRUE },
	{ PALCFG_STEREO,            PALCFG_BOOLEAN,  "STEREO",             6, TRUE,  FALSE, TRUE },								// Default for stereo audio
	{ PALCFG_USEEMBEDDEDFONTS,  PALCFG_BOOLEAN,  "USEEMBEDDEDFONTS",  16, TRUE,  FALSE, TRUE },								// Default for using embedded fonts in DOS version
	{ PALCFG_USESURROUNDOPL,    PALCFG_BOOLEAN,  "USESURROUNDOPL",    14, TRUE,  FALSE, TRUE },								// Default for using surround opl
	{ PALCFG_USETOUCHOVERLAY,   PALCFG_BOOLEAN,  "USETOUCHOVERLAY",   15, PAL_HAS_TOUCH, FALSE, TRUE },

	{ PALCFG_SURROUNDOPLOFFSET, PALCFG_INTEGER,  "SURROUNDOPLOFFSET", 17, 384,   INT32_MIN, INT32_MAX },

	{ PALCFG_AUDIOBUFFERSIZE,   PALCFG_UNSIGNED, "AUDIOBUFFERSIZE",   15, PAL_AUDIO_DEFAULT_BUFFER_SIZE, 2, 32768 },
	{ PALCFG_CODEPAGE,          PALCFG_UNSIGNED, "CODEPAGE",           8, CP_BIG5, CP_BIG5, CP_MAX - 1 },											// Default for BIG5
	{ PALCFG_OPLSAMPLERATE,     PALCFG_UNSIGNED, "OPLSAMPLERATE",     13, 49716,   0, UINT32_MAX },
	{ PALCFG_RESAMPLEQUALITY,   PALCFG_UNSIGNED, "RESAMPLEQUALITY",   15, RESAMPLER_QUALITY_MAX, RESAMPLER_QUALITY_MIN, RESAMPLER_QUALITY_MAX },	// Default for best quality
	{ PALCFG_SAMPLERATE,        PALCFG_UNSIGNED, "SAMPLERATE",        10, 44100,   0, PAL_MAX_SAMPLERATE },
	{ PALCFG_MUSICVOLUME,       PALCFG_UNSIGNED, "MUSICVOLUME",       11, PAL_MAX_VOLUME, 0, PAL_MAX_VOLUME },										// Default for maximum volume
	{ PALCFG_SOUNDVOLUME,       PALCFG_UNSIGNED, "SOUNDVOLUME",       11, PAL_MAX_VOLUME, 0, PAL_MAX_VOLUME },										// Default for maximum volume
	{ PALCFG_WINDOWHEIGHT,      PALCFG_UNSIGNED, "WINDOWHEIGHT",      12, PAL_DEFAULT_WINDOW_HEIGHT, 0, UINT32_MAX },
	{ PALCFG_WINDOWWIDTH,       PALCFG_UNSIGNED, "WINDOWWIDTH",       11, PAL_DEFAULT_WINDOW_WIDTH,  0, UINT32_MAX },

	{ PALCFG_CD,                PALCFG_STRING,   "CD",                 2, "OGG", NULL, NULL },
	{ PALCFG_GAMEPATH,          PALCFG_STRING,   "GAMEPATH",           8, NULL, NULL, NULL },
	{ PALCFG_MESSAGEFILE,       PALCFG_STRING,   "MESSAGEFILENAME",   15, NULL, NULL, NULL },
	{ PALCFG_BDFFILE,           PALCFG_STRING,   "BDFFILENAME",       11, NULL, NULL, NULL },
	{ PALCFG_MUSIC,             PALCFG_STRING,   "MUSIC",              5, "RIX", NULL, NULL },
	{ PALCFG_OPL,               PALCFG_STRING,   "OPL",                3, "DOSBOX", NULL, NULL },
	{ PALCFG_RIXEXTRAINIT,      PALCFG_STRING,   "RIXEXTRAINIT",      12, NULL, NULL, NULL },
};


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
		if (ptr = strchr(line, '='))
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
						if (gConfigItems[i].Type != PALCFG_STRING)
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
							}
						}
						else
						{
							//
							// Skip leading spaces
							//
							while (*ptr && isspace(*ptr)) ptr++;
							pValue->sValue = ptr;
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

VOID
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
				{
					int n = strlen(value.sValue);
					while (n > 0 && isspace(value.sValue[n - 1])) n--;
					if (n > 0)
					{
						gConfig.pszMsgFile = (char *)realloc(gConfig.pszMsgFile, n + 1);
						memcpy(gConfig.pszMsgFile, value.sValue, n);
						gConfig.pszMsgFile[n] = '\0';
					}
					break;
				}
				case PALCFG_BDFFILE:
				{
					int n = strlen(value.sValue);
					while (n > 0 && isspace(value.sValue[n - 1])) n--;
					if (n > 0)
					{
						gConfig.pszBdfFile = (char *)realloc(gConfig.pszBdfFile, n + 1);
						memcpy(gConfig.pszBdfFile, value.sValue, n);
						gConfig.pszBdfFile[n] = '\0';
					}
					break;
				}
				case PALCFG_GAMEPATH:
				{
					int n = strlen(value.sValue);
					while (n > 0 && isspace(value.sValue[n - 1])) n--;
					if (n > 0)
					{
						gConfig.pszGamePath = (char *)realloc(gConfig.pszGamePath, n + 1);
						memcpy(gConfig.pszGamePath, value.sValue, n);
						gConfig.pszGamePath[n] = '\0';
					}
					break;
				}
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
#if SDL_VERSION_ATLEAST(2,0,0)
	gConfig.fKeepAspectRatio = values[PALCFG_KEEPASPECTRATIO].bValue;
#endif
	gConfig.fFullScreen = values[PALCFG_FULLSCREEN].bValue;
	gConfig.iAudioChannels = values[PALCFG_STEREO].bValue ? 2 : 1;

	gConfig.iSurroundOPLOffset = values[PALCFG_SURROUNDOPLOFFSET].iValue;

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
	VOID
)
{
	static const char *music_types[] = { "MIDI", "RIX", "MP3", "OGG", "RAW" };
	static const char *opl_types[] = { "DOSBOX", "MAME", "DOSBOXNEW" };
	char buf[512];
	FILE *fp = fopen(va("%ssdlpal.cfg", PAL_CONFIG_PREFIX), "w");

	if (fp)
	{
#if SDL_VERSION_ATLEAST(2,0,0)
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_KEEPASPECTRATIO), gConfig.fKeepAspectRatio); fputs(buf, fp);
#endif
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_FULLSCREEN), gConfig.fFullScreen); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_LAUNCHSETTING), gConfig.fLaunchSetting); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_STEREO), gConfig.iAudioChannels == 2 ? TRUE : FALSE); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_USEEMBEDDEDFONTS), gConfig.fUseEmbeddedFonts); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_USESURROUNDOPL), gConfig.fUseSurroundOPL); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_USETOUCHOVERLAY), gConfig.fUseTouchOverlay); fputs(buf, fp);

		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_SURROUNDOPLOFFSET), gConfig.iSurroundOPLOffset); fputs(buf, fp);

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

		if (gConfig.pszGamePath) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_GAMEPATH), gConfig.pszGamePath); fputs(buf, fp); }
		if (gConfig.pszMsgFile) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_MESSAGEFILE), gConfig.pszMsgFile); fputs(buf, fp); }
		if (gConfig.pszBdfFile) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_BDFFILE), gConfig.pszBdfFile); fputs(buf, fp); }

		fclose(fp);

		return TRUE;
	}
	else
		return FALSE;
}
