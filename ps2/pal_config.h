/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2026, SDLPAL development team.
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
// unix/pal_config.h: Linux & Unix specific header.
//           @Author: Lou Yihua <louyihua@21cn.com>, 2017.
//

#ifndef PAL_CONFIG_H
# define PAL_CONFIG_H
# define PAL_HAS_JOYSTICKS     1
# define PAL_HAS_MP3           1
# define PAL_HAS_OGG           1
# define PAL_HAS_OPUS          1

# define PAL_PREFIX            ""
# define PAL_SAVE_PREFIX       ""

# define PAL_DEFAULT_WINDOW_WIDTH   320
# define PAL_DEFAULT_WINDOW_HEIGHT  200

#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_FULLSCREEN_DESKTOP)

# define PAL_SDL_INIT_FLAGS	(SDL_INIT_VIDEO | SDL_INIT_AUDIO  | SDL_INIT_JOYSTICK)
//# define PAL_SDL_INIT_FLAGS   ( SDL_INIT_AUDIO | SDL_INIT_JOYSTICK)

# define PAL_SCALE_SCREEN     TRUE

# define PAL_PLATFORM         "PS2"
# define PAL_CREDIT           "Wolf3s"
# define PAL_PORTYEAR         "2026"

# define PAL_FILESYSTEM_IGNORE_CASE         1
# define PAL_HAS_PLATFORM_SPECIFIC_UTILS    1
# define PAL_HAS_PLATFORM_STARTUP           1

# define PAL_FORCE_UPDATE_ON_PALETTE_SET    1

# include <strings.h>
# include <ctype.h>
# include <sys/time.h>
# include <alloca.h>

#endif