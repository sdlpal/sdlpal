#ifndef PAL_CONFIG_H
# define PAL_CONFIG_H

#if defined (__SYMBIAN32__)

#undef  _WIN32
#undef  SDL_INIT_JOYSTICK
#define SDL_INIT_JOYSTICK     0
#define PAL_HAS_MOUSE         1
#define PAL_PREFIX            "e:/data/pal/"
#define PAL_SAVE_PREFIX       "e:/data/pal/"
# ifdef __S60_5X__
#  define PAL_DEFAULT_WINDOW_WIDTH   640
#  define PAL_DEFAULT_WINDOW_HEIGHT  360
# else
#  define PAL_DEFAULT_WINDOW_WIDTH   320
#  define PAL_DEFAULT_WINDOW_HEIGHT  240
# endif
# define PAL_DEFAULT_TEXTURE_WIDTH   640
# define PAL_DEFAULT_TEXTURE_HEIGHT  480
# if SDL_VERSION_ATLEAST(2,0,0)
#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE)
# else
#  define PAL_VIDEO_INIT_FLAGS  (SDL_SWSURFACE | (gConfig.fFullScreen ? SDL_FULLSCREEN : 0))
#  define PAL_FATAL_OUTPUT(s)   UTIL_WriteLog(LOG_DEBUG,"[0x%08x][%s][%s] - %s",(long)TerminateOnError,"TerminateOnError",__FILE__, (s)); SDL_Delay(3000)
# endif

# define PAL_SDL_INIT_FLAGS	(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_CDROM | SDL_INIT_NOPARACHUTE | SDL_INIT_JOYSTICK)

# define PAL_PLATFORM         "  Symbian S60 \x79FB\x690D (c) 2009, netwan."
# define PAL_CREDIT           "netwan"
# define PAL_PORTYEAR         "2009"

# define PAL_LARGE           static
# define PAL_FORCE_UPDATE_ON_PALETTE_SET

# if !defined (__S60_5X__)
#  define PAL_SCALE_SCREEN   FALSE
# endif

#elif defined(NDS)

#define PAL_PREFIX            "./"
#define PAL_SAVE_PREFIX       "./"

#  define PAL_DEFAULT_WINDOW_WIDTH   293
#  define PAL_DEFAULT_WINDOW_HEIGHT  196

# if SDL_VERSION_ATLEAST(2,0,0)
#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN)
# else
#  define PAL_VIDEO_INIT_FLAGS  (SDL_SWSURFACE | SDL_FULLSCREEN)
# endif

# define PAL_SDL_INIT_FLAGS	(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_CDROM | SDL_INIT_NOPARACHUTE | SDL_INIT_JOYSTICK)

# define PAL_PLATFORM         "Nintendo DS"
# define PAL_CREDIT           "(Unknown)"
# define PAL_PORTYEAR         "2012"

#endif

#endif
