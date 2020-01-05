/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2011-2020, SDLPAL development team.
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
// video_glsl.h: hacky SDL2 renderer that compatible of retroarch-style
// multipass shader preset header by palxex, 2018
//


#ifndef video_glsl_h
#define video_glsl_h

#include "main.h"

#if PAL_HAS_GLSL
# include "mini_glloader.h"
#endif

PAL_C_LINKAGE_BEGIN

extern SDL_Texture *VIDEO_GLSL_CreateTexture(int width, int height);
extern void VIDEO_GLSL_RenderCopy();

extern void VIDEO_GLSL_Init();
extern void VIDEO_GLSL_Setup();

extern void VIDEO_GLSL_Destroy();

PAL_C_LINKAGE_END

#endif /* video_glsl_h */
