
#include "main.h"
#if defined(NDS)
#include <fat.h>
#endif

#if defined(__SYMBIAN32__)
static int input_event_filter(const SDL_Event *lpEvent, PALINPUTSTATE *state)
{
	//
	// Symbian-specific stuff
	//
	switch (lpEvent->type)
	{
	case SDLK_0:
		VIDEO_ToggleScaleScreen();
		return 1;
	case SDLK_1:
		AUDIO_DecreaseVolume();
		return 1;
	case SDLK_3:
		AUDIO_IncreaseVolume();
		return 1;
	}
	return 0;
}
#endif

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
#if defined(NDS)
	fatInitDefault();
#endif
#if defined(__SYMBIAN32__)
	PAL_RegisterInputFilter(NULL, input_event_filter, NULL);
#endif
	gConfig.fLaunchSetting = FALSE;
	return 0;
}

VOID
UTIL_Platform_Quit(
	VOID
)
{
#if defined (NDS)
   while (1);
#endif
}
