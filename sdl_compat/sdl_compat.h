/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
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
// sdl_compat.h: SDL1/2/3 compatible header by palxex, 2018
//

#ifndef SDL_COMPAT_H
#define SDL_COMPAT_H

#ifdef __APPLE__
#define GL_GLEXT_PROTOTYPES
#endif


#if USE_SDL3

#define SDL_OK      (1)
#define SDL_FAIL    (0)

#include <SDL3/SDL.h>
#else

#define SDL_OK      (0)
#define SDL_FAIL    (-1)

#include <SDL.h>
#include <SDL_endian.h>
#include <SDL_events.h>
#include <SDL_video.h>
#endif

#if SDL_VERSION_ATLEAST(3,0,0)
#undef SDL_PauseAudio
extern void SDL_PauseAudio(bool);

#define SDL_WINDOW_SHOWN				0
#define SDL_WINDOW_FULLSCREEN_DESKTOP	( SDL_WINDOW_FULLSCREEN | 0x00001000 )
#define SDL_INIT_NOPARACHUTE			0
#define SDL_HINT_RENDER_SCALE_QUALITY	"SDL_RENDER_SCALE_QUALITY"
#undef SDL_WINDOW_ALLOW_HIGHDPI
#define SDL_WINDOW_ALLOW_HIGHDPI SDL_WINDOW_HIGH_PIXEL_DENSITY

#undef SDL_bool
#define SDL_bool bool
#undef SDL_TRUE
#define SDL_TRUE true
#undef SDL_FALSE
#define SDL_FALSE false
#undef SDL_ENABLE
#define SDL_ENABLE true
#undef SDL_DISABLE
#define SDL_DISABLE false

#undef SDL_JOYBUTTONDOWN
#define SDL_JOYBUTTONDOWN SDL_EVENT_JOYSTICK_BUTTON_DOWN
#undef SDL_JOYDEVICEADDED
#define SDL_JOYDEVICEADDED SDL_EVENT_JOYSTICK_ADDED
#undef SDL_JOYDEVICEREMOVED
#define SDL_JOYDEVICEREMOVED SDL_EVENT_JOYSTICK_REMOVED
#undef SDL_JOYAXISMOTION
#define SDL_JOYAXISMOTION SDL_EVENT_JOYSTICK_AXIS_MOTION
#undef SDL_JOYHATMOTION
#define SDL_JOYHATMOTION SDL_EVENT_JOYSTICK_HAT_MOTION
#undef SDL_APP_WILLENTERBACKGROUND
#define SDL_APP_WILLENTERBACKGROUND SDL_EVENT_WILL_ENTER_BACKGROUND
#undef SDL_APP_DIDENTERBACKGROUND
#define SDL_APP_DIDENTERBACKGROUND SDL_EVENT_DID_ENTER_BACKGROUND
#undef SDL_APP_DIDENTERFOREGROUND
#define SDL_APP_DIDENTERFOREGROUND SDL_EVENT_DID_ENTER_FOREGROUND
#undef SDL_QUIT
#define SDL_QUIT SDL_EVENT_QUIT
#undef SDL_KEYDOWN
#define SDL_KEYDOWN SDL_EVENT_KEY_DOWN

#undef SDLK_r
#define SDLK_r SDLK_R
#undef SDLK_a
#define SDLK_a SDLK_A
#undef SDLK_d
#define SDLK_d SDLK_D
#undef SDLK_e
#define SDLK_e SDLK_E
#undef SDLK_w
#define SDLK_w SDLK_W
#undef SDLK_q
#define SDLK_q SDLK_Q
#undef SDLK_f
#define SDLK_f SDLK_F
#undef SDLK_s
#define SDLK_s SDLK_S
#undef SDLK_p
#define SDLK_p SDLK_P
#undef SDLK_z
#define SDLK_z SDLK_Z
#undef SDLK_x
#define SDLK_x SDLK_X

#undef KMOD_ALT
#define KMOD_ALT SDL_KMOD_ALT

