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
// mpu401.h: MPU-401 UART mode driver for DOS (DJGPP).
//           @Author: palxex, 2026
//

#ifndef MPU401_H
#define MPU401_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------- Configuration -------- */
/* Default MPU-401 base I/O address (commonly 0x330, 0x320, or 0x310) */
#define MPU401_DEFAULT_PORT 0x330U

/* -------- Port offsets -------- */
#define MPU401_DATA_PORT_OFFSET   0
#define MPU401_STATUS_PORT_OFFSET 1

/* -------- Status register bits -------- */
#define MPU401_STAT_TX_READY  0x40   /* Data register ready to accept output */
#define MPU401_STAT_RX_READY  0x80   /* Data register has received data */

/* -------- Commands -------- */
#define MPU401_CMD_UART_MODE  0x3F   /* Switch to UART mode */
#define MPU401_CMD_RESET      0xFF   /* Software reset */

/* -------- Return codes -------- */
#define MPU401_OK         0
#define MPU401_ERR_TIMEOUT -1
#define MPU401_ERR_BUSY    -2
#define MPU401_ERR_NO_ACK  -3
#define MPU401_ERR_RETRY   -4

/* ========================================
   ISR-SAFE functions (can be called from vhook)
   ======================================== */

/**
 * Write a single byte to the MPU-401 data port.
 * This function is NON-BLOCKING and ISR-SAFE.
 * It checks TX_READY before writing.
 *
 * @param byte  MIDI byte to send.
 * @return MPU401_OK if written, MPU401_ERR_BUSY if not ready.
 */
int mpu401_write_byte(uint8_t byte);

/**
 * Check if the MPU-401 is ready to accept a new data byte.
 * This function is ISR-SAFE (simple port read).
 *
 * @return 1 if TX_READY, 0 otherwise.
 */
int mpu401_is_tx_ready(void);

/* ========================================
   NON-ISR-SAFE functions (for initialization / control - blocking)
   ======================================== */

/**
 * Initialize MPU-401 hardware:
 *   - Perform software reset
 *   - Switch to UART mode with retries
 *   - Verify acknowledge
 * This function uses blocking waits and MUST NOT be called from ISR.
 *
 * @return MPU401_OK on success, negative error code on failure.
 */
int mpu401_init(void);

/**
 * Reset the MPU-401 (send 0xFF command) and wait for acknowledge.
 * This will turn off all notes and reset internal state.
 * This function is BLOCKING and MUST NOT be called from ISR.
 *
 * @return MPU401_OK on success, negative on failure.
 */
int mpu401_reset(void);

/**
 * Read a byte from the data port (for acknowledge or inbound MIDI).
 * This is a blocking read with a timeout (in milliseconds).
 * This function is BLOCKING and MUST NOT be called from ISR.
 *
 * @param timeout_ms  maximum time to wait in milliseconds (0 = poll once).
 * @param out_byte    pointer to store the read byte.
 * @return MPU401_OK on success, MPU401_ERR_TIMEOUT if no data.
 */
int mpu401_read_byte(uint32_t timeout_ms, uint8_t *out_byte);

#ifdef __cplusplus
}
#endif

#endif /* MPU401_H */