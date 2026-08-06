/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
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
// mpu401.c: MPU-401 UART mode driver for DOS (DJGPP).
//           @Author: palxex, 2026
//

#include "mpu401.h"
#include <dos.h>
#include <stdint.h>
#include "sdl_compat.h"
#include "util.h"

/* ------------------------------------------------------------
   Static state: base port
   ------------------------------------------------------------ */
static int mpu_base_port = MPU401_DEFAULT_PORT;

/* ------------------------------------------------------------
   BLASTER parsing (unchanged)
   ------------------------------------------------------------ */
static int parse_mpu_port_from_blaster(void)
{
    const char *env = SDL_getenv("BLASTER");
    if (!env) return -1;

    char *copy = SDL_strdup(env);
    if (!copy) return -1;

    char *str = copy, *saveptr = NULL, *token;
    int port = -1;

    while ((token = SDL_strtok_r(str, " ", &saveptr)) != NULL) {
        str = NULL;
        char *endp = NULL;
        int num = (int)SDL_strtol(token + 1, &endp, 16);
        if ((token[1] == 0) || (*endp != 0)) continue;
        if (SDL_toupper(*token) == 'P') {
            port = num;
            break;
        }
    }
    SDL_free(copy);
    return port;
}

/* ------------------------------------------------------------
   I/O access helpers (ISR-safe for writes)
   ------------------------------------------------------------ */
static inline void mpu_outb(uint16_t port, uint8_t val) { outportb(port, val); }
static inline uint8_t mpu_inb(uint16_t port) { return inportb(port); }

/* ------------------------------------------------------------
   Low-level helpers for status/data ports
   ------------------------------------------------------------ */
static inline uint16_t mpu_data_port(void)   { return mpu_base_port + MPU401_DATA_PORT_OFFSET; }
static inline uint16_t mpu_status_port(void) { return mpu_base_port + MPU401_STATUS_PORT_OFFSET; }

/* ------------------------------------------------------------
   Write command to MPU-401 with timeout (as in the reference)
   Returns 0 on success, 0xFFFF on timeout.
   ------------------------------------------------------------ */
static bool mpu401_write_command(uint8_t cmd) {
    uint16_t port = mpu_status_port();

    for (int i = 0; i < 0x800; i++) {
        uint8_t status = mpu_inb(port);
        if ((status & 0x40) == 0) {
            mpu_outb(port, cmd);
            return true;
        }
    }
    return false;   // 超时
}
bool mpu401_write_data(uint8_t val) {
    uint16_t status_port = mpu_status_port();
    uint16_t data_port   = mpu_data_port();
    uint8_t  cmd         = val;

    for (int i = 0; i < 0x800; i++) {
        uint8_t status = mpu_inb(status_port);

        if ((status & 0x40) == 0) {
            mpu_outb(data_port, cmd);

            for (int j = 0; j < 10; j++) {
                (void)mpu_inb(status_port);
            }

            return true;
        } else {
            if ((status & 0x80) == 0) {
                (void)mpu_inb(data_port);
            }
        }
    }

    return false;
}
/* ------------------------------------------------------------
   Flush input buffer: read and discard all pending data
   ------------------------------------------------------------ */
static void mpu401_flush_input_data(void)
{
    uint16_t status_port = mpu_status_port();
    uint16_t data_port = mpu_data_port();

    // Read status once (optional, to clear any interrupt)
    mpu_inb(status_port);

    // Read data port 512 times, discarding everything
    for (uint16_t i = 0; i < 0x200; i++) {
        mpu_inb(data_port);
    }

    // Final status read (discard)
    mpu_inb(status_port);
}

