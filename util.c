/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2018, SDLPAL development team.
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

static char internal_buffer[PAL_MAX_GLOBAL_BUFFERS + 1][PAL_GLOBAL_BUFFER_SIZE];
#define INTERNAL_BUFFER_SIZE_ARGS internal_buffer[PAL_MAX_GLOBAL_BUFFERS], PAL_GLOBAL_BUFFER_SIZE

void UTIL_MsgBox(char *string)
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
    extern SDL_Window *gpWindow;
    char buffer[300];
    SDL_MessageBoxButtonData buttons[] = { { 0, 0, "OK" } };
    SDL_MessageBoxData mbd = { SDL_MESSAGEBOX_WARNING, gpWindow, "Alert",buffer, 1, buttons, NULL };
    int btnid;
    sprintf(buffer, "%s\n", string);
    SDL_ShowMessageBox(&mbd, &btnid);
#endif
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
UTIL_va(
	char       *buffer,
	int         buflen,
	const char *format,
	...
)
{
   if (buflen > 0 && buffer)
   {
	   va_list     argptr;

	   va_start(argptr, format);
	   vsnprintf(buffer, buflen, format, argptr);
	   va_end(argptr);

	   return buffer;
   }
   else
   {
	   return NULL;
   }
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

   PAL_ProcessEvent();

   while (!SDL_TICKS_PASSED(SDL_GetTicks(), t))
   {
      SDL_Delay(1);
      PAL_ProcessEvent();
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
#if PAL_HAS_CONFIG_PAGE
	  sprintf(buffer, "%sLaunch setting dialog on next start?\n", string);
	  if (SDL_ShowMessageBox(&mbd, &btnid) == 0 && btnid == 0)
	  {
		  gConfig.fLaunchSetting = TRUE;
		  PAL_SaveConfig();
	  }
#else
	  sprintf(buffer, "%s\n", string);
	  mbd.numbuttons=1;
	  SDL_ShowMessageBox(&mbd, &btnid);
#endif
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
       fp = fopen(lpszFileName, szMode);
   }

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
	//
	// If lpszFileName is an absolute path, use its last element as filename
	//
	if (UTIL_IsAbsolutePath(lpszFileName))
	{
		char *temp = strdup(lpszFileName), *filename = temp;
		FILE *fp = NULL;
		for (char *next = strpbrk(filename, PAL_PATH_SEPARATORS); next; next = strpbrk(filename = next + 1, PAL_PATH_SEPARATORS));
		if (*filename)
		{
			filename[-1] = '\0';
			fp = UTIL_OpenFileAtPathForMode(*temp ? temp : "/", filename, szMode);
		}
		free(temp);
		return fp;
	}

	return UTIL_OpenFileAtPathForMode(gConfig.pszGamePath, lpszFileName, szMode);
}

FILE *
UTIL_OpenFileAtPath(
	LPCSTR              lpszPath,
	LPCSTR              lpszFileName
)
{
	return UTIL_OpenFileAtPathForMode(lpszPath, lpszFileName, "rb");
}

