/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2010 Simon Peter, <dn.tlp@gmx.net>, et al.
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
 * surroundopl.h - Wrapper class to provide a surround/harmonic effect
 *   for another OPL emulator, by Adam Nielsen <malvineous@shikadi.net>
 *
 * Stereo harmonic algorithm by Adam Nielsen <malvineous@shikadi.net>
 * Please give credit if you use this algorithm elsewhere :-)
 */

#ifndef H_ADPLUG_SURROUNDOPL
#define H_ADPLUG_SURROUNDOPL

//#include <stdint.h> // for uintxx_t
#include "opl.h"

// The right-channel is increased in frequency by itself divided by this amount.
// The right value should not noticeably change the pitch, but it should provide
// a nice stereo harmonic effect.
// This value should be well tuned to get best sound quality.
// Currently, 384.0 is the best choice. (**sdlpal_tune, not used now**)
#define FREQ_OFFSET 384.0//128.0//96.0

// Number of FNums away from the upper/lower limit before switching to the next
// block (octave.)  By rights it should be zero, but for some reason this seems
// to cut it to close and the transposed OPL doesn't hit the right note all the
// time.  Setting it higher means it will switch blocks sooner and that seems
// to help.  Don't set it too high or it'll get stuck in an infinite loop if
// one block is too high and the adjacent block is too low ;-)
#define NEWBLOCK_LIMIT  32

class CSurroundopl: public Copl
{
	private:
		Copl *a, *b;
		short *lbuf, *rbuf;
		double freq_offset, opl_freq;
		unsigned char iFMReg[256];
		unsigned char iTweakedFMReg[256];
		unsigned char iCurrentTweakedBlock[9]; // Current value of the Block in the tweaked OPL chip
		unsigned char iCurrentFNum[9];         // Current value of the FNum in the tweaked OPL chip
		short bufsize;
		bool use16bit;

	public:

		CSurroundopl(Copl *a, Copl *b, bool use16bit, double opl_freq, double freq_offset);
		~CSurroundopl();

		void update(short *buf, int samples);
		void write(int reg, int val);

		void init();

};

#endif
