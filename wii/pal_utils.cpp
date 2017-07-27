
#include "main.h"
#include <fat.h>
#include <ogcsys.h>
#include <gccore.h>
#include <fcntl.h>
#include <network.h>
#include <debug.h>
#include <errno.h>
#include <unistd.h>
#ifdef DEBUG
extern "C"{
# include <net_print.h>
}
#endif

static int input_event_filter(const SDL_Event *lpEvent, volatile PALINPUTSTATE *state)
{
	switch (lpEvent->type)
	{
	case SDL_JOYHATMOTION:
		switch (lpEvent->jhat.value)
		{
		case SDL_HAT_LEFT:
			state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
			state->dir = kDirWest;
			state->dwKeyPress = kKeyLeft;
			break;

		case SDL_HAT_RIGHT:
			state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
			state->dir = kDirEast;
			state->dwKeyPress = kKeyRight;
			break;

		case SDL_HAT_UP:
			state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
			state->dir = kDirNorth;
			state->dwKeyPress = kKeyUp;
			break;

		case SDL_HAT_DOWN:
			state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
			state->dir = kDirSouth;
			state->dwKeyPress = kKeyDown;
			break;
		}
		return 1;

	case SDL_JOYBUTTONDOWN:
		switch (lpEvent->jbutton.button)
		{
		case 2:
			state->dwKeyPress |= kKeyMenu;
			return 1;

		case 3:
			state->dwKeyPress |= kKeySearch;
			return 1;
		}
	}
	return 0;
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

int UTIL_Platform_Startup(
	int argc,
	char* argv[]
)
{
#if defined(DEBUG)
   VIDEO_SetBlack(FALSE);
   VIDEO_Flush();
   VIDEO_WaitVSync();
   char localip[16] = {0};
   char gateway[16] = {0};
   char netmask[16] = {0};
   int ret = if_config ( localip, netmask, gateway, TRUE);
   if (ret>=0) {
      printf ("\n network configured, ip: %s, gw: %s, mask %s\n", localip, gateway, netmask);
      net_print_init(NULL,0);
      
      printf("net_print_init() called.\n");
      net_print_string(__FILE__,__LINE__, "initial net_print from %s...\n",localip);

      DEBUG_Init(100,5656);
      printf("after DEBUG_Init()...\n");
      
      printf("Before _break() is called.\n");
      _break();
      printf("After _break() is called.\n");
   }
#endif
	fatInitDefault();
	return 0;
}

INT
UTIL_Platform_Init(
	int argc,
	char* argv[]
)
{
#ifdef DEBUG
	UTIL_LogAddOutputCallback([](LOGLEVEL, const char* str, const char*)->void {
      net_print_string(__FILE__,__LINE__, "%s\n",str);
	}, PAL_DEFAULT_LOGLEVEL);
#endif
	UTIL_LogOutput(LOGLEVEL_WARNING, "try net_print logout");
	PAL_RegisterInputFilter(NULL, input_event_filter, NULL);
	gConfig.fLaunchSetting = FALSE;
	return 0;
}

VOID
UTIL_Platform_Quit(
	VOID
)
{
}
