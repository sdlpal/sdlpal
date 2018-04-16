/*
 * AdPlug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2005 Simon Peter <dn.tlp@gmx.net>, et al.
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
 * nukedopl.cpp - Adapter of Nuked OPL, by Lou Yihua
 */

#include "nukedopl.h"
#include <stdlib.h>

#ifndef NULL
#define NULL     0
#endif

CNukedopl::CNukedopl(int rate, bool bit16, bool usestereo)
	: buffer(NULL), bufsamples(0), rate(rate)
	, use16bit(bit16), stereo(usestereo) {
   currType = TYPE_OPL3;
   init();
}

CNukedopl::~CNukedopl() {
	free(buffer);
}

void CNukedopl::update(short *buf, int samples) {
	// ensure that our buffer is adequately sized
	// OPL3 always produces stereo output
	if (bufsamples < samples && (!use16bit || !stereo)) {
		buffer = (Bit16s*)realloc(buffer, (bufsamples = samples) * sizeof(Bit16s) * 2);
	}

	//all of the following rendering code produces 16bit output
	Bit16s *tempbuf = (use16bit && stereo) ? buf : buffer;
	Bit16s *outbuf = use16bit ? buf : buffer;

	switch (currType) {
	case TYPE_OPL2:			// unsupported
	case TYPE_DUAL_OPL2:	// unsupported
      break;

	case TYPE_OPL3:
		OPL3_GenerateStream(&opl, tempbuf, samples);

		//output mono: we need to mix the two channels into buf
		if (!stereo) {
			for (int i = 0; i < samples; i++) {
				outbuf[i] = (tempbuf[i * 2] >> 1) + (tempbuf[i * 2 + 1] >> 1);
			}
		}
		break;
	}
	
	//now reduce to 8bit if we need to
	if (!use16bit) {
		for (int i = 0; i < (stereo ? samples * 2 : samples); i++)
			((char *)buf)[i] = (outbuf[i] >> 8) ^ 0x80;
	}
}

void CNukedopl::write(int reg, int val) {
	switch (currType) {
	case TYPE_OPL2:		// unsupported
	case TYPE_DUAL_OPL2:// unsupported
		break;
	case TYPE_OPL3:
		OPL3_WriteRegBuffered(&opl, (Bit16u)reg, (Bit8u)val);
		break;
	}
}

void CNukedopl::init() {
	OPL3_Reset(&opl, rate);
	currChip = 0;
}

void CNukedopl::settype(ChipType type) {
	currType = TYPE_OPL3;
}
