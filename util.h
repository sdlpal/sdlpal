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

#ifndef UTIL_H
#define UTIL_H

#include "common.h"

//#define ENABLE_LOG 1

#ifdef __cplusplus
extern "C"
{
#endif

void
trim(
   char *str
);

char *va(
   const char *format,
   ...
);

int
RandomLong(
   int from,
   int to
);

float
RandomFloat(
   float from,
   float to
);

void
UTIL_Delay(
   unsigned int ms
);

void
TerminateOnError(
   const char *fmt,
   ...
);

void *
UTIL_malloc(
   size_t               buffer_size
);

void *
UTIL_calloc(
   size_t               n,
   size_t               size
);

FILE *
UTIL_OpenRequiredFile(
   LPCSTR               lpszFileName
);

FILE *
UTIL_OpenFile(
   LPCSTR               lpszFileName
);

VOID
UTIL_CloseFile(
   FILE                *fp
);
    
#ifdef __IOS__

LPCSTR
UTIL_IOS_BasePath(
   VOID
);

LPCSTR
UTIL_IOS_SavePath(
   VOID
);
    
#endif

#define _PATH_LOG           PAL_PREFIX "log.txt"
#define LOG_EMERG           0 /* system is unusable */
#define LOG_ALERT           1 /* action must be taken immediately */
#define LOG_CRIT            2 /* critical conditions */
#define LOG_ERR             3 /* error conditions */
#define LOG_WARNING         4 /* warning conditions */
#define LOG_NOTICE          5 /* normal but significant condition */
#define LOG_INFO            6 /* informational */
#define LOG_DEBUG           7 /* debug-level messages */
#define LOG_LAST_PRIORITY   8 /* last level */

#ifdef ENABLE_LOG

FILE *
UTIL_OpenLog(
   VOID
);

VOID
UTIL_CloseLog(
   VOID
);

VOID
UTIL_WriteLog(
   int             Priority,
   const char     *Fmt,
   ...
);

#else

#define UTIL_OpenLog()       ((void)(0))
#define UTIL_CloseLog()      ((void)(0))
#ifdef _MSC_VER
__forceinline VOID UTIL_WriteLog(int i, const char *p, ...) {}
#else
#define UTIL_WriteLog(...)   ((void)(0))
#endif

#endif

#ifdef __cplusplus
}
#endif

#endif
