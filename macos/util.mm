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


#include <Foundation/Foundation.h>
#include "palcfg.h"
#include "util.h"

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
    return lpszFileName && lpszFileName[0] == '/';
}

static void LogCallBack(LOGLEVEL, const char* str, const char*)
{
    NSLog(@"%s",str);
}

BOOL isUnitTesting() {
    NSDictionary *env = [NSProcessInfo processInfo].environment;
    return env[@"XCTestConfigurationFilePath"] != nil;
}

INT UTIL_Platform_Startup(
	int argc,
	char* argv[]
)
{
    if( getppid() == 1 ) //detect whether is debugging; for ease of specify resource dir in debugger
    {
        char *p = strstr(argv[0], "/Pal.app/");
        
        if (p != NULL)
        {
            char buf[4096];
            strcpy(buf, argv[0]);
            buf[p - argv[0]] = '\0';
            chdir(buf);
        }
    }
    return isUnitTesting() ? -1 : 0;
}

INT
UTIL_Platform_Init(
                   int argc,
                   char* argv[]
                   )
{
    UTIL_LogAddOutputCallback(LogCallBack, PAL_DEFAULT_LOGLEVEL);
    gConfig.fLaunchSetting = FALSE;
    return 0;
}

VOID
UTIL_Platform_Quit(
                   VOID
                   )
{
}
