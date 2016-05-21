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
// Modified by Lou Yihua <louyihua@21cn.com> with Unicode support, 2015
//

#include "main.h"
#include "resampler.h"
#include "palcfg.h"

static GLOBALVARS _gGlobals;
GLOBALVARS * const  gpGlobals = &_gGlobals;

CONFIGURATION gConfig;

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define DO_BYTESWAP(buf, size)
#else
#define DO_BYTESWAP(buf, size)                                   \
   do {                                                          \
      int i;                                                     \
      for (i = 0; i < (size) / 2; i++)                           \
      {                                                          \
         ((LPWORD)(buf))[i] = SDL_SwapLE16(((LPWORD)(buf))[i]);  \
      }                                                          \
   } while(0)
#endif

#define LOAD_DATA(buf, size, chunknum, fp)                       \
   do {                                                          \
      PAL_MKFReadChunk((LPBYTE)(buf), (size), (chunknum), (fp)); \
      DO_BYTESWAP(buf, size);                                    \
   } while(0)

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

	gConfig.fIsWIN95 = !values[PALCFG_DOS].bValue;
	gConfig.fUseEmbeddedFonts = values[PALCFG_DOS].bValue && values[PALCFG_USEEMBEDDEDFONTS].bValue;
	gConfig.fUseSurroundOPL = values[PALCFG_STEREO].bValue && values[PALCFG_USESURROUNDOPL].bValue;
	gConfig.fLaunchSetting = values[PALCFG_LAUNCHSETTING].bValue;
#if PAL_HAS_TOUCH
	gConfig.fUseTouchOverlay = values[PALCFG_USETOUCHOVERLAY].bValue;
#endif
#if SDL_VERSION_ATLEAST(2,0,0)
	gConfig.fKeepAspectRatio = values[PALCFG_KEEPASPECTRATIO].bValue;
#else
	gConfig.fFullScreen = values[PALCFG_FULLSCREEN].bValue;
#endif
	gConfig.iAudioChannels = values[PALCFG_STEREO].bValue ? 2 : 1;

	gConfig.iSurroundOPLOffset = values[PALCFG_SURROUNDOPLOFFSET].iValue;

	gConfig.iSampleRate = values[PALCFG_SAMPLERATE].uValue;
	gConfig.iOPLSampleRate = values[PALCFG_OPLSAMPLERATE].uValue;
	gConfig.iResampleQuality = values[PALCFG_RESAMPLEQUALITY].uValue;
	gConfig.uCodePage = values[PALCFG_CODEPAGE].uValue;
	gConfig.wAudioBufferSize = (WORD)values[PALCFG_AUDIOBUFFERSIZE].uValue;
	gConfig.iVolume = SDL_MIX_MAXVOLUME * values[PALCFG_VOLUME].uValue / 100;

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
	static const char *music_types[] = { "RIX", "MIDI", "MP3", "OGG", "RAW" };
	static const char *opl_types[] = { "DOSBOX", "MAME" };
	char buf[512];
	FILE *fp = fopen(va("%ssdlpal.cfg", PAL_CONFIG_PREFIX), "w");

	if (fp)
	{
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_DOS), !gConfig.fIsWIN95); fputs(buf, fp);
#if SDL_VERSION_ATLEAST(2,0,0)
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_KEEPASPECTRATIO), gConfig.fKeepAspectRatio); fputs(buf, fp);
#else
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_FULLSCREEN), gConfig.fKeepAspectRatio); fputs(buf, fp);
#endif
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_LAUNCHSETTING), gConfig.fLaunchSetting); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_STEREO), gConfig.iAudioChannels == 2 ? TRUE : FALSE); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_USEEMBEDDEDFONTS), gConfig.fUseEmbeddedFonts); fputs(buf, fp);
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_USESURROUNDOPL), gConfig.fUseSurroundOPL); fputs(buf, fp);
#if PAL_HAS_TOUCH
		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_USETOUCHOVERLAY), gConfig.fUseTouchOverlay); fputs(buf, fp);
#endif

		sprintf(buf, "%s=%d\n", PAL_ConfigName(PALCFG_SURROUNDOPLOFFSET), gConfig.iSurroundOPLOffset); fputs(buf, fp);

		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_AUDIOBUFFERSIZE), gConfig.wAudioBufferSize); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_CODEPAGE), gConfig.uCodePage); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_OPLSAMPLERATE), gConfig.iOPLSampleRate); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_RESAMPLEQUALITY), gConfig.iResampleQuality); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_SAMPLERATE), gConfig.iSampleRate); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_VOLUME), gConfig.iVolume); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_WINDOWHEIGHT), gConfig.dwScreenHeight); fputs(buf, fp);
		sprintf(buf, "%s=%u\n", PAL_ConfigName(PALCFG_WINDOWWIDTH), gConfig.dwScreenWidth); fputs(buf, fp);

		sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_CD), music_types[gConfig.eCDType]); fputs(buf, fp);
		sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_MUSIC), music_types[gConfig.eMusicType]); fputs(buf, fp);
		sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_OPL), opl_types[gConfig.eOPLType]); fputs(buf, fp);

		if (gConfig.pszGamePath) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_GAMEPATH), gConfig.pszGamePath); fputs(buf, fp); }
		if (gConfig.pszMsgFile) { sprintf(buf, "%s=%s\n", PAL_ConfigName(PALCFG_MESSAGEFILE), gConfig.pszMsgFile); fputs(buf, fp); }

		fclose(fp);

		return TRUE;
	}
	else
		return FALSE;
}

INT
PAL_InitGlobals(
   VOID
)
/*++
  Purpose:

    Initialize global data.

  Parameters:

    None.

  Return value:

    0 = success, -1 = error.

--*/
{
   //
   // Set decompress function
   //
   Decompress = gConfig.fIsWIN95 ? YJ2_Decompress : YJ1_Decompress;

   //
   // Open files
   //
   gpGlobals->f.fpFBP = UTIL_OpenRequiredFile("fbp.mkf");
   gpGlobals->f.fpMGO = UTIL_OpenRequiredFile("mgo.mkf");
   gpGlobals->f.fpBALL = UTIL_OpenRequiredFile("ball.mkf");
   gpGlobals->f.fpDATA = UTIL_OpenRequiredFile("data.mkf");
   gpGlobals->f.fpF = UTIL_OpenRequiredFile("f.mkf");
   gpGlobals->f.fpFIRE = UTIL_OpenRequiredFile("fire.mkf");
   gpGlobals->f.fpRGM = UTIL_OpenRequiredFile("rgm.mkf");
   gpGlobals->f.fpSSS = UTIL_OpenRequiredFile("sss.mkf");

   gpGlobals->lpObjectDesc = gConfig.fIsWIN95 ? NULL : PAL_LoadObjectDesc(va("%s%s", gConfig.pszGamePath, "desc.dat"));
   gpGlobals->bCurrentSaveSlot = 1;

   return 0;
}

