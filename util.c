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

#include "util.h"
#include "input.h"
#include "global.h"
#include "palcfg.h"
#include <errno.h>

#include "midi.h"
#if SDL_VERSION_ATLEAST(2, 0, 0)
#include "SDL_video.h"
#include "SDL_messagebox.h"
#endif

void UTIL_MsgBox(char *string){
    extern SDL_Window *gpWindow;
    char buffer[300];
    SDL_MessageBoxButtonData buttons[] = { { 0, 0, "OK" } };
    SDL_MessageBoxData mbd = { SDL_MESSAGEBOX_WARNING, gpWindow, "Alert",buffer, 1, buttons, NULL };
    int btnid;
    sprintf(buffer, "%s\n", string);
    SDL_ShowMessageBox(&mbd, &btnid);
}

long
flength(
   FILE *fp
)
{
   long old_pos = ftell(fp), length;
   if (old_pos == -1) return -1;
   if (fseek(fp, 0, SEEK_END) == -1) return -1;
   length = ftell(fp); fseek(fp, old_pos, SEEK_SET);
   return length;
}

void
trim(
   char *str
)
/*++
  Purpose:

    Remove the leading and trailing spaces in a string.

  Parameters:

    str - the string to proceed.

  Return value:

    None.

--*/
{
   int pos = 0;
   char *dest = str;

   //
   // skip leading blanks
   //
   while (str[pos] <= ' ' && str[pos] > 0)
      pos++;

   while (str[pos])
   {
      *(dest++) = str[pos];
      pos++;
   }

   *(dest--) = '\0'; // store the null

   //
   // remove trailing blanks
   //
   while (dest >= str && *dest <= ' ' && *dest > 0)
      *(dest--) = '\0';
}

char *
va(
   const char *format,
   ...
)
/*++
  Purpose:

    Does a varargs printf into a temp buffer, so we don't need to have
    varargs versions of all text functions.

  Parameters:

    format - the format string.

  Return value:

    Pointer to the result string.

--*/
{
   static char string[1024];
   va_list     argptr;

   va_start(argptr, format);
   vsnprintf(string, sizeof(string), format, argptr);
   va_end(argptr);

   return string;
}

/*
 * RNG code based on RACC by Pierre-Marie Baty.
 * http://racc.bots-united.com
 *
 * Copyright (c) 2004, Pierre-Marie Baty
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the RACC nor the names of its contributors
 * may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

//
// Our random number generator's seed.
//
static int glSeed = 0;

static void
lsrand(
   unsigned int iInitialSeed
)
/*++
  Purpose:

    This function initializes the random seed based on the initial seed value passed in the
    iInitialSeed parameter.

  Parameters:

    [IN]  iInitialSeed - The initial random seed.

  Return value:

    None.

--*/
{
   //
   // fill in the initial seed of the random number generator
   //
   glSeed = 1664525L * iInitialSeed + 1013904223L;
}

static int
lrand(
   void
)
/*++
  Purpose:

    This function is the equivalent of the rand() standard C library function, except that
    whereas rand() works only with short integers (i.e. not above 32767), this function is
    able to generate 32-bit random numbers.

  Parameters:

    None.

  Return value:

    The generated random number.

--*/
{
   if (glSeed == 0) // if the random seed isn't initialized...
      lsrand((unsigned int)time(NULL)); // initialize it first
   glSeed = 1664525L * glSeed + 1013904223L; // do some twisted math (infinite suite)
   return ((glSeed >> 1) + 1073741824L); // and return the result.
}

int
RandomLong(
   int from,
   int to
)
/*++
  Purpose:

    This function returns a random integer number between (and including) the starting and
    ending values passed by parameters from and to.

  Parameters:

    from - the starting value.

    to - the ending value.

  Return value:

    The generated random number.

--*/
{
   if (to <= from)
      return from;

   return from + lrand() / (INT_MAX / (to - from + 1));
}

float
RandomFloat(
   float from,
   float to
)
/*++
  Purpose:

    This function returns a random floating-point number between (and including) the starting
    and ending values passed by parameters from and to.

  Parameters:

    from - the starting value.

    to - the ending value.

  Return value:

    The generated random number.

--*/
{
   if (to <= from)
      return from;

   return from + (float)lrand() / (INT_MAX / (to - from));
}

