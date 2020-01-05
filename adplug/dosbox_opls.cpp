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
 * dosbox_opls.cpp - Wrapper of DOSBOX's OPL cores for SDLPAL by Lou Yihua.
 *
 */

#include "dosbox/dosbox.h"
#include "dosbox_opls.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

namespace DBOPL2
{
#	undef OPLTYPE_IS_OPL3
#	include "dosbox/opl.cpp.h"
}

namespace DBOPL3
{
#	define OPLTYPE_IS_OPL3
#	include "dosbox/opl.cpp.h"
#	undef OPLTYPE_IS_OPL3
}

namespace DBOPL
{
#	include "dosbox/dbopl.cpp.h"
	static bool doneTables = InitTables();
}
