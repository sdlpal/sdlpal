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
 * Copyright (c) 2011-2017, SDLPAL development team.
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

#include "common.h"
#include "emuopls.h"
#include <stdint.h>
#include <string.h>

// ===========================================================================================
// CMAMEopl implements a OPL2 wrapper using the M.A.M.E. core
// Assumes a 16-bit, mono output sample buffer
extern "C" {
#include "fmopl.h"
}
class CMAMEopl: public Copl {
public:
	CMAMEopl(int rate, ChipType type = TYPE_OPL2) : Copl(type), opl(NULL), rate(rate) { init(); }

	~CMAMEopl() { OPLDestroy(opl); }

	bool getstereo() { return false; }

	void update(short *buf, int samples) { YM3812UpdateOne(opl, buf, samples); }
	
	void write(int reg, int val) {
		OPLWrite(opl, 0, reg);
		OPLWrite(opl, 1, val);
	}

	void init() {
		if (opl) OPLDestroy(opl);
		opl = OPLCreate(OPL_TYPE_YM3812, 3579545, rate);
		OPLResetChip(opl);
	}

private:
	FM_OPL* opl;	// OPL2 emulator data
	int		rate;
};
// ===========================================================================================

// ===========================================================================================
// CDOSBOXopl implements a OPL2 wrapper using the DOSBOX core (old version)
// Assumes a 16-bit, mono output sample buffer
#include "dosbox_opl.h"
class CDOSBOXopl : public Copl
{
public:
	CDOSBOXopl(int rate) : Copl(TYPE_OPL2), chip(DOSBOX_OPL::adlib_init(rate)), rate(rate) {}
	~CDOSBOXopl() { DOSBOX_OPL::adlib_release(chip); }

	bool getstereo() { return false; }

	void update(short *buf, int samples) { DOSBOX_OPL::adlib_getsample(chip, buf, samples); }

	// template methods
	void write(int reg, int val) { DOSBOX_OPL::adlib_write(chip, reg, val); }

	void init() {
		DOSBOX_OPL::adlib_release(chip);
		chip = DOSBOX_OPL::adlib_init(rate);
	}

protected:
	DOSBOX_OPL::opl_chip * chip;
	int rate;
};
// ===========================================================================================

// ===========================================================================================
// CDBopl implements a OPL2/OPL3 wrapper using the DOSBOX core (new version)
// Assumes a 16-bit, mono output sample buffer on OPL2 mode (default)
// Assumes a 16-bit, stereo output sample buffer on OPL3 mode
#include "dbopl.h"
class CDBopl : public Copl
{
public:
	CDBopl(int rate, Copl::ChipType type = TYPE_OPL2)
	: Copl(type), buffer(NULL), rate(rate), len(0)
	, chv(type == TYPE_OPL3 ? 0x30 : 0x00) {
		init();
	}
	~CDBopl() { delete[] buffer; }

	bool getstereo() { return currType == TYPE_OPL3; }

	void update(short *buf, int samples) {
		// The DBOPL core generates 32-bit int sample data
		// So it needs to be converted to 16-bit int before used
		if (len < samples) {
			len = samples;
			if (buffer) delete[] buffer;
			buffer = new int32_t[len * (chip.opl3Active ? 2 : 1)];
		}
		if (chip.opl3Active) {
			chip.GenerateBlock3(samples, buffer);
			for (int i = 0; i < samples * 2; i++) {
				buf[i] = conver_to_int16_with_saturate(buffer[i]);
			}
		}
		else {
			chip.GenerateBlock2(samples, buffer);
			for (int i = 0; i < samples; i++) {
				buf[i] = conver_to_int16_with_saturate(buffer[i]);
			}
		}
	}

	// template methods
	void write(int reg, int val) {
		if (currType == TYPE_OPL3 && reg >> 4 == 0xC) {
			val |= chv;
		}
		chip.WriteReg(reg, (Bit8u)val);
	}

	void init() {
		chip.Setup(rate);
		if (currType == TYPE_OPL3) {
			// Enable OPL3 mode
			chip.WriteReg(0x105, 1);
		}
	}

	virtual void setch(bool left, bool right) {
		if (currType == TYPE_OPL3) {
			left ? (chv |= 0x10) : (chv &= 0xef);
			right ? (chv |= 0x20) : (chv &= 0xdf);
		}
	}

protected:
	DBOPL::Chip chip;
	int32_t* buffer;
	int rate, len, chv;

	static bool _inited;

	static inline int16_t conver_to_int16_with_saturate(int32_t sample)
	{
		if (sample > 32767)
			return 32767;
		else if (sample < -32768)
			return -32768;
		else
			return (int16_t)sample;
	}
};
bool CDBopl::_inited = DBOPL::InitTables();
// ===========================================================================================

// ===========================================================================================
// CNukedopl implements a OPL3 wrapper using the Nuked OPL3 core v1.8
// Assumes a 16-bit, stereo output sample buffer
// When working at OPL2 compatible mode, it behaviors like a dual-opl2 chip
extern "C" {
#include "opl3.h"
}
class CNukedopl : public Copl {
public:
	CNukedopl(int rate, Copl::ChipType type = TYPE_OPL3)
	: Copl(type), rate(rate), chv(type == TYPE_OPL3 ? 0x30 : 0x00) {
		init();
	}

	bool getstereo() { return currType == TYPE_OPL3; }

	void update(short *buf, int samples) { OPL3_GenerateStream(&opl, buf, samples); }

	void write(int reg, int val) {
		if (currType == TYPE_OPL3 && reg >> 4 == 0xC) {
			val |= chv;
		}
		OPL3_WriteRegBuffered(&opl, (Bit16u)reg, (Bit8u)val);
	}

	void init() {
		OPL3_Reset(&opl, rate);
		if (currType == TYPE_OPL3) {
			// Enable OPL3 mode
			OPL3_WriteReg(&opl, 0x105, 1);
		}
	}

	virtual void setch(bool left, bool right) {
		if (currType == TYPE_OPL3) {
			left  ? (chv |= 0x10) : (chv &= 0xef);
			right ? (chv |= 0x20) : (chv &= 0xdf);
		}
	}

private:
	opl3_chip  opl;		// OPL3 emulator data
	int        rate, chv;
};
// ===========================================================================================

Copl* CreateOPLWrapper(const char* type, int rate)
{
	if (type && strcasecmp(type, "MAME") == 0)
		return new CMAMEopl(rate);
	else if (type && strcasecmp(type, "DOSBOX") == 0)
		return new CDOSBOXopl(rate);
	else if (type && strcasecmp(type, "DOSBOX_NEW") == 0)
		return new CDBopl(rate);
	else if (type && strcasecmp(type, "NUKED") == 0)
		return new CNukedopl(rate);
	else
		return NULL;
}
