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
    gConfig.fLaunchSetting = FALSE;
    return 0;
}

VOID
UTIL_Platform_Quit(
                   VOID
                   )
{
}
