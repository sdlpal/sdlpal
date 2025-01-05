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

#include <SDL_libretro.h>
#include "../../3rd/SDL/src/video/dummy/SDL_nullevents_c.h"

void DUMMY_PumpEvents(_THIS)
{
}

void DUMMY_InitOSKeymap(_THIS)
{
}

void SDL_libretro_KeyboardCallback(bool down,
                                   unsigned keycode,
                                   uint32_t character,
                                   uint16_t key_modifiers)
{
    SDL_keysym sym;
    sym.sym = keycode;
    sym.mod = key_modifiers;
    sym.unicode = character;
    SDL_PrivateKeyboard(down ? SDL_PRESSED : SDL_RELEASED, &sym);
}
