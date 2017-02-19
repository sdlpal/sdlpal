#pragma once

#define PAL_PREFIX            UTIL_BasePath()
#define PAL_SAVE_PREFIX       UTIL_SavePath()
#define PAL_CONFIG_PREFIX     UTIL_ConfigPath()
#define PAL_SCREENSHOT_PREFIX UTIL_ScreenShotPath()
#define PAL_HAS_TOUCH         1
#define PAL_AUDIO_DEFAULT_BUFFER_SIZE   4096
#define PAL_DEFAULT_WINDOW_WIDTH   320
#define PAL_DEFAULT_WINDOW_HEIGHT  200

#if SDL_VERSION_ATLEAST(2,0,0)
# define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN)
#else
# define PAL_VIDEO_INIT_FLAGS  (SDL_HWSURFACE | SDL_RESIZABLE | (gConfig.fFullScreen ? SDL_FULLSCREEN : 0))
#endif

#define PAL_SDL_INIT_FLAGS	(SDL_INIT_VIDEO | SDL_INIT_AUDIO)

#define PAL_PLATFORM         "Windows Runtime"
#define PAL_CREDIT           "(Unknown)"
#define PAL_PORTYEAR         "2015"

#define PAL_HAS_CONFIG_PAGE  1

#define PAL_FILESYSTEM_IGNORE_CASE 1

LPCSTR
UTIL_BasePath(
   VOID
);

LPCSTR
UTIL_SavePath(
   VOID
);

LPCSTR
UTIL_ConfigPath(
   VOID
);

LPCSTR
UTIL_ScreenShotPath(
   VOID
);

BOOL
UTIL_TouchEnabled(
   VOID
);
