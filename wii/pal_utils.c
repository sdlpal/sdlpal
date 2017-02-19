
#include "main.h"
#include <fat.h>

static int input_event_filter(const SDL_Event *lpEvent, PALINPUTSTATE *state)
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
	fatInitDefault();
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