VOID
PAL_FreeGlobals(
   VOID
)
/*++
  Purpose:

    Free global data.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   //
   // Close all opened files
   //
   UTIL_CloseFile(gpGlobals->f.fpFBP);
   UTIL_CloseFile(gpGlobals->f.fpMGO);
   UTIL_CloseFile(gpGlobals->f.fpBALL);
   UTIL_CloseFile(gpGlobals->f.fpDATA);
   UTIL_CloseFile(gpGlobals->f.fpF);
   UTIL_CloseFile(gpGlobals->f.fpFIRE);
   UTIL_CloseFile(gpGlobals->f.fpRGM);
   UTIL_CloseFile(gpGlobals->f.fpSSS);

   //
   // Free the game data
   //
   free(gpGlobals->g.lprgEventObject);
   free(gpGlobals->g.lprgScriptEntry);
   free(gpGlobals->g.lprgStore);
   free(gpGlobals->g.lprgEnemy);
   free(gpGlobals->g.lprgEnemyTeam);
   free(gpGlobals->g.lprgMagic);
   free(gpGlobals->g.lprgBattleField);
   free(gpGlobals->g.lprgLevelUpMagic);

   //
   // Free the object description data
   //
   if (!gConfig.fIsWIN95)
      PAL_FreeObjectDesc(gpGlobals->lpObjectDesc);

#if USE_RIX_EXTRA_INIT
   free(gConfig.pExtraFMRegs);
   free(gConfig.pExtraFMVals);
   free(gConfig.dwExtraLength);
#endif
   free(gConfig.pszMsgFile);
   free(gConfig.pszGamePath);

   //
   // Clear the instance
   //
   memset(gpGlobals, 0, sizeof(GLOBALVARS));
   memset(&gConfig, 0, sizeof(CONFIGURATION));
}


static VOID
PAL_ReadGlobalGameData(
   VOID
)
/*++
  Purpose:

    Read global game data from data files.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   const GAMEDATA    *p = &gpGlobals->g;

   LOAD_DATA(p->lprgScriptEntry, p->nScriptEntry * sizeof(SCRIPTENTRY),
      4, gpGlobals->f.fpSSS);

   LOAD_DATA(p->lprgStore, p->nStore * sizeof(STORE), 0, gpGlobals->f.fpDATA);
   LOAD_DATA(p->lprgEnemy, p->nEnemy * sizeof(ENEMY), 1, gpGlobals->f.fpDATA);
   LOAD_DATA(p->lprgEnemyTeam, p->nEnemyTeam * sizeof(ENEMYTEAM),
      2, gpGlobals->f.fpDATA);
   LOAD_DATA(p->lprgMagic, p->nMagic * sizeof(MAGIC), 4, gpGlobals->f.fpDATA);
   LOAD_DATA(p->lprgBattleField, p->nBattleField * sizeof(BATTLEFIELD),
      5, gpGlobals->f.fpDATA);
   LOAD_DATA(p->lprgLevelUpMagic, p->nLevelUpMagic * sizeof(LEVELUPMAGIC_ALL),
      6, gpGlobals->f.fpDATA);
   LOAD_DATA(p->rgwBattleEffectIndex, sizeof(p->rgwBattleEffectIndex),
      11, gpGlobals->f.fpDATA);
   PAL_MKFReadChunk((LPBYTE)&(p->EnemyPos), sizeof(p->EnemyPos),
      13, gpGlobals->f.fpDATA);
   DO_BYTESWAP(&(p->EnemyPos), sizeof(p->EnemyPos));
   PAL_MKFReadChunk((LPBYTE)(p->rgLevelUpExp), sizeof(p->rgLevelUpExp),
      14, gpGlobals->f.fpDATA);
   DO_BYTESWAP(p->rgLevelUpExp, sizeof(p->rgLevelUpExp));

   // FIX: Enemy magic sound may be negative
   for (int i = 0; i < p->nEnemy; i++)
   {
	   if (p->lprgEnemy[i].wMagicSound >= 32768)
	   {
		   p->lprgEnemy[i].wMagicSound = -(short)p->lprgEnemy[i].wMagicSound;
	   }
   }
}

static VOID
PAL_InitGlobalGameData(
   VOID
)
/*++
  Purpose:

    Initialize global game data.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int        len;

#define PAL_DOALLOCATE(fp, num, type, lptype, ptr, n)                            \
   {                                                                             \
      len = PAL_MKFGetChunkSize(num, fp);                                        \
      ptr = (lptype)malloc(len);                                                 \
      n = len / sizeof(type);                                                    \
      if (ptr == NULL)                                                           \
      {                                                                          \
         TerminateOnError("PAL_InitGlobalGameData(): Memory allocation error!"); \
      }                                                                          \
   }

   //
   // If the memory has not been allocated, allocate first.
   //
   if (gpGlobals->g.lprgEventObject == NULL)
   {
      PAL_DOALLOCATE(gpGlobals->f.fpSSS, 0, EVENTOBJECT, LPEVENTOBJECT,
         gpGlobals->g.lprgEventObject, gpGlobals->g.nEventObject);

      PAL_DOALLOCATE(gpGlobals->f.fpSSS, 4, SCRIPTENTRY, LPSCRIPTENTRY,
         gpGlobals->g.lprgScriptEntry, gpGlobals->g.nScriptEntry);

      PAL_DOALLOCATE(gpGlobals->f.fpDATA, 0, STORE, LPSTORE,
         gpGlobals->g.lprgStore, gpGlobals->g.nStore);

      PAL_DOALLOCATE(gpGlobals->f.fpDATA, 1, ENEMY, LPENEMY,
         gpGlobals->g.lprgEnemy, gpGlobals->g.nEnemy);

      PAL_DOALLOCATE(gpGlobals->f.fpDATA, 2, ENEMYTEAM, LPENEMYTEAM,
         gpGlobals->g.lprgEnemyTeam, gpGlobals->g.nEnemyTeam);

      PAL_DOALLOCATE(gpGlobals->f.fpDATA, 4, MAGIC, LPMAGIC,
         gpGlobals->g.lprgMagic, gpGlobals->g.nMagic);

      PAL_DOALLOCATE(gpGlobals->f.fpDATA, 5, BATTLEFIELD, LPBATTLEFIELD,
         gpGlobals->g.lprgBattleField, gpGlobals->g.nBattleField);

      PAL_DOALLOCATE(gpGlobals->f.fpDATA, 6, LEVELUPMAGIC_ALL, LPLEVELUPMAGIC_ALL,
         gpGlobals->g.lprgLevelUpMagic, gpGlobals->g.nLevelUpMagic);

      PAL_ReadGlobalGameData();
   }
#undef PAL_DOALLOCATE
}

static VOID
PAL_LoadDefaultGame(
   VOID
)
/*++
  Purpose:

    Load the default game data.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   GAMEDATA    *p = &gpGlobals->g;
   UINT32       i;

   //
   // Load the default data from the game data files.
   //
   LOAD_DATA(p->lprgEventObject, p->nEventObject * sizeof(EVENTOBJECT),
      0, gpGlobals->f.fpSSS);
   PAL_MKFReadChunk((LPBYTE)(p->rgScene), sizeof(p->rgScene), 1, gpGlobals->f.fpSSS);
   DO_BYTESWAP(p->rgScene, sizeof(p->rgScene));
   if (gConfig.fIsWIN95)
   {
      PAL_MKFReadChunk((LPBYTE)(p->rgObject), sizeof(p->rgObject), 2, gpGlobals->f.fpSSS);
      DO_BYTESWAP(p->rgObject, sizeof(p->rgObject));
   }
   else
   {
      OBJECT_DOS objects[MAX_OBJECTS];
	  PAL_MKFReadChunk((LPBYTE)(objects), sizeof(objects), 2, gpGlobals->f.fpSSS);
	  DO_BYTESWAP(objects, sizeof(objects));
      //
      // Convert the DOS-style data structure to WIN-style data structure
      //
      for (i = 0; i < MAX_OBJECTS; i++)
      {
         memcpy(&p->rgObject[i], &objects[i], sizeof(OBJECT_DOS));
	     if (i >= OBJECT_ITEM_START && i <= OBJECT_MAGIC_END)
         {
            p->rgObject[i].rgwData[6] = objects[i].rgwData[5];     // wFlags
			p->rgObject[i].rgwData[5] = 0;                         // wScriptDesc or wReserved2
         }
         else
         {
            p->rgObject[i].rgwData[6] = 0;
         }
      }
   }

   PAL_MKFReadChunk((LPBYTE)(&(p->PlayerRoles)), sizeof(PLAYERROLES),
      3, gpGlobals->f.fpDATA);
   DO_BYTESWAP(&(p->PlayerRoles), sizeof(PLAYERROLES));

   //
   // Set some other default data.
   //
   gpGlobals->dwCash = 0;
   gpGlobals->wNumMusic = 0;
   gpGlobals->wNumPalette = 0;
   gpGlobals->wNumScene = 1;
   gpGlobals->wCollectValue = 0;
   gpGlobals->fNightPalette = FALSE;
   gpGlobals->wMaxPartyMemberIndex = 0;
   gpGlobals->viewport = PAL_XY(0, 0);
   gpGlobals->wLayer = 0;
   gpGlobals->wChaseRange = 1;
#ifndef PAL_CLASSIC
   gpGlobals->bBattleSpeed = 2;
#endif

   memset(gpGlobals->rgInventory, 0, sizeof(gpGlobals->rgInventory));
   memset(gpGlobals->rgPoisonStatus, 0, sizeof(gpGlobals->rgPoisonStatus));
   memset(gpGlobals->rgParty, 0, sizeof(gpGlobals->rgParty));
   memset(gpGlobals->rgTrail, 0, sizeof(gpGlobals->rgTrail));
   memset(&(gpGlobals->Exp), 0, sizeof(gpGlobals->Exp));

   for (i = 0; i < MAX_PLAYER_ROLES; i++)
   {
      gpGlobals->Exp.rgPrimaryExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
      gpGlobals->Exp.rgHealthExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
      gpGlobals->Exp.rgMagicExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
      gpGlobals->Exp.rgAttackExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
      gpGlobals->Exp.rgMagicPowerExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
      gpGlobals->Exp.rgDefenseExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
      gpGlobals->Exp.rgDexterityExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
      gpGlobals->Exp.rgFleeExp[i].wLevel = p->PlayerRoles.rgwLevel[i];
   }

   gpGlobals->fEnteringScene = TRUE;
}

typedef struct tagSAVEDGAME_COMMON
{
	WORD             wSavedTimes;             // saved times
	WORD             wViewportX, wViewportY;  // viewport location
	WORD             nPartyMember;            // number of members in party
	WORD             wNumScene;               // scene number
	WORD             wPaletteOffset;
	WORD             wPartyDirection;         // party direction
	WORD             wNumMusic;               // music number
	WORD             wNumBattleMusic;         // battle music number
	WORD             wNumBattleField;         // battle field number
	WORD             wScreenWave;             // level of screen waving
	WORD             wBattleSpeed;            // battle speed
	WORD             wCollectValue;           // value of "collected" items
	WORD             wLayer;
	WORD             wChaseRange;
	WORD             wChasespeedChangeCycles;
	WORD             nFollower;
	WORD             rgwReserved2[3];         // unused
	DWORD            dwCash;                  // amount of cash
	PARTY            rgParty[MAX_PLAYABLE_PLAYER_ROLES];       // player party
	TRAIL            rgTrail[MAX_PLAYABLE_PLAYER_ROLES];       // player trail
	ALLEXPERIENCE    Exp;                     // experience data
	PLAYERROLES      PlayerRoles;
	POISONSTATUS     rgPoisonStatus[MAX_POISONS][MAX_PLAYABLE_PLAYER_ROLES]; // poison status
	INVENTORY        rgInventory[MAX_INVENTORY];               // inventory status
	SCENE            rgScene[MAX_SCENES];
} SAVEDGAME_COMMON, *LPSAVEDGAME_COMMON;

typedef struct tagSAVEDGAME_DOS
{
	WORD             wSavedTimes;             // saved times
	WORD             wViewportX, wViewportY;  // viewport location
	WORD             nPartyMember;            // number of members in party
	WORD             wNumScene;               // scene number
	WORD             wPaletteOffset;
	WORD             wPartyDirection;         // party direction
	WORD             wNumMusic;               // music number
	WORD             wNumBattleMusic;         // battle music number
	WORD             wNumBattleField;         // battle field number
	WORD             wScreenWave;             // level of screen waving
	WORD             wBattleSpeed;            // battle speed
	WORD             wCollectValue;           // value of "collected" items
	WORD             wLayer;
	WORD             wChaseRange;
	WORD             wChasespeedChangeCycles;
	WORD             nFollower;
	WORD             rgwReserved2[3];         // unused
	DWORD            dwCash;                  // amount of cash
	PARTY            rgParty[MAX_PLAYABLE_PLAYER_ROLES];       // player party
	TRAIL            rgTrail[MAX_PLAYABLE_PLAYER_ROLES];       // player trail
	ALLEXPERIENCE    Exp;                     // experience data
	PLAYERROLES      PlayerRoles;
	POISONSTATUS     rgPoisonStatus[MAX_POISONS][MAX_PLAYABLE_PLAYER_ROLES]; // poison status
	INVENTORY        rgInventory[MAX_INVENTORY];               // inventory status
	SCENE            rgScene[MAX_SCENES];
	OBJECT_DOS       rgObject[MAX_OBJECTS];
	EVENTOBJECT      rgEventObject[MAX_EVENT_OBJECTS];
} SAVEDGAME_DOS, *LPSAVEDGAME_DOS;

typedef struct tagSAVEDGAME_WIN
{
	WORD             wSavedTimes;             // saved times
	WORD             wViewportX, wViewportY;  // viewport location
	WORD             nPartyMember;            // number of members in party
	WORD             wNumScene;               // scene number
	WORD             wPaletteOffset;
	WORD             wPartyDirection;         // party direction
	WORD             wNumMusic;               // music number
	WORD             wNumBattleMusic;         // battle music number
	WORD             wNumBattleField;         // battle field number
	WORD             wScreenWave;             // level of screen waving
	WORD             wBattleSpeed;            // battle speed
	WORD             wCollectValue;           // value of "collected" items
	WORD             wLayer;
	WORD             wChaseRange;
	WORD             wChasespeedChangeCycles;
	WORD             nFollower;
	WORD             rgwReserved2[3];         // unused
	DWORD            dwCash;                  // amount of cash
	PARTY            rgParty[MAX_PLAYABLE_PLAYER_ROLES];       // player party
	TRAIL            rgTrail[MAX_PLAYABLE_PLAYER_ROLES];       // player trail
	ALLEXPERIENCE    Exp;                     // experience data
	PLAYERROLES      PlayerRoles;
	POISONSTATUS     rgPoisonStatus[MAX_POISONS][MAX_PLAYABLE_PLAYER_ROLES]; // poison status
	INVENTORY        rgInventory[MAX_INVENTORY];               // inventory status
	SCENE            rgScene[MAX_SCENES];
	OBJECT           rgObject[MAX_OBJECTS];
	EVENTOBJECT      rgEventObject[MAX_EVENT_OBJECTS];
} SAVEDGAME_WIN, *LPSAVEDGAME_WIN;

static VOID
PAL_LoadGame_Common(
	const LPSAVEDGAME_COMMON s
)
{
	gpGlobals->viewport = PAL_XY(s->wViewportX, s->wViewportY);
	gpGlobals->wMaxPartyMemberIndex = s->nPartyMember;
	gpGlobals->wNumScene = s->wNumScene;
	gpGlobals->fNightPalette = (s->wPaletteOffset != 0);
	gpGlobals->wPartyDirection = s->wPartyDirection;
	gpGlobals->wNumMusic = s->wNumMusic;
	gpGlobals->wNumBattleMusic = s->wNumBattleMusic;
	gpGlobals->wNumBattleField = s->wNumBattleField;
	gpGlobals->wScreenWave = s->wScreenWave;
	gpGlobals->sWaveProgression = 0;
	gpGlobals->wCollectValue = s->wCollectValue;
	gpGlobals->wLayer = s->wLayer;
	gpGlobals->wChaseRange = s->wChaseRange;
	gpGlobals->wChasespeedChangeCycles = s->wChasespeedChangeCycles;
	gpGlobals->nFollower = s->nFollower;
	gpGlobals->dwCash = s->dwCash;
#ifndef PAL_CLASSIC
	gpGlobals->bBattleSpeed = s->wBattleSpeed;
	if (gpGlobals->bBattleSpeed > 5 || gpGlobals->bBattleSpeed == 0)
	{
		gpGlobals->bBattleSpeed = 2;
	}
#endif

	memcpy(gpGlobals->rgParty, s->rgParty, sizeof(gpGlobals->rgParty));
	memcpy(gpGlobals->rgTrail, s->rgTrail, sizeof(gpGlobals->rgTrail));
	gpGlobals->Exp = s->Exp;
	gpGlobals->g.PlayerRoles = s->PlayerRoles;
	memset(gpGlobals->rgPoisonStatus, 0, sizeof(gpGlobals->rgPoisonStatus));
	memcpy(gpGlobals->rgInventory, s->rgInventory, sizeof(gpGlobals->rgInventory));
	memcpy(gpGlobals->g.rgScene, s->rgScene, sizeof(gpGlobals->g.rgScene));
}

static INT
PAL_LoadGame_DOS(
   LPCSTR         szFileName
)
/*++
  Purpose:

    Load a saved game.

  Parameters:

    [IN]  szFileName - file name of saved game.

  Return value:

    0 if success, -1 if failed.

--*/
{
   FILE                     *fp;
   PAL_LARGE SAVEDGAME_DOS   s;
   int                       i;

   //
   // Try to open the specified file
   //
   fp = fopen(szFileName, "rb");
   if (fp == NULL)
   {
      return -1;
   }

   //
   // Read all data from the file and close.
   //
   fread(&s, sizeof(SAVEDGAME_DOS), 1, fp);
   fclose(fp);

   //
   // Adjust endianness
   //
   DO_BYTESWAP(&s, sizeof(SAVEDGAME_DOS));

   //
   // Cash amount is in DWORD, so do a wordswap in Big-Endian.
   //
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
   s.dwCash = ((s.dwCash >> 16) | (s.dwCash << 16));
#endif

   //
   // Get all the data from the saved game struct.
   //
   PAL_LoadGame_Common((LPSAVEDGAME_COMMON)&s);
   //
   // Convert the DOS-style data structure to WIN-style data structure
   //
   for (i = 0; i < MAX_OBJECTS; i++)
   {
      memcpy(&gpGlobals->g.rgObject[i], &s.rgObject[i], sizeof(OBJECT_DOS));
	  if (i >= OBJECT_ITEM_START && i <= OBJECT_MAGIC_END)
	  {
         gpGlobals->g.rgObject[i].rgwData[6] = s.rgObject[i].rgwData[5];     // wFlags
         gpGlobals->g.rgObject[i].rgwData[5] = 0;                            // wScriptDesc or wReserved2
	  }
	  else
	  {
         gpGlobals->g.rgObject[i].rgwData[6] = 0;
      }
   }
   memcpy(gpGlobals->g.lprgEventObject, s.rgEventObject,
      sizeof(EVENTOBJECT) * gpGlobals->g.nEventObject);

   gpGlobals->fEnteringScene = FALSE;

   PAL_CompressInventory();

   //
   // Success
   //
   return 0;
}

