/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2018, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ios/pal_config.h: iOS specific header.
//          @Author: Lou Yihua <louyihua@21cn.com>, 2017.
//

#ifndef PAL_CONFIG_H
# define PAL_CONFIG_H

# define PAL_PREFIX            UTIL_BasePath()
# define PAL_SAVE_PREFIX       UTIL_SavePath()
# define PAL_HAS_TOUCH         1
# define PAL_DEFAULT_WINDOW_WIDTH   320
# define PAL_DEFAULT_WINDOW_HEIGHT  200
# define PAL_DEFAULT_TEXTURE_WIDTH   1280
# define PAL_DEFAULT_TEXTURE_HEIGHT  800

# if SDL_VERSION_ATLEAST(2,0,0)
#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN)
# else
#  define PAL_VIDEO_INIT_FLAGS  (SDL_HWSURFACE | SDL_FULLSCREEN)
# endif

# define PAL_SDL_INIT_FLAGS	(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_CDROM | SDL_INIT_NOPARACHUTE | SDL_INIT_JOYSTICK)

# define PAL_PLATFORM         "Apple iOS"
# define PAL_CREDIT           "(Unknown)"
# define PAL_PORTYEAR         "2015"

# define PAL_IS_VALID_JOYSTICK(s)  (strcmp((s), "applesmc") != 0)

#define PAL_HAS_NATIVEMIDI  1

#define PAL_HAS_CONFIG_PAGE 1

#define PAL_HAS_PLATFORM_SPECIFIC_UTILS 1

PAL_C_LINKAGE_BEGIN

LPCSTR
UTIL_BasePath(
   VOID
);

LPCSTR
UTIL_SavePath(
   VOID
);

PAL_C_LINKAGE_END

#include <sys/time.h>

#define PAL_HAS_GLSL 1

#endif
