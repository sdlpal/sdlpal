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
// wii/pal_config.h: Nintendo Wii specific header.
//

#ifndef PAL_CONFIG_H
# define PAL_CONFIG_H

# define PAL_HAS_JOYSTICKS     1
# define PAL_HAS_MOUSE         0
# define PAL_PREFIX            "sd:/sdlpal/"
# define PAL_SAVE_PREFIX       "sd:/sdlpal/"

# define PAL_DEFAULT_WINDOW_WIDTH   640
# define PAL_DEFAULT_WINDOW_HEIGHT  480

# if SDL_VERSION_ATLEAST(2,0,0)
#  define PAL_VIDEO_INIT_FLAGS  (SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE)
# else
#  define PAL_VIDEO_INIT_FLAGS  (SDL_SWSURFACE | (gConfig.fFullScreen ? SDL_FULLSCREEN : 0))
# endif

# define PAL_SDL_INIT_FLAGS	(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_CDROM | SDL_INIT_NOPARACHUTE | SDL_INIT_JOYSTICK)
# define PAL_SCALE_SCREEN   FALSE

# define PAL_PLATFORM         "Nintendo WII"
# define PAL_CREDIT           "Rikku2000,palxex"
# define PAL_PORTYEAR         "2017"

# define PAL_FILESYSTEM_IGNORE_CASE         1
# define PAL_HAS_PLATFORM_SPECIFIC_UTILS    1
# define PAL_HAS_PLATFORM_STARTUP           1

# include <gctypes.h>
# include <dirent.h>
# include <strings.h>

extern int alphasort(const struct dirent **d1, const struct dirent **d2);

#endif
