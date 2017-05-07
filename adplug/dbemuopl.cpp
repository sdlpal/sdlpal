/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
// All rights reserved.
// Created by Lou Yihua <louyihua@21cn.com>, 2015-08-03.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "dbemuopl.h"
#include <stdlib.h>

bool CDBemuopl::_inited = DBOPL::InitTables();

static inline int16_t conver_to_int16(int32_t sample)
{
	if (sample > 32767)
		return 32767;
	else if (sample < -32768)
		return -32768;
	else
		return (int16_t)sample;
}

static inline uint8_t conver_to_uint8(int32_t sample)
{
	if (sample > 32767)
		return 0xff;
	else if (sample < -32768)
		return 0;
	else
		return (uint8_t)(sample >> 8) ^ 0x80;
}

CDBemuopl::CDBemuopl(int rate, bool bit16, bool usestereo)
	: buffer(NULL), rate(rate), maxlen(0)
	, use16bit(bit16), stereo(usestereo)
{
	currType = TYPE_OPL2;
	chip.Setup(rate);
}

CDBemuopl::~CDBemuopl()
{
	if (buffer) delete[] buffer;
}

void CDBemuopl::init()
{
	chip.Setup(rate);
}

void CDBemuopl::update(short *buf, int samples)
{
	if (chip.opl3Active)
		update_opl3(buf, samples);
	else
		update_opl2(buf, samples);
}

void CDBemuopl::write(int reg, int val)
{
	chip.WriteReg(reg, (Bit8u)val);
}

// OPL3 generate stereo samples
void CDBemuopl::update_opl3(short *buf, int samples)
{
	if (maxlen < samples * 2)
	{
		if (buffer) delete[] buffer;
		buffer = new int32_t[maxlen = samples * 2];
	}

	chip.GenerateBlock3(samples, buffer);

	if (use16bit)
	{
		if (stereo)
		{
			for (int i = 0; i < samples * 2; i++)
				buf[i] = conver_to_int16(buffer[i]);
		}
		else
		{
			for (int i = 0; i < samples; i++)
				buf[i] = conver_to_int16((buffer[i * 2] + buffer[i * 2 + 1]) >> 1);
		}
	}
	else
	{
		uint8_t* outbuf = (uint8_t*)buf;
		if (stereo)
		{
			for (int i = 0; i < samples * 2; i++)
				outbuf[i] = conver_to_uint8(buffer[i]);
		}
		else
		{
			for (int i = 0; i < samples; i++)
				outbuf[i] = conver_to_uint8((buffer[i * 2] + buffer[i * 2 + 1]) >> 1);
		}
	}
}

// OPL2 generate mono samples
void CDBemuopl::update_opl2(short *buf, int samples)
{
	if (maxlen < samples)
	{
		if (buffer) delete[] buffer;
		buffer = new int32_t[maxlen = samples];
	}

	chip.GenerateBlock2(samples, buffer);

	if (use16bit)
	{
		if (stereo)
		{
			for (int i = 0; i < samples; i++)
				buf[i * 2 + 1] = buf[i * 2] = conver_to_int16(buffer[i]);
		}
		else
		{
			for (int i = 0; i < samples; i++)
				buf[i] = conver_to_int16(buffer[i]);
		}
	}
	else
	{
		uint8_t* outbuf = (uint8_t*)buf;
		if (stereo)
		{
			for (int i = 0; i < samples; i++)
				outbuf[i * 2 + 1] = outbuf[i * 2] = conver_to_uint8(buffer[i]);
		}
		else
		{
			for (int i = 0; i < samples; i++)
				outbuf[i] = conver_to_uint8(buffer[i]);
		}
	}
}