#undef SDL_PREALLOC
#define SDL_PREALLOC SDL_SURFACE_PREALLOCATED
#undef SDL_GLattr
#define SDL_GLattr SDL_GLAttr

#undef SDL_GetRendererOutputSize
#define SDL_GetRendererOutputSize SDL_GetCurrentRenderOutputSize
#undef SDL_RenderCopy
#define SDL_RenderCopy SDL_RenderTexture

#undef SDL_SoftStretch
extern int SDL_SoftStretch(SDL_Surface* src, const SDL_Rect* srcrect, SDL_Surface* dst, const SDL_Rect* dstrect);
#undef SDL_BlitScaled
extern int SDL_BlitScaled(SDL_Surface* src, const SDL_Rect* srcrect, SDL_Surface* dst, const SDL_Rect* dstrect);

#undef SDL_AllocPalette
#define SDL_AllocPalette SDL_CreatePalette
#undef SDL_FreePalette
#define SDL_FreePalette SDL_DestroyPalette

#define SDL_SWSURFACE 0UL
#undef SDL_FreeSurface
#define SDL_FreeSurface SDL_DestroySurface
#undef SDL_FillRect
#define SDL_FillRect SDL_FillSurfaceRect
extern SDL_Surface* SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
extern SDL_Surface* SDL_CreateRGBSurfaceWithFormat(Uint32 flags, int width, int height, int depth, Uint32 format);
extern SDL_Surface* SDL_CreateRGBSurfaceFrom(void* pixels, int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask);
extern SDL_Surface* SDL_CreateRGBSurfaceWithFormatFrom(void* pixels, int width, int height, int depth, int pitch, Uint32 format);
extern int SDL_GL_BindTexture(SDL_Texture* texture, float* texw, float* texh);

#undef RW_SEEK_CUR
#define RW_SEEK_CUR SDL_IO_SEEK_CUR
#undef RW_SEEK_SET
#define RW_SEEK_SET SDL_IO_SEEK_SET
#undef RW_SEEK_END
#define RW_SEEK_END SDL_IO_SEEK_END
#undef SDL_RWops
#define SDL_RWops SDL_IOStream
#undef SDL_RWclose
#define SDL_RWclose SDL_CloseIO
#undef SDL_RWseek
#define SDL_RWseek SDL_SeekIO
#undef SDL_RWtell
#define SDL_RWtell SDL_TellIO
#undef SDL_RWsize
#define SDL_RWsize SDL_GetIOSize
#undef SDL_RWFromFile
#define SDL_RWFromFile SDL_IOFromFile
#undef SDL_RWFromConstMem
#define SDL_RWFromConstMem SDL_IOFromConstMem
#undef SDL_LoadBMP_RW
#define SDL_LoadBMP_RW SDL_LoadBMP_IO
#undef SDL_RWread
extern size_t SDL_RWread(SDL_IOStream* stream, void* ptr, size_t size, size_t nitems);
#undef SDL_FreeRW
#define SDL_FreeRW(x)

#define SDL_MIX_MAXVOLUME 128
#undef AUDIO_U8
#define AUDIO_U8 SDL_AUDIO_U8
#undef AUDIO_S8
#define AUDIO_S8 SDL_AUDIO_S8
#undef AUDIO_S16
#define AUDIO_S16 SDL_AUDIO_S16LE
#undef AUDIO_S16SYS
#define AUDIO_S16SYS SDL_AUDIO_S16
#undef AUDIO_S16LSB
#define AUDIO_S16LSB SDL_AUDIO_S16LE
#undef AUDIO_S16MSB
#define AUDIO_S16MSB SDL_AUDIO_S16BE
#undef AUDIO_U16LSB //SDL3 removed
#define AUDIO_U16LSB SDL_AUDIO_S16LE
#undef AUDIO_U16MSB //SDL3 removed
#define AUDIO_U16MSB SDL_AUDIO_S16BE
#undef AUDIO_S32LSB
#define AUDIO_S32LSB SDL_AUDIO_S32LE
#undef AUDIO_S32MSB
#define AUDIO_S32MSB SDL_AUDIO_S32BE
#undef AUDIO_F32SYS
#define AUDIO_F32SYS SDL_AUDIO_F32

