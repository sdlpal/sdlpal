/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2020, SDLPAL development team.
// All rights reserved.
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

#ifndef AVIPLAY_H
#define AVIPLAY_H

#include "common.h"

PAL_C_LINKAGE_BEGIN

VOID
PAL_AVIInit(
    VOID
);

VOID
PAL_AVIShutdown(
    VOID
);

BOOL
PAL_PlayAVI(
    const char *lpszPath
);

void SDLCALL
AVI_FillAudioBuffer(
    void       *udata,
    uint8_t    *stream,
    int         len
);

void *
AVI_GetPlayState(
	void
);

PAL_C_LINKAGE_END

#endif
