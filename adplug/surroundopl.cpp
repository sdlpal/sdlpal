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
 * -------------------------------------------------------------------------
 * SDLPAL
 * Copyright (c) 2011-2020, SDLPAL development team.
 * All rights reserved.
 *
 * This file is part of SDLPAL.
 *
 * SDLPAL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * -------------------------------------------------------------------------
 *
 * Adaptered for generic OPL2/OPL3 version by Lou Yihua <supermouselyh@hotmail.com>.
 *
 */

#include <math.h> // for pow()
#include <stdlib.h>
#include "surroundopl.h"

// Number of FNums away from the upper/lower limit before switching to the next
// block (octave.)  By rights it should be zero, but for some reason this seems
// to cut it to close and the transposed OPL doesn't hit the right note all the
// time.  Setting it higher means it will switch blocks sooner and that seems
// to help.  Don't set it too high or it'll get stuck in an infinite loop if
// one block is too high and the adjacent block is too low ;-)
#define NEWBLOCK_LIMIT  32

#define calcFNum() ((dbOriginalFreq + (dbOriginalFreq / freq_offset)) / (opl_freq * pow(2.0, iNewBlock - 20)))

static bool TweakFMReg(double freq_offset, double opl_freq, int reg, int val, uint8_t iFMReg[32], uint8_t iTweakedFMReg[32])
{
	int iRegister = reg & 0x1F;
	int iChannel = reg & 0xF;

	// Remember the FM state, so that the harmonic effect can access
	// previously assigned register values.
	iFMReg[iRegister] = val;

	uint8_t iBlock = (iFMReg[iChannel | 0x10] >> 2) & 0x07, iNewBlock = iBlock;
	uint16_t iFNum = ((iFMReg[iChannel | 0x10] & 0x03) << 8) | iFMReg[iChannel], iNewFNum;
	double dbOriginalFreq = opl_freq * (double)iFNum * pow(2.0, iBlock - 20);
	double dbNewFNum = calcFNum();	// Adjust the frequency and calculate the new FNum

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
		}
		else {
			iNewBlock++;
			iNewFNum = (uint16_t)calcFNum();
		}
	}
	else if (dbNewFNum < 0 + NEWBLOCK_LIMIT) {
		// It's too low, so move down one block (octave) and recalculate

		if (iNewBlock == 0) {
			// Uh oh, we're already at the lowest octave!
			//				AdPlug_LogWrite("OPL WARN: FNum %d/B#%d would need block -1 after being transposed (new FNum is %d)!\n",
			//					iFNum, iBlock, (int)dbNewFNum);
			// The best we can do here is to just play the same note out of the second OPL, so at least it shouldn't
			// sound *too* bad (hopefully it will just miss out on the nice harmonic.)
			iNewBlock = iBlock;
			iNewFNum = iFNum;
		}
		else {
			iNewBlock--;
			iNewFNum = (uint16_t)calcFNum();
		}
	}
	else {
		// Original calculation is within range, use that
		iNewFNum = (uint16_t)dbNewFNum;
	}

	// Sanity check
	if (iNewFNum > 1023) {
		// Uh oh, the new FNum is still out of range! (This shouldn't happen)
		//AdPlug_LogWrite("OPL ERR: Original note (FNum %d/B#%d is still out of range after change to FNum %d/B#%d!\n",
		//	iFNum, iBlock, iNewFNum, iNewBlock);
		// The best we can do here is to just play the same note out of the second OPL, so at least it shouldn't
		// sound *too* bad (hopefully it will just miss out on the nice harmonic.)
		iNewBlock = iBlock;
		iNewFNum = iFNum;
	}

	if (reg >= 0xB0 && reg <= 0xB8) {
		// Overwrite the supplied value with the new F-Number and Block.
		iTweakedFMReg[iRegister] = (val & ~0x1F) | (iNewBlock << 2) | ((iNewFNum >> 8) & 0x03);

		if (iTweakedFMReg[iChannel] != (iNewFNum & 0xFF)) {
			// Need to write out low bits
			iTweakedFMReg[iChannel] = iNewFNum & 0xFF;
			return true;
		}
	}
	else if (reg >= 0xA0 && reg <= 0xA8) {
		// Overwrite the supplied value with the new F-Number.
		iTweakedFMReg[iRegister] = iNewFNum & 0xFF;

		// See if we need to update the block number, which is stored in a different register
		unsigned char iNewB0Value = (iFMReg[iChannel | 0x10] & ~0x1F) | (iNewBlock << 2) | ((iNewFNum >> 8) & 0x03);
		if ((iNewB0Value & 0x20) && // but only update if there's a note currently playing (otherwise we can just wait
			(iTweakedFMReg[iChannel | 0x10] != iNewB0Value)   // until the next noteon and update it then)
			) {
			//AdPlug_LogWrite("OPL INFO: CH%d - FNum %d/B#%d -> FNum %d/B#%d == keyon register update!\n",
			//	iChannel, iFNum, iBlock, iNewFNum, iNewBlock);
			// The note is already playing, so we need to adjust the upper bits too
			iTweakedFMReg[iChannel | 0x10] = iNewB0Value;
			return true;
		} // else the note is not playing, the upper bits will be set when the note is next played
	} // if (register 0xB0 or 0xA0)

	return false;
}

