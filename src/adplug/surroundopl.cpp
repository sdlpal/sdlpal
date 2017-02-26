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
 * surroundopl.cpp - Wrapper class to provide a surround/harmonic effect
 *   for another OPL emulator, by Adam Nielsen <malvineous@shikadi.net>
 *
 * Stereo harmonic algorithm by Adam Nielsen <malvineous@shikadi.net>
 * Please give credit if you use this algorithm elsewhere :-)
 */

#include <math.h> // for pow()
#include "surroundopl.h"
//#include "debug.h"

CSurroundopl::CSurroundopl(Copl *a, Copl *b, bool use16bit, double opl_freq, double freq_offset)
	: use16bit(use16bit), bufsize(4096), a(a), b(b), opl_freq(opl_freq), freq_offset(freq_offset)
{
	currType = TYPE_OPL2;
	this->lbuf = new short[this->bufsize];
	this->rbuf = new short[this->bufsize];
};

CSurroundopl::~CSurroundopl()
{
	delete[] this->rbuf;
	delete[] this->lbuf;
	delete a;
	delete b;
}

void CSurroundopl::update(short *buf, int samples)
{
	if (samples * 2 > this->bufsize) {
		// Need to realloc the buffer
		delete[] this->rbuf;
		delete[] this->lbuf;
		this->bufsize = samples * 2;
		this->lbuf = new short[this->bufsize];
		this->rbuf = new short[this->bufsize];
	}

	a->update(this->lbuf, samples);
	b->update(this->rbuf, samples);

	// Copy the two mono OPL buffers into the stereo buffer
	for (int i = 0; i < samples; i++) {
		if (this->use16bit) {
			buf[i * 2] = this->lbuf[i];
			buf[i * 2 + 1] = this->rbuf[i];
		} else {
			((char *)buf)[i * 2] = ((char *)this->lbuf)[i];
			((char *)buf)[i * 2 + 1] = ((char *)this->rbuf)[i];
		}
	}

}

