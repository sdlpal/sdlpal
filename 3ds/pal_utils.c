
#include "main.h"

static void init_filter()
{
	SDL_N3DSKeyBind(KEY_A, SDLK_RETURN);
	SDL_N3DSKeyBind(KEY_B, SDLK_ESCAPE);
	SDL_N3DSKeyBind(KEY_CPAD_UP, SDLK_UP);
	SDL_N3DSKeyBind(KEY_CPAD_DOWN, SDLK_DOWN);
	SDL_N3DSKeyBind(KEY_CPAD_LEFT, SDLK_LEFT);
	SDL_N3DSKeyBind(KEY_CPAD_RIGHT, SDLK_RIGHT);
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
	PAL_RegisterInputFilter(init_filter, NULL, NULL);
	gConfig.fLaunchSetting = FALSE;
	return 0;
}

VOID
UTIL_Platform_Quit(
	VOID
)
{
  gfxExit();
}