CSurroundopl::CSurroundopl(double rate, double offset, Copl* opl1, Copl* opl2)
	: Copl(opl1->gettype() == TYPE_OPL3 ? TYPE_OPL3 : TYPE_DUAL_OPL2)
	, buffer(NULL), rate(rate), offset(offset), bufsize(0), percussion(false)
{
	opls[0] = opl1;
	opls[1] = opl2;
	init();

	if (opl1->gettype() == TYPE_OPL3 || opl1->gettype() == TYPE_DUAL_OPL2)
	{
		updater = &CSurroundopl::update_opl3;
		if (opl1->gettype() == TYPE_OPL3)
		{
			writer = &CSurroundopl::write_opl3;
			opl1->write(OPL3_MODE_REGISTER, 1);
		}
		else
		{
			writer = &CSurroundopl::write_dual_opl2;
		}
	}
	else
	{
		opl1->setchip(0);
		opl2->setchip(0);
		if (opl1->getstereo() && opl2->getstereo())
			updater = &CSurroundopl::update_opl2_stereo_stereo;
		else if (opl1->getstereo() && !opl2->getstereo())
			updater = &CSurroundopl::update_opl2_stereo_mono;
		else if (!opl1->getstereo() && opl2->getstereo())
			updater = &CSurroundopl::update_opl2_mono_stereo;
		else
			updater = &CSurroundopl::update_opl2_mono_mono;
		writer = &CSurroundopl::write_opl2;

		// Disable opl2's OPL3 mode
		if (opl2->gettype() == TYPE_OPL3)
		{
			opl2->write(OPL3_MODE_REGISTER, 0);
		}
	}
}

void CSurroundopl::write_opl2(int reg, int val)
{
	// Transpose the other channel to produce the harmonic effect

	// Write the original OPL data to chip 0
	opls[0]->write(reg, val);

	if (reg == 0xBD)
	{
		// Bit 5 in register 0xBD controls whether percussion mode is enabled
		percussion = ((val & 0x20) != 0);
		opls[1]->write(reg, val);
	}
	else if ((reg >> 4 == 0xA || reg >> 4 == 0xB) && ((reg & 0xF) <= 5 || !percussion))
	{
		// This is a register we're interested in
		if (TweakFMReg(offset, rate, reg, val, iFMReg, iTweakedFMReg))
		{
			// We need to adjust the upper/lower bits
			// If current reg is within [0xA0, 0xA8], we should write to [0xB0, 0xB8] on chip 1
			// If current reg is within [0xB0, 0xB8], we should write to [0xA0, 0xA8] on chip 1
			int iRegister = reg + ((reg >> 4 == 0xA) ? 0x10 : -0x10);
			opls[1]->write(iRegister, iTweakedFMReg[iRegister & 0x1F]);
		}

		// Now write to the corresponding register with a possibly modified value
		opls[1]->write(reg, iTweakedFMReg[reg & 0x1F]);
	}
	else
	{
		// Write the same data to the same register on chip 1
		opls[1]->write(reg, val);
	}
}

void CSurroundopl::write_dual_opl2(int reg, int val)
{
	// Transpose the other channel to produce the harmonic effect

	// Write the original OPL data to chip 0
	opls[0]->setchip(0);
	opls[0]->write(reg, val);

	opls[0]->setchip(1);
	if (reg == 0xBD)
	{
		// Bit 5 in register 0xBD controls whether percussion mode is enabled
		percussion = ((val & 0x20) != 0);
		opls[0]->write(reg, val);
	}
	else if ((reg >> 4 == 0xA || reg >> 4 == 0xB) && ((reg & 0xF) <= 5 || !percussion))
	{
		// This is a register we're interested in
		if (TweakFMReg(offset, rate, reg, val, iFMReg, iTweakedFMReg))
		{
			// We need to adjust the upper/lower bits
			// If current reg is within [0xA0, 0xA8], we should write to [0xB0, 0xB8] on chip 1
			// If current reg is within [0xB0, 0xB8], we should write to [0xA0, 0xA8] on chip 1
			int iRegister = reg + ((reg >> 4 == 0xA) ? 0x10 : -0x10);
			opls[0]->write(iRegister, iTweakedFMReg[iRegister & 0x1F]);
		}

		// Now write to the corresponding register with a possibly modified value
		opls[0]->write(reg, iTweakedFMReg[reg & 0x1F]);
	}
	else
	{
		// Write the same data to the same register on chip 1
		opls[0]->write(reg, val);
	}
}

