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


#include <Foundation/Foundation.h>
#include <UIKit/UIKit.h>
#include "common.h"
#include "palcfg.h"
#include "util.h"

LPCSTR
UTIL_BasePath(
   VOID
)
{
   static char buf[4096] = "";

   if (buf[0] == '\0')
   {
#ifdef CYDIA
      char *p = SDL_GetBasePath();
      if (p != NULL)
      {
         strcpy(buf, p);
         free(p);
      }
#else
      NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
      NSString *documentsDirectory = [[paths objectAtIndex:0] stringByAppendingString:@"/"];
      strcpy(buf, [documentsDirectory UTF8String]);
#endif
   }

   return buf;
}

LPCSTR
UTIL_SavePath(
   VOID
)
{
   static char buf[4096] = "";

   if (buf[0] == '\0')
   {
      NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
      NSString *documentsDirectory = [[paths objectAtIndex:0] stringByAppendingString:@"/"];
      strcpy(buf, [documentsDirectory UTF8String]);
   }

   return buf;
}

BOOL
UTIL_GetScreenSize(
                   DWORD *pdwScreenWidth,
                   DWORD *pdwScreenHeight
                   )
{
    return (pdwScreenWidth && pdwScreenHeight && *pdwScreenWidth && *pdwScreenHeight);
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
    UTIL_LogAddOutputCallback([](LOGLEVEL, const char* str, const char*)->void {
        NSLog(@"%s",str);
    }, PAL_DEFAULT_LOGLEVEL);
    gConfig.fLaunchSetting = NO;
    return 0;
}

VOID
UTIL_Platform_Quit(
                   VOID
                   )
{
}
