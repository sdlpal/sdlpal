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

#ifndef UTIL_H
#define UTIL_H

#include "common.h"

PAL_C_LINKAGE_BEGIN

void
UTIL_MsgBox(
   char *string
);

long
flength(
   FILE *fp
);

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
UTIL_OpenRequiredFileForMode(
   LPCSTR               lpszFileName,
   LPCSTR               szMode
);

FILE *
UTIL_OpenFile(
   LPCSTR               lpszFileName
);

FILE *
UTIL_OpenFileForMode(
   LPCSTR               lpszFileName,
   LPCSTR               szMode
);

VOID
UTIL_CloseFile(
   FILE                *fp
);


/*
 * Platform-specific utilities
 */

BOOL
UTIL_GetScreenSize(
	DWORD *pdwScreenWidth,
	DWORD *pdwScreenHeight
);

BOOL
UTIL_IsAbsolutePath(
	const char *lpszFileName
);

int
UTIL_Platform_Init(
	int   argc,
	char *argv[]
);

void
UTIL_Platform_Quit(
	void
);


/*
 * Logging utilities
 */

typedef enum LOGLEVEL
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

/*++
  Purpose:

    The pointer to callback function that produces actual log output.

  Parameters:

    [IN]  level    - The log level of this output call.
	[IN]  full_log - The full log string produced by UTIL_LogOutput.
	[IN]  user_log - The log string produced by user-provided format.

  Return value:

    None.

--*/
typedef void(*LOGCALLBACK)(LOGLEVEL level, const char *full_log, const char *user_log);

/*++
  Purpose:

    Initialize the internal log system.

  Parameters:

    [IN]  callback     - The callback function to be called at each
	                     call of UTIL_LogOutput.
    [IN]  maxloglen    - The max buffer size that holds the output
	                     correspoind to user-provided format string,
					     not including the terminal null-character.
    [IN]  staticbuffer - Whether UTIL_LogOutput should generate the
	                     output string into a one-time allocated global
						 buffer, or to a per-call allocated locall buffer.

  Return value:

    None.

--*/
void
UTIL_LogSetOutput(
	LOGCALLBACK    callback,
	int            maxloglen,
	BOOL           staticbuffer
);

/*++
  Purpose:

    Set the minimal log level that could be output.
	Any level below this level will produce no output.

  Parameters:

    [IN]  minlevel - The minimal log level, must be within the
	                 range [LOGLEVEL_MIN, LOGLEVEL_MAX].

  Return value:

    None.

--*/
void
UTIL_LogOutput(
	LOGLEVEL       level,
	const char    *fmt,
	...
);

/*++
  Purpose:

    Set the minimal log level that could be output.
	Any level below this level will produce no output.

  Parameters:

    [IN]  minlevel - The minimal log level, must be within the
	                 range [LOGLEVEL_MIN, LOGLEVEL_MAX].

  Return value:

    None.

--*/
void
UTIL_LogSetLevel(
	LOGLEVEL       minlevel
);

PAL_C_LINKAGE_END

#endif