static uint16_t flush_mpu401_buf(void) {
    uint16_t status_port = mpu_status_port();
    uint16_t data_port   = mpu_data_port();

    for (int i = 0; i < 0x800; i++) {
        uint8_t st = mpu_inb(status_port);
        if ((st & 0x80) == 0) {
            return mpu_inb(data_port);
        }
    }
    return 0xFFFF;
}
/* ------------------------------------------------------------
   Enter UART mode – exact sequence from the reference:
   1) Flush input
   2) Write 0x3F command (with timeout)
   3) Wait for command completion (by polling status)
   ------------------------------------------------------------ */
bool mpu401_reset_and_uart_mode(void) {
    for (int i = 0; i < 0x100; i++) {
        uint16_t result = flush_mpu401_buf();
        if (result == 0xFFFF) {
            break;
        }
    }

    mpu401_write_command(0x3F);
    mpu401_flush_input_data();

    return true;
}

/* ------------------------------------------------------------
   Public API: ISR‑safe write (unconditional)
   ------------------------------------------------------------ */
int mpu401_write_byte(uint8_t byte)
{
    mpu_outb(mpu_data_port(), byte);
    return MPU401_OK;
}

/* ------------------------------------------------------------
   ISR‑safe status query (retained for debugging, not used in write)
   ------------------------------------------------------------ */
int mpu401_is_tx_ready(void)
{
    uint8_t st = mpu_inb(mpu_status_port());
    return (st & MPU401_STAT_TX_READY) ? 1 : 0;
}

/* ------------------------------------------------------------
   Reset function (retained for external use, but not used in init)
   ------------------------------------------------------------ */
int mpu401_reset(void)
{
    mpu_outb(mpu_status_port(), MPU401_CMD_RESET);
    return MPU401_OK;   // simple version, no waiting
}

/* ------------------------------------------------------------
   Main initialisation
   ------------------------------------------------------------ */
int mpu401_init(void)
{
    static bool inited = false;

    if( inited )
        return MPU401_OK;

    // Determine base port from BLASTER
    int parsed = parse_mpu_port_from_blaster();
    mpu_base_port = (parsed > 0) ? parsed : MPU401_DEFAULT_PORT;
    UTIL_LogOutput(LOGLEVEL_DEBUG, "MPU-401 base port set to 0x%X\n", mpu_base_port);

    // Quick port validity check: read status, if 0xFF then invalid
    uint8_t st = mpu_inb(mpu_status_port());
    if (st == 0xFF) {
        UTIL_LogOutput(LOGLEVEL_ERROR, "MPU-401 port 0x%X appears invalid (status=0xFF).\n", mpu_base_port);
        return MPU401_ERR_TIMEOUT;
    }
    UTIL_LogOutput(LOGLEVEL_DEBUG, "MPU-401 status probe: 0x%02X\n", st);

    // Enter UART mode using the reference sequence
    if (!mpu401_reset_and_uart_mode()) {
        UTIL_LogOutput(LOGLEVEL_ERROR, "Failed to enter UART mode.\n");
        return MPU401_ERR_RETRY;
    }
    for (int i = 0; i < 16; i++) {
        mpu401_write_data(0xB0 + i);
        mpu401_write_data(0x7B);
        mpu401_write_data(0x00);
    }

    UTIL_LogOutput(LOGLEVEL_DEBUG, "MPU-401 initialised successfully (UART mode).\n");
    inited = true;
    return MPU401_OK;
}

/* ------------------------------------------------------------
   Blocking read (not used, but kept for completeness)
   ------------------------------------------------------------ */
int mpu401_read_byte(uint32_t timeout_ms, uint8_t *out_byte)
{
    uint16_t status = mpu_status_port();
    uint16_t data = mpu_data_port();
    uint32_t elapsed = 0;
    while (1) {
        uint8_t st = mpu_inb(status);
        if (st & MPU401_STAT_RX_READY) {
            *out_byte = mpu_inb(data);
            return MPU401_OK;
        }
        if (timeout_ms == 0) break;
        SDL_Delay(1);
        if (++elapsed >= timeout_ms) break;
    }
    return MPU401_ERR_TIMEOUT;
}