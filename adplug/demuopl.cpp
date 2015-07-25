//
//  demuopl.cpp
//  SDLPal
//
//  Created by palxex on 14-7-20.
//  Copyright (c) 2014 Wei Mingzhi. All rights reserved.
//

#include "demuopl.h"
#include <string.h>

CDemuopl::CDemuopl(int rate, bool bit16, bool usestereo)
	: use16bit(bit16), stereo(usestereo), rate(rate), chip(adlib_init(rate))
{
	currType = TYPE_OPL2;
}

CDemuopl::~CDemuopl()
{
	adlib_release(chip);
}

void CDemuopl::init()
{
	if (chip) adlib_release(chip);
	chip = adlib_init(rate);
}

void CDemuopl::update(short *buf, int samples)
{
	if (!chip) return;

	short *mixbuf1 = NULL;
	short *outbuf;
	if (use16bit) outbuf = buf;
	else{
		mixbuf1 = new short[samples * 2];
		outbuf = mixbuf1;
	}
	adlib_getsample(chip, outbuf, samples);
	if (stereo)
		for (int i = samples - 1; i >= 0; i--) {
			outbuf[i * 2] = outbuf[i];
			outbuf[i * 2 + 1] = outbuf[i];
		}
	//now reduce to 8bit if we need to
	if (!use16bit) {
		for (int i = 0; i < (stereo ? samples * 2 : samples); i++)
			((char *)buf)[i] = (outbuf[i] >> 8) ^ 0x80;
		delete[] mixbuf1;
	}
}

// template methods
void CDemuopl::write(int reg, int val)
{
	if (chip) adlib_write(chip, reg, val);
}
