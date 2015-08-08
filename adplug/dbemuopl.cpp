//
//  dbemuopl.cpp
//  SDLPal
//
//  Created by louyihua on 15-08-03.
//  Copyright (c) 2014 Wei Mingzhi. All rights reserved.
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
	: use16bit(bit16), stereo(usestereo), rate(rate)
	, maxlen(0), buffer(NULL)
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
