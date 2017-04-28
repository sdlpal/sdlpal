#undef _NDEBUG
#include <assert.h>

#include "main.h"

#if defined(_WIN32) && !defined(__WINRT__)

#include <vfw.h>
#include <SDL.h>
#include <SDL_syswm.h>

int PAL_PlayAVI(const char *szFilename) {
        SDL_SysWMinfo wm;
        HWND hw;
        DWORD len, starttime;
        Uint32 prevw, prevh, curw, curh;
#if SDL_VERSION_ATLEAST(2, 0, 0)
	extern SDL_Window *gpWindow;
#else
	extern SDL_Surface *gpScreenReal;
#endif
	static BOOL aviDisabled = FALSE;
	char realPath[1024];

	if(aviDisabled) return -1;

        SDL_VERSION(&wm.version);
#if SDL_VERSION_ATLEAST(2, 0, 0)
        SDL_GetWindowWMInfo(gpWindow, &wm);
        hw = MCIWndCreate(wm.info.win.window, (HINSTANCE)GetModuleHandle(NULL),
                WS_CHILD | WS_VISIBLE | MCIWNDF_NOMENU | MCIWNDF_NOPLAYBAR | MCIWNDF_NOERRORDLG, NULL);
#else
	SDL_GetWMInfo(&wm);
        hw = MCIWndCreate(wm.window, (HINSTANCE)GetModuleHandle(NULL),
                WS_CHILD | WS_VISIBLE | MCIWNDF_NOMENU | MCIWNDF_NOPLAYBAR | MCIWNDF_NOERRORDLG, NULL);
#endif


        if (aviDisabled = (hw == NULL)) return -1;

	memset(realPath, 0, sizeof(realPath));
	strncpy(realPath, PAL_PREFIX, 512);
	strncat(realPath, szFilename, 512);

        if (MCIWndOpen(hw, realPath, 0) != 0) {
		aviDisabled = TRUE;
                MCIWndDestroy(hw);
                return -1;
        }

        SDL_FillRect(gpScreen, NULL, 0);
        VIDEO_UpdateScreen(NULL);

        MCIWndSetZoom(hw, 100);
        MCIWndPlay(hw);

        PAL_ClearKeyState();
        len = MCIWndGetLength(hw) * (1000 / 15);
        starttime = SDL_GetTicks();

#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_GetWindowSize(gpWindow, &curw, &curh);
#else
        curw = gpScreenReal->w;
        curh = gpScreenReal->h;
#endif

	prevw = curw;
	prevh = curh;
        MoveWindow(hw, 0, 0, curw, curh, TRUE);

        while (SDL_GetTicks() - starttime < len) {
                if (g_InputState.dwKeyPress != 0) break;
#if SDL_VERSION_ATLEAST(2, 0, 0)
		SDL_GetWindowSize(gpWindow, &curw, &curh);
#else
		curw = gpScreenReal->w;
		curh = gpScreenReal->h;
#endif
                if (prevw != curw || prevh != curh) {
                        MoveWindow(hw, 0, 0, curw, curh, TRUE);
                        prevw = curw;
                        prevh = curh;
                }
                UTIL_Delay(500);
        }

        PAL_ClearKeyState();

        MCIWndStop(hw);
        MCIWndDestroy(hw);

        return 0;
}
#endif

