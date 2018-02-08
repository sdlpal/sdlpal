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
// 3ds/pal_config.h: Nintendo 3DS specific header.
//          @Author: Lou Yihua <louyihua@21cn.com>, 2017.
//

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
# define PAL_DEFAULT_TEXTURE_WIDTH   640
# define PAL_DEFAULT_TEXTURE_HEIGHT  480

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
