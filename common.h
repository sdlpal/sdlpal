//
// Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
// All rights reserved.
// Modified by Lou Yihua <louyihua@21cn.com> with unicode support, 2015.
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

#ifndef _COMMON_H
#define _COMMON_H

#ifndef ENABLE_REVISIED_BATTLE
# define PAL_CLASSIC        1
#endif

#ifdef __cplusplus
extern "C"
{
#endif

#if defined(_MSC_VER) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <stdarg.h>
#include <assert.h>

#include "SDL.h"
#include "SDL_endian.h"

#if SDL_VERSION_ATLEAST(2,0,0)

#define SDLK_KP1     SDLK_KP_1
#define SDLK_KP2     SDLK_KP_2
#define SDLK_KP3     SDLK_KP_3
#define SDLK_KP4     SDLK_KP_4
#define SDLK_KP5     SDLK_KP_5
#define SDLK_KP6     SDLK_KP_6
#define SDLK_KP7     SDLK_KP_7
#define SDLK_KP8     SDLK_KP_8
#define SDLK_KP9     SDLK_KP_9
#define SDLK_KP0     SDLK_KP_0

#define SDL_HWSURFACE     0

#endif

#ifndef max
#define max(a, b)    (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a, b)    (((a) < (b)) ? (a) : (b))
#endif

/* This is need when compiled with SDL 1.2 */
#ifndef SDL_FORCE_INLINE
#if defined(_MSC_VER)
#define SDL_FORCE_INLINE __forceinline
#elif ( (defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__) )
#define SDL_FORCE_INLINE __attribute__((always_inline)) static __inline__
#else
#define SDL_FORCE_INLINE static SDL_INLINE
#endif
#endif /* SDL_FORCE_INLINE not defined */

#if defined(_MSC_VER)
#define PAL_FORCE_INLINE static SDL_FORCE_INLINE
#else
#define PAL_FORCE_INLINE SDL_FORCE_INLINE
#endif

#if defined (__SYMBIAN32__)

#undef  _WIN32
#undef  SDL_INIT_JOYSTICK
#define SDL_INIT_JOYSTICK     0
#define PAL_HAS_MOUSE         1
#define PAL_PREFIX            "e:/data/pal/"
#define PAL_SAVE_PREFIX       "e:/data/pal/"
# ifdef __S60_5X__
#  define PAL_DEFAULT_WINDOW_WIDTH   640
#  define PAL_DEFAULT_WINDOW_HEIGHT  360
# else
#  define PAL_DEFAULT_WINDOW_WIDTH   320
#  define PAL_DEFAULT_WINDOW_HEIGHT  240
# endif
# if SDL_VERSION_ATLEAST(2,0,0)
#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE)
# else
#  define PAL_VIDEO_INIT_FLAGS  (SDL_SWSURFACE | (gConfig.fFullScreen ? SDL_FULLSCREEN : 0))
# endif

# define PAL_PLATFORM         "  Symbian S60 \x79FB\x690D (c) 2009, netwan."
# define PAL_CREDIT           "netwan"
# define PAL_PORTYEAR         "2009"

#elif defined (GEKKO)

#define PAL_HAS_JOYSTICKS     1
#define PAL_HAS_MOUSE         0
#define PAL_PREFIX            "SD:/apps/sdlpal/"
#define PAL_SAVE_PREFIX       "SD:/apps/sdlpal/"

#define PAL_DEFAULT_WINDOW_WIDTH   640
#define PAL_DEFAULT_WINDOW_HEIGHT  480

# if SDL_VERSION_ATLEAST(2,0,0)
#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE)
# else
#  define PAL_VIDEO_INIT_FLAGS  (SDL_SWSURFACE | (gConfig.fFullScreen ? SDL_FULLSCREEN : 0))
# endif

# define PAL_PLATFORM         "Nintendo WII"
# define PAL_CREDIT           "Rikku2000"
# define PAL_PORTYEAR         "2012"

#elif defined (PSP)

#define PAL_HAS_JOYSTICKS     0
#define PAL_PREFIX            "ms0:/"
#define PAL_SAVE_PREFIX       "ms0:/PSP/SAVEDATA/SDLPAL/"

#define PAL_DEFAULT_WINDOW_WIDTH   320
#define PAL_DEFAULT_WINDOW_HEIGHT  240

# if SDL_VERSION_ATLEAST(2,0,0)
#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN)
# else
#  define PAL_VIDEO_INIT_FLAGS  (SDL_SWSURFACE | SDL_FULLSCREEN)
# endif

# define PAL_PLATFORM         "Sony PSP"
# define PAL_CREDIT           "(Unknown)"
# define PAL_PORTYEAR         "2011"

#elif defined(GPH) || defined(DINGOO)

#define PAL_PREFIX            "./"
#define PAL_SAVE_PREFIX       "./"

#  define PAL_DEFAULT_WINDOW_WIDTH   320
#  define PAL_DEFAULT_WINDOW_HEIGHT  240

# if SDL_VERSION_ATLEAST(2,0,0)
#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN)
# else
#  define PAL_VIDEO_INIT_FLAGS  (SDL_SWSURFACE | SDL_FULLSCREEN)
# endif

# if defined(GPH)
#  define PAL_PLATFORM         "GPH Caanoo & Wiz"
# else
#  define PAL_PLATFORM         "DINGOO & Dingux"
# endif
# define PAL_CREDIT           "Rikku2000"
# define PAL_PORTYEAR         "2011"

#elif defined(NDS)

#define PAL_PREFIX            "./"
#define PAL_SAVE_PREFIX       "./"

#  define PAL_DEFAULT_WINDOW_WIDTH   293
#  define PAL_DEFAULT_WINDOW_HEIGHT  196

# if SDL_VERSION_ATLEAST(2,0,0)
#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN)
# else
#  define PAL_VIDEO_INIT_FLAGS  (SDL_SWSURFACE | SDL_FULLSCREEN)
# endif

# define PAL_PLATFORM         "Nintendo DS"
# define PAL_CREDIT           "(Unknown)"
# define PAL_PORTYEAR         "2012"

#elif defined (__IOS__)

#define PAL_PREFIX            UTIL_BasePath()
#define PAL_SAVE_PREFIX       UTIL_SavePath()
#define PAL_HAS_TOUCH         1
#define PAL_DEFAULT_WINDOW_WIDTH   320
#define PAL_DEFAULT_WINDOW_HEIGHT  200

# if SDL_VERSION_ATLEAST(2,0,0)
#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN)
# else
#  define PAL_VIDEO_INIT_FLAGS  (SDL_HWSURFACE | SDL_FULLSCREEN)
# endif

# define PAL_PLATFORM         "Apple iOS"
# define PAL_CREDIT           "(Unknown)"
# define PAL_PORTYEAR         "2015"

#elif defined (__ANDROID__)

#define PAL_PREFIX            "/mnt/sdcard/sdlpal/"
#define PAL_SAVE_PREFIX       "/mnt/sdcard/sdlpal/"
#define PAL_HAS_TOUCH         1
#define PAL_DEFAULT_WINDOW_WIDTH   320
#define PAL_DEFAULT_WINDOW_HEIGHT  200

# if SDL_VERSION_ATLEAST(2,0,0)
#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN)
# else
#  define PAL_VIDEO_INIT_FLAGS  (SDL_HWSURFACE | SDL_FULLSCREEN)
# endif

# define PAL_PLATFORM         "Android"
# define PAL_CREDIT           "Rikku2000"
# define PAL_PORTYEAR         "2013"

#elif defined(__WINRT__)

#define PAL_PREFIX            UTIL_BasePath()
#define PAL_SAVE_PREFIX       UTIL_SavePath()
#define PAL_CONFIG_PREFIX     UTIL_ConfigPath()
#define PAL_SCREENSHOT_PREFIX UTIL_ScreenShotPath()
#define PAL_HAS_TOUCH         1
#define PAL_AUDIO_DEFAULT_BUFFER_SIZE   4096
#define PAL_DEFAULT_WINDOW_WIDTH   320
#define PAL_DEFAULT_WINDOW_HEIGHT  200

# if SDL_VERSION_ATLEAST(2,0,0)
#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN)
# else
#  define PAL_VIDEO_INIT_FLAGS  (SDL_HWSURFACE | SDL_RESIZABLE | (gConfig.fFullScreen ? SDL_FULLSCREEN : 0))
# endif

# define PAL_PLATFORM         "Windows Runtime"
# define PAL_CREDIT           "(Unknown)"
# define PAL_PORTYEAR         "2015"

#else

# ifndef PAL_HAS_JOYSTICKS
#  define PAL_HAS_JOYSTICKS    1
# endif

#if SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION <= 2
# define PAL_ALLOW_KEYREPEAT   1
# define PAL_HAS_SDLCD         1
#endif

# define PAL_PREFIX            "./"
# define PAL_SAVE_PREFIX       "./"

# define PAL_DEFAULT_WINDOW_WIDTH   640
# define PAL_DEFAULT_WINDOW_HEIGHT  400
# define PAL_DEFAULT_FULLSCREEN_HEIGHT 480

# if SDL_VERSION_ATLEAST(2,0,0)
#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | (gConfig.fFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0))
# else
#  define PAL_VIDEO_INIT_FLAGS  (SDL_HWSURFACE | SDL_RESIZABLE | (gConfig.fFullScreen ? SDL_FULLSCREEN : 0))
# endif

# define PAL_PLATFORM         NULL
# define PAL_CREDIT           NULL
# define PAL_PORTYEAR         NULL

#endif

#ifndef PAL_DEFAULT_FULLSCREEN_HEIGHT
# define PAL_DEFAULT_FULLSCREEN_HEIGHT PAL_DEFAULT_WINDOW_HEIGHT
#endif

/* Default for 1024 samples */
#ifndef PAL_AUDIO_DEFAULT_BUFFER_SIZE
# define PAL_AUDIO_DEFAULT_BUFFER_SIZE   1024
#endif

#ifndef PAL_HAS_SDLCD
# define PAL_HAS_SDLCD        0
#endif

#ifndef PAL_HAS_MP3
# define PAL_HAS_MP3          1   /* Try always enable MP3. If compilation/run failed, please change this value to 0. */
#endif
#ifndef PAL_HAS_OGG
# define PAL_HAS_OGG          1   /* Try always enable OGG. If compilation/run failed, please change this value to 0. */
#endif

#ifndef SDL_INIT_CDROM
# define SDL_INIT_CDROM       0	  /* Compatibility with SDL 1.2 */
#endif

#ifndef SDL_AUDIO_BITSIZE
# define SDL_AUDIO_BITSIZE(x)         (x & 0xFF)
#endif

#ifndef PAL_CONFIG_PREFIX
# define PAL_CONFIG_PREFIX PAL_PREFIX
#endif

#ifndef PAL_SCREENSHOT_PREFIX
# define PAL_SCREENSHOT_PREFIX PAL_SAVE_PREFIX
#endif

#ifdef _WIN32

#include <windows.h>
#include <io.h>

#if defined(_MSC_VER)
# if _MSC_VER < 1900
#  define vsnprintf _vsnprintf
#  define snprintf _snprintf
# endif
# define strdup _strdup
# pragma warning (disable:4244)
#endif

#ifndef _LPCBYTE_DEFINED
# define _LPCBYTE_DEFINED
typedef const BYTE *LPCBYTE;
#endif

#ifndef __WINRT__
# define PAL_HAS_NATIVEMIDI  1
#endif

#else

#include <unistd.h>
#include <dirent.h>

#ifndef FALSE
#define FALSE               0
#endif
#ifndef TRUE
#define TRUE                1
#endif
#define VOID                void
typedef char                CHAR;
typedef wchar_t             WCHAR;
typedef short               SHORT;
typedef long                LONG;

typedef unsigned long       ULONG, *PULONG;
typedef unsigned short      USHORT, *PUSHORT;
typedef unsigned char       UCHAR, *PUCHAR;

typedef unsigned short      WORD, *LPWORD;
typedef unsigned int        DWORD, *LPDWORD;
typedef int                 INT, *LPINT;
#ifndef __OBJC__
typedef int                 BOOL, *LPBOOL;
#endif
typedef unsigned int        UINT, *PUINT, UINT32, *PUINT32;
typedef unsigned char       BYTE, *LPBYTE;
typedef const BYTE         *LPCBYTE;
typedef float               FLOAT, *LPFLOAT;
typedef void               *LPVOID;
typedef const void         *LPCVOID;
typedef CHAR               *LPSTR;
typedef const CHAR         *LPCSTR;
typedef WCHAR              *LPWSTR;
typedef const WCHAR        *LPCWSTR;

#endif

#ifndef PAL_HAS_NATIVEMIDI
#define PAL_HAS_NATIVEMIDI  0
#endif

#if defined (__SYMBIAN32__)
#define PAL_LARGE           static
#else
#define PAL_LARGE           /* */
#endif

#define __WIDETEXT(quote) L##quote
#define WIDETEXT(quote) __WIDETEXT(quote)

// For SDL 1.2 compatibility
#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(A, B)  ((Sint32)((B) - (A)) <= 0)
#endif

typedef enum tagCODEPAGE {
	CP_MIN = 0,
	CP_BIG5 = 0,
	CP_GBK = 1,
	CP_SHIFTJIS = 2,
	CP_JISX0208 = 3,
	CP_MAX = CP_GBK + 1,
	CP_UTF_8 = CP_MAX + 1
} CODEPAGE;

#ifdef __cplusplus
}
#endif

#endif
