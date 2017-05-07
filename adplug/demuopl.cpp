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
*
* demuopl.cpp - Emulated OPL using DOSBOX's emulator, by Pal Lockheart
*               <palxex@gmail.com> on 2014-07-20.
*/

#include "demuopl.h"
#include <string.h>

CDemuopl::CDemuopl(int rate, bool bit16, bool usestereo)
	: chip(adlib_init(rate)), rate(rate), use16bit(bit16), stereo(usestereo)
{
	currType = TYPE_OPL2;
}

CDemuopl::~CDemuopl()
{
	adlib_release(chip);
}

void CDemuopl::init()
{
	adlib_release(chip);
	chip = adlib_init(rate);
}

void CDemuopl::update(short *buf, int samples)
{
	if (!chip) return;

	short *mixbuf1 = NULL;
	short *outbuf;
	if (use16bit)
		outbuf = buf;
	else
		outbuf = mixbuf1 = new short[samples * 2];
	adlib_getsample(chip, outbuf, samples);
	if (stereo) {
		for (int i = samples - 1; i >= 0; i--) {
			outbuf[i * 2] = outbuf[i];
			outbuf[i * 2 + 1] = outbuf[i];
		}
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
