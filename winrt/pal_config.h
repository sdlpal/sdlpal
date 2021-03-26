/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2021, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// winrt/pal_config.h: Windows Runtime specific header.
//            @Author: Lou Yihua <louyihua@21cn.com>, 2017.
//

#pragma once

#define PAL_PREFIX            ""
#define PAL_SAVE_PREFIX       ""
#define PAL_CONFIG_PREFIX     UTIL_ConfigPath()
#define PAL_HAS_TOUCH         1
#define PAL_AUDIO_DEFAULT_BUFFER_SIZE   4096
#define PAL_DEFAULT_WINDOW_WIDTH   640
#define PAL_DEFAULT_WINDOW_HEIGHT  400

#if SDL_VERSION_ATLEAST(2,0,0)
# define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI)
#else
# define PAL_VIDEO_INIT_FLAGS  (SDL_HWSURFACE | SDL_RESIZABLE | (gConfig.fFullScreen ? SDL_FULLSCREEN : 0))
#endif

#define PAL_SDL_INIT_FLAGS	(SDL_INIT_VIDEO | SDL_INIT_AUDIO)

#define PAL_PLATFORM         "Windows Runtime"
#define PAL_CREDIT           "(Unknown)"
#define PAL_PORTYEAR         "2015"

#define PAL_HAS_CONFIG_PAGE  1

#if NTDDI_VERSION >= NTDDI_WIN10
# define PAL_HAS_NATIVEMIDI  1
#endif

#define PAL_FILESYSTEM_IGNORE_CASE 1

#define PAL_NATIVE_PATH_SEPARATOR "\\"
#define PAL_PATH_SEPARATORS "\\/"

#define PAL_IS_PATH_SEPARATOR(x) ((x) == '\\' || (x) == '/')

#define strtok_r strtok_s
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define wcsdup _wcsdup

PAL_C_LINKAGE char* strcasestr(const char *, const char *);
#define PAL_NEED_STRCASESTR 1

#if WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP
#define PAL_HAS_GLSL 1
#endif

PAL_C_LINKAGE_BEGIN

LPCSTR
UTIL_BasePath(
   VOID
);

LPCSTR
UTIL_SavePath(
   VOID
);

LPCSTR
UTIL_ConfigPath(
   VOID
);

LPCSTR
UTIL_ScreenShotPath(
   VOID
);

BOOL
UTIL_TouchEnabled(
   VOID
);

PAL_C_LINKAGE_END
