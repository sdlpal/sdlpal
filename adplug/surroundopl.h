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

#ifndef H_ADPLUG_SURROUNDOPL
#define H_ADPLUG_SURROUNDOPL

#include <stdint.h>
#include "opl.h"

class CSurroundopl : public Copl
{
private:
	typedef void (CSurroundopl::*Updater)(short* buf, int samples);
	typedef void (CSurroundopl::*Writer)(int reg, int val);

	Copl*   opls[2];
	Updater updater;
	Writer  writer;
	short*  buffer;
	double  rate;
	double  offset;            // 
	int     bufsize;
	uint8_t iFMReg[32];        // The original values of OPL registers Ax and Bx
	uint8_t iTweakedFMReg[32]; // The modified values of OPL registers Ax and Bx
	bool    percussion;        // Is the OPL chip worked at percussion mode?

	void write_opl2(int reg, int val);
	void write_dual_opl2(int reg, int val);
	void write_opl3(int reg, int val);

	void update_opl2_stereo_stereo(short *buf, int samples);
	void update_opl2_stereo_mono(short *buf, int samples);
	void update_opl2_mono_stereo(short *buf, int samples);
	void update_opl2_mono_mono(short *buf, int samples);

	void update_opl3(short *buf, int samples) { opls[0]->update(buf, samples); }

public:
	CSurroundopl(double rate, double offset, Copl* opl1, Copl* opl2 = NULL);
	~CSurroundopl() {
		if (opls[0]) delete opls[0];
		if (opls[1]) delete opls[1];
		if (buffer) delete[] buffer;
	}

	bool getstereo() { return true; }
	void update(short *buf, int samples) { (this->*updater)(buf, samples); }
	void write(int reg, int val) { (this->*writer)(reg, val); }

	void init() {
		if (opls[0]) opls[0]->init();
		if (opls[1]) opls[1]->init();
		percussion = false;
	}
};
#endif