static INT
PAL_LoadGame_WIN(
   LPCSTR         szFileName
)
/*++
  Purpose:

    Load a saved game.

  Parameters:

    [IN]  szFileName - file name of saved game.

  Return value:

    0 if success, -1 if failed.

--*/
{
   FILE                     *fp;
   PAL_LARGE SAVEDGAME_WIN   s;

   //
   // Try to open the specified file
   //
   fp = fopen(szFileName, "rb");
   if (fp == NULL)
   {
      return -1;
   }

   //
   // Read all data from the file and close.
   //
   fread(&s, sizeof(SAVEDGAME_WIN), 1, fp);
   fclose(fp);

   //
   // Adjust endianness
   //
   DO_BYTESWAP(&s, sizeof(SAVEDGAME_WIN));

   //
   // Cash amount is in DWORD, so do a wordswap in Big-Endian.
   //
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
   s.dwCash = ((s.dwCash >> 16) | (s.dwCash << 16));
#endif

   //
   // Get all the data from the saved game struct.
   //
   PAL_LoadGame_Common((LPSAVEDGAME_COMMON)&s);
   memcpy(gpGlobals->g.rgObject, s.rgObject, sizeof(gpGlobals->g.rgObject));
   memcpy(gpGlobals->g.lprgEventObject, s.rgEventObject,
      sizeof(EVENTOBJECT) * gpGlobals->g.nEventObject);

   gpGlobals->fEnteringScene = FALSE;

   PAL_CompressInventory();

   //
   // Success
   //
   return 0;
}

