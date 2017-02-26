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
    CDemuopl(int rate, bool bit16, bool usestereo);
	~CDemuopl();

    void update(short *buf, int samples);
    
    // template methods
    void write(int reg, int val);
    
    void init();
    
protected:
	opl_chip* chip;
	int rate;
	bool use16bit, stereo;
};

#endif