void CSurroundopl::write_opl3(int reg, int val)
{
	// Transpose the other channel to produce the harmonic effect
	int group = reg >> 4;

	if (group == 0xC)
	{
		// When OPL3 is enabled, bit 4 & 5 in OPL registers [0xC0, 0xC8] and
		// [0x1C0, 0x1C8] controls wheter the data written to a FM channel
		// generates waveforms to the left channel or the right channel on output

		// Since a OPL3 chip has 18 channels, the higher 9 channels can act as
		// another OPL2 chip on generateing the harmonic effect. However, the
		// the percussion channels have no counterparts on higher channels,
		// so they should be treated separately.
		if ((reg & 0xF) <= 5 || !percussion)
		{
			opls[0]->write(reg, val | 0x10);                    // Lower channels goes to left
			opls[0]->write(reg | OPL3_EXTREG_BASE, val | 0x20); // Higher channels goes to right
		}
		else
		{
			opls[0]->write(reg, val | 0x30);                    // Percussion channels goes to both
		}
	}
	else
	{
		opls[0]->write(reg, val);                               // Write the original OPL data
		if (reg != (OPL3_4OP_REGISTER  & 0xFF) &&
			reg != (OPL3_MODE_REGISTER & 0xFF) &&               // 0x104 & 0x105 has special meanings in OPL3
			group != 0xA && group != 0xB &&                     // Group 0xA & 0xB are the key to harmonic effect 
			(!percussion || (group & 0x1) == 0x0))              // For groups 0x2-0x9 and 0xE-0xF, percussion channels
		{                                                       // resides in odd-group registers.
			opls[0]->write(reg | OPL3_EXTREG_BASE, val);        // Write the same data into counterpart registers
		}
	}

	if (reg == 0xBD)
	{
		// Bit 5 in register 0xBD controls whether percussion mode is enabled
		percussion = ((val & 0x20) != 0);
	}
	else if ((group == 0xA || group == 0xB) && ((reg & 0xF) <= 5 || !percussion))
	{
		// This is a register we're interested in
		if (TweakFMReg(offset, rate, reg, val, iFMReg, iTweakedFMReg))
		{
			// We need to adjust the upper/lower bits
			// If current reg is within [0xA0, 0xA8], we should write to [0x1B0, 0x1B8]
			// If current reg is within [0xB0, 0xB8], we should write to [0x1A0, 0x1A8]
			int iRegister = (reg | OPL3_EXTREG_BASE) + ((reg >> 4 == 0xA) ? 0x10 : -0x10);
			opls[0]->write(iRegister, iTweakedFMReg[iRegister & 0x1F]);
		}

		// Now write to the counterpart with a possibly modified value
		opls[0]->write(reg | OPL3_EXTREG_BASE, iTweakedFMReg[reg & 0x1F]);
	}
}

void CSurroundopl::update_opl2_stereo_stereo(short *buf, int samples)
{
	if (bufsize < samples)
	{
		if (buffer) delete[] buffer;
		buffer = new short[(bufsize = samples) * 2];
	}

	opls[0]->update(buf, samples);
	opls[1]->update(buffer, samples);

	// Output buffer combines from opl1 & opl2's left channel
	for (int i = 0; i < samples * 2; i += 2)
	{
		buf[i + 1] = buffer[i];
	}
}

void CSurroundopl::update_opl2_stereo_mono(short *buf, int samples)
{
	if (bufsize < samples)
	{
		if (buffer) delete[] buffer;
		buffer = new short[(bufsize = samples)];
	}

	opls[0]->update(buf, samples);
	opls[1]->update(buffer, samples);

	// Output buffer combines from opl1's left channel & opl2
	for (int i = 1, j = 0; i < samples * 2; i += 2, j++)
	{
		buf[i] = buffer[j];
	}
}

void CSurroundopl::update_opl2_mono_stereo(short *buf, int samples)
{
	if (bufsize < samples)
	{
		if (buffer) delete[] buffer;
		buffer = new short[(bufsize = samples)];
	}

	opls[0]->update(buffer, samples);
	opls[1]->update(buf, samples);

	// Output buffer combines from opl1 & opl2's left channel
	for (int i = 0, j = 0; i < samples * 2; i += 2, j++)
	{
		buf[i + 1] = buf[i];
		buf[i] = buffer[j];
	}
}

void CSurroundopl::update_opl2_mono_mono(short *buf, int samples)
{
	if (bufsize < samples)
	{
		if (buffer) delete[] buffer;
		buffer = new short[(bufsize = samples)];
	}

	opls[0]->update(buffer, samples);
	opls[1]->update(buf + samples, samples);

	// Output buffer combines from opl1 & opl2
	for (int i = 0, j = 0; i < samples * 2; i += 2, j++)
	{
		buf[i] = buffer[j];
		buf[i + 1] = buf[j + samples];
	}
}
