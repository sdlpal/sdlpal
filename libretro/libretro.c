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

#include <unistd.h>
#include <libgen.h>
#include <SDL_libretro.h>


static void                        fallback_log(enum retro_log_level level, const char *fmt, ...);
static retro_log_printf_t          log_cb = fallback_log;
static retro_video_refresh_t       video_cb;
static retro_input_poll_t          input_poll_cb;
static retro_input_state_t         input_state_cb;
static retro_audio_sample_batch_t  audio_batch_cb;
static retro_environment_t         environ_cb;
static SDL_Thread                 *sdlpal_thread;


static void fallback_log(enum retro_log_level level, const char *fmt, ...)
{
    (void)level;
    va_list va;
    va_start(va, fmt);
    vfprintf(stderr, fmt, va);
    va_end(va);
}

unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

void retro_set_environment(retro_environment_t cb)
{
    struct retro_log_callback log;
    bool no_game = true;

    environ_cb = cb;
    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
        log_cb = log.log;

    environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_game);
}

void retro_set_video_refresh(retro_video_refresh_t cb)
{
    video_cb = cb;
}

void retro_set_audio_sample(retro_audio_sample_t cb)
{
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb)
{
    audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb)
{
    input_state_cb = cb;
}

void retro_get_system_info(struct retro_system_info *info)
{
    info->need_fullpath    = true;
    info->valid_extensions = "cfg";
    info->library_version  = "2.0.1";
    info->library_name     = "sdlpal";
    info->block_extract    = false;
}

void retro_get_system_av_info(struct retro_system_av_info *info)
{
    int width                   = 320;
    int height                  = 200;
    info->geometry.base_width   = width;
    info->geometry.base_height  = height;
    info->geometry.max_width    = width;
    info->geometry.max_height   = height;
    info->geometry.aspect_ratio = 0.0;
    info->timing.fps            = 60.0;
    info->timing.sample_rate    = 44100.0;
}

void retro_init(void)
{
    enum retro_pixel_format pixfmt = RETRO_PIXEL_FORMAT_RGB565;
    environ_cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &pixfmt);
}

int main(int argc, char *argv[]);
static int sdlpal_main(void *data)
{
    char *argv[] = {""};
    main(1, argv);
    environ_cb(RETRO_ENVIRONMENT_SHUTDOWN, NULL);
    return 0;
}

bool retro_load_game(const struct retro_game_info *game)
{
    if (game && game->path) {
        const char *gamedir = NULL;
        gamedir = dirname(strdup(game->path));
        chdir(gamedir);
        log_cb(RETRO_LOG_INFO, "Load game from %s\n", gamedir);
    } else {
        char *systemdir = NULL;
        environ_cb(RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY, &systemdir);
        chdir(systemdir);
        chdir("sdlpal");
        log_cb(RETRO_LOG_INFO, "Load game from %s/sdlpal\n", systemdir);
    }

    sdlpal_thread = SDL_CreateThread(sdlpal_main, NULL);

    return true;
}

void retro_unload_game(void)
{
    SDL_Event event;
    event.type = SDL_QUIT;
    SDL_PushEvent(&event);
    SDL_WaitThread(sdlpal_thread, NULL);
}

void retro_set_controller_port_device(unsigned port, unsigned device)
{
}

void retro_deinit(void)
{
}

void retro_reset(void)
{
}

static void pump_keyboard_events(void)
{
    static int16_t keys[RETROK_LAST] = {0};
    static SDL_keysym sym;
    for (int i = 0; i < RETROK_LAST; ++i) {
        int16_t state = input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, i);
        if (keys[i] != state) {
            keys[i] = state;
            sym.scancode = i;
            sym.sym = i;
            SDL_PrivateKeyboard(state ? SDL_PRESSED : SDL_RELEASED, &sym);
        }
    }

    static int16_t buttons[16] = {0};
    static const int bkeys[16] = {
        [RETRO_DEVICE_ID_JOYPAD_B]      = SDLK_ESCAPE, /* Menu */
        [RETRO_DEVICE_ID_JOYPAD_Y]      = SDLK_q,      /* Flee */
        [RETRO_DEVICE_ID_JOYPAD_SELECT] = SDLK_ESCAPE, /* Menu */
        [RETRO_DEVICE_ID_JOYPAD_START]  = SDLK_RETURN, /* Search */
        [RETRO_DEVICE_ID_JOYPAD_UP]     = SDLK_UP,
        [RETRO_DEVICE_ID_JOYPAD_DOWN]   = SDLK_DOWN,
        [RETRO_DEVICE_ID_JOYPAD_LEFT]   = SDLK_LEFT,
        [RETRO_DEVICE_ID_JOYPAD_RIGHT]  = SDLK_RIGHT,
        [RETRO_DEVICE_ID_JOYPAD_A]      = SDLK_RETURN, /* Search */
        [RETRO_DEVICE_ID_JOYPAD_X]      = SDLK_r,      /* Repeat */
        [RETRO_DEVICE_ID_JOYPAD_L]      = SDLK_PAGEUP,
        [RETRO_DEVICE_ID_JOYPAD_R]      = SDLK_PAGEDOWN,
        [RETRO_DEVICE_ID_JOYPAD_L2]     = SDLK_HOME,
        [RETRO_DEVICE_ID_JOYPAD_R2]     = SDLK_END,
        [RETRO_DEVICE_ID_JOYPAD_L3]     = SDLK_s, /* Status */
        [RETRO_DEVICE_ID_JOYPAD_R3]     = SDLK_a, /* Auto */
    };
    for (int i = 0; i < 16; ++i) {
        int16_t state = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i);
        int k = bkeys[i];
        if (k && buttons[i] != state) {
            buttons[i] = state;
            sym.scancode = k;
            sym.sym = k;
            SDL_PrivateKeyboard(state ? SDL_PRESSED : SDL_RELEASED, &sym);
        }
    }
}

void retro_run(void)
{
    const int frames = 44100 / 60;
    int16_t stream[frames * 2];
    SDL_Surface *surface = SDL_GetRetroVideoSurface();
    SDL_AudioSpec *spec = SDL_GetRetroAudioSpec();

    input_poll_cb();
    pump_keyboard_events();

    if (surface) {
        SDL_LockRetroVideoSurface();
        video_cb(surface->pixels, surface->w, surface->h, surface->pitch);
        SDL_UnlockRetroVideoSurface();
    }

    if (spec && SDL_GetAudioStatus() == SDL_AUDIO_PLAYING) {
        SDL_LockAudio();
        memset(stream, 0, frames * 4);
        spec->callback(NULL, (uint8_t *)stream, frames * 4);
        audio_batch_cb(stream, frames);
        SDL_UnlockAudio();
    }
}

size_t retro_serialize_size(void)
{
    return 0;
}

bool retro_serialize(void *data, size_t size)
{
    return false;
}

bool retro_unserialize(const void *data, size_t size)
{
    return false;
}

void retro_cheat_reset(void) {}
void retro_cheat_set(unsigned index, bool enabled, const char *code) {}

bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info)
{
    return false;
}


unsigned retro_get_region(void)
{
    return RETRO_REGION_NTSC;
}

void *retro_get_memory_data(unsigned id)
{
    return 0;
}

size_t retro_get_memory_size(unsigned id)
{
    return 0;
}
