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

#ifndef _COMMON_H
#define _COMMON_H

#ifndef ENABLE_REVISIED_BATTLE
# define PAL_CLASSIC        1
#endif

#include "defines.h"

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
#include <stdint.h>
#include <stdbool.h>

#include "SDL.h"
#include "SDL_endian.h"

#define __WIDETEXT(quote) L##quote
#define WIDETEXT(quote) __WIDETEXT(quote)

#if !defined(fmax) || !defined(fmin)
# include <math.h>
#endif

#include <float.h>

#ifndef max
# define max fmax
#endif

#ifndef min
# define min fmin
#endif

// For SDL 1.2 compatibility
#ifndef SDL_TICKS_PASSED
#define SDL_TICKS_PASSED(A, B)  ((Sint32)((B) - (A)) <= 0)
#endif

#ifndef SDL_INIT_CDROM
# define SDL_INIT_CDROM       0	  /* Compatibility with SDL 1.2 */
#endif

#ifndef SDL_AUDIO_BITSIZE
# define SDL_AUDIO_BITSIZE(x)         (x & 0xFF)
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
# define PAL_FORCE_INLINE static SDL_FORCE_INLINE
#else
# define PAL_FORCE_INLINE SDL_FORCE_INLINE
#endif

#ifdef _WIN32

# include <windows.h>
# include <io.h>

# if defined(_MSC_VER)
#  if _MSC_VER < 1900
#   define vsnprintf _vsnprintf
#   define snprintf _snprintf
#  endif
#  define strdup _strdup
#  define access _access
#  pragma warning (disable:4244)
# endif

# ifndef _LPCBYTE_DEFINED
#  define _LPCBYTE_DEFINED
typedef const BYTE *LPCBYTE;
# endif

# define PAL_MAX_PATH  MAX_PATH

#else

# include <unistd.h>
# include <dirent.h>
# ifdef __APPLE__
#  include <objc/objc.h>
# endif

# ifndef FALSE
#  define FALSE               0
# endif
# ifndef TRUE
#  define TRUE                1
# endif
# define VOID                void
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
# if !defined( __APPLE__ ) && !defined( GEKKO )
typedef int                 BOOL, *LPBOOL;
# endif
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

# define PAL_MAX_PATH  PATH_MAX

#endif

#ifdef __cplusplus
# define PAL_C_LINKAGE       extern "C"
# define PAL_C_LINKAGE_BEGIN PAL_C_LINKAGE {
# define PAL_C_LINKAGE_END   }
#else
# define PAL_C_LINKAGE
# define PAL_C_LINKAGE_BEGIN
# define PAL_C_LINKAGE_END
#endif

/* When porting SDLPAL to a new platform, please make a separate directory and put a file 
   named 'pal_config.h' that contains marco definitions & header includes into the directory.
   The example of this file can be found in directories of existing portings.
 */
#include "pal_config.h"

#if !SDL_VERSION_ATLEAST(2,0,0)
# if PAL_HAS_GLSL
#  undef PAL_HAS_GLSL
# endif
#define SDL_strcasecmp strcasecmp
#define SDL_setenv(a,b,c) 
#endif

#ifndef PAL_DEFAULT_FULLSCREEN_HEIGHT
# define PAL_DEFAULT_FULLSCREEN_HEIGHT PAL_DEFAULT_WINDOW_HEIGHT
#endif

#ifndef PAL_DEFAULT_TEXTURE_WIDTH
# define PAL_DEFAULT_TEXTURE_WIDTH     PAL_DEFAULT_WINDOW_WIDTH
#endif

#ifndef PAL_DEFAULT_TEXTURE_HEIGHT
# define PAL_DEFAULT_TEXTURE_HEIGHT    PAL_DEFAULT_WINDOW_HEIGHT
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
#ifndef PAL_HAS_OPUS
# define PAL_HAS_OPUS         1   /* Try always enable OPUS. If compilation/run failed, please change this value to 0. */
#endif

#ifndef PAL_CONFIG_PREFIX
# define PAL_CONFIG_PREFIX PAL_PREFIX
#endif

#ifndef PAL_HAS_NATIVEMIDI
# define PAL_HAS_NATIVEMIDI  0
#endif

#ifndef PAL_LARGE
# define PAL_LARGE
#endif

#ifndef PAL_SCALE_SCREEN
# define PAL_SCALE_SCREEN   TRUE
#endif

#ifndef PAL_IS_VALID_JOYSTICK
# define PAL_IS_VALID_JOYSTICK(s)  TRUE
#endif

#ifndef PAL_FATAL_OUTPUT
# define PAL_FATAL_OUTPUT(s)
#endif

#ifndef PAL_CONVERT_UTF8
# define PAL_CONVERT_UTF8(s) s
#endif

#ifndef PAL_NATIVE_PATH_SEPARATOR
# define PAL_NATIVE_PATH_SEPARATOR "/"
#endif

#define PAL_fread(buf, elem, num, fp) if (fread((buf), (elem), (num), (fp)) < (num)) return -1

typedef enum tagLOGLEVEL
{
	LOGLEVEL_MIN,
	LOGLEVEL_VERBOSE = LOGLEVEL_MIN,
	LOGLEVEL_DEBUG,
	LOGLEVEL_INFO,
	LOGLEVEL_WARNING,
	LOGLEVEL_ERROR,
	LOGLEVEL_FATAL,
	LOGLEVEL_MAX = LOGLEVEL_FATAL,
} LOGLEVEL;

#define PAL_LOG_MAX_OUTPUTS   (LOGLEVEL_MAX + 1)

#if defined(DEBUG) || defined(_DEBUG)
# define PAL_DEFAULT_LOGLEVEL  LOGLEVEL_MIN
#else
# define PAL_DEFAULT_LOGLEVEL  LOGLEVEL_MAX
#endif

#ifndef PAL_HAS_CONFIG_PAGE
# define PAL_HAS_CONFIG_PAGE   FALSE
#endif

#define PAL_MAX_GLOBAL_BUFFERS 4
#define PAL_GLOBAL_BUFFER_SIZE 1024

//
// PAL_PATH_SEPARATORS contains all vaild path separators under a specific OS
// If you define this constant, please put the default separator at first.
//
#ifndef PAL_PATH_SEPARATORS
# define PAL_PATH_SEPARATORS "/"
#endif

#ifndef PAL_IS_PATH_SEPARATOR
# define PAL_IS_PATH_SEPARATOR(x) ((x) == '/')
#endif

#include "adplug/opltypes.h"

#endif
