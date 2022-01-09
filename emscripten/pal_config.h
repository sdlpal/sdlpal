/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2022, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
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
// emscripten/pal_config.h: Javascript specific header.
//                 @Author: Lou Yihua <louyihua@21cn.com>, 2017.
//

#ifndef PAL_CONFIG_H
# define PAL_CONFIG_H

# include <emscripten.h>
# define SDL_Delay emscripten_sleep

# define PAL_PREFIX            "/data/"
# define PAL_SAVE_PREFIX       "/data/"
# define PAL_HAS_TOUCH         0
# define PAL_DEFAULT_WINDOW_WIDTH   640
# define PAL_DEFAULT_WINDOW_HEIGHT  400
# define PAL_DEFAULT_TEXTURE_WIDTH   640
# define PAL_DEFAULT_TEXTURE_HEIGHT  400

# if SDL_VERSION_ATLEAST(2,0,0)
#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN)
# else
#  define PAL_VIDEO_INIT_FLAGS  (SDL_HWSURFACE | SDL_FULLSCREEN)
# endif

#define PAL_SDL_INIT_FLAGS	(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_CDROM | SDL_INIT_NOPARACHUTE | SDL_INIT_JOYSTICK)

# define PAL_PLATFORM         "Emscripten"
# define PAL_CREDIT           "palxex"
# define PAL_PORTYEAR         "2016"

# include <ctype.h>
# include <sys/time.h>

#define strcasestr(a,b) strstr(toupper((a)),toupper((b)))

#define PAL_HAS_GLSL 1
#define PAL_HAS_OPUS 0

#endif
