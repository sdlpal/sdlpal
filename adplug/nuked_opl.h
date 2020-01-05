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

#ifndef SDLPAL_NUKED_OPL_H
#define SDLPAL_NUKED_OPL_H

#include "emuopls.h"

#include <stdint.h>
extern "C" {
#include "nuked/opl3.h"
}

class NUKEDOPL3 : public OPLCORE
{
public:
	NUKEDOPL3(uint32_t samplerate) : OPLCORE(samplerate) {}

	void Reset() { OPL3_Reset(&chip, rate); }
	void Write(uint32_t reg, uint8_t val) {
		if (reg == OPL3_4OP_REGISTER || reg == OPL3_MODE_REGISTER) {
			OPL3_WriteReg(&chip, (uint16_t)reg, val);
		}
		else {
			OPL3_WriteRegBuffered(&chip, (uint16_t)reg, val);
		}
	}
	void Generate(short* buf, int samples) { OPL3_GenerateStream(&chip, buf, samples); }
	OPLCORE* Duplicate() { return new NUKEDOPL3(rate); }

private:
	opl3_chip chip;
};

#endif
