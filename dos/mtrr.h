/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
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
// mtrr.h: MTRR Write-Combining (WC) enabler for VGA and VESA LFB under DOS.
//         @Author: palxex, 2026
//
#ifndef MTRR_H
#define MTRR_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Initialize MTRR Write-Combining (WC) for VGA (0xA0000) and VESA LFB.
 * Should be called inside UTIL_Platform_Init() after SDL_Init().
 */
void DOS_MTRR_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* MTRR_H */