static INT
PAL_LoadGame(
   LPCSTR         szFileName
)
{
	return gConfig.fIsWIN95 ? PAL_LoadGame_WIN(szFileName) : PAL_LoadGame_DOS(szFileName);
}

static VOID
PAL_SaveGame_Common(
    const LPSAVEDGAME_COMMON s
)
{
	s->wViewportX = PAL_X(gpGlobals->viewport);
	s->wViewportY = PAL_Y(gpGlobals->viewport);
	s->nPartyMember = gpGlobals->wMaxPartyMemberIndex;
	s->wNumScene = gpGlobals->wNumScene;
	s->wPaletteOffset = (gpGlobals->fNightPalette ? 0x180 : 0);
	s->wPartyDirection = gpGlobals->wPartyDirection;
	s->wNumMusic = gpGlobals->wNumMusic;
	s->wNumBattleMusic = gpGlobals->wNumBattleMusic;
	s->wNumBattleField = gpGlobals->wNumBattleField;
	s->wScreenWave = gpGlobals->wScreenWave;
	s->wCollectValue = gpGlobals->wCollectValue;
	s->wLayer = gpGlobals->wLayer;
	s->wChaseRange = gpGlobals->wChaseRange;
	s->wChasespeedChangeCycles = gpGlobals->wChasespeedChangeCycles;
	s->nFollower = gpGlobals->nFollower;
	s->dwCash = gpGlobals->dwCash;
#ifndef PAL_CLASSIC
	s->wBattleSpeed = gpGlobals->bBattleSpeed;
#else
	s->wBattleSpeed = 2;
#endif

	memcpy(s->rgParty, gpGlobals->rgParty, sizeof(gpGlobals->rgParty));
	memcpy(s->rgTrail, gpGlobals->rgTrail, sizeof(gpGlobals->rgTrail));
	s->Exp = gpGlobals->Exp;
	s->PlayerRoles = gpGlobals->g.PlayerRoles;
	memcpy(s->rgPoisonStatus, gpGlobals->rgPoisonStatus, sizeof(gpGlobals->rgPoisonStatus));
	memcpy(s->rgInventory, gpGlobals->rgInventory, sizeof(gpGlobals->rgInventory));
	memcpy(s->rgScene, gpGlobals->g.rgScene, sizeof(gpGlobals->g.rgScene));
}

