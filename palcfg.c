
#include "global.h"
#include "resampler.h"
#include "palcfg.h"
#include <stdint.h>

static const ConfigItem gConfigItems[PALCFG_ALL_MAX] = {
	{ PALCFG_DOS,               PALCFG_BOOLEAN,  "DOS",                3, TRUE,  FALSE, TRUE },								// Default for DOS
	{ PALCFG_FULLSCREEN,        PALCFG_BOOLEAN,  "FULLSCREEN",        10, FALSE, FALSE, TRUE },
	{ PALCFG_KEEPASPECTRATIO,   PALCFG_BOOLEAN,  "KEEPASPECTRATIO",   15, TRUE,  FALSE, TRUE },
	{ PALCFG_LAUNCHSETTING,     PALCFG_BOOLEAN,  "LAUNCHSETTING",     13, TRUE,  FALSE, TRUE },
	{ PALCFG_STEREO,            PALCFG_BOOLEAN,  "STEREO",             6, TRUE,  FALSE, TRUE },								// Default for stereo audio
	{ PALCFG_USEEMBEDDEDFONTS,  PALCFG_BOOLEAN,  "USEEMBEDDEDFONTS",  16, TRUE,  FALSE, TRUE },								// Default for using embedded fonts in DOS version
	{ PALCFG_USESURROUNDOPL,    PALCFG_BOOLEAN,  "USESURROUNDOPL",    14, TRUE,  FALSE, TRUE },								// Default for using surround opl
	{ PALCFG_USETOUCHOVERLAY,   PALCFG_BOOLEAN,  "USETOUCHOVERLAY",   15, TRUE,  FALSE, TRUE },

	{ PALCFG_SURROUNDOPLOFFSET, PALCFG_INTEGER,  "SURROUNDOPLOFFSET", 17, 384,   INT32_MIN, INT32_MAX },

	{ PALCFG_AUDIOBUFFERSIZE,   PALCFG_UNSIGNED, "AUDIOBUFFERSIZE",   15, PAL_AUDIO_DEFAULT_BUFFER_SIZE, 2, 32768 },
	{ PALCFG_CODEPAGE,          PALCFG_UNSIGNED, "CODEPAGE",           8, CP_BIG5, CP_BIG5, CP_MAX - 1 },											// Default for BIG5
	{ PALCFG_OPLSAMPLERATE,     PALCFG_UNSIGNED, "OPLSAMPLERATE",     13, 49716,   0, UINT32_MAX },
	{ PALCFG_RESAMPLEQUALITY,   PALCFG_UNSIGNED, "RESAMPLEQUALITY",   15, RESAMPLER_QUALITY_MAX, RESAMPLER_QUALITY_MIN, RESAMPLER_QUALITY_MAX },	// Default for best quality
	{ PALCFG_SAMPLERATE,        PALCFG_UNSIGNED, "SAMPLERATE",        10, 44100,   0, PAL_MAX_SAMPLERATE },
	{ PALCFG_VOLUME,            PALCFG_UNSIGNED, "VOLUME",             6, 100,     0, 100 },														// Default for maximum volume
	{ PALCFG_WINDOWHEIGHT,      PALCFG_UNSIGNED, "WINDOWHEIGHT",      12, PAL_DEFAULT_WINDOW_HEIGHT, 0, UINT32_MAX },
	{ PALCFG_WINDOWWIDTH,       PALCFG_UNSIGNED, "WINDOWWIDTH",       11, PAL_DEFAULT_WINDOW_WIDTH,  0, UINT32_MAX },

	{ PALCFG_CD,                PALCFG_STRING,   "CD",                 2, "OGG", NULL, NULL },
	{ PALCFG_GAMEPATH,          PALCFG_STRING,   "GAMEPATH",           8, NULL, NULL, NULL },
	{ PALCFG_MESSAGEFILE,       PALCFG_STRING,   "MESSAGEFILENAME",   15, NULL, NULL, NULL },
	{ PALCFG_MUSIC,             PALCFG_STRING,   "MUSIC",              5, "RIX", NULL, NULL },
	{ PALCFG_OPL,               PALCFG_STRING,   "OPL",                3, "DOSBOX", NULL, NULL },
	{ PALCFG_RIXEXTRAINIT,      PALCFG_STRING,   "RIXEXTRAINIT",      12, NULL, NULL, NULL },
	{ PALCFG_SAVEPATH,          PALCFG_STRING,   "SAVEPATH",           8, NULL, NULL, NULL },
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

static char *move_to_next_line(char *ptr)
{
	while (*ptr && (*ptr != '\r' && *ptr != '\n')) ptr++;
	while (*ptr && (*ptr == '\r' || *ptr == '\n')) ptr++;
	return (*ptr == '\0') ? NULL : ptr;
}
