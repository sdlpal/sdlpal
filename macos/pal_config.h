/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2021, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// macos/pal_config.h: macOS specific header.
//            @Author: Lou Yihua <louyihua@21cn.com>, 2017.
//

#ifndef PAL_CONFIG_H
# define PAL_CONFIG_H

# ifndef PAL_HAS_JOYSTICKS
#  define PAL_HAS_JOYSTICKS    1
# endif

# if SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION <= 2
#  define PAL_HAS_SDLCD         1
# endif

# define PAL_PREFIX            "./"
# define PAL_SAVE_PREFIX       "./"

# define PAL_DEFAULT_WINDOW_WIDTH   640
# define PAL_DEFAULT_WINDOW_HEIGHT  400

# if SDL_VERSION_ATLEAST(2,0,0)
#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | (gConfig.fFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0) | SDL_WINDOW_ALLOW_HIGHDPI)
# else
#  define PAL_VIDEO_INIT_FLAGS  (SDL_HWSURFACE | SDL_RESIZABLE | (gConfig.fFullScreen ? SDL_FULLSCREEN : 0))
# endif

# define PAL_SDL_INIT_FLAGS	(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_CDROM | SDL_INIT_NOPARACHUTE | SDL_INIT_JOYSTICK)

# define PAL_PLATFORM         NULL
# define PAL_CREDIT           NULL
# define PAL_PORTYEAR         NULL

#ifndef PAL_HAS_NATIVEMIDI
#define PAL_HAS_NATIVEMIDI  1
#endif

#define PAL_HAS_MOUSE 0

#define PAL_HAS_CONFIG_PAGE 0

#define PAL_FILESYSTEM_IGNORE_CASE 1

#define PAL_HAS_PLATFORM_SPECIFIC_UTILS 1
#define PAL_HAS_PLATFORM_STARTUP        1

#if SDL_VERSION_ATLEAST(2,0,0)
# define PAL_HAS_GLSL 1
#endif

#include <sys/time.h>

#endif