static VOID
PAL_SaveGame_DOS(
   LPCSTR         szFileName,
   WORD           wSavedTimes
)
/*++
  Purpose:

    Save the current game state to file.

  Parameters:

    [IN]  szFileName - file name of saved game.

  Return value:

    None.

--*/
{
   FILE                     *fp;
   PAL_LARGE SAVEDGAME_DOS   s;
   UINT32                    i;

   //
   // Put all the data to the saved game struct.
   //
   PAL_SaveGame_Common((LPSAVEDGAME_COMMON)&s);
   //
   // Convert the WIN-style data structure to DOS-style data structure
   //
   for (i = 0; i < MAX_OBJECTS; i++)
   {
      memcpy(&s.rgObject[i], &gpGlobals->g.rgObject[i], sizeof(OBJECT_DOS));
	  if (i >= OBJECT_ITEM_START && i <= OBJECT_MAGIC_END)
	  {
         s.rgObject[i].rgwData[5] = gpGlobals->g.rgObject[i].rgwData[6];     // wFlags
	  }
   }
   memcpy(s.rgEventObject, gpGlobals->g.lprgEventObject,
      sizeof(EVENTOBJECT) * gpGlobals->g.nEventObject);

   s.wSavedTimes = wSavedTimes;

   //
   // Adjust endianness
   //
   DO_BYTESWAP(&s, sizeof(SAVEDGAME));

   //
   // Cash amount is in DWORD, so do a wordswap in Big-Endian.
   //
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
   s.dwCash = ((s.dwCash >> 16) | (s.dwCash << 16));
#endif

   //
   // Try writing to file
   //
   fp = fopen(szFileName, "wb");
   if (fp == NULL)
   {
      return;
   }

   i = PAL_MKFGetChunkSize(0, gpGlobals->f.fpSSS);
   i += sizeof(SAVEDGAME_DOS) - sizeof(EVENTOBJECT) * MAX_EVENT_OBJECTS;

   fwrite(&s, i, 1, fp);
   fclose(fp);
}

static VOID
PAL_SaveGame_WIN(
   LPCSTR         szFileName,
   WORD           wSavedTimes
)
/*++
  Purpose:

    Save the current game state to file.

  Parameters:

    [IN]  szFileName - file name of saved game.

  Return value:

    None.

--*/
{
   FILE                     *fp;
   PAL_LARGE SAVEDGAME_WIN   s;
   UINT32                    i;

   //
   // Put all the data to the saved game struct.
   //
   PAL_SaveGame_Common((LPSAVEDGAME_COMMON)&s);
   memcpy(s.rgObject, gpGlobals->g.rgObject, sizeof(gpGlobals->g.rgObject));
   memcpy(s.rgEventObject, gpGlobals->g.lprgEventObject,
      sizeof(EVENTOBJECT) * gpGlobals->g.nEventObject);

   s.wSavedTimes = wSavedTimes;

   //
   // Adjust endianness
   //
   DO_BYTESWAP(&s, sizeof(SAVEDGAME));

   //
   // Cash amount is in DWORD, so do a wordswap in Big-Endian.
   //
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
   s.dwCash = ((s.dwCash >> 16) | (s.dwCash << 16));
#endif

   //
   // Try writing to file
   //
   fp = fopen(szFileName, "wb");
   if (fp == NULL)
   {
      return;
   }

   i = PAL_MKFGetChunkSize(0, gpGlobals->f.fpSSS);
   i += sizeof(SAVEDGAME_WIN) - sizeof(EVENTOBJECT) * MAX_EVENT_OBJECTS;

   fwrite(&s, i, 1, fp);
   fclose(fp);
}

VOID
PAL_SaveGame(
   LPCSTR         szFileName,
   WORD           wSavedTimes
)
{
	if (gConfig.fIsWIN95)
		PAL_SaveGame_WIN(szFileName, wSavedTimes);
	else
		PAL_SaveGame_DOS(szFileName, wSavedTimes);
}

VOID
PAL_InitGameData(
   INT         iSaveSlot
)
/*++
  Purpose:

    Initialize the game data (used when starting a new game or loading a saved game).

  Parameters:

    [IN]  iSaveSlot - Slot of saved game.

  Return value:

    None.

--*/
{
   PAL_InitGlobalGameData();

   gpGlobals->bCurrentSaveSlot = (BYTE)iSaveSlot;

   //
   // try loading from the saved game file.
   //
   if (iSaveSlot == 0 || PAL_LoadGame(va("%s%d%s", PAL_SAVE_PREFIX, iSaveSlot, ".rpg")) != 0)
   {
      //
      // Cannot load the saved game file. Load the defaults.
      //
      PAL_LoadDefaultGame();
   }

   gpGlobals->fGameStart = TRUE;
   gpGlobals->fNeedToFadeIn = FALSE;
   gpGlobals->iCurInvMenuItem = 0;
   gpGlobals->fInBattle = FALSE;

   memset(gpGlobals->rgPlayerStatus, 0, sizeof(gpGlobals->rgPlayerStatus));

   PAL_UpdateEquipments();
}