FILE *
UTIL_OpenFileAtPathForMode(
	LPCSTR              lpszPath,
	LPCSTR              lpszFileName,
	LPCSTR              szMode
)
{
	if (!lpszPath || !lpszFileName || !szMode) return NULL;

	//
	// Construct full path according to lpszPath and lpszFileName
	//
	const char *path = UTIL_GetFullPathName(INTERNAL_BUFFER_SIZE_ARGS, lpszPath, lpszFileName);

	//
	// If no matching path, check the open mode
	//
	if (path)
	{
		return fopen(path, szMode);
	}
	else if (szMode[0] != 'r')
	{
		return fopen(UTIL_CombinePath(INTERNAL_BUFFER_SIZE_ARGS, 2, lpszPath, lpszFileName), szMode);
	}
	else
	{
		return NULL;
	}
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


const char *
UTIL_GetFullPathName(
	char       *buffer,
	size_t      buflen,
	const char *basepath,
	const char *subpath
)
{
	if (!buffer || !basepath || !subpath || buflen == 0) return NULL;

	int sublen = strlen(subpath);
	if (sublen == 0) return NULL;

	char *_base = strdup(basepath), *_sub = strdup(subpath);
	const char *result = NULL;

	if (access(UTIL_CombinePath(INTERNAL_BUFFER_SIZE_ARGS, 2, _base, _sub), 0) == 0)
	{
		result = internal_buffer[PAL_MAX_GLOBAL_BUFFERS];
	}

#if !defined(PAL_FILESYSTEM_IGNORE_CASE) || !PAL_FILESYSTEM_IGNORE_CASE
	if (result == NULL)
	{
		size_t pos = strspn(_sub, PAL_PATH_SEPARATORS);

		if (pos < sublen)
		{
			char *start = _sub + pos;
			char *end = strpbrk(start, PAL_PATH_SEPARATORS);
			if (end) *end = '\0';

			//
			// try to find the matching file in the directory.
			//
			struct dirent **list;
			int n = scandir(_base, &list, 0, alphasort);
			while (n-- > 0)
			{
				if (!result && strcasecmp(list[n]->d_name, start) == 0)
				{
					result = UTIL_CombinePath(INTERNAL_BUFFER_SIZE_ARGS, 2, _base, list[n]->d_name);
					if (end)
						result = UTIL_GetFullPathName(INTERNAL_BUFFER_SIZE_ARGS, result, end + 1);
					else if (access(result, 0) != 0)
						result = NULL;
				}
				free(list[n]);
			}
			free(list);
		}
	}
#endif
	if (result != NULL)
	{
		size_t dstlen = min(buflen - 1, strlen(result));
		result = (char *)memmove(buffer, result, dstlen);
		buffer[dstlen] = '\0';
	}

	free(_base);
	free(_sub);

	return result;
}

const char *
UTIL_CombinePath(
	char       *buffer,
	size_t      buflen,
	int         numentry,
	...
)
{
	if (buffer && buflen > 0 && numentry > 0)
	{
		const char *retval = buffer;
		va_list argptr;

		va_start(argptr, numentry);
		for (int i = 0; i < numentry && buflen > 1; i++)
		{
			const char *path = va_arg(argptr, const char *);
			int path_len = path ? strlen(path) : 0;
			int append_delim = (i < numentry - 1 && path_len > 0 && !PAL_IS_PATH_SEPARATOR(path[path_len - 1]));
			
			for (int is_sep = 0, j = 0; j < path_len && buflen > (size_t)append_delim + 1; j++)
			{
				//
				// Skip continuous path separators
				// 
				if (PAL_IS_PATH_SEPARATOR(path[j]))
				{
					if (is_sep)
						continue;
					else
						is_sep = 1;
				}
				else
				{
					is_sep = 0;
				}
				*buffer++ = path[j];
				buflen--;
			}
			//
			// Make sure a path delimeter is append to the destination if this is not the last entry
			// 
			if (append_delim)
			{
				*buffer++ = PAL_PATH_SEPARATORS[0];
				buflen--;
			}
		}
		va_end(argptr);

		*buffer = '\0';

		return retval;
	}
	else
	{
		return NULL;
	}
}


char *
UTIL_GlobalBuffer(
	int         index
)
{
	return (index >= 0 && index < PAL_MAX_GLOBAL_BUFFERS) ? internal_buffer[index] : NULL;
}


PALFILE
UTIL_CheckResourceFiles(
	const char *path,
	const char *msgfile
)
{
	const char *common_files[] = {
		"abc.mkf", "ball.mkf", "data.mkf", "f.mkf",
		"fbp.mkf", "fire.mkf", "gop.mkf",  "map.mkf",
		"mgo.mkf", "pat.mkf",  "rgm.mkf",  "rng.mkf",
		"sss.mkf"
	};
	const char *msg_files[][2] = {
		{ msgfile, "m.msg"    },
		{ msgfile, "word.dat" }
	};
	const char *sound_files[2] = { "voc.mkf", "sounds.mkf" };
	const char *music_files[2] = { "midi.mkf", "mus.mkf" };
	int msgidx = !(msgfile && *msgfile);
	PALFILE retval = (PALFILE)0;

	for (int i = 0; i < sizeof(common_files) / sizeof(common_files[0]); i++)
	{
		if (!UTIL_GetFullPathName(INTERNAL_BUFFER_SIZE_ARGS, path, common_files[i]))
		{
			retval |= (PALFILE)(1 << i);
		}
	}

	for (int i = 0; i < sizeof(msg_files[0]) / sizeof(msg_files[0][0]); i++)
	{
		if (!UTIL_GetFullPathName(INTERNAL_BUFFER_SIZE_ARGS, path, msg_files[i][msgidx]))
		{
			retval |= (PALFILE)(1 << ((i + 1) * msgidx + 13));
		}
	}

	for (int i = 0; i < sizeof(sound_files) / sizeof(sound_files[0]); i++)
	{
		if (!UTIL_GetFullPathName(INTERNAL_BUFFER_SIZE_ARGS, path, sound_files[i]))
		{
			retval |= (PALFILE)(1 << (i + 16));
		}
	}

	for (int i = 0; i < sizeof(music_files) / sizeof(music_files[0]); i++)
	{
		if (!UTIL_GetFullPathName(INTERNAL_BUFFER_SIZE_ARGS, path, music_files[i]))
		{
			retval |= (PALFILE)(1 << (i + 18));
		}
	}

	return retval;
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

#ifndef PAL_LOG_BUFFER_SIZE
# define PAL_LOG_BUFFER_SIZE      4096
#endif

#define PAL_LOG_BUFFER_EXTRA_SIZE 32+sizeof(_log_prelude)

static char _log_prelude[80];
static LOGCALLBACK _log_callbacks[PAL_LOG_MAX_OUTPUTS];
static LOGLEVEL _log_callback_levels[PAL_LOG_MAX_OUTPUTS];
static char _log_buffer[PAL_LOG_BUFFER_SIZE + PAL_LOG_BUFFER_EXTRA_SIZE];

static const char * const _loglevel_str[] = {
	"[VERBOSE]",
	"  [DEBUG]",
	"   [INFO]",
	"[WARNING]",
	"  [ERROR]",
	"  [FATAL]",
};

int
UTIL_LogAddOutputCallback(
	LOGCALLBACK    callback,
	LOGLEVEL       loglevel
)
{
	if (!callback) return -1;

	// De-duplication
	for (int i = 0; i < PAL_LOG_MAX_OUTPUTS; i++)
	{
		if (!_log_callbacks[i])
		{
			_log_callbacks[i] = callback;
		}
		if (_log_callbacks[i] == callback)
		{
			_log_callback_levels[i] = loglevel;
			return i;
		}
	}

	return -1;
}

void
UTIL_LogRemoveOutputCallback(
	int            id
)
{
	if (id < 0 || id >= PAL_LOG_MAX_OUTPUTS) return;

	while (id < PAL_LOG_MAX_OUTPUTS - 1)
	{
		_log_callbacks[id] = _log_callbacks[id + 1];
		_log_callback_levels[id] = _log_callback_levels[id + 1];
		id++;
	}
	_log_callbacks[id] = NULL;
	_log_callback_levels[id] = LOGLEVEL_MIN;
}

void
UTIL_LogOutput(
	LOGLEVEL       level,
	const char    *fmt,
	...
)
{
	va_list    va;
	time_t     tv = time(NULL);
	struct tm *tmval = localtime(&tv);
	int        id, n;

	if (level < gConfig.iLogLevel || !_log_callbacks[0]) return;
	if (level > LOGLEVEL_MAX) level = LOGLEVEL_MAX;

	snprintf(_log_buffer, PAL_LOG_BUFFER_EXTRA_SIZE,
		"%04d-%02d-%02d %02d:%02d:%02d %s: ",
		tmval->tm_year + 1900, tmval->tm_mon, tmval->tm_mday,
		tmval->tm_hour, tmval->tm_min, tmval->tm_sec,
		_loglevel_str[level]);
	if( strlen(_log_prelude) > 0 )
		strncat(_log_buffer, _log_prelude, PAL_LOG_BUFFER_EXTRA_SIZE);

	va_start(va, fmt);
	n = vsnprintf(_log_buffer + strnlen(_log_buffer, PAL_LOG_BUFFER_EXTRA_SIZE), PAL_LOG_BUFFER_SIZE, fmt, va);
	va_end(va);
	n = (n == -1) ? PAL_LOG_BUFFER_EXTRA_SIZE + PAL_LOG_BUFFER_SIZE - 1 : n + PAL_LOG_BUFFER_EXTRA_SIZE;
	_log_buffer[n--] = '\0';
	if (_log_buffer[n] != '\n') _log_buffer[n] = '\n';

	for(id = 0; id < PAL_LOG_MAX_OUTPUTS && _log_callbacks[id]; id++)
	{
		if (level >= _log_callback_levels[id])
		{
			_log_callbacks[id](level, _log_buffer, _log_buffer + PAL_LOG_BUFFER_EXTRA_SIZE - 1);
		}
	}
    
    if( level == LOGLEVEL_FATAL )
        TerminateOnError(_log_buffer);
}

void
UTIL_LogSetLevel(
	LOGLEVEL       minlevel
)
{
	if (minlevel < LOGLEVEL_MIN)
		gConfig.iLogLevel = LOGLEVEL_MIN;
	else if (minlevel > LOGLEVEL_MAX)
		gConfig.iLogLevel = LOGLEVEL_MAX;
	else
		gConfig.iLogLevel = minlevel;
}

void
UTIL_LogToFile(
	LOGLEVEL       _,
	const char    *string,
	const char    *__
)
{
	FILE *fp = UTIL_OpenFileForMode(gConfig.pszLogFile, "a");
	if (fp)
	{
		fputs(string, fp);
		fclose(fp);
	}
}

void
UTIL_LogSetPrelude(
                   const char    *prelude
)
{
    memset(_log_prelude, 0, sizeof(_log_prelude));
    if( prelude )
        strncpy(_log_prelude, prelude, sizeof(_log_prelude));
}
