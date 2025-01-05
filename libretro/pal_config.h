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
// libretro/pal_config.h: Libretro specific header.
//           @Author: iyzsong <iyzsong@envs.net>, 2024.
//

#ifndef PAL_CONFIG_H
#define PAL_CONFIG_H

#if SDL_VERSION_ATLEAST(2,0,0)
#error "this libretro port must build with SDL 1.2, please checkout 3rd/SDL with v1.2.15"
#endif

#define PAL_HAS_OGG         0
#define PAL_HAS_OPUS        0
#define PAL_HAS_MP3         0
#define PAL_HAS_NATIVEMIDI  0
#define PAL_HAS_JOYSTICKS   0
#define PAL_HAS_SDLCD       0

#define PAL_PREFIX          "./"
#define PAL_SAVE_PREFIX     "./"
#define PAL_HAS_CONFIG_PAGE 0
#define PAL_HAS_PLATFORM_SPECIFIC_UTILS 1

#define PAL_DEFAULT_WINDOW_WIDTH    320
#define PAL_DEFAULT_WINDOW_HEIGHT   200
#define PAL_VIDEO_INIT_FLAGS        (SDL_SWSURFACE | SDL_FULLSCREEN)
#define PAL_SDL_INIT_FLAGS          (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE)

#define PAL_PLATFORM         "Libretro"
#define PAL_CREDIT           NULL
#define PAL_PORTYEAR         "2024"

#include <sys/time.h>
#include <ctype.h>

#endif
