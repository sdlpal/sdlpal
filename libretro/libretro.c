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
#include "util.h"
#include "palcfg.h"


static void                        fallback_log(enum retro_log_level level, const char *fmt, ...);
static retro_log_printf_t          log_cb = fallback_log;
static retro_video_refresh_t       video_cb;
static retro_input_poll_t          input_poll_cb;
static retro_input_state_t         input_state_cb;
static retro_audio_sample_batch_t  audio_batch_cb;
static retro_environment_t         environ_cb;
static SDL_Thread                 *sdlpal_thread;
static bool                        platform_init_done = false;
static double                      speed_scale = 1.0;
static Uint32                      ticks = 0;


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

static void frame_cb(retro_usec_t usec)
{
    ticks += speed_scale * usec / 1000;
}

void retro_set_environment(retro_environment_t cb)
{
    struct retro_log_callback log;
    struct retro_frame_time_callback frame;
    bool no_game = true;

    struct retro_core_option_definition opts[] = {
        {
            .key = "sdlpal_speed_scale",
            .desc = "Game speed scale",
            .values = {{"0.5"},{"1.0"},{"2.0"},{"3.0"},{"4.0"},{"5.0"},{"6.0"},{"7.0"},{"8.0"},{"9.0"},{"10.0"}},
            .default_value = "1.0",
        },
        {
            .key = "sdlpal_sample_rate",
            .desc = "Audio sample rate (restart required)",
            .values = {{"44100"},{"22050"},{NULL}},
            .default_value = "44100",
        },
        {
            .key = "sdlpal_resample_quality",
            .desc = "Audio quality (restart required)",
            .values = {{"0"},{"1"},{"2"},{"3"},{"4"},{NULL}},
            .default_value = "4",
        },
        {
            .key = "sdlpal_surround_opl",
            .desc = "Use surround OPL (restart required)",
            .values = {{"true"},{"false"},{NULL}},
            .default_value = "true",
        },
        {
            .key = "sdlpal_opl_core",
            .desc = "OPL emulator core (restart required)",
            .values = {{"MAME"},{"DBINT"},{"DBFLT"},{"NUKED"},{NULL}},
            .default_value = "DBFLT",
        },
        {
            .key = "sdlpal_opl_chip",
            .desc = "OPL chip type (restart required)",
            .values = {{"OPL2"},{"OPL3"},{NULL}},
            .default_value = "OPL2",
        },
        {
            .key = "sdlpal_music_volume",
            .desc = "Music volume (restart required)",
            .values = {{"0"},{"20"},{"40"},{"60"},{"80"},{"100"}},
            .default_value = "100",
        },
        {
            .key = "sdlpal_sound_volume",
            .desc = "Sound volume (restart required)",
            .values = {{"0"},{"20"},{"40"},{"60"},{"80"},{"100"}},
            .default_value = "100",
        },
        { NULL, NULL, NULL, {{0}}, NULL },
    };
    struct retro_input_descriptor inputs[] = {
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Menu" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y, "Flee" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT, "Status" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_START, "Auto" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Search" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X, "Repeat" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L, "PageUp" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R, "PageDown" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L2, "Defend" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R2, "UseItem" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L3, "ThrowItem" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R3, "Force" },
        { 0, 0, 0, 0, NULL },
    };

    environ_cb = cb;
    if (environ_cb(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log))
        log_cb = log.log;

    environ_cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_game);
    environ_cb(RETRO_ENVIRONMENT_SET_CORE_OPTIONS, &opts);
    environ_cb(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, &inputs);

    frame.callback = frame_cb;
    frame.reference = 1000000 / 60;
    environ_cb(RETRO_ENVIRONMENT_SET_FRAME_TIME_CALLBACK, &frame);
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
    int width                   = PAL_DEFAULT_WINDOW_WIDTH;
    int height                  = PAL_DEFAULT_WINDOW_HEIGHT;
    info->geometry.base_width   = width;
    info->geometry.base_height  = height;
    info->geometry.max_width    = width;
    info->geometry.max_height   = height;
    info->geometry.aspect_ratio = 0.0;
    info->timing.fps            = 60;
    info->timing.sample_rate    = gConfig.iSampleRate;
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

    platform_init_done = false;
    sdlpal_thread = SDL_CreateThread(sdlpal_main, NULL);
    while (!platform_init_done)
        SDL_Delay(10);

    struct retro_keyboard_callback keyboard = {
        .callback = SDL_libretro_KeyboardCallback,
    };
    environ_cb(RETRO_ENVIRONMENT_SET_KEYBOARD_CALLBACK, &keyboard);

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

