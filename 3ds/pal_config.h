#ifndef PAL_CONFIG_H
# define PAL_CONFIG_H

# define PAL_PREFIX            "sdmc:/3ds/sdlpal/"
# define PAL_SAVE_PREFIX       "sdmc:/3ds/sdlpal/"
# define PAL_CONFIG_PREFIX     "sdmc:/3ds/sdlpal/"
# define PAL_SCREENSHOT_PREFIX "sdmc:/3ds/sdlpal/"

# define PAL_AUDIO_DEFAULT_BUFFER_SIZE   2048

# define PAL_HAS_JOYSTICKS     0
# define PAL_HAS_MP3           0
# define PAL_HAS_OGG           0
# define PAL_HAS_TOUCH         0

# define PAL_DEFAULT_WINDOW_WIDTH   320
# define PAL_DEFAULT_WINDOW_HEIGHT  240

# define PAL_VIDEO_INIT_FLAGS  (SDL_SWSURFACE | SDL_TOPSCR | SDL_CONSOLEBOTTOM | SDL_FULLSCREEN)

# define PAL_SDL_INIT_FLAGS	   (SDL_INIT_VIDEO | SDL_INIT_AUDIO)

# define PAL_PLATFORM         "Nintendo 3DS"
# define PAL_CREDIT           "ZephRay"
# define PAL_PORTYEAR         "2017"

# define PAL_LARGE           static
# define PAL_FORCE_UPDATE_ON_PALETTE_SET

# define PAL_FILESYSTEM_IGNORE_CASE 1

# define PAL_SCALE_SCREEN   FALSE

# include <3ds.h>

#endif
