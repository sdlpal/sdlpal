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
 * nukedopl.h - Adapter of Nuked OPL, by Lou Yihua
 */

#ifndef H_ADPLUG_NUKEDOPL
#define H_ADPLUG_NUKEDOPL


#include "opl.h"
extern "C" {
#include "opl3.h"
}

class CNukedopl: public Copl {
public:
	CNukedopl(int rate, bool bit16, bool usestereo);	// rate = sample rate
   virtual ~CNukedopl();

   void update(short *buf, int samples);			// fill buffer
   void write(int reg, int val);

   void init();
   void settype(ChipType type);

private:
   opl3_chip  opl;				// OPL3 emulator data
   Bit16s    *buffer;
   int        bufsamples, rate;
   bool       use16bit, stereo;
};

#endif
