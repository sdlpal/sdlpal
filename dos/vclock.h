/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2011-2026, SDLPAL development team.
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
// vclock.h: DOS virtual clock and IRQ0 hook interface.
//           @Author: palxex, 2026
//

#ifndef VCLOCK_H
#define VCLOCK_H

#include <stdint.h>

#define VCLOCKS_PER_SEC 1000000UL  // Time resolution: 1 tick = 1 microsecond

typedef uint64_t vclock_t;

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define VCLOCK_DEFAULT_HZ 100

/**
 * User-defined periodic hook function type.
 * Each hook receives a user-provided pointer on each invocation.
 */
typedef void (*vhook_fn)(void *userdata);

void vclock_setup(int freq);
// Time accessors
vclock_t   vclock(void);                     // Current timestamp in microseconds

// Delay primitive
void       vclock_delay(uint32_t ms);        // Blocking delay using busy wait

// Hook API
int        vhook_register(vhook_fn fn, uint16_t hz, void *userdata); // Register callback at frequency
int        vhook_unregister(vhook_fn fn);                             // Unregister callback by function pointer

#ifdef __cplusplus
}
#endif
#endif  // VCLOCK_H