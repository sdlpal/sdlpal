#ifndef PAL_CONFIG_H
# define PAL_CONFIG_H

# define PAL_PREFIX            "/mnt/sdcard/sdlpal/"
# define PAL_SAVE_PREFIX       "/mnt/sdcard/sdlpal/"
# define PAL_HAS_TOUCH         1
# define PAL_DEFAULT_WINDOW_WIDTH   320
# define PAL_DEFAULT_WINDOW_HEIGHT  200

# if SDL_VERSION_ATLEAST(2,0,0)
#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN)
# else
#  define PAL_VIDEO_INIT_FLAGS  (SDL_HWSURFACE | SDL_FULLSCREEN)
# endif

#define PAL_SDL_INIT_FLAGS	(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_CDROM | SDL_INIT_NOPARACHUTE | SDL_INIT_JOYSTICK)

# define PAL_PLATFORM         "Android"
# define PAL_CREDIT           "Rikku2000"
# define PAL_PORTYEAR         "2013"

#endif
