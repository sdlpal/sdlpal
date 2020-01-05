/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2005 Simon Peter, <dn.tlp@gmx.net>, et al.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * -------------------------------------------------------------------------
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
 * -------------------------------------------------------------------------
 *
 * wrappers.cpp - Wrapper of several OPL2 emulators by Lou Yihua.
 *
 */

#include "dosbox_opls.h"
#include "mame_opls.h"
#include "nuked_opl.h"

Copl* CEmuopl::CreateEmuopl(OPLCORE::TYPE core, Copl::ChipType type, int rate) {

	if (type != Copl::TYPE_OPL2 && type != Copl::TYPE_DUAL_OPL2 && type != Copl::TYPE_OPL3) {
		return NULL;
	}

	switch (core) {
	case OPLCORE::MAME:  return new CEmuopl(type == Copl::TYPE_OPL3 ? (OPLCORE*)(new MAMEOPL3(rate))  : (OPLCORE*)(new MAMEOPL2(rate)) , type);
	case OPLCORE::DBFLT: return new CEmuopl(type == Copl::TYPE_OPL3 ? (OPLCORE*)(new DBFLTOPL3(rate)) : (OPLCORE*)(new DBFLTOPL2(rate)), type);
	case OPLCORE::DBINT: return new CEmuopl(type == Copl::TYPE_OPL3 ? (OPLCORE*)(new DBINTOPL3(rate)) : (OPLCORE*)(new DBINTOPL2(rate)), type);
	case OPLCORE::NUKED: return new CEmuopl(new NUKEDOPL3(rate), TYPE_OPL3);
	default: return NULL;
	}
}
