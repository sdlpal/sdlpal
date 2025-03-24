#include "palcommon.h"
#include "global.h"
#include "palcfg.h"
#include "util.h"

#include <emscripten.h>
#include <fcntl.h>

extern "C" char *_stringtolower(char *s) 
{
	char *orig = s;
	do{	*s=tolower(*s); }while(*++s);
	return orig;
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

extern "C" int UTIL_Platform_Startup(int argc, char *argv[]) {
	// Defaults log to debug output
	UTIL_LogAddOutputCallback([](LOGLEVEL, const char* str, const char*)->void {
		EM_ASM(Module.print(UTF8ToString($0)), str);
	}, LOGLEVEL_MIN);

    // Move any URL params that start with "SDL_" over to environment
    //  variables, so the hint system can pick them up, etc, much like a user
    //  can set them from a shell prompt on a desktop machine. Ignore all
    //  other params, in case the app wants to use them for something.
    MAIN_THREAD_EM_ASM({
        var parms = new URLSearchParams(window.location.search);
        for (const [key, value] of parms) {
            if (key.startsWith("SDL_")) {
                var ckey = stringToNewUTF8(key);
                var cvalue = stringToNewUTF8(value);
                if ((ckey != 0) && (cvalue != 0)) {
                    //console.log("Setting SDL env var '" + key + "' to '" + value + "' ...");
                    dynCall('iiii', $0, [ckey, cvalue, 1]);
                }
                _free(ckey);  // these must use free(), not SDL_free()!
                _free(cvalue);
            }
        }
    }, SDL_setenv_unsafe);

	return 0;
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

extern "C" FILE *
EMSCRIPTEN_fopen(
    char *fname,
	char *opts
)
{
	return fopen(_stringtolower(fname),opts);
}
extern "C" int
EMSCRIPTEN_fclose(
    FILE *stream
)
{
	int mode = fcntl(fileno(stream), F_GETFL);
	int ret = fclose(stream);
	if ((mode & O_ACCMODE) != O_RDONLY) {
		EM_ASM({FS.syncfs(false, function (err) {});});
	}
	return ret;
}
