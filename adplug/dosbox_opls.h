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

#ifndef SDLPAL_DOSBOX_OPLS_H
#define SDLPAL_DOSBOX_OPLS_H

#include "common.h"
#include "dosbox/dosbox.h"
#include "emuopls.h"

namespace DBOPL2
{
#	undef SDLPAL_DOSBOXOPL_H
#	undef OPLTYPE_IS_OPL3
#	include "dosbox/opl.h"
}

namespace DBOPL3
{
#	undef SDLPAL_DOSBOXOPL_H
#	define OPLTYPE_IS_OPL3
#	include "dosbox/opl.h"
#	undef OPLTYPE_IS_OPL3
}

namespace DBOPL
{
#	undef SDLPAL_DBOPL_H
#	include "dosbox/dbopl.h"
}

class DBFLTOPL2 : public OPLCORE
{
public:
	DBFLTOPL2(uint32_t samplerate) : OPLCORE(samplerate) {}

	void Reset() { DBOPL2::adlib_init(&chip, rate); }
	void Write(uint32_t reg, uint8_t val) { DBOPL2::adlib_write(&chip, reg, val); }
	void Generate(short* buf, int samples) { DBOPL2::adlib_getsample(&chip, buf, samples); }
	OPLCORE* Duplicate() { return new DBFLTOPL2(rate); }

private:
	DBOPL2::opl_chip chip;
};

class DBFLTOPL3 : public OPLCORE
{
public:
	DBFLTOPL3(uint32_t samplerate) : OPLCORE(samplerate) {}

	void Reset() { DBOPL3::adlib_init(&chip, rate); }
	void Write(uint32_t reg, uint8_t val) { DBOPL3::adlib_write(&chip, reg, val); }
	void Generate(short* buf, int samples) { DBOPL3::adlib_getsample(&chip, buf, samples); }
	OPLCORE* Duplicate() { return new DBFLTOPL3(rate); }

private:
	DBOPL3::opl_chip chip;
};

static inline short clip_sample(int32_t sample) {
	if (sample > 32767)
		return 32767;
	else if (sample < -32768)
		return -32768;
	else
		return sample;
}

class DBINTOPL2 : public OPLCORE
{
public:
	DBINTOPL2(uint32_t samplerate) : OPLCORE(samplerate) {}

	void Reset() { chip.Setup(rate); }
	void Write(uint32_t reg, uint8_t val) { chip.WriteReg(reg, val); }
	void Generate(short* buf, int samples)
	{
		auto buffer = (int32_t*)alloca(samples * sizeof(int32_t));
		chip.GenerateBlock2(samples, buffer);
		for (int i = 0; i < samples; i++) {
			buf[i] = clip_sample(buffer[i]);
		}
	}
	OPLCORE* Duplicate() { return new DBINTOPL2(rate); }

private:
	DBOPL::Chip chip;
};

class DBINTOPL3 : public OPLCORE
{
public:
	DBINTOPL3(uint32_t samplerate) : OPLCORE(samplerate) {}

	void Reset() { chip.Setup(rate); }
	void Write(uint32_t reg, uint8_t val) { chip.WriteReg(reg, val); }
	void Generate(short* buf, int samples)
	{
		if (chip.opl3Active) {
			auto buffer = (int32_t*)alloca(samples * sizeof(int32_t) * 2);
			chip.GenerateBlock3(samples, buffer);
			for (int i = 0; i < samples * 2; i++) {
				buf[i] = clip_sample(buffer[i]);
			}
		}
		else {
			auto buffer = (int32_t*)alloca(samples * sizeof(int32_t));
			chip.GenerateBlock2(samples, buffer);
			for (int i = 0, j = 0; i < samples; i++, j += 2) {
				buf[j + 1] = buf[j] = clip_sample(buffer[i]);
			}
		}
	}
	OPLCORE* Duplicate() { return new DBINTOPL3(rate); }

private:
	DBOPL::Chip chip;
};

#endif
