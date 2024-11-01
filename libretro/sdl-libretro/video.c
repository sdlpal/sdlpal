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
#include "../../3rd/SDL/src/video/SDL_pixels_c.h"
#include "../../3rd/SDL/src/video/dummy/SDL_nullvideo.h"
#include "../../3rd/SDL/src/video/dummy/SDL_nullevents_c.h"
#include "../../3rd/SDL/src/video/dummy/SDL_nullmouse_c.h"


#define DUMMYVID_DRIVER_NAME "dummy"

static SDL_Surface *_surface = NULL;

/* Initialization/Query functions */
static int DUMMY_VideoInit(_THIS, SDL_PixelFormat *vformat);
static SDL_Rect **DUMMY_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags);
static SDL_Surface *DUMMY_SetVideoMode(_THIS, SDL_Surface *current, int width, int height, int bpp, Uint32 flags);
static int DUMMY_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors);
static void DUMMY_VideoQuit(_THIS);

/* Hardware surface functions */
static int DUMMY_AllocHWSurface(_THIS, SDL_Surface *surface);
static int DUMMY_LockHWSurface(_THIS, SDL_Surface *surface);
static void DUMMY_UnlockHWSurface(_THIS, SDL_Surface *surface);
static void DUMMY_FreeHWSurface(_THIS, SDL_Surface *surface);

/* etc. */
static void DUMMY_UpdateRects(_THIS, int numrects, SDL_Rect *rects);

/* DUMMY driver bootstrap functions */

static int DUMMY_Available(void)
{
    return 1;
}

static void DUMMY_DeleteDevice(SDL_VideoDevice *device)
{
    SDL_free(device->hidden);
    SDL_free(device);
}

static SDL_VideoDevice *DUMMY_CreateDevice(int devindex)
{
    SDL_VideoDevice *device;

    /* Initialize all variables that we clean on shutdown */
    device = (SDL_VideoDevice *)SDL_malloc(sizeof(SDL_VideoDevice));
    if (device) {
        SDL_memset(device, 0, (sizeof *device));
        device->hidden = (struct SDL_PrivateVideoData *)
            SDL_malloc(sizeof(*device->hidden));
    }
    if (device == NULL || device->hidden == NULL) {
        SDL_OutOfMemory();
        if (device) {
            SDL_free(device);
        }
        return 0;
    }
    SDL_memset(device->hidden, 0, sizeof(*device->hidden));

    /* Set the function pointers */
    device->VideoInit = DUMMY_VideoInit;
    device->ListModes = DUMMY_ListModes;
    device->SetVideoMode = DUMMY_SetVideoMode;
    device->CreateYUVOverlay = NULL;
    device->SetColors = DUMMY_SetColors;
    device->UpdateRects = DUMMY_UpdateRects;
    device->VideoQuit = DUMMY_VideoQuit;
    device->AllocHWSurface = DUMMY_AllocHWSurface;
    device->CheckHWBlit = NULL;
    device->FillHWRect = NULL;
    device->SetHWColorKey = NULL;
    device->SetHWAlpha = NULL;
    device->LockHWSurface = DUMMY_LockHWSurface;
    device->UnlockHWSurface = DUMMY_UnlockHWSurface;
    device->FlipHWSurface = NULL;
    device->FreeHWSurface = DUMMY_FreeHWSurface;
    device->SetCaption = NULL;
    device->SetIcon = NULL;
    device->IconifyWindow = NULL;
    device->GrabInput = NULL;
    device->GetWMInfo = NULL;
    device->InitOSKeymap = DUMMY_InitOSKeymap;
    device->PumpEvents = DUMMY_PumpEvents;

    device->free = DUMMY_DeleteDevice;

    return device;
}

VideoBootStrap DUMMY_bootstrap = {
    DUMMYVID_DRIVER_NAME, "SDL dummy video driver",
    DUMMY_Available, DUMMY_CreateDevice
};


int DUMMY_VideoInit(_THIS, SDL_PixelFormat *vformat)
{
    /*
      fprintf(stderr, "WARNING: You are using the SDL dummy video driver!\n");
    */

    /* Determine the screen depth (use default 8-bit depth) */
    /* we change this during the SDL_SetVideoMode implementation... */
    vformat->BitsPerPixel = 8;
    vformat->BytesPerPixel = 1;

    /* We're done! */
    return 0;
}

SDL_Rect **DUMMY_ListModes(_THIS, SDL_PixelFormat *format, Uint32 flags)
{
    return (SDL_Rect **) -1;
}

SDL_Surface *DUMMY_SetVideoMode(_THIS, SDL_Surface *current,
				int width, int height, int bpp, Uint32 flags)
{
    if (this->hidden->buffer) {
        SDL_free(this->hidden->buffer);
    }

    this->hidden->buffer = SDL_malloc(width * height * (bpp / 8));
    if (!this->hidden->buffer) {
        SDL_SetError("Couldn't allocate buffer for requested mode");
        return(NULL);
    }

    SDL_memset(this->hidden->buffer, 0, width * height * (bpp / 8));

    /* Allocate the new pixel format for the screen */
    if (!SDL_ReallocFormat(current, bpp, 0, 0, 0, 0)) {
        SDL_free(this->hidden->buffer);
        this->hidden->buffer = NULL;
        SDL_SetError("Couldn't allocate new pixel format for requested mode");
        return(NULL);
    }

    /* Set up the new mode framebuffer */
    current->flags = flags & (SDL_FULLSCREEN);
    this->hidden->w = current->w = width;
    this->hidden->h = current->h = height;
    current->pitch = current->w * (bpp / 8);
    current->pixels = this->hidden->buffer;

    if (_surface == NULL) {
        _surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 16,
                                        0xf800, 0x07e0, 0x001f, 0x0000);
        if (_surface == NULL) {
            SDL_free(this->hidden->buffer);
            this->hidden->buffer = NULL;
            SDL_SetError("Couldn't create video surface");
            return NULL;
        }
    }

    /* We're done */
    return current;
}

/* We don't actually allow hardware surfaces other than the main one */
static int DUMMY_AllocHWSurface(_THIS, SDL_Surface *surface)
{
    return 0;
}
static void DUMMY_FreeHWSurface(_THIS, SDL_Surface *surface)
{
    return;
}

/* We need to wait for vertical retrace on page flipped displays */
static int DUMMY_LockHWSurface(_THIS, SDL_Surface *surface)
{
    return 0;
}

static void DUMMY_UnlockHWSurface(_THIS, SDL_Surface *surface)
{
    return;
}

static void DUMMY_UpdateRects(_THIS, int numrects, SDL_Rect *rects)
{
    SDL_Surface *screen = SDL_GetVideoSurface();
    SDL_BlitSurface(screen, NULL, _surface, NULL);
}

int DUMMY_SetColors(_THIS, int firstcolor, int ncolors, SDL_Color *colors)
{
    /* do nothing of note. */
    return 1;
}

/* Note:  If we are terminated, this could be called in the middle of
   another SDL video routine -- notably UpdateRects.
*/
void DUMMY_VideoQuit(_THIS)
{
    if (this->screen->pixels != NULL) {
        SDL_free(this->screen->pixels);
        this->screen->pixels = NULL;
    }

    if (_surface) {
        SDL_FreeSurface(_surface);
        _surface = NULL;
    }
}

void SDL_libretro_RefreshVideo(retro_video_refresh_t video_cb)
{
	video_cb(_surface->pixels, _surface->w, _surface->h, _surface->pitch);
}
