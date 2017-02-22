
#include "main.h"

static int input_event_filter(const SDL_Event *lpEvent, PALINPUTSTATE *state)
{
	switch (lpEvent->type)
	{
	case SDLK_SPACE:
		state->dwKeyPress = kKeyMenu;
		return 1;

	case SDLK_LCTRL:
		state->dwKeyPress = kKeySearch;
		return 1;
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
}
