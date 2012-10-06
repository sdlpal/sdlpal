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
 * demuopl.h - Emulated OPL using DOSBOX's emulator, by Wei Mingzhi
 *             <whistler_wmz@users.sf.net>.
 */

#ifndef H_ADPLUG_DEMUOPL
#define H_ADPLUG_DEMUOPL

#include "opl.h"
#include "dosbox_opl.h"

#include <assert.h>

class CDemuopl: public Copl
{
public:
  CDemuopl(int rate, bool bit16, bool usestereo)
	  :use16bit(bit16), stereo(usestereo)
    {
      adlib_init(rate);
      currType = TYPE_OPL2;
    };

  void update(short *buf, int samples)
    {
      short *mixbuf0 = new short[samples*2],*mixbuf1 = new short[samples*2];
      short *outbuf;
      if(use16bit) outbuf = buf;
      else outbuf = mixbuf1;
      //if(use16bit) samples *= 2;
      //if(stereo) samples *= 2;
      adlib_getsample(outbuf, samples);
      if(stereo)
	for(int i=samples-1;i>=0;i--) {
	  outbuf[i*2] = outbuf[i];
	  outbuf[i*2+1] = outbuf[i];
	}
      //now reduce to 8bit if we need to
      if(!use16bit)
	for(int i=0;i<(stereo ? samples*2 : samples);i++)
	  ((char *)buf)[i] = (outbuf[i] >> 8) ^ 0x80;
      delete[] mixbuf0; delete[] mixbuf1;
    }

  // template methods
  void write(int reg, int val)
    {
      if(currChip == 0)
	adlib_write(reg, val);
    };

  void init() {};

protected:
  bool use16bit,stereo;
};

#endif