BOOL
PAL_AddItemToInventory(
   WORD          wObjectID,
   INT           iNum
)
/*++
  Purpose:

    Add or remove the specified kind of item in the inventory.

  Parameters:

    [IN]  wObjectID - object number of the item.

    [IN]  iNum - number to be added (positive value) or removed (negative value).

  Return value:

    TRUE if succeeded, FALSE if failed.

--*/
{
   int          index;
   BOOL         fFound;

   if (wObjectID == 0)
   {
      return FALSE;
   }

   if (iNum == 0)
   {
      iNum = 1;
   }

   index = 0;
   fFound = FALSE;

   //
   // Search for the specified item in the inventory
   //
   while (index < MAX_INVENTORY)
   {
      if (gpGlobals->rgInventory[index].wItem == wObjectID)
      {
         fFound = TRUE;
         break;
      }
      else if (gpGlobals->rgInventory[index].wItem == 0)
      {
         break;
      }
      index++;
   }

   if (iNum > 0)
   {
      //
      // Add item
      //
      if (index >= MAX_INVENTORY)
      {
         //
         // inventory is full. cannot add item
         //
         return FALSE;
      }

      if (fFound)
      {
         gpGlobals->rgInventory[index].nAmount += iNum;
         if (gpGlobals->rgInventory[index].nAmount > 99)
         {
            //
            // Maximum number is 99
            //
            gpGlobals->rgInventory[index].nAmount = 99;
         }
      }
      else
      {
         gpGlobals->rgInventory[index].wItem = wObjectID;
         if (iNum > 99)
         {
            iNum = 99;
         }
         gpGlobals->rgInventory[index].nAmount = iNum;
      }

      return TRUE;
   }
   else
   {
      //
      // Remove item
      //
      if (fFound)
      {
         iNum *= -1;
         if (gpGlobals->rgInventory[index].nAmount < iNum)
         {
            //
            // This item has been run out
            //
            gpGlobals->rgInventory[index].nAmount = 0;
            return FALSE;
         }

         gpGlobals->rgInventory[index].nAmount -= iNum;
         return TRUE;
      }

      return FALSE;
   }
}

INT
PAL_GetItemAmount(
   WORD        wItem
)
/*++
  Purpose:

    Get the amount of the specified item in the inventory.

  Parameters:

    [IN]  wItem - the object ID of the item.

  Return value:

    The amount of the item in the inventory.

--*/
{
   int i;

   for (i = 0; i < MAX_INVENTORY; i++)
   {
      if (gpGlobals->rgInventory[i].wItem == 0)
      {
         break;
      }

      if (gpGlobals->rgInventory[i].wItem == wItem)
      {
         return gpGlobals->rgInventory[i].nAmount;
      }
   }

   return 0;
}

VOID
PAL_CompressInventory(
   VOID
)
/*++
  Purpose:

    Remove all the items in inventory which has a number of zero.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int i, j;

   j = 0;

   for (i = 0; i < MAX_INVENTORY; i++)
   {
      if (gpGlobals->rgInventory[i].wItem == 0)
      {
         break;
      }

      if (gpGlobals->rgInventory[i].nAmount > 0)
      {
         gpGlobals->rgInventory[j] = gpGlobals->rgInventory[i];
         j++;
      }
   }

   for (; j < MAX_INVENTORY; j++)
   {
      gpGlobals->rgInventory[j].nAmount = 0;
      gpGlobals->rgInventory[j].nAmountInUse = 0;
      gpGlobals->rgInventory[j].wItem = 0;
   }
}

BOOL
PAL_IncreaseHPMP(
   WORD          wPlayerRole,
   SHORT         sHP,
   SHORT         sMP
)
/*++
  Purpose:

    Increase or decrease player's HP and/or MP.

  Parameters:

    [IN]  wPlayerRole - the number of player role.

    [IN]  sHP - number of HP to be increased (positive value) or decrased
                (negative value).

    [IN]  sMP - number of MP to be increased (positive value) or decrased
                (negative value).

  Return value:

    TRUE if the operation is succeeded, FALSE if not.

--*/
{
   BOOL           fSuccess = FALSE;

   //
   // Only care about alive players
   //
   if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] > 0)
   {
      //
      // change HP
      //
      gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] += sHP;

      if ((SHORT)(gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole]) < 0)
      {
         gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] = 0;
      }
      else if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] >
         gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole])
      {
         gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] =
            gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole];
      }

      //
      // Change MP
      //
      gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] += sMP;

      if ((SHORT)(gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole]) < 0)
      {
         gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] = 0;
      }
      else if (gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] >
         gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole])
      {
         gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] =
            gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole];
      }

      fSuccess = TRUE;
   }

   return fSuccess;
}

VOID
PAL_UpdateEquipments(
   VOID
)
/*++
  Purpose:

    Update the effects of all equipped items for all players.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int      i, j;
   WORD     w;

   memset(&(gpGlobals->rgEquipmentEffect), 0, sizeof(gpGlobals->rgEquipmentEffect));

   for (i = 0; i < MAX_PLAYER_ROLES; i++)
   {
      for (j = 0; j < MAX_PLAYER_EQUIPMENTS; j++)
      {
         w = gpGlobals->g.PlayerRoles.rgwEquipment[j][i];

         if (w != 0)
         {
            gpGlobals->g.rgObject[w].item.wScriptOnEquip =
               PAL_RunTriggerScript(gpGlobals->g.rgObject[w].item.wScriptOnEquip, (WORD)i);
         }
      }
   }
}

VOID
PAL_RemoveEquipmentEffect(
   WORD         wPlayerRole,
   WORD         wEquipPart
)
/*++
  Purpose:

    Remove all the effects of the equipment for the player.

  Parameters:

    [IN]  wPlayerRole - the player role.

    [IN]  wEquipPart - the part of the equipment.

  Return value:

    None.

--*/
{
   WORD       *p;
   int         i, j;

   p = (WORD *)(&gpGlobals->rgEquipmentEffect[wEquipPart]); // HACKHACK

   for (i = 0; i < sizeof(PLAYERROLES) / sizeof(PLAYERS); i++)
   {
      p[i * MAX_PLAYER_ROLES + wPlayerRole] = 0;
   }

   //
   // Reset some parameters to default when appropriate
   //
   if (wEquipPart == kBodyPartHand)
   {
      //
      // reset the dual attack status
      //
      gpGlobals->rgPlayerStatus[wPlayerRole][kStatusDualAttack] = 0;
   }
   else if (wEquipPart == kBodyPartWear)
   {
      //
      // Remove all poisons leveled 99
      //
      for (i = 0; i <= (short)gpGlobals->wMaxPartyMemberIndex; i++)
      {
         if (gpGlobals->rgParty[i].wPlayerRole == wPlayerRole)
         {
            wPlayerRole = i;
            break;
         }
      }

      if (i <= (short)gpGlobals->wMaxPartyMemberIndex)
      {
         j = 0;

         for (i = 0; i < MAX_POISONS; i++)
         {
            WORD w = gpGlobals->rgPoisonStatus[i][wPlayerRole].wPoisonID;

            if (w == 0)
            {
               break;
            }

            if (gpGlobals->g.rgObject[w].poison.wPoisonLevel < 99)
            {
               gpGlobals->rgPoisonStatus[j][wPlayerRole] =
                  gpGlobals->rgPoisonStatus[i][wPlayerRole];
               j++;
            }
         }

         while (j < MAX_POISONS)
         {
            gpGlobals->rgPoisonStatus[j][wPlayerRole].wPoisonID = 0;
            gpGlobals->rgPoisonStatus[j][wPlayerRole].wPoisonScript = 0;
            j++;
         }
      }
   }
}

VOID
PAL_AddPoisonForPlayer(
   WORD           wPlayerRole,
   WORD           wPoisonID
)
/*++
  Purpose:

    Add the specified poison to the player.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wPoisonID - the poison to be added.

  Return value:

    None.

--*/
{
   int         i, index;
   WORD        w;

   for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
   {
      if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
      {
         break;
      }
   }

   if (index > gpGlobals->wMaxPartyMemberIndex)
   {
      return; // don't go further
   }

   for (i = 0; i < MAX_POISONS; i++)
   {
      w = gpGlobals->rgPoisonStatus[i][index].wPoisonID;

      if (w == 0)
      {
         break;
      }

      if (w == wPoisonID)
      {
         return; // already poisoned
      }
   }

   if (i < MAX_POISONS)
   {
      gpGlobals->rgPoisonStatus[i][index].wPoisonID = wPoisonID;
      gpGlobals->rgPoisonStatus[i][index].wPoisonScript =
         gpGlobals->g.rgObject[wPoisonID].poison.wPlayerScript;
   }
}

