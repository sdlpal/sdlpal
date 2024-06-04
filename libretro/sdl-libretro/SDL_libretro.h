/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2024, SDLPAL development team.
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

#ifndef _SDL_LIBRETRO_H
#define _SDL_LIBRETRO_H
#include <SDL.h>
#include <libretro.h>

#ifdef __cplusplus
extern "C" {
#endif

void    SDL_libretro_RefreshVideo(retro_video_refresh_t video_cb);
void    SDL_libretro_ProduceAudio(retro_audio_sample_batch_t audio_batch_cb);
int     SDL_PrivateKeyboard(Uint8 state, SDL_keysym *key);

#ifdef __cplusplus
}
#endif
#endif