// template methods
void CSurroundopl::write(int reg, int val)
{
	a->write(reg, val);

	// Transpose the other channel to produce the harmonic effect

	int iChannel = -1;
	int iRegister = reg; // temp
	int iValue = val; // temp
	if ((iRegister >> 4 == 0xA) || (iRegister >> 4 == 0xB)) iChannel = iRegister & 0x0F;

	// Remember the FM state, so that the harmonic effect can access
	// previously assigned register values.
	/*if (((iRegister >> 4 == 0xB) && (iValue & 0x20) && !(this->iFMReg[iRegister] & 0x20)) ||
		(iRegister == 0xBD) && (
			((iValue & 0x01) && !(this->iFMReg[0xBD] & 0x01))
		)) {
		this->iFMReg[iRegister] = iValue;
	}*/
	this->iFMReg[iRegister] = iValue;

	if ((iChannel >= 0)) {// && (i == 1)) {
		unsigned char iBlock = (this->iFMReg[0xB0 + iChannel] >> 2) & 0x07;
		unsigned short iFNum = ((this->iFMReg[0xB0 + iChannel] & 0x03) << 8) | this->iFMReg[0xA0 + iChannel];
		//double dbOriginalFreq = 50000.0 * (double)iFNum * pow(2, iBlock - 20);
		double dbOriginalFreq = opl_freq * (double)iFNum * pow(2.0, iBlock - 20);

		unsigned char iNewBlock = iBlock;
		unsigned short iNewFNum;

		// Adjust the frequency and calculate the new FNum
		//double dbNewFNum = (dbOriginalFreq+(dbOriginalFreq/FREQ_OFFSET)) / (50000.0 * pow(2.0, iNewBlock - 20));
		//#define calcFNum() ((dbOriginalFreq+(dbOriginalFreq/FREQ_OFFSET)) / (50000.0 * pow(2.0, iNewBlock - 20)))
		#define calcFNum() ((dbOriginalFreq + (dbOriginalFreq / freq_offset)) / (opl_freq * pow(2.0, iNewBlock - 20)))
		double dbNewFNum = calcFNum();

		// Make sure it's in range for the OPL chip
		if (dbNewFNum > 1023 - NEWBLOCK_LIMIT) {
			// It's too high, so move up one block (octave) and recalculate

			if (iNewBlock > 6) {
				// Uh oh, we're already at the highest octave!
//				AdPlug_LogWrite("OPL WARN: FNum %d/B#%d would need block 8+ after being transposed (new FNum is %d)\n",
//					iFNum, iBlock, (int)dbNewFNum);
				// The best we can do here is to just play the same note out of the second OPL, so at least it shouldn't
				// sound *too* bad (hopefully it will just miss out on the nice harmonic.)
				iNewBlock = iBlock;
				iNewFNum = iFNum;
			} else {
				iNewBlock++;
				iNewFNum = (unsigned short)calcFNum();
			}
		} else if (dbNewFNum < 0 + NEWBLOCK_LIMIT) {
			// It's too low, so move down one block (octave) and recalculate

			if (iNewBlock == 0) {
				// Uh oh, we're already at the lowest octave!
//				AdPlug_LogWrite("OPL WARN: FNum %d/B#%d would need block -1 after being transposed (new FNum is %d)!\n",
//					iFNum, iBlock, (int)dbNewFNum);
				// The best we can do here is to just play the same note out of the second OPL, so at least it shouldn't
				// sound *too* bad (hopefully it will just miss out on the nice harmonic.)
				iNewBlock = iBlock;
				iNewFNum = iFNum;
			} else {
				iNewBlock--;
				iNewFNum = (unsigned short)calcFNum();
			}
		} else {
			// Original calculation is within range, use that
			iNewFNum = (unsigned short)dbNewFNum;
		}

		// Sanity check
		if (iNewFNum > 1023) {
			// Uh oh, the new FNum is still out of range! (This shouldn't happen)
//			AdPlug_LogWrite("OPL ERR: Original note (FNum %d/B#%d is still out of range after change to FNum %d/B#%d!\n",
//				iFNum, iBlock, iNewFNum, iNewBlock);
			// The best we can do here is to just play the same note out of the second OPL, so at least it shouldn't
			// sound *too* bad (hopefully it will just miss out on the nice harmonic.)
			iNewBlock = iBlock;
			iNewFNum = iFNum;
		}

		if ((iRegister >= 0xB0) && (iRegister <= 0xB8)) {

			// Overwrite the supplied value with the new F-Number and Block.
			iValue = (iValue & ~0x1F) | (iNewBlock << 2) | ((iNewFNum >> 8) & 0x03);

			this->iCurrentTweakedBlock[iChannel] = iNewBlock; // save it so we don't have to update register 0xB0 later on
			this->iCurrentFNum[iChannel] = (unsigned char)iNewFNum;

			if (this->iTweakedFMReg[0xA0 + iChannel] != (iNewFNum & 0xFF)) {
				// Need to write out low bits
				unsigned char iAdditionalReg = 0xA0 + iChannel;
				unsigned char iAdditionalValue = iNewFNum & 0xFF;
				b->write(iAdditionalReg, iAdditionalValue);
				this->iTweakedFMReg[iAdditionalReg] = iAdditionalValue;
			}
		} else if ((iRegister >= 0xA0) && (iRegister <= 0xA8)) {

			// Overwrite the supplied value with the new F-Number.
			iValue = iNewFNum & 0xFF;

			// See if we need to update the block number, which is stored in a different register
			unsigned char iNewB0Value = (this->iFMReg[0xB0 + iChannel] & ~0x1F) | (iNewBlock << 2) | ((iNewFNum >> 8) & 0x03);
			if (
				(iNewB0Value & 0x20) && // but only update if there's a note currently playing (otherwise we can just wait
				(this->iTweakedFMReg[0xB0 + iChannel] != iNewB0Value)   // until the next noteon and update it then)
			) {
//				AdPlug_LogWrite("OPL INFO: CH%d - FNum %d/B#%d -> FNum %d/B#%d == keyon register update!\n",
//					iChannel, iFNum, iBlock, iNewFNum, iNewBlock);
					// The note is already playing, so we need to adjust the upper bits too
					unsigned char iAdditionalReg = 0xB0 + iChannel;
					b->write(iAdditionalReg, iNewB0Value);
					this->iTweakedFMReg[iAdditionalReg] = iNewB0Value;
			} // else the note is not playing, the upper bits will be set when the note is next played

		} // if (register 0xB0 or 0xA0)

	} // if (a register we're interested in)

	// Now write to the original register with a possibly modified value
	b->write(iRegister, iValue);
	this->iTweakedFMReg[iRegister] = iValue;

};

void CSurroundopl::init()
{
	a->init();
	b->init();
}
