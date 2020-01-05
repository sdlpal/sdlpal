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
 * mame_opls.cpp - Wrapper of MAME's OPL cores by Lou Yihua.
 *
 */

#include "mame_opls.h"

#define SDLPAL_BUILD_OPL_CORE

namespace MAME
{
	namespace OPL2
	{
#		include "mame/fmopl.cpp.h"
	}
	namespace OPL3
	{
#		include "mame/ymf262.cpp.h"
	}
}
