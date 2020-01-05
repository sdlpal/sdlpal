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
		const uint32_t INTERNAL_FREQ = 3579545;   // The OPL2 operates at 3.58MHz
	}
	namespace OPL3
	{
#		include "mame/ymf262.h"
		const uint32_t INTERNAL_FREQ = 14318180;  // The OPL3 operates at 14.318MHz
	}
}

class MAMEOPL2 : public OPLCORE
{
public:
	MAMEOPL2(uint32_t samplerate) : OPLCORE(samplerate), chip(MAME::OPL2::ym3812_init(NULL, MAME::OPL2::INTERNAL_FREQ, samplerate)) { }
	~MAMEOPL2() { MAME::OPL2::ym3812_shutdown(chip); }

	void Reset() { MAME::OPL2::ym3812_reset_chip(chip); }
	void Write(uint32_t reg, uint8_t val) {
		MAME::OPL2::ym3812_write(chip, 0, reg);
		MAME::OPL2::ym3812_write(chip, 1, val);
	}
	void Generate(short* buf, int samples) { MAME::OPL2::ym3812_update_one(chip, buf, samples); }
	OPLCORE* Duplicate() { return new MAMEOPL2(rate); }

private:
	void* chip;
};

class MAMEOPL3 : public OPLCORE
{
public:
	MAMEOPL3(uint32_t samplerate) : OPLCORE(samplerate), chip(MAME::OPL3::ymf262_init(NULL, MAME::OPL3::INTERNAL_FREQ, samplerate)) { }
	~MAMEOPL3() { MAME::OPL3::ymf262_shutdown(chip); }

	void Reset() { MAME::OPL3::ymf262_reset_chip(chip); }
	void Write(uint32_t reg, uint8_t val) {
		MAME::OPL3::ymf262_write(chip, ((reg >> 7) & 0x2)    , reg & 0xff);
		MAME::OPL3::ymf262_write(chip, ((reg >> 7) & 0x2) + 1, val);
	}
	void Generate(short* buf, int samples) { MAME::OPL3::ymf262_update_one(chip, buf, samples); }
	OPLCORE* Duplicate() { return new MAMEOPL3(rate); }

private:
	void* chip;
};

#endif
