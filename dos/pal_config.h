/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
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
// win32/pal_config.h: WIN32-specific header.
//            @Author: Lou Yihua <louyihua@21cn.com>, 2016.
//

#pragma once

#define PAL_PREFIX            "./"
#define PAL_SAVE_PREFIX       "./"

#define PAL_DEFAULT_WINDOW_WIDTH   320
#define PAL_DEFAULT_WINDOW_HEIGHT  200
#define PAL_DEFAULT_FULLSCREEN_HEIGHT 200

#define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | (gConfig.fFullScreen ? SDL_WINDOW_FULLSCREEN : 0))

#define PAL_SDL_INIT_FLAGS	(SDL_INIT_VIDEO | SDL_INIT_CDROM | SDL_INIT_NOPARACHUTE | SDL_INIT_JOYSTICK)

#undef SDL_RENDERER_ACCELERATED
#define SDL_RENDERER_ACCELERATED 0
#undef SDL_RENDERER_PRESENTVSYNC
#define SDL_RENDERER_PRESENTVSYNC 0

#define PAL_PLATFORM         NULL
#define PAL_CREDIT           NULL
#define PAL_PORTYEAR         NULL

#define PAL_HAS_CONFIG_PAGE 0

#define PAL_FILESYSTEM_IGNORE_CASE 1

#define PAL_PATH_SEPARATORS "\\/"

#define PAL_IS_PATH_SEPARATOR(x) ((x) == '\\' || (x) == '/')

PAL_C_LINKAGE char* strcasestr(const char *, const char *);
#define PAL_NEED_STRCASESTR 1

# define PAL_HAS_GLSL          0
# define PAL_HAS_JOYSTICKS     0
# define PAL_HAS_TOUCH         0
# define PAL_HAS_MOUSE         0
# define PAL_HAS_MP3           0
# define PAL_HAS_OGG           0
# define PAL_HAS_OPUS          0
# define PAL_HAS_NATIVEMIDI    0 //388!

#include <malloc.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <wctype.h>
#include <math.h>
