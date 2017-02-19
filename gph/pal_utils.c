
#include "main.h"

static int input_event_filter(const SDL_Event *lpEvent, PALINPUTSTATE *state)
{
	switch (lpEvent->type)
	{
	case SDL_JOYAXISMOTION:
		switch (lpEvent->jaxis.axis)
		{
		case 0:
			//
			// X axis
			//
			if (lpEvent->jaxis.value > MAX_DEADZONE) {
				state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
				state->dir = kDirEast;
				state->dwKeyPress = kKeyRight;
			}
			else if (lpEvent->jaxis.value < MIN_DEADZONE) {
				state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
				state->dir = kDirWest;
				state->dwKeyPress = kKeyLeft;
			}
			else {
				state->dir = kDirUnknown;
			}
			return 1;
		case 1:
			//
			// Y axis
			//
			if (lpEvent->jaxis.value > MAX_DEADZONE) {
				state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
				state->dir = kDirSouth;
				state->dwKeyPress = kKeyDown;
			}
			else if (lpEvent->jaxis.value < MIN_DEADZONE) {
				state->prevdir = (gpGlobals->fInBattle ? kDirUnknown : state->dir);
				state->dir = kDirNorth;
				state->dwKeyPress = kKeyUp;
			}
			else {
				state->dir = kDirUnknown;
			}
			return 1;
		}
		break;

	case SDL_JOYBUTTONDOWN:
		switch (lpEvent->jbutton.button)
		{
#if defined(GP2XWIZ)
		case 14:
#elif defined(CAANOO)
		case 3:
#endif
			state->dwKeyPress = kKeyMenu;
			return 1;

#if defined(GP2XWIZ)
		case 13:
#elif defined(CAANOO)
		case 2:
#endif
			state->dwKeyPress = kKeySearch;
			return 1;
		}
		break;
	}
	return 0;
}

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
	PAL_RegisterInputFilter(NULL, input_event_filter, NULL);
	gConfig.fLaunchSetting = FALSE;
	return 0;
}

VOID
UTIL_Platform_Quit(
	VOID
)
{
	chdir("/usr/gp2x");
	execl("./gp2xmenu", "./gp2xmenu", NULL);
}
