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
 * emuopls.h - Wrapper of several OPL2 emulators by Lou Yihua.
 *
 */

#ifndef SDLPAL_EMUOPLS_H
#define SDLPAL_EMUOPLS_H

#include "../common.h"
#include "opl.h"

class OPLCORE {
public:
	enum TYPE {
		MAME  = OPLCORE_MAME,
		DBFLT = OPLCORE_DBFLT,
		DBINT = OPLCORE_DBINT,
		NUKED = OPLCORE_NUKED,
	};

	OPLCORE(uint32_t rate) : rate(rate) {}
	virtual ~OPLCORE() {}

	virtual void Reset() = 0;
	virtual void Write(uint32_t reg, uint8_t val) = 0;
	virtual void Generate(short* buf, int samples) = 0;
	virtual OPLCORE* Duplicate() = 0;

protected:
	uint32_t rate;
};

// CEmuopl implements the base class of a OPL wrapper
// The DUALOPL2 mode should be implemented by a OPL3 core
class CEmuopl : public Copl {
public:
	static Copl* CreateEmuopl(OPLCORE::TYPE core, ChipType type, int rate);

	~CEmuopl() {
		if (currType == TYPE_DUAL_OPL2) {
			delete opl[1];
		}
		delete opl[0];
	}

	// Assumes a 16-bit, mono output sample buffer @ OPL2 mode
	// Assumes a 16-bit, stereo output sample buffer @ OPL3/DUAL_OPL2 mode
	void update(short *buf, int samples) {
		if (currType == TYPE_DUAL_OPL2) {
			auto lbuf = (short*)alloca(sizeof(short) * samples);
			opl[0]->Generate(lbuf, samples);
			opl[1]->Generate(buf + samples, samples);
			for (int i = 0, j = 0; i < samples; i++) {
				buf[j++] = lbuf[i];
				buf[j++] = buf[i + samples];
			}
		}
		else {
			opl[0]->Generate(buf, samples);
		}
	}

	void write(int reg, int val) {
		if (reg == 0x105 && currType == TYPE_OPL3) {
			opl3mode = ((val & 0x1) == 0x1);
		}
		else {
			reg &= opl3mode ? 0x1FF : 0xFF;
		}
		opl[currChip]->Write(reg, (uint8_t)val);
	}

	void init() {
		opl[0]->Reset();
		if (currType == TYPE_DUAL_OPL2) {
			opl[1]->Reset();
		}
		if (opl3mode) {
			opl[0]->Write(0x105, 1);
		}
	}

protected:
	CEmuopl(OPLCORE* core, ChipType type) : Copl(type), opl3mode(false) {
		opl[0] = core;
		opl[1] = (type == TYPE_DUAL_OPL2) ? core->Duplicate() : NULL;
		init();
	}

	OPLCORE* opl[2];
	bool     opl3mode;
};

#endif