#undef SDL_SwapLE32
#define SDL_SwapLE32 SDL_Swap32LE
#undef SDL_SwapBE32
#define SDL_SwapBE32 SDL_Swap32BE
#undef SDL_SwapLE16
#define SDL_SwapLE16 SDL_Swap16LE
#undef SDL_SwapBE16
#define SDL_SwapBE16 SDL_Swap16BE

#undef SDL_mutex
#define SDL_mutex SDL_Mutex
#define SDL_mutexP SDL_LockMutex
#define SDL_mutexV SDL_UnlockMutex

#define SDL_AUDIOCVT_MAX_FILTERS 9
struct SDL_AudioCVT;
typedef void (SDLCALL* SDL_AudioFilter) (struct SDL_AudioCVT* cvt, SDL_AudioFormat format);
#if defined(__GNUC__) && !defined(__CHERI_PURE_CAPABILITY__)
/* This structure is 84 bytes on 32-bit architectures, make sure GCC doesn't
   pad it out to 88 bytes to guarantee ABI compatibility between compilers.
   This is not a concern on CHERI architectures, where pointers must be stored
   at aligned locations otherwise they will become invalid, and thus structs
   containing pointers cannot be packed without giving a warning or error.
*/
#define SDL_AUDIOCVT_PACKED __attribute__((packed))
#else
#define SDL_AUDIOCVT_PACKED
#endif
typedef struct SDL_AudioCVT
{
    int needed;                 /**< Set to 1 if conversion possible */
    SDL_AudioFormat src_format; /**< Source audio format */
    SDL_AudioFormat dst_format; /**< Target audio format */
    double rate_incr;           /**< Rate conversion increment */
    Uint8* buf;                 /**< Buffer to hold entire audio data */
    int len;                    /**< Length of original audio buffer */
    int len_cvt;                /**< Length of converted audio buffer */
    int len_mult;               /**< buffer must be len*len_mult big */
    double len_ratio;           /**< Given len, final size is len*len_ratio */
    SDL_AudioFilter filters[SDL_AUDIOCVT_MAX_FILTERS + 1]; /**< NULL-terminated list of filter functions */
    int filter_index;           /**< Current audio conversion function */
} SDL_AUDIOCVT_PACKED SDL_AudioCVT;
extern SDL_DECLSPEC int SDLCALL SDL_BuildAudioCVT(SDL_AudioCVT* cvt,
    SDL_AudioFormat src_format,
    Uint8 src_channels,
    int src_rate,
    SDL_AudioFormat dst_format,
    Uint8 dst_channels,
    int dst_rate);
extern SDL_DECLSPEC int SDLCALL SDL_ConvertAudio(SDL_AudioCVT* cvt);

#undef SDL_JoystickEventState
#define SDL_JoystickEventState SDL_SetJoystickEventsEnabled
#undef SDL_JoystickClose
#define SDL_JoystickClose SDL_CloseJoystick
#undef SDL_JoystickGetAxis
#define SDL_JoystickGetAxis SDL_GetJoystickAxis

#undef SDL_FINGERDOWN
#define SDL_FINGERDOWN SDL_EVENT_FINGER_DOWN
#undef SDL_FINGERUP
#define SDL_FINGERUP SDL_EVENT_FINGER_UP
#undef SDL_FINGERMOTION
#define SDL_FINGERMOTION SDL_EVENT_FINGER_MOTION

#undef SDL_MOUSEBUTTONDOWN
#define SDL_MOUSEBUTTONDOWN SDL_EVENT_MOUSE_BUTTON_DOWN
#undef SDL_MOUSEBUTTONUP
#define SDL_MOUSEBUTTONUP SDL_EVENT_MOUSE_BUTTON_UP

#undef SDL_sem 
#define SDL_sem SDL_Semaphore
#undef SDL_SemPost 
#define SDL_SemPost SDL_SignalSemaphore
#undef SDL_SemWait
#define SDL_SemWait SDL_WaitSemaphore


#endif


#endif //SDL_COMPAT_H