VOID
PAL_CurePoisonByKind(
   WORD           wPlayerRole,
   WORD           wPoisonID
)
/*++
  Purpose:

    Remove the specified poison from the player.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wPoisonID - the poison to be removed.

  Return value:

    None.

--*/
{
   int i, index;

   for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
   {
      if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
      {
         break;
      }
   }

   if (index > gpGlobals->wMaxPartyMemberIndex)
   {
      return; // don't go further
   }

   for (i = 0; i < MAX_POISONS; i++)
   {
      if (gpGlobals->rgPoisonStatus[i][index].wPoisonID == wPoisonID)
      {
         gpGlobals->rgPoisonStatus[i][index].wPoisonID = 0;
         gpGlobals->rgPoisonStatus[i][index].wPoisonScript = 0;
      }
   }
}

VOID
PAL_CurePoisonByLevel(
   WORD           wPlayerRole,
   WORD           wMaxLevel
)
/*++
  Purpose:

    Remove the poisons which have a maximum level of wMaxLevel from the player.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wMaxLevel - the maximum level of poisons to be removed.

  Return value:

    None.

--*/
{
   int        i, index;
   WORD       w;

   for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
   {
      if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
      {
         break;
      }
   }

   if (index > gpGlobals->wMaxPartyMemberIndex)
   {
      return; // don't go further
   }

   for (i = 0; i < MAX_POISONS; i++)
   {
      w = gpGlobals->rgPoisonStatus[i][index].wPoisonID;

      if (gpGlobals->g.rgObject[w].poison.wPoisonLevel <= wMaxLevel)
      {
         gpGlobals->rgPoisonStatus[i][index].wPoisonID = 0;
         gpGlobals->rgPoisonStatus[i][index].wPoisonScript = 0;
      }
   }
}

BOOL
PAL_IsPlayerPoisonedByLevel(
   WORD           wPlayerRole,
   WORD           wMinLevel
)
/*++
  Purpose:

    Check if the player is poisoned by poisons at a minimum level of wMinLevel.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wMinLevel - the minimum level of poison.

  Return value:

    TRUE if the player is poisoned by poisons at a minimum level of wMinLevel;
    FALSE if not.

--*/
{
   int         i, index;
   WORD        w;

   for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
   {
      if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
      {
         break;
      }
   }

   if (index > gpGlobals->wMaxPartyMemberIndex)
   {
      return FALSE; // don't go further
   }

   for (i = 0; i < MAX_POISONS; i++)
   {
      w = gpGlobals->rgPoisonStatus[i][index].wPoisonID;
      w = gpGlobals->g.rgObject[w].poison.wPoisonLevel;

      if (w >= 99)
      {
         //
         // Ignore poisons which has a level of 99 (usually effect of equipment)
         //
         continue;
      }

      if (w >= wMinLevel)
      {
         return TRUE;
      }
   }

   return FALSE;
}

BOOL
PAL_IsPlayerPoisonedByKind(
   WORD           wPlayerRole,
   WORD           wPoisonID
)
/*++
  Purpose:

    Check if the player is poisoned by the specified poison.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wPoisonID - the poison to be checked.

  Return value:

    TRUE if player is poisoned by the specified poison;
    FALSE if not.

--*/
{
   int i, index;

   for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
   {
      if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
      {
         break;
      }
   }

   if (index > gpGlobals->wMaxPartyMemberIndex)
   {
      return FALSE; // don't go further
   }

   for (i = 0; i < MAX_POISONS; i++)
   {
      if (gpGlobals->rgPoisonStatus[i][index].wPoisonID == wPoisonID)
      {
         return TRUE;
      }
   }

   return FALSE;
}

WORD
PAL_GetPlayerAttackStrength(
   WORD           wPlayerRole
)
/*++
  Purpose:

    Get the player's attack strength, count in the effect of equipments.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    The total attack strength of the player.

--*/
{
   WORD       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwAttackStrength[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwAttackStrength[wPlayerRole];
   }

   return w;
}

WORD
PAL_GetPlayerMagicStrength(
   WORD           wPlayerRole
)
/*++
  Purpose:

    Get the player's magic strength, count in the effect of equipments.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    The total magic strength of the player.

--*/
{
   WORD       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwMagicStrength[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwMagicStrength[wPlayerRole];
   }

   return w;
}

WORD
PAL_GetPlayerDefense(
   WORD           wPlayerRole
)
/*++
  Purpose:

    Get the player's defense value, count in the effect of equipments.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    The total defense value of the player.

--*/
{
   WORD       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwDefense[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwDefense[wPlayerRole];
   }

   return w;
}

WORD
PAL_GetPlayerDexterity(
   WORD           wPlayerRole
)
/*++
  Purpose:

    Get the player's dexterity, count in the effect of equipments.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    The total dexterity of the player.

--*/
{
   WORD       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwDexterity[wPlayerRole];

#ifdef PAL_CLASSIC
   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
#else
   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS - 1; i++)
#endif
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwDexterity[wPlayerRole];
   }

   return w;
}

WORD
PAL_GetPlayerFleeRate(
   WORD           wPlayerRole
)
/*++
  Purpose:

    Get the player's flee rate, count in the effect of equipments.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    The total flee rate of the player.

--*/
{
   WORD       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwFleeRate[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwFleeRate[wPlayerRole];
   }

   return w;
}

WORD
PAL_GetPlayerPoisonResistance(
   WORD           wPlayerRole
)
/*++
  Purpose:

    Get the player's resistance to poisons, count in the effect of equipments.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    The total resistance to poisons of the player.

--*/
{
   WORD       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwPoisonResistance[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwPoisonResistance[wPlayerRole];
   }

   if (w > 100)
   {
      w = 100;
   }

   return w;
}

WORD
PAL_GetPlayerElementalResistance(
   WORD           wPlayerRole,
   INT            iAttrib
)
/*++
  Purpose:

    Get the player's resistance to attributed magics, count in the effect
    of equipments.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  iAttrib - the attribute of magics.

  Return value:

    The total resistance to the attributed magics of the player.

--*/
{
   WORD       w;
   int        i;

   w = gpGlobals->g.PlayerRoles.rgwElementalResistance[iAttrib][wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwElementalResistance[iAttrib][wPlayerRole];
   }

   if (w > 100)
   {
      w = 100;
   }

   return w;
}

WORD
PAL_GetPlayerBattleSprite(
   WORD             wPlayerRole
)
/*++
  Purpose:

    Get player's battle sprite.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    Number of the player's battle sprite.

--*/
{
   int       i;
   WORD      w;

   w = gpGlobals->g.PlayerRoles.rgwSpriteNumInBattle[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      if (gpGlobals->rgEquipmentEffect[i].rgwSpriteNumInBattle[wPlayerRole] != 0)
      {
         w = gpGlobals->rgEquipmentEffect[i].rgwSpriteNumInBattle[wPlayerRole];
      }
   }

   return w;
}

