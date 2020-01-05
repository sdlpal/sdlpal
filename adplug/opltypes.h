/*
 * SDLPAL
 * Copyright (c) 2011-2020, SDLPAL development team.
 * All rights reserved.
 *
 * This file is part of SDLPAL.
 *
 * SDLPAL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * opltypes.h - Define enumerates for OPL chips for SDLPAL by Lou Yihua.
 *
 */

#ifndef SDLPAL_OPLTYPES_H
#define SDLPAL_OPLTYPES_H

typedef enum _OPLCORE_TYPE {
	OPLCORE_MAME,
	OPLCORE_DBFLT,
	OPLCORE_DBINT,
	OPLCORE_NUKED,
} OPLCORE_TYPE;

typedef enum _OPLCHIP_TYPE {
	OPLCHIP_OPL2,
	OPLCHIP_OPL3,

	OPLCHIP_DUAL_OPL2,
} OPLCHIP_TYPE;

#define OPL3_EXTREG_BASE    0x100
#define OPL3_4OP_REGISTER   0x104
#define OPL3_MODE_REGISTER  0x105

#endif
