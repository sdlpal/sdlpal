/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2011-2026, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// ail32_drv.h: AIL/32 driver interface for DOS.
//              @Author: palxex, 2026
//

#ifndef AIL32_DRV_H
#define AIL32_DRV_H

#include "ail32.h"

#ifdef __cplusplus
extern "C" {
#endif

int ail32_drv_init(const char *driver_path);
void ail32_drv_shutdown(void);
HDRIVER ail32_drv_handle(void);
drvr_desc *ail32_drv_description(void);
int ail32_drv_install_sequence_timbres(HSEQUENCE sequence, const char *gtl_path);

#ifdef __cplusplus
}
#endif

#endif
