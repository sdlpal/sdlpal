//
//  demuopl.cpp
//  SDLPal
//
//  Created by palxex on 14-7-20.
//  Copyright (c) 2014年 Wei Mingzhi. All rights reserved.
//

#include "demuopl.h"
#include <math.h>
#include <stdlib.h> // rand()
#include <string.h>

int CDemuopl::channels = 0;

struct OPLHandler {
    virtual void init(int rate) = 0;
    virtual void getsample(short *buf,int samples) = 0;
    virtual void write(int reg,int val) = 0;
};

namespace OPLCore {
#include "dosbox_opl.cpp"
	struct DOSBoxOPLHandler : OPLHandler {
		void init(int rate) {
			adlib_init(rate);
		}
		void getsample(short *buf, int samples) {
			adlib_getsample(buf, samples);
		}
		void write(int reg, int val) {
			adlib_write(reg, val);
		}
	};
}

namespace OPLCore2 {
#include "dosbox_opl.cpp"
	struct DOSBoxOPLHandler : OPLHandler {
		void init(int rate) {
			adlib_init(rate);
		}
		void getsample(short *buf, int samples) {
			adlib_getsample(buf, samples);
		}
		void write(int reg, int val) {
			adlib_write(reg, val);
		}
	};
}

CDemuopl::CDemuopl(int rate, bool bit16, bool usestereo)
	:use16bit(bit16), stereo(usestereo)
{
	if (channels++ == 0)
		pHandler = new OPLCore::DOSBoxOPLHandler;
	else
		pHandler = new OPLCore2::DOSBoxOPLHandler;
	pHandler->init(rate);
	currType = TYPE_OPL2;
}

void CDemuopl::update(short *buf, int samples)
{
	short *mixbuf1 = NULL;
	short *outbuf;
	if (use16bit) outbuf = buf;
	else{
		mixbuf1 = new short[samples * 2];
		outbuf = mixbuf1;
	}
	pHandler->getsample(outbuf, samples);
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
    pHandler->write(reg, val);
};