static void pump_joypad_events(void)
{
    static SDL_keysym sym;

    static int16_t buttons[16] = {0};
    static const int bkeys[16] = {
        [RETRO_DEVICE_ID_JOYPAD_B]      = SDLK_ESCAPE, /* Menu */
        [RETRO_DEVICE_ID_JOYPAD_Y]      = SDLK_q,      /* Flee */
        [RETRO_DEVICE_ID_JOYPAD_SELECT] = SDLK_s,      /* Status */
        [RETRO_DEVICE_ID_JOYPAD_START]  = SDLK_a,      /* Auto */
        [RETRO_DEVICE_ID_JOYPAD_UP]     = SDLK_UP,
        [RETRO_DEVICE_ID_JOYPAD_DOWN]   = SDLK_DOWN,
        [RETRO_DEVICE_ID_JOYPAD_LEFT]   = SDLK_LEFT,
        [RETRO_DEVICE_ID_JOYPAD_RIGHT]  = SDLK_RIGHT,
        [RETRO_DEVICE_ID_JOYPAD_A]      = SDLK_RETURN, /* Search */
        [RETRO_DEVICE_ID_JOYPAD_X]      = SDLK_r,      /* Repeat */
        [RETRO_DEVICE_ID_JOYPAD_L]      = SDLK_PAGEUP,
        [RETRO_DEVICE_ID_JOYPAD_R]      = SDLK_PAGEDOWN,
        [RETRO_DEVICE_ID_JOYPAD_L2]     = SDLK_d,      /* Defend */
        [RETRO_DEVICE_ID_JOYPAD_R2]     = SDLK_e,      /* UseItem */
        [RETRO_DEVICE_ID_JOYPAD_L3]     = SDLK_w,      /* ThrowItem */
        [RETRO_DEVICE_ID_JOYPAD_R3]     = SDLK_f,      /* Force */
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

static void apply_variables(void)
{
    struct retro_variable var = {0};
    var.key = "sdlpal_speed_scale";
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var)) {
        speed_scale = SDL_atof(var.value);
    }
}

void retro_run(void)
{
    input_poll_cb();
    pump_joypad_events();
    SDL_libretro_RefreshVideo(video_cb);
    SDL_libretro_ProduceAudio(audio_batch_cb);
    apply_variables();
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

BOOL UTIL_GetScreenSize(DWORD *pdwScreenWidth, DWORD *pdwScreenHeight)
{
    return FALSE;
}

BOOL UTIL_IsAbsolutePath(LPCSTR lpszFileName)
{
    return FALSE;
}

INT UTIL_Platform_Init(int argc, char *argv[])
{
    struct retro_variable var = {0};
    gConfig.iAudioChannels = 2;
    gConfig.fFullScreen = TRUE;
    gConfig.dwScreenWidth = PAL_DEFAULT_WINDOW_WIDTH;
    gConfig.dwScreenHeight = PAL_DEFAULT_WINDOW_HEIGHT;

    var.key = "sdlpal_sample_rate";
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var)) {
        gConfig.iSampleRate = atoi(var.value);
    }
    gConfig.wAudioBufferSize = 1;
    while (gConfig.wAudioBufferSize < 4 * gConfig.iSampleRate / 60)
        gConfig.wAudioBufferSize *= 2;

    var.key = "sdlpal_resample_quality";
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var)) {
        gConfig.iResampleQuality = atoi(var.value);
    }
    var.key = "sdlpal_surround_opl";
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var)) {
        if (strcmp(var.value, "true") == 0) {
            gConfig.fUseSurroundOPL = TRUE;
        } else {
            gConfig.fUseSurroundOPL = FALSE;
        }
    }
    var.key = "sdlpal_opl_core";
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var)) {
        if (strcmp(var.value, "MAME") == 0)
            gConfig.eOPLCore = OPLCORE_MAME;
        if (strcmp(var.value, "DBINT") == 0)
            gConfig.eOPLCore = OPLCORE_DBINT;
        if (strcmp(var.value, "DBFLT") == 0)
            gConfig.eOPLCore = OPLCORE_DBFLT;
        if (strcmp(var.value, "NUKED") == 0)
            gConfig.eOPLCore = OPLCORE_NUKED;
    }
    var.key = "sdlpal_opl_chip";
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var)) {
        if (strcmp(var.value, "OPL2") == 0)
            gConfig.eOPLChip = OPLCHIP_OPL2;
        if (strcmp(var.value, "OPL3") == 0)
            gConfig.eOPLChip = OPLCHIP_OPL3;
    }
    var.key = "sdlpal_music_volume";
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var)) {
        gConfig.iMusicVolume = atoi(var.value);
    }
    var.key = "sdlpal_sound_volume";
    if (environ_cb(RETRO_ENVIRONMENT_GET_VARIABLE, &var)) {
        gConfig.iSoundVolume = atoi(var.value);
    }

    platform_init_done = true;
    return 0;
}

VOID UTIL_Platform_Quit(VOID)
{
}

Uint32 SDL_GetTicksReal(void)
{
    return ticks;
}