void
UTIL_Delay(
   unsigned int ms
)
{
   unsigned int t = SDL_GetTicks() + ms;

   while (PAL_PollEvent(NULL));

   while (!SDL_TICKS_PASSED(SDL_GetTicks(), t))
   {
      SDL_Delay(1);
      while (PAL_PollEvent(NULL));
   }
}

void
TerminateOnError(
   const char *fmt,
   ...
)
// This function terminates the game because of an error and
// prints the message string pointed to by fmt both in the
// console and in a messagebox.
{
   va_list argptr;
   char string[256];
   extern VOID PAL_Shutdown(int);

   // concatenate all the arguments in one string
   va_start(argptr, fmt);
   vsnprintf(string, sizeof(string), fmt, argptr);
   va_end(argptr);

   fprintf(stderr, "\nFATAL ERROR: %s\n", string);

#if SDL_VERSION_ATLEAST(2, 0, 0)
   {
	  extern SDL_Window *gpWindow;
	  char buffer[300];
	  SDL_MessageBoxButtonData buttons[2] = { { 0, 0, "Yes" },{ 0, 1, "No" } };
	  SDL_MessageBoxData mbd = { SDL_MESSAGEBOX_ERROR, gpWindow, "FATAL ERROR", buffer, 2, buttons, NULL };
	  int btnid;
	  sprintf(buffer, "%sLaunch setting dialog on next start?\n", string);
	  if (SDL_ShowMessageBox(&mbd, &btnid) == 0 && btnid == 0)
	  {
		  gConfig.fLaunchSetting = TRUE;
		  PAL_SaveConfig();
	  }
	  PAL_Shutdown(255);
   }
#else
   PAL_FATAL_OUTPUT(string);
#endif

#ifdef _DEBUG
   assert(!"TerminateOnError()"); // allows jumping to debugger
#endif

   PAL_Shutdown(255);
}

void *
UTIL_malloc(
   size_t               buffer_size
)
{
   // handy wrapper for operations we always forget, like checking malloc's returned pointer.

   void *buffer;

   // first off, check if buffer size is valid
   if (buffer_size == 0)
      TerminateOnError("UTIL_malloc() called with invalid buffer size: %d\n", buffer_size);

   buffer = malloc(buffer_size); // allocate real memory space

   // last check, check if malloc call succeeded
   if (buffer == NULL)
      TerminateOnError("UTIL_malloc() failure for %d bytes (out of memory?)\n", buffer_size);

   return buffer; // nothing went wrong, so return buffer pointer
}

void *
UTIL_calloc(
   size_t               n,
   size_t               size
)
{
   // handy wrapper for operations we always forget, like checking calloc's returned pointer.

   void *buffer;

   // first off, check if buffer size is valid
   if (n == 0 || size == 0)
      TerminateOnError ("UTIL_calloc() called with invalid parameters\n");

   buffer = calloc(n, size); // allocate real memory space

   // last check, check if malloc call succeeded
   if (buffer == NULL)
      TerminateOnError("UTIL_calloc() failure for %d bytes (out of memory?)\n", size * n);

   return buffer; // nothing went wrong, so return buffer pointer
}

FILE *
UTIL_OpenRequiredFile(
   LPCSTR            lpszFileName
)
/*++
  Purpose:

    Open a required file. If fails, quit the program.

  Parameters:

    [IN]  lpszFileName - file name to open.

  Return value:

    Pointer to the file.

--*/
{
   return UTIL_OpenRequiredFileForMode(lpszFileName, "rb");
}

FILE *
UTIL_OpenRequiredFileForMode(
   LPCSTR            lpszFileName,
   LPCSTR            szMode
)
/*++
  Purpose:

    Open a required file. If fails, quit the program.

  Parameters:

    [IN]  lpszFileName - file name to open.
    [IN]  szMode - file open mode.

  Return value:

    Pointer to the file.

--*/
{
   FILE *fp = UTIL_OpenFileForMode(lpszFileName, szMode);

   if (fp == NULL)
   {
	   TerminateOnError("File open error(%d): %s!\n", errno, lpszFileName);
   }

   return fp;
}

FILE *
UTIL_OpenFile(
   LPCSTR            lpszFileName
)
/*++
  Purpose:

    Open a file. If fails, return NULL.

  Parameters:

    [IN]  lpszFileName - file name to open.

  Return value:

    Pointer to the file.

--*/
{
   return UTIL_OpenFileForMode(lpszFileName, "rb");
}

