/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2024, SDLPAL development team.
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

#include <SDL_libretro.h>
#include "../../3rd/SDL/src/audio/SDL_audiomem.h"
#include "../../3rd/SDL/src/audio/SDL_audio_c.h"
#include "../../3rd/SDL/src/audio/SDL_audiodev_c.h"
#include "../../3rd/SDL/src/audio/dummy/SDL_dummyaudio.h"

/* The tag name used by DUMMY audio */
#define DUMMYAUD_DRIVER_NAME         "dummy"

/* Audio driver functions */
static int DUMMYAUD_OpenAudio(_THIS, SDL_AudioSpec *spec);
static void DUMMYAUD_WaitAudio(_THIS);
static void DUMMYAUD_PlayAudio(_THIS);
static Uint8 *DUMMYAUD_GetAudioBuf(_THIS);
static void DUMMYAUD_CloseAudio(_THIS);

static SDL_AudioSpec *_spec = NULL;

/* Audio driver bootstrap functions */
static int DUMMYAUD_Available(void)
{
    return 1;
}

static void DUMMYAUD_DeleteDevice(SDL_AudioDevice *device)
{
    SDL_free(device->hidden);
    SDL_free(device);
}

static SDL_AudioDevice *DUMMYAUD_CreateDevice(int devindex)
{
    SDL_AudioDevice *this;

    /* Initialize all variables that we clean on shutdown */
    this = (SDL_AudioDevice *)SDL_malloc(sizeof(SDL_AudioDevice));
    if (this) {
        SDL_memset(this, 0, (sizeof *this));
        this->hidden = (struct SDL_PrivateAudioData *)
            SDL_malloc((sizeof *this->hidden));
    }
    if (this == NULL || this->hidden == NULL) {
        SDL_OutOfMemory();
        if (this) {
            SDL_free(this);
        }
        return 0;
    }
    SDL_memset(this->hidden, 0, (sizeof *this->hidden));

    /* Set the function pointers */
    this->OpenAudio = DUMMYAUD_OpenAudio;
    this->WaitAudio = DUMMYAUD_WaitAudio;
    this->PlayAudio = DUMMYAUD_PlayAudio;
    this->GetAudioBuf = DUMMYAUD_GetAudioBuf;
    this->CloseAudio = DUMMYAUD_CloseAudio;

    this->free = DUMMYAUD_DeleteDevice;

    return this;
}

AudioBootStrap DUMMYAUD_bootstrap = {
    DUMMYAUD_DRIVER_NAME, "SDL dummy audio driver",
    DUMMYAUD_Available, DUMMYAUD_CreateDevice
};


static void DUMMYAUD_WaitAudio(_THIS)
{
}

static void DUMMYAUD_PlayAudio(_THIS)
{
}

static Uint8 *DUMMYAUD_GetAudioBuf(_THIS)
{
    return NULL;
}

static void DUMMYAUD_CloseAudio(_THIS)
{
}


static int DUMMYAUD_OpenAudio(_THIS, SDL_AudioSpec *spec)
{
    _spec = spec;

    /* Don't spawn thread for SDL_RunAudio */
    return 1;
}

void SDL_libretro_ProduceAudio(retro_audio_sample_batch_t audio_batch_cb)
{
    static int16_t stream[44100 / 60 * 2];
    size_t size = sizeof(stream);
    size_t samples = size / 4;

    if (SDL_GetAudioStatus() != SDL_AUDIO_PLAYING)
        return;

    SDL_LockAudio();
    SDL_memset(stream, 0, size);
    _spec->callback(_spec->userdata, (uint8_t *)stream, size);
    audio_batch_cb(stream, samples);
    SDL_UnlockAudio();
}
