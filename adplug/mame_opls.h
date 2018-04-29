/*
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
 *
 * dosbox_opls.h - Wrapper of DOSBOX's OPL cores for SDLPAL by Lou Yihua.
 *
 */

#ifndef SDLPAL_MAME_OPLS_H
#define SDLPAL_MAME_OPLS_H

#include <stdint.h>
#include "mame/mame.h"
#include "emuopls.h"

namespace MAME
{
	namespace OPL2
	{
#		include "mame/fmopl.h"
	}
	namespace OPL3
	{
#		include "mame/ymf262.h"
	}

	const uint32_t OPL2_INTERNAL_FREQ = 3600000;   // The OPL2 operates at 3.6MHz
	const uint32_t OPL3_INTERNAL_FREQ = 14400000;  // The OPL3 operates at 14.4MHz
}

class MAMEOPL2 : public OPLCORE
{
public:
	MAMEOPL2(uint32_t samplerate) : OPLCORE(samplerate), chip(MAME::OPL2::ym3812_init(NULL, MAME::OPL2_INTERNAL_FREQ, samplerate)) { }
	~MAMEOPL2() { MAME::OPL2::ym3812_shutdown(chip); }

	void Reset() { MAME::OPL2::ym3812_reset_chip(chip); }
	void Write(uint32_t reg, uint8_t val) { MAME::OPL2::ym3812_write(chip, 0, reg); MAME::OPL2::ym3812_write(chip, 1, val); }
	void Generate(short* buf, int samples) { MAME::OPL2::ym3812_update_one(chip, buf, samples); }
	OPLCORE* Duplicate() { return new MAMEOPL2(rate); }

private:
	void* chip;
};

class MAMEOPL3 : public OPLCORE
{
public:
	MAMEOPL3(uint32_t samplerate) : OPLCORE(samplerate), chip(MAME::OPL3::ymf262_init(NULL, MAME::OPL3_INTERNAL_FREQ, samplerate)) { }
	~MAMEOPL3() { MAME::OPL3::ymf262_shutdown(chip); }

	void Reset() { MAME::OPL3::ymf262_reset_chip(chip); }
	void Write(uint32_t reg, uint8_t val) { MAME::OPL3::ymf262_write(chip, addr_port[reg >> 8], reg); MAME::OPL3::ymf262_write(chip, data_port[reg >> 8], val); }
	void Generate(short* buf, int samples) { MAME::OPL3::ymf262_update_one(chip, buf, samples); }
	OPLCORE* Duplicate() { return new MAMEOPL3(rate); }

private:
	void* chip;
	const int addr_port[2] = { 0, 2 };
	const int data_port[2] = { 1, 3 };
};

#endif
