/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
// All rights reserved.
// Created by Lou Yihua <louyihua@21cn.com>, 2015-08-03.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef H_ADPLUG_DBEMUOPL
#define H_ADPLUG_DBEMUOPL

#include "opl.h"
#include "dbopl.h"

class CDBemuopl : public Copl
{
public:
	CDBemuopl(int rate, bool bit16, bool usestereo);
	~CDBemuopl();

	void update(short *buf, int samples);

	// template methods
	void write(int reg, int val);

	void init();

protected:
	DBOPL::Chip chip;
	int32_t* buffer;
	int rate, maxlen;
	bool use16bit, stereo;

	static bool _inited;

	void update_opl3(short *buf, int samples);
	void update_opl2(short *buf, int samples);
};

#endif
