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
// ail32_drv.c: AIL/32 driver loading and timbre management for DOS.
//              @Author: palxex, 2026
//

#include "ail32_drv.h"
#include "common.h"

/* common.h already owns the project's BYTE/WORD/LONG typedefs */
#define TYPEDEFS
#include "dll.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static HDRIVER g_driver = -1;
static drvr_desc *g_desc = NULL;
static void *g_driver_image = NULL;
static void *g_timbre_cache = NULL;
static int g_started = 0;

static void *read_file(const char *path, ULONG *size)
{
    FILE *file = fopen(path, "rb");
    long length;
    void *data;

    if (!file) return NULL;
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    length = ftell(file);
    if (length <= 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    data = malloc((size_t)length);
    if (!data) {
        fclose(file);
        return NULL;
    }
    if (fread(data, 1, (size_t)length, file) != (size_t)length) {
        free(data);
        fclose(file);
        return NULL;
    }
    fclose(file);
    if (size) *size = (ULONG)length;
    return data;
}

int ail32_drv_init(const char *driver_path)
{
    const char *path = driver_path && *driver_path ? driver_path : "a32mt32.dll";
    ULONG image_size;

    if (g_started) {
        return g_driver >= 0 ? 0 : -1;
    }
    g_started = 1;
    AIL_startup();

    g_driver_image = read_file(path, &image_size);
    (void)image_size;
    if (!g_driver_image) {
        goto fail;
    }

    {
        void *loaded = DLL_load(g_driver_image, DLLMEM_ALLOC | DLLSRC_MEM, NULL);
        free(g_driver_image);
        g_driver_image = loaded;
    }
    if (!g_driver_image) {
        goto fail;
    }

    g_driver = AIL_register_driver(g_driver_image);
    if (g_driver < 0) {
        goto fail;
    }

    g_desc = AIL_describe_driver(g_driver);
    if (!g_desc) {
        goto fail;
    }
    if (g_desc->drvr_type != XMIDI_DRVR) {
        goto fail;
    }
    if (!AIL_detect_device(g_driver, (unsigned)g_desc->default_IO,
                           (unsigned)g_desc->default_IRQ,
                           (unsigned)g_desc->default_DMA,
                           (unsigned)g_desc->default_DRQ)) {
        goto fail;
    }

    AIL_init_driver(g_driver, (unsigned)g_desc->default_IO,
                    (unsigned)g_desc->default_IRQ,
                    (unsigned)g_desc->default_DMA,
                    (unsigned)g_desc->default_DRQ);

    {
        unsigned cache_size = AIL_default_timbre_cache_size(g_driver);
        if (cache_size) {
            g_timbre_cache = malloc(cache_size);
            if (!g_timbre_cache) goto fail;
            AIL_define_timbre_cache(g_driver, g_timbre_cache, cache_size);
        }
    }
    return 0;

fail:
    ail32_drv_shutdown();
    return -1;
}

void ail32_drv_shutdown(void)
{
    if (g_driver >= 0) {
        AIL_shutdown_driver(g_driver, NULL);
        AIL_release_driver_handle(g_driver);
    }
    g_driver = -1;
    g_desc = NULL;
    free(g_driver_image);
    free(g_timbre_cache);
    g_driver_image = NULL;
    g_timbre_cache = NULL;
    if (g_started) AIL_shutdown(NULL);
    g_started = 0;
}

HDRIVER ail32_drv_handle(void)
{
    return g_driver;
}

drvr_desc *ail32_drv_description(void)
{
    return g_desc;
}

static void *load_timbre(FILE *file, unsigned bank, unsigned patch)
{
#pragma pack(push, 1)
    struct TimbreHeader {
        signed char patch;
        signed char bank;
        unsigned long offset;
    } header;
#pragma pack(pop)
    unsigned short length;
    void *data;

    fseek(file, 0, SEEK_SET);
    for (;;) {
        if (fread(&header, sizeof(header), 1, file) != 1) return NULL;
        if (header.bank == -1) return NULL;
        if ((unsigned char)header.bank == bank && (unsigned char)header.patch == patch)
            break;
    }
    if (fseek(file, (long)header.offset, SEEK_SET) != 0) return NULL;
    if (fread(&length, sizeof(length), 1, file) != 1 || length < 2) return NULL;
    data = malloc(length);
    if (!data) return NULL;
    memcpy(data, &length, sizeof(length));
    if (fread((unsigned char *)data + 2, 1, length - 2, file) != length - 2) {
        free(data);
        return NULL;
    }
    return data;
}

int ail32_drv_install_sequence_timbres(HSEQUENCE sequence, const char *gtl_path)
{
    FILE *file;
    char default_path[32];
    const char *path = gtl_path;
    unsigned request;

    if (g_driver < 0) return -1;
    if (!path || !*path) {
        if (!g_desc) return -1;
        snprintf(default_path, sizeof(default_path), "fat.%s", g_desc->data_suffix);
        path = default_path;
    }
    file = fopen(path, "rb");
    if (!file) return -1;

        while ((request = AIL_timbre_request(g_driver, sequence)) != 0xFFFFU &&
            request != 0xFFFFFFFFUL) {
        unsigned bank = request >> 8;
        unsigned patch = request & 0xFFU;
        void *timbre = load_timbre(file, bank, patch);
        if (!timbre) {
            fclose(file);
            return -1;
        }
        AIL_install_timbre(g_driver, (int)bank, (int)patch, timbre);
        free(timbre);
    }
    fclose(file);
    return 0;
}