WORD
PAL_GetPlayerCooperativeMagic(
   WORD             wPlayerRole
)
/*++
  Purpose:

    Get player's cooperative magic.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    Object ID of the player's cooperative magic.

--*/
{
   int       i;
   WORD      w;

   w = gpGlobals->g.PlayerRoles.rgwCooperativeMagic[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      if (gpGlobals->rgEquipmentEffect[i].rgwCooperativeMagic[wPlayerRole] != 0)
      {
         w = gpGlobals->rgEquipmentEffect[i].rgwCooperativeMagic[wPlayerRole];
      }
   }

   return w;
}

BOOL
PAL_PlayerCanAttackAll(
   WORD        wPlayerRole
)
/*++
  Purpose:

    Check if the player can attack all of the enemies in one move.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    TRUE if player can attack all of the enemies in one move, FALSE if not.

--*/
{
   int       i;
   BOOL      f;

   f = FALSE;

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      if (gpGlobals->rgEquipmentEffect[i].rgwAttackAll[wPlayerRole] != 0)
      {
         f = TRUE;
         break;
      }
   }

   return f;
}

BOOL
PAL_AddMagic(
   WORD           wPlayerRole,
   WORD           wMagic
)
/*++
  Purpose:

    Add a magic to the player.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wMagic - the object ID of the magic.

  Return value:

    TRUE if succeeded, FALSE if failed.

--*/
{
   int            i;

   for (i = 0; i < MAX_PLAYER_MAGICS; i++)
   {
      if (gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] == wMagic)
      {
         //
         // already have this magic
         //
         return FALSE;
      }
   }

   for (i = 0; i < MAX_PLAYER_MAGICS; i++)
   {
      if (gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] == 0)
      {
         break;
      }
   }

   if (i >= MAX_PLAYER_MAGICS)
   {
      //
      // Not enough slots
      //
      return FALSE;
   }

   gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] = wMagic;
   return TRUE;
}

VOID
PAL_RemoveMagic(
   WORD           wPlayerRole,
   WORD           wMagic
)
/*++
  Purpose:

    Remove a magic to the player.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wMagic - the object ID of the magic.

  Return value:

    None.

--*/
{
   int            i;

   for (i = 0; i < MAX_PLAYER_MAGICS; i++)
   {
      if (gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] == wMagic)
      {
         gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] = 0;
         break;
      }
   }
}

VOID
PAL_SetPlayerStatus(
   WORD         wPlayerRole,
   WORD         wStatusID,
   WORD         wNumRound
)
/*++
  Purpose:

    Set one of the statuses for the player.

  Parameters:

    [IN]  wPlayerRole - the player ID.

    [IN]  wStatusID - the status to be set.

    [IN]  wNumRound - the effective rounds of the status.

  Return value:

    None.

--*/
{
#ifndef PAL_CLASSIC
   if (wStatusID == kStatusSlow &&
      gpGlobals->rgPlayerStatus[wPlayerRole][kStatusHaste] > 0)
   {
      //
      // Remove the haste status
      //
      PAL_RemovePlayerStatus(wPlayerRole, kStatusHaste);
      return;
   }

   if (wStatusID == kStatusHaste &&
      gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSlow] > 0)
   {
      //
      // Remove the slow status
      //
      PAL_RemovePlayerStatus(wPlayerRole, kStatusSlow);
      return;
   }
#endif

   switch (wStatusID)
   {
   case kStatusConfused:
   case kStatusSleep:
   case kStatusSilence:
#ifdef PAL_CLASSIC
   case kStatusParalyzed:
#else
   case kStatusSlow:
#endif
      //
      // for "bad" statuses, don't set the status when we already have it
      //
      if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] != 0 &&
         gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] == 0)
      {
         gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = wNumRound;
      }
      break;

   case kStatusPuppet:
      //
      // only allow dead players for "puppet" status
      //
      if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0 &&
         gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] < wNumRound)
      {
         gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = wNumRound;
      }
      break;

   case kStatusBravery:
   case kStatusProtect:
   case kStatusDualAttack:
   case kStatusHaste:
      //
      // for "good" statuses, reset the status if the status to be set lasts longer
      //
      if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] != 0 &&
         gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] < wNumRound)
      {
         gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = wNumRound;
      }
      break;

   default:
      assert(FALSE);
      break;
   }
}

VOID
PAL_RemovePlayerStatus(
   WORD         wPlayerRole,
   WORD         wStatusID
)
/*++
  Purpose:

    Remove one of the status for player.

  Parameters:

    [IN]  wPlayerRole - the player ID.

    [IN]  wStatusID - the status to be set.

  Return value:

    None.

--*/
{
   //
   // Don't remove effects of equipments
   //
   if (gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] <= 999)
   {
      gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = 0;
   }
}

VOID
PAL_ClearAllPlayerStatus(
   VOID
)
/*++
  Purpose:

    Clear all player status.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int      i, j;

   for (i = 0; i < MAX_PLAYER_ROLES; i++)
   {
      for (j = 0; j < kStatusAll; j++)
      {
         //
         // Don't remove effects of equipments
         //
         if (gpGlobals->rgPlayerStatus[i][j] <= 999)
         {
            gpGlobals->rgPlayerStatus[i][j] = 0;
         }
      }
   }
}

VOID
PAL_PlayerLevelUp(
   WORD          wPlayerRole,
   WORD          wNumLevel
)
/*++
  Purpose:

    Increase the player's level by wLevels.

  Parameters:

    [IN]  wPlayerRole - player role ID.

    [IN]  wNumLevel - number of levels to be increased.

  Return value:

    None.

--*/
{
   WORD          i;

   //
   // Add the level
   //
   gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] += wNumLevel;
   if (gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] > MAX_LEVELS)
   {
      gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole] = MAX_LEVELS;
   }

   for (i = 0; i < wNumLevel; i++)
   {
      //
      // Increase player's stats
      //
      gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole] += 10 + RandomLong(0, 8);
      gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole] += 8 + RandomLong(0, 6);
      gpGlobals->g.PlayerRoles.rgwAttackStrength[wPlayerRole] += 4 + RandomLong(0, 1);
      gpGlobals->g.PlayerRoles.rgwMagicStrength[wPlayerRole] += 4 + RandomLong(0, 1);
      gpGlobals->g.PlayerRoles.rgwDefense[wPlayerRole] += 2 + RandomLong(0, 1);
      gpGlobals->g.PlayerRoles.rgwDexterity[wPlayerRole] += 2 + RandomLong(0, 1);
      gpGlobals->g.PlayerRoles.rgwFleeRate[wPlayerRole] += 2;
   }

#define STAT_LIMIT(t) { if ((t) > 999) (t) = 999; }
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwMaxMP[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwAttackStrength[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwMagicStrength[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwDefense[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwDexterity[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles.rgwFleeRate[wPlayerRole]);
#undef STAT_LIMIT

   //
   // Reset experience points to zero
   //
   gpGlobals->Exp.rgPrimaryExp[wPlayerRole].wExp = 0;
   gpGlobals->Exp.rgPrimaryExp[wPlayerRole].wLevel =
      gpGlobals->g.PlayerRoles.rgwLevel[wPlayerRole];
}
