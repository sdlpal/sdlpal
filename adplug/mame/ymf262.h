// license:GPL-2.0+
// copyright-holders:Jarek Burczynski
/*
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
 *
 * ymf262.h - Adaptered for SDLPAL by Lou Yihua.
 *
 */
#ifndef MAME_SOUND_YMF262_H
#define MAME_SOUND_YMF262_H

#pragma once

#include "mame.h"

/* select number of output bits: 8 or 16 */
#define OPL3_SAMPLE_BITS 16

#if (OPL3_SAMPLE_BITS==16)
typedef int16_t OPL3SAMPLE;
#endif
#if (OPL3_SAMPLE_BITS==8)
typedef int8_t OPL3SAMPLE;
#endif

typedef void (*OPL3_TIMERHANDLER)(device_t *device,int timer,const attotime &period);
typedef void (*OPL3_IRQHANDLER)(device_t *device,int irq);
typedef void (*OPL3_UPDATEHANDLER)(device_t *device,int min_interval_us);


void *ymf262_init(device_t *device, int clock, int rate);
void ymf262_post_load(void *chip);
void ymf262_shutdown(void *chip);
void ymf262_reset_chip(void *chip);
int  ymf262_write(void *chip, int a, int v);
unsigned char ymf262_read(void *chip, int a);
int  ymf262_timer_over(void *chip, int c);
void ymf262_update_one(void *chip, OPL3SAMPLE /***buffers*/*buffer, int length);

void ymf262_set_timer_handler(void *chip, OPL3_TIMERHANDLER TimerHandler, device_t *device);
void ymf262_set_irq_handler(void *chip, OPL3_IRQHANDLER IRQHandler, device_t *device);
void ymf262_set_update_handler(void *chip, OPL3_UPDATEHANDLER UpdateHandler, device_t *device);


#endif // MAME_SOUND_YMF262_H
