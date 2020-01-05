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
 * convertopl.h - Adapter of different OPL output formats, by Lou Yihua
 *
 */

#ifndef H_ADPLUG_CONVERTOPL
#define H_ADPLUG_CONVERTOPL

#include "opl.h"

class CConvertopl: public Copl {
protected:
	typedef void (CConvertopl::*Updater)(short* buf, int samples);

	Copl*   opl;
	Updater updater;
	short*  buffer;
	int     bufsamples;

	// Resize the internal buffer is necessary before update data into it
	void update_to_internal_buffer(int samples) {
		if (bufsamples < samples) {
			if (buffer) delete[] buffer;
			buffer = new short[(bufsamples = samples) * 2];
		}
		opl->update(buffer, samples);
	}

	// Same specification
	void update_direct(short* buf, int samples) {
		opl->update(buf, samples);
	}

	// 16bit, stereo -> 16bit, mono
	void update_16s_16m(short* buf, int samples) {
		update_to_internal_buffer(samples);
		for (int i = 0, j = 0; i < samples; i++, j += 2) {
			buf[i] = (buffer[j] >> 1) + (buffer[j + 1] >> 1);
		}
	}

	// 16bit, stereo -> 8bit, stereo
	void update_16s_8s(short* buf, int samples) {
		update_to_internal_buffer(samples);
		for (int i = 0; i < samples * 2; i++) {
			reinterpret_cast<char*>(buf)[i] = (buffer[i] >> 8) ^ 0x80;
		}
	}

	// 16bit, stereo -> 8bit, mono
	void update_16s_8m(short* buf, int samples) {
		update_to_internal_buffer(samples);
		for (int i = 0, j = 0; i < samples; i++, j += 2) {
			reinterpret_cast<char*>(buf)[i] = (((buffer[j] >> 1) + (buffer[j + 1] >> 1)) >> 8) ^ 0x80;
		}
	}

	// 16bit, mono -> 16bit, stereo
	void update_16m_16s(short* buf, int samples) {
		opl->update(buf, samples);
		for (int i = samples - 1, j = i * 2; i >= 0; i--, j -= 2) {
			buf[j] = buf[j + 1] = buf[i];
		}
	}

	// 16bit, mono -> 8bit, stereo
	void update_16m_8s(short* buf, int samples) {
		opl->update(buf, samples);
		for (int i = 0; i < samples; i++) {
			buf[i] = (((buf[i] >> 8) & 0x00FF) | (buf[i] & 0xFF00)) ^ 0x8080;
		}
	}

	// 16bit, mono -> 8bit, mono
	void update_16m_8m(short* buf, int samples) {
		update_to_internal_buffer(samples);
		for (int i = 0; i < samples; i++) {
			reinterpret_cast<char*>(buf)[i] = (buffer[i] >> 8) ^ 0x80;
		}
	}

public:
	CConvertopl(Copl* opl, bool use16bit, bool stereo)
		: Copl(opl->gettype()), opl(opl)
		, buffer(NULL), bufsamples(0) {
		if (opl->getstereo()) {
			if (stereo && !use16bit) {
				updater = &CConvertopl::update_16s_8s;
			}
			else if (!stereo && use16bit) {
				updater = &CConvertopl::update_16s_16m;
			}
			else if (!stereo && !use16bit) {
				updater = &CConvertopl::update_16s_8m;
			}
			else {
				updater = &CConvertopl::update_direct;
			}
		}
		else {
			if (stereo && use16bit) {
				updater = &CConvertopl::update_16m_16s;
			}
			else if (stereo && !use16bit) {
				updater = &CConvertopl::update_16m_8s;
			}
			else if (!stereo && !use16bit) {
				updater = &CConvertopl::update_16m_8m;
			}
			else {
				updater = &CConvertopl::update_direct;
			}
		}
	}

	virtual ~CConvertopl() { delete[] buffer; delete opl; }

	virtual void init() { opl->init(); }

	virtual void write(int reg, int val) { opl->write(reg, val); }

	virtual void setchip(int n) { Copl::setchip(n); opl->setchip(n); }

	virtual void update(short *buf, int samples) { (this->*updater)(buf, samples); }

	virtual bool getstereo() { return opl->getstereo(); }
};

#endif