FILE *
UTIL_OpenFileForMode(
   LPCSTR            lpszFileName,
   LPCSTR            szMode
)
/*++
  Purpose:

    Open a file. If fails, return NULL.

  Parameters:

    [IN]  lpszFileName - file name to open.
    [IN]  szMode - file open mode.

  Return value:

    Pointer to the file.

--*/
{
	FILE         *fp;

	if (UTIL_IsAbsolutePath(lpszFileName))
		fp = fopen(lpszFileName, szMode);
	else
		fp = fopen(va("%s%s", gConfig.pszGamePath, lpszFileName), szMode);

#if !defined(PAL_FILESYSTEM_IGNORE_CASE) || !PAL_FILESYSTEM_IGNORE_CASE
	if (fp == NULL)
	{
		//
		// try to find the matching file in the directory.
		//
		struct dirent **list;
		int n = scandir(gConfig.pszGamePath, &list, 0, alphasort);
		while (n-- > 0)
		{
			if (!fp && strcasecmp(list[n]->d_name, lpszFileName) == 0)
				fp = fopen(va("%s%s", gConfig.pszGamePath, list[n]->d_name), szMode);
			free(list[n]);
		}
		free(list);
	}
#endif

	return fp;
}

VOID
UTIL_CloseFile(
   FILE             *fp
)
/*++
  Purpose:

    Close a file.

  Parameters:

    [IN]  fp - file handle to be closed.

  Return value:

    None.

--*/
{
   if (fp != NULL)
   {
      fclose(fp);
   }
}

#if !defined(PAL_HAS_PLATFORM_SPECIFIC_UTILS)

BOOL
UTIL_GetScreenSize(
   DWORD *pdwScreenWidth,
   DWORD *pdwScreenHeight
)
{
   return FALSE;
}

BOOL
UTIL_IsAbsolutePath(
	LPCSTR  lpszFileName
)
{
	return FALSE;
}

INT
UTIL_Platform_Init(
   int argc,
   char* argv[]
)
{
   gConfig.fLaunchSetting = FALSE;
   return 0;
}

VOID
UTIL_Platform_Quit(
   VOID
)
{
}

#endif

/*
* Logging utilities
*/

static LOGLEVEL _min_loglevel = LOGLEVEL_WARNING;

static void(*_log_callback)(const char *, const char *) = NULL;
static int _max_log_length = 0;

static const char * const _loglevel_str[] = {
	"[VERBOSE]",
	"  [DEBUG]",
	"   [INFO]",
	"[WARNING]",
	"  [ERROR]",
	"  [FATAL]",
};

void
UTIL_LogSetOutput(
	LOGCALLBACK    callback,
	int            maxloglen
)
{
	_log_callback = callback;
	_max_log_length = maxloglen;
}

void
UTIL_LogOutput(
	LOGLEVEL       level,
	const char    *fmt,
	...
)
{
	va_list      va;
	time_t       tv = time(NULL);
	struct tm   *tmval = localtime(&tv);
	LOGCALLBACK  callback = _log_callback;
	int          maxloglen = _max_log_length;
	char        *buf;

	if (level < _min_loglevel || !callback || maxloglen <= 0 ||
		NULL == (buf = (char *)malloc(maxloglen + 32)))
		return;

	snprintf(buf, 32, "%04d-%02d-%02d %02d:%02d:%02d %s: ",
		tmval->tm_year, tmval->tm_mon, tmval->tm_mday,
		tmval->tm_hour, tmval->tm_min, tmval->tm_sec,
		_loglevel_str[level > LOGLEVEL_MAX ? LOGLEVEL_MAX : level]);

	va_start(va, fmt);
	vsnprintf(buf + 31, maxloglen + 1, fmt, va);
	va_end(va);

	callback(buf, buf + 31);
	free(buf);
}

void
UTIL_LogSetLevel(
	LOGLEVEL       minlevel
)
{
	if (minlevel < LOGLEVEL_MIN)
		_min_loglevel = LOGLEVEL_MIN;
	else if (minlevel > LOGLEVEL_MAX)
		_min_loglevel = LOGLEVEL_MAX;
	else
		_min_loglevel = minlevel;
}
