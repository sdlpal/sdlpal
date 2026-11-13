/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2026, SDLPAL development team.
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
#include <float.h>
#include "main.h"

// Screen buffer
SDL_Surface              *gpScreen           = NULL;

// Backup screen buffer
SDL_Surface              *gpScreenBak        = NULL;

// The global palette
static SDL_Palette       *gpPalette          = NULL;

#if SDL_VERSION_ATLEAST(2,0,0)
SDL_Window               *gpWindow           = NULL;
SDL_Renderer      *gpRenderer         = NULL;
SDL_Texture       *gpTexture          = NULL;
SDL_Texture       *gpTouchOverlay     = NULL;
SDL_Rect           gOverlayRect;
SDL_Rect           gTextureRect;

static struct RenderBackend {
    void (*Init)();
    void (*Setup)();
    SDL_Texture *(*CreateTexture)(int width, int height);
    void (*RenderCopy)();
} gRenderBackend;
#else
#undef PAL_HAS_GLSL
#endif

// The real screen surface
SDL_Surface       *gpScreenReal       = NULL;

volatile BOOL g_bRenderPaused = FALSE;

// Menus also set g_bRenderPaused while they draw, so that alone does not mean this.
volatile BOOL g_bAppInBackground = FALSE;

static BOOL bScaleScreen = PAL_SCALE_SCREEN;

// Shake times and level
static WORD               g_wShakeTime       = 0;
static WORD               g_wShakeLevel      = 0;

#if PAL_HAS_GLSL
#include "video_glsl.h"
#endif

#if PAL_HAS_SDL_GPU
static BOOL gfUseGPU = FALSE;
#else
#define gfUseGPU FALSE
#endif

#if SDL_VERSION_ATLEAST(3,0,0)
SDL_ScaleMode VIDEO_GetScaleMode() {
    static SDL_ScaleMode scaleMode = -1;
    if (scaleMode == -1) {
        if (gConfig.pszScaleQuality == NULL)
            scaleMode = SDL_SCALEMODE_NEAREST;
        else
            scaleMode = SDL_atoi(gConfig.pszScaleQuality);
    }
    return scaleMode;
}
#endif

#if SDL_VERSION_ATLEAST(2, 0, 0)
void VIDEO_SetupTouchArea(int window_w, int window_h, int draw_w, int draw_h)
{
	gOverlayRect.x = (window_w - draw_w) / 2;
	gOverlayRect.y = (window_h - draw_h) / 2;
	gOverlayRect.w = draw_w;
	gOverlayRect.h = draw_h;
#if PAL_HAS_TOUCH
	PAL_SetTouchBounds(window_w, window_h, gOverlayRect);
#endif
}

#if !SDL_VERSION_ATLEAST(3, 0, 0)
#define SDL_SoftStretch SDL_UpperBlit
#endif
static SDL_Texture *VIDEO_CreateTexture(int width, int height)
{
    SDL_Texture* texture = NULL;
	int texture_width, texture_height;
	double ratio = (double)width / (double)height;
	ratio *= 1.6f * (double)gConfig.dwTextureHeight / (float)gConfig.dwTextureWidth;
	//
	// Check whether to keep the aspect ratio
	//
	if (gConfig.fKeepAspectRatio && fabs(ratio - 1.6f) > FLT_EPSILON)
	{
		if (ratio > 1.6f)
		{
			texture_height = 200;
			texture_width = (int)(200 * ratio) & ~0x3;
			ratio = (float)height / 200.0f;
		}
		else
		{
			texture_width = 320;
			texture_height = (int)(320 / ratio) & ~0x3;
			ratio = (float)width / 320.0f;
		}

		WORD w = (WORD)(ratio * 320.0f) & ~0x3;
		WORD h = (WORD)(ratio * 200.0f) & ~0x3;
		gTextureRect.x = (texture_width - 320) / 2;
		gTextureRect.y = (texture_height - 200) / 2;
		gTextureRect.w = 320; gTextureRect.h = 200;
		
		VIDEO_SetupTouchArea(width,height,w,h);
	}
	else
	{
		texture_width = 320;
		texture_height = 200;
		gTextureRect.x = gTextureRect.y = 0;
		gTextureRect.w = 320; gTextureRect.h = 200;
		
		VIDEO_SetupTouchArea(width,height,width,height);
	}

	//
	// Create texture for screen.
	//
	texture = SDL_CreateTexture(gpRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, texture_width, texture_height);
#if SDL_VERSION_ATLEAST(3,0,0) && SDL_MINOR_VERSION < 3
    SDL_SetTextureScaleMode(texture, VIDEO_GetScaleMode());
#endif
    return texture;
}
#endif

void NullFunc() {}

//
// Everything that reaches the player's screen goes through these two functions. The
// SDL_GPU backend keeps palette indices where gpScreenReal would hold RGB pixels.
//
static VOID
VIDEO_BlitToScreenReal(
   SDL_Surface    *lpSrc,
   SDL_Rect       *lpSrcRect,
   SDL_Rect       *lpDstRect
)
{
#if PAL_HAS_SDL_GPU
   if (gfUseGPU)
   {
      VIDEO_GPU_WriteScreen(lpSrc, lpSrcRect, lpDstRect);
      return;
   }
#endif
   SDL_SoftStretch(lpSrc, lpSrcRect, gpScreenReal, lpDstRect);
}

static VOID
VIDEO_FillScreenRealBlack(
   SDL_Rect       *lpRect
)
{
#if PAL_HAS_SDL_GPU
   if (gfUseGPU)
   {
      VIDEO_GPU_SetBlackBand(lpRect);
      return;
   }
#endif
   SDL_FillRect(gpScreenReal, lpRect, 0);
}

#if PAL_HAS_SDL_GPU
static VOID
VIDEO_GPU_SetupTouchArea(
   int             iWidth,
   int             iHeight
)
{
   SDL_Rect viewport = VIDEO_GPU_CalcViewport(iWidth, iHeight);
   VIDEO_SetupTouchArea(iWidth, iHeight, viewport.w, viewport.h);
}
#endif

INT
VIDEO_Startup(
   VOID
)
/*++
  Purpose:

    Initialze the video subsystem.

  Parameters:

    None.

  Return value:

    0 = success, -1 = fail to create the screen surface,
    -2 = fail to create screen buffer.

--*/
{
	extern SDL_Surface* STBIMG_Load(const char* file);
	extern char *dirname(char *path);
#if APPIMAGE
	SDL_Surface *surf = STBIMG_Load( PAL_va(0, "%s%s", dirname(dirname(dirname(gExecutablePath))), "/usr/share/icons/hicolor/256x256/apps/sdlpal.png" ) );
#endif

#if SDL_VERSION_ATLEAST(2,0,0)
   int render_w, render_h;

   gRenderBackend.Init = NullFunc;
   gRenderBackend.Setup = NullFunc;
   gRenderBackend.CreateTexture = VIDEO_CreateTexture;
   gRenderBackend.RenderCopy = VIDEO_RenderCopy;

#if PAL_HAS_GLSL
   if( gConfig.fEnableGLSL) {
	   gRenderBackend.Init = VIDEO_GLSL_Init;
	   gRenderBackend.Setup = VIDEO_GLSL_Setup;
	   gRenderBackend.CreateTexture = VIDEO_GLSL_CreateTexture;
	   gRenderBackend.RenderCopy = VIDEO_GLSL_RenderCopy;
   }
#endif
	
   gRenderBackend.Init();

#if PAL_HAS_SDL_GPU
   //
   // The GLSL backend is built on OpenGL, so it keeps using the SDL renderer. So does
   // any system where SDL_GPU cannot start.
   //
   gfUseGPU = gConfig.fEnableGPU && !gConfig.fEnableGLSL && bScaleScreen && VIDEO_GPU_Init();
#endif

   //
   // Before we can render anything, we need a window and a renderer.
   //
   if (gpWindow == NULL)
#if SDL_VERSION_ATLEAST(3,0,0)
       gpWindow = SDL_CreateWindow("Pal", gConfig.dwScreenWidth, gConfig.dwScreenHeight, PAL_VIDEO_INIT_FLAGS);
   //TODO: Figure Whether Borderless Support Needed?
#else
       gpWindow = SDL_CreateWindow("Pal", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               gConfig.dwScreenWidth, gConfig.dwScreenHeight, PAL_VIDEO_INIT_FLAGS);
#endif

   if (gpWindow == NULL)
   {
      return -1;
   }

# if APPIMAGE
	if(surf){
		SDL_SetWindowIcon(gpWindow, surf);
	}
# endif

#if PAL_HAS_SDL_GPU
   if (gfUseGPU && VIDEO_GPU_ClaimWindow(gpWindow))
   {
      gpScreen = VIDEO_GPU_CreateSurface(320, 200);
      gpScreenBak = VIDEO_GPU_CreateSurface(320, 200);

      if (gpScreen != NULL && gpScreenBak != NULL)
      {
         gRenderBackend.RenderCopy = VIDEO_GPU_Present;
      }
      else
      {
         //
         // Without these there is nothing to draw on, so the SDL renderer takes over.
         //
         VIDEO_GPU_Shutdown();
         SDL_FreeSurface(gpScreen);
         SDL_FreeSurface(gpScreenBak);
         gpScreen = gpScreenBak = NULL;
         gfUseGPU = FALSE;
      }
   }
   else
   {
      gfUseGPU = FALSE;
   }
#endif

   if (!gfUseGPU)
   {
#if SDL_VERSION_ATLEAST(3,0,0)
      gpRenderer = SDL_CreateRenderer(gpWindow, NULL);
      UTIL_LogOutput(LOGLEVEL_DEBUG, "renderer backend:%s\n", SDL_GetRendererName(gpRenderer));
      UTIL_LogOutput(LOGLEVEL_DEBUG, "Trying to enable VRR\n");
      if (SDL_SetRenderVSync(gpRenderer, SDL_RENDERER_VSYNC_ADAPTIVE) != SDL_OK) {
         UTIL_LogOutput(LOGLEVEL_DEBUG, "Failed, fallback to vsync\n");
         SDL_SetRenderVSync(gpRenderer, 1);
      }
      else {
         UTIL_LogOutput(LOGLEVEL_DEBUG, "Got VRR works\n");
      }
#else
      gpRenderer = SDL_CreateRenderer(gpWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#endif

#if SDL_VERSION_ATLEAST(3,3,0)
      SDL_SetDefaultTextureScaleMode(gpRenderer, VIDEO_GetScaleMode());
#endif

      gRenderBackend.Setup();

      if (gpRenderer == NULL)
      {
         return -1;
      }
   }

#if defined (__IOS__)
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
   SDL_GL_SetAttribute(SDL_GL_RETAINED_BACKING, 1);
#endif

   //
   // Create the screen buffer and the backup screen buffer.
   //
#if PAL_HAS_SDL_GPU
   if (gfUseGPU)
   {
      //
      // The screen is on the GPU, and this one only says what size it is.
      //
      gpScreenReal = SDL_CreateRGBSurfaceFrom(NULL, 320, 200, 32, 320 * 4,
                                              0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
   }
   else
#endif
   {
      gpScreen = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 8, 0, 0, 0, 0);
      gpScreenBak = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 8, 0, 0, 0, 0);
      gpScreenReal = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 32,
                                          0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
   }

   //
   // Create texture for screen.
   //
#if PAL_HAS_SDL_GPU
   if (gfUseGPU)
   {
      SDL_GetWindowSizeInPixels(gpWindow, &render_w, &render_h);
      VIDEO_GPU_SetupTouchArea(render_w, render_h);
   }
   else
#endif
   {
      SDL_GetRendererOutputSize(gpRenderer, &render_w, &render_h);
#if SDL_MAJOR_VERSION<3
      if(!gConfig.fEnableGLSL)
         SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, gConfig.pszScaleQuality);
#endif
      gpTexture = gRenderBackend.CreateTexture(render_w, render_h);
#if SDL_MAJOR_VERSION<3
      SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
#endif
   }

   //
   // Create palette object
   //
   gpPalette = SDL_AllocPalette(256);

   //
   // Failed?
   //
   if (gpScreen == NULL || gpScreenBak == NULL || gpScreenReal == NULL || (!gfUseGPU && gpTexture == NULL) || gpPalette == NULL)
   {
      VIDEO_Shutdown();
      return -2;
   }

   //
   // Create texture for overlay.
   //
   if (gConfig.fUseTouchOverlay)
   {
      extern const unsigned char bmpData[];
      extern unsigned int bmpLen;

      void *bmp = UTIL_malloc(bmpLen);
      YJ1_Decompress(bmpData, bmp, bmpLen);
      SDL_Surface *overlay = SDL_LoadBMP_RW(SDL_RWFromConstMem(bmp, bmpLen), 1);
      free(bmp);

      if (overlay != NULL)
      {
#if SDL_VERSION_ATLEAST(3,0,0)
         SDL_SetSurfaceColorKey(overlay, true, SDL_MapRGB(SDL_GetPixelFormatDetails(overlay->format), gpPalette, 255, 0, 255));
#else
         SDL_SetColorKey(overlay, SDL_RLEACCEL, SDL_MapRGB(overlay->format, 255, 0, 255));
#endif
#if PAL_HAS_SDL_GPU
         if (gfUseGPU)
         {
            VIDEO_GPU_SetOverlay(overlay, TOUCHOVERLAY_ALPHAMOD);
         }
         else
#endif
         {
#if SDL_MAJOR_VERSION<3
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, gConfig.pszScaleQuality);
#endif
            gpTouchOverlay = SDL_CreateTextureFromSurface(gpRenderer, overlay);
#if SDL_VERSION_ATLEAST(3,0,0) && SDL_MINOR_VERSION < 3
            SDL_SetTextureScaleMode(gpTouchOverlay, VIDEO_GetScaleMode());
#endif
#if SDL_MAJOR_VERSION<3
            SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
#endif
            SDL_SetTextureAlphaMod(gpTouchOverlay, TOUCHOVERLAY_ALPHAMOD );
         }
         SDL_FreeSurface(overlay);
      }
   }
# if PAL_HAS_GLSL
	// notice: power of 2
#  define PIXELS 1
	// We need a total empty texture in case of not using touch overlay.
	// Or GL runtime will pick the previous texture - the main screen itself
	// and reuse it - that makes color seems overexposed
	else if( gConfig.fEnableGLSL )
	{
		BYTE pixels[4*PIXELS*PIXELS];
		memset(pixels, 0, sizeof(pixels));
		SDL_Surface *temp = SDL_CreateRGBSurfaceFrom(pixels, PIXELS, PIXELS, 32, 4*PIXELS, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
		gpTouchOverlay = SDL_CreateTextureFromSurface(gpRenderer, temp);
		SDL_FreeSurface(temp);
	}
# endif
#else

# if APPIMAGE
	if(surf){
		SDL_WM_SetIcon(surf, NULL);
	}
# endif

   //
   // Create the screen surface.
   //
   gpScreenReal = SDL_SetVideoMode(gConfig.dwScreenWidth, gConfig.dwScreenHeight, 8, PAL_VIDEO_INIT_FLAGS);

   if (gpScreenReal == NULL)
   {
      //
      // Fall back to 640x480 software mode.
      //
      gpScreenReal = SDL_SetVideoMode(640, 480, 8,
         SDL_SWSURFACE | (gConfig.fFullScreen ? SDL_FULLSCREEN : 0));
   }

   //
   // Still fail?
   //
   if (gpScreenReal == NULL)
   {
      return -1;
   }

   gpPalette = gpScreenReal->format->palette;

   //
   // Create the screen buffer and the backup screen buffer.
   //
   gpScreen = SDL_CreateRGBSurface(gpScreenReal->flags & ~SDL_HWSURFACE, 320, 200, 8,
      gpScreenReal->format->Rmask, gpScreenReal->format->Gmask,
      gpScreenReal->format->Bmask, gpScreenReal->format->Amask);

   gpScreenBak = SDL_CreateRGBSurface(gpScreenReal->flags & ~SDL_HWSURFACE, 320, 200, 8,
      gpScreenReal->format->Rmask, gpScreenReal->format->Gmask,
      gpScreenReal->format->Bmask, gpScreenReal->format->Amask);

   //
   // Failed?
   //
   if (gpScreen == NULL || gpScreenBak == NULL)
   {
      VIDEO_Shutdown();
      return -2;
   }

   if (gConfig.fFullScreen)
   {
      SDL_ShowCursor(FALSE);
   }

#endif

#if APPIMAGE
	if(surf)
		SDL_FreeSurface(surf);
#endif

   return 0;
}

VOID
VIDEO_Shutdown(
   VOID
)
/*++
  Purpose:

    Shutdown the video subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
#if PAL_HAS_GLSL
    // since gConfig is cleared already we'd to detect on side effects
	if( gRenderBackend.Init == VIDEO_GLSL_Init ) {
		VIDEO_GLSL_Destroy();
	}
#endif

#if PAL_HAS_SDL_GPU
   //
   // The window must not be destroyed while the GPU device still owns it.
   //
   if (gfUseGPU)
   {
      VIDEO_GPU_Shutdown();
      gfUseGPU = FALSE;
   }
#endif

   if (gpScreen != NULL)
   {
      SDL_FreeSurface(gpScreen);
   }
   gpScreen = NULL;

   if (gpScreenBak != NULL)
   {
      SDL_FreeSurface(gpScreenBak);
   }
   gpScreenBak = NULL;

#if SDL_VERSION_ATLEAST(2,0,0)

   if (gpTouchOverlay)
   {
      SDL_DestroyTexture(gpTouchOverlay);
   }
   gpTouchOverlay = NULL;

   if (gpTexture)
   {
	  SDL_DestroyTexture(gpTexture);
   }
   gpTexture = NULL;

   if (gpRenderer)
   {
      SDL_DestroyRenderer(gpRenderer);
   }
   gpRenderer = NULL;

   if (gpWindow)
   {
      SDL_DestroyWindow(gpWindow);
   }
   gpWindow = NULL;

   if (gpPalette)
   {
      SDL_FreePalette(gpPalette);
   }
#endif
   gpPalette = NULL;

   if (gpScreenReal != NULL)
   {
      SDL_FreeSurface(gpScreenReal);
   }
   gpScreenReal = NULL;
}

#if SDL_VERSION_ATLEAST(2,0,0)
VOID
VIDEO_RenderCopy(
   VOID
)
{
	void *texture_pixels;
	int texture_pitch;

	SDL_LockTexture(gpTexture, NULL, &texture_pixels, &texture_pitch);
	memset(texture_pixels, 0, gTextureRect.y * texture_pitch);
	uint8_t *pixels = (uint8_t *)texture_pixels + gTextureRect.y * texture_pitch;
	uint8_t *src = (uint8_t *)gpScreenReal->pixels;
	int left_pitch = gTextureRect.x << 2;
	int right_pitch = texture_pitch - ((gTextureRect.x + gTextureRect.w) << 2);
	for (int y = 0; y < gTextureRect.h; y++, src += gpScreenReal->pitch)
	{
		memset(pixels, 0, left_pitch); pixels += left_pitch;
		memcpy(pixels, src, 320 << 2); pixels += 320 << 2;
		memset(pixels, 0, right_pitch); pixels += right_pitch;
	}
	memset(pixels, 0, gTextureRect.y * texture_pitch);
	SDL_UnlockTexture(gpTexture);

	SDL_RenderClear(gpRenderer);
	SDL_RenderCopy(gpRenderer, gpTexture, NULL, NULL);
	if (gConfig.fUseTouchOverlay)
	{
#if SDL_VERSION_ATLEAST(3,0,0)
        SDL_FRect fOverlayRect = { gOverlayRect.x, gOverlayRect.y, gOverlayRect.w, gOverlayRect.h };
        SDL_RenderTexture(gpRenderer, gpTouchOverlay, NULL, &fOverlayRect);
#else
		SDL_RenderCopy(gpRenderer, gpTouchOverlay, NULL, &gOverlayRect);
#endif
	}
	SDL_RenderPresent(gpRenderer);
}
#endif

#if SDL_VERSION_ATLEAST(2,0,0)
static VOID
VIDEO_ShowFrame(
   VOID
)
{
   //
   // Rendering from the background gets an iOS app terminated. PAL_ProcessEvent() holds
   // the game there, and coming back redraws the screen, so this frame is not missed.
   //
   if (!g_bAppInBackground)
   {
      gRenderBackend.RenderCopy();
   }
}
#endif

VOID
VIDEO_UpdateScreen(
   const SDL_Rect  *lpRect
)
/*++
  Purpose:

    Update the screen area specified by lpRect.

  Parameters:

    [IN]  lpRect - Screen area to update.

  Return value:

    None.

--*/
{
   SDL_Rect        srcrect, dstrect;
   short           offset = 240 - 200;
   short           screenRealHeight = gpScreenReal->h;
   short           screenRealY = 0;

#if SDL_VERSION_ATLEAST(2,0,0)
   if (g_bRenderPaused)
   {
	   return;
   }
#endif

   //
   // Lock surface if needed
   //
   if (SDL_MUSTLOCK(gpScreenReal))
   {
      if (SDL_LockSurface(gpScreenReal) < 0)
         return;
   }

   if (!bScaleScreen)
   {
      screenRealHeight -= offset;
      screenRealY = offset / 2;
   }

   if (lpRect != NULL)
   {
      dstrect.x = (SHORT)((INT)(lpRect->x) * gpScreenReal->w / gpScreen->w);
      dstrect.y = (SHORT)((INT)(screenRealY + lpRect->y) * screenRealHeight / gpScreen->h);
      dstrect.w = (WORD)((DWORD)(lpRect->w) * gpScreenReal->w / gpScreen->w);
      dstrect.h = (WORD)((DWORD)(lpRect->h) * screenRealHeight / gpScreen->h);

      VIDEO_BlitToScreenReal(gpScreen, (SDL_Rect *)lpRect, &dstrect);
   }
   else if (g_wShakeTime != 0)
   {
      //
      // Shake the screen
      //
      srcrect.x = 0;
      srcrect.y = 0;
      srcrect.w = 320;
      srcrect.h = 200 - g_wShakeLevel;

      dstrect.x = 0;
      dstrect.y = screenRealY;
      dstrect.w = 320 * gpScreenReal->w / gpScreen->w;
      dstrect.h = (200 - g_wShakeLevel) * screenRealHeight / gpScreen->h;

      if (g_wShakeTime & 1)
      {
         srcrect.y = g_wShakeLevel;
      }
      else
      {
         dstrect.y = (screenRealY + g_wShakeLevel) * screenRealHeight / gpScreen->h;
      }

      VIDEO_BlitToScreenReal(gpScreen, &srcrect, &dstrect);

      if (g_wShakeTime & 1)
      {
         dstrect.y = (screenRealY + screenRealHeight - g_wShakeLevel) * screenRealHeight / gpScreen->h;
      }
      else
      {
         dstrect.y = screenRealY;
      }

      dstrect.h = g_wShakeLevel * screenRealHeight / gpScreen->h;

      VIDEO_FillScreenRealBlack(&dstrect);

#if SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION <= 2
      dstrect.x = dstrect.y = 0;
      dstrect.w = gpScreenReal->w;
      dstrect.h = gpScreenReal->h;
#endif
      g_wShakeTime--;
   }
   else
   {
      dstrect.x = 0;
      dstrect.y = screenRealY;
      dstrect.w = gpScreenReal->w;
      dstrect.h = screenRealHeight;

      VIDEO_BlitToScreenReal(gpScreen, NULL, &dstrect);

#if SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION <= 2
      dstrect.x = dstrect.y = 0;
      dstrect.w = gpScreenReal->w;
      dstrect.h = gpScreenReal->h;
#endif
   }

#if SDL_VERSION_ATLEAST(2,0,0)
   VIDEO_ShowFrame();
#else
   SDL_UpdateRect(gpScreenReal, dstrect.x, dstrect.y, dstrect.w, dstrect.h);
#endif

   if (SDL_MUSTLOCK(gpScreenReal))
   {
	   SDL_UnlockSurface(gpScreenReal);
   }
}

VOID
VIDEO_SetPalette(
   SDL_Color        rgPalette[256]
)
/*++
  Purpose:

    Set the palette of the screen.

  Parameters:

    [IN]  rgPalette - array of 256 colors.

  Return value:

    None.

--*/
{
#if SDL_VERSION_ATLEAST(2,0,0)
   SDL_Rect rect;

   rect.x = 0;
   rect.y = 0;
   rect.w = 320;
   rect.h = 200;

#if PAL_HAS_SDL_GPU
   if (gfUseGPU && !VIDEO_GPU_OnSetPalette(rgPalette))
   {
      VIDEO_UpdateScreen(&rect);
      return;
   }
#endif

   SDL_SetPaletteColors(gpPalette, rgPalette, 0, 256);

   SDL_SetSurfacePalette(gpScreen, gpPalette);
   SDL_SetSurfacePalette(gpScreenBak, gpPalette);

   //
   // HACKHACK: need to invalidate gpScreen->map otherwise the palette
   // would not be effective during blit
   //
   SDL_SetSurfaceColorMod(gpScreen, 0, 0, 0);
   SDL_SetSurfaceColorMod(gpScreen, 0xFF, 0xFF, 0xFF);
   SDL_SetSurfaceColorMod(gpScreenBak, 0, 0, 0);
   SDL_SetSurfaceColorMod(gpScreenBak, 0xFF, 0xFF, 0xFF);

   VIDEO_UpdateScreen(&rect);
#else
   SDL_SetPalette(gpScreen, SDL_LOGPAL | SDL_PHYSPAL, rgPalette, 0, 256);
   SDL_SetPalette(gpScreenBak, SDL_LOGPAL | SDL_PHYSPAL, rgPalette, 0, 256);
   SDL_SetPalette(gpScreenReal, SDL_LOGPAL | SDL_PHYSPAL, rgPalette, 0, 256);
# if defined(PAL_FORCE_UPDATE_ON_PALETTE_SET)
   {
      static UINT32 time = 0;
      if (SDL_GetTicks() - time > 50)
      {
	      SDL_UpdateRect(gpScreenReal, 0, 0, gpScreenReal->w, gpScreenReal->h);
	      time = SDL_GetTicks();
      }
   }
# endif
#endif
}

BOOL
VIDEO_SetPaletteFade(
   INT              iMode,
   const SDL_Color *rgPalette,
   const SDL_Color *rgPaletteTo,
   INT              iStep,
   BYTE             bColor
)
/*++
  Purpose:

    Show one step of a palette fade without working out the resulting palette, for
    backends that can do that themselves.

  Parameters:

    [IN]  iMode - one of the kPaletteFade values.

    [IN]  rgPalette - the palette to fade.

    [IN]  rgPaletteTo - the palette to fade to, for kPaletteFadeBlend.

    [IN]  iStep - how far the fade has gone; the meaning depends on the mode.

    [IN]  bColor - the palette entry to fade from or to, where the mode has one.

  Return value:

    TRUE if done. FALSE if the caller has to calculate the palette and set it.

--*/
{
#if PAL_HAS_SDL_GPU
   if (gfUseGPU && VIDEO_GPU_SetPaletteFade(iMode, rgPalette, rgPaletteTo, iStep, bColor))
   {
      SDL_Rect rect = { 0, 0, 320, 200 };
      VIDEO_UpdateScreen(&rect);
      return TRUE;
   }
#endif
   return FALSE;
}

VOID
VIDEO_Blit(
   SDL_Surface    *lpSrc,
   const SDL_Rect *lpSrcRect,
   SDL_Surface    *lpDst,
   const SDL_Rect *lpDstRect
)
/*++
  Purpose:

    Copy an area between two surfaces.

  Parameters:

    [IN]  lpSrc - the surface to copy from.

    [IN]  lpSrcRect - the area to copy, NULL for the whole surface.

    [OUT] lpDst - the surface to copy to.

    [IN]  lpDstRect - where to copy to, NULL for the top left corner.

  Return value:

    None.

--*/
{
   if (!VIDEO_GPU_Blit(lpSrc, lpSrcRect, lpDst, lpDstRect))
   {
      //
      // SDL_BlitSurface changes the destination rectangle in some versions.
      //
      SDL_Rect dstrect;

      if (lpDstRect != NULL)
      {
         dstrect = *lpDstRect;
      }
      SDL_BlitSurface(lpSrc, (SDL_Rect *)lpSrcRect, lpDst, (lpDstRect != NULL) ? &dstrect : NULL);
   }
}

VOID
VIDEO_FillRect(
   SDL_Surface    *lpDst,
   const SDL_Rect *lpRect,
   BYTE            bColor
)
/*++
  Purpose:

    Fill an area of a surface with one color.

  Parameters:

    [OUT] lpDst - the surface.

    [IN]  lpRect - the area, NULL for the whole surface.

    [IN]  bColor - the color.

  Return value:

    None.

--*/
{
   if (!VIDEO_GPU_Fill(lpDst, lpRect, bColor))
   {
      SDL_FillRect(lpDst, (SDL_Rect *)lpRect, bColor);
   }
}

VOID
VIDEO_FreeSurface(
   SDL_Surface    *lpSurface
)
{
#if PAL_HAS_SDL_GPU
   VIDEO_GPU_UnregisterSurface(lpSurface);
#endif
   SDL_FreeSurface(lpSurface);
}

VOID
VIDEO_Resize(
   INT             w,
   INT             h
)
/*++
  Purpose:

    This function is called when user resized the window.

  Parameters:

    [IN]  w - width of the window after resizing.

    [IN]  h - height of the window after resizing.

  Return value:

    None.

--*/
{
#if SDL_VERSION_ATLEAST(2,0,0)
   SDL_Rect rect;

#if PAL_HAS_SDL_GPU
   if (gfUseGPU)
   {
      VIDEO_GPU_SetupTouchArea(w, h);
   }
   else
#endif
   {
      if (gpTexture)
      {
         SDL_DestroyTexture(gpTexture);
      }

      gpTexture = gRenderBackend.CreateTexture(w, h);

      if (gpTexture == NULL)
      {
         TerminateOnError("Re-creating texture failed on window resize!\n");
      }
   }

   rect.x = 0;
   rect.y = 0;
   rect.w = 320;
   rect.h = 200;

   VIDEO_UpdateScreen(&rect);
#else
   DWORD                    flags;
   PAL_LARGE SDL_Color      palette[256];
   int                      i, bpp;

   //
   // Get the original palette.
   //
   if (gpScreenReal->format->palette != NULL)
   {
      for (i = 0; i < gpScreenReal->format->palette->ncolors; i++)
      {
         palette[i] = gpScreenReal->format->palette->colors[i];
      }
   }
   else i = 0;

   //
   // Create the screen surface.
   //
   flags = gpScreenReal->flags;
   bpp = gpScreenReal->format->BitsPerPixel;

   SDL_FreeSurface(gpScreenReal);
   gpScreenReal = SDL_SetVideoMode(w, h, bpp, flags);

   if (gpScreenReal == NULL)
   {
      //
      // Fall back to software windowed mode in default size.
      //
      gpScreenReal = SDL_SetVideoMode(PAL_DEFAULT_WINDOW_WIDTH, PAL_DEFAULT_WINDOW_HEIGHT, bpp, SDL_SWSURFACE);
   }

   SDL_SetPalette(gpScreenReal, SDL_PHYSPAL | SDL_LOGPAL, palette, 0, i);
   VIDEO_UpdateScreen(NULL);

   gpPalette = gpScreenReal->format->palette;
#endif
}

SDL_Color *
VIDEO_GetPalette(
   VOID
)
/*++
  Purpose:

    Get the current palette of the screen.

  Parameters:

    None.

  Return value:

    Pointer to the current palette.

--*/
{
#if PAL_HAS_SDL_GPU
   if (gfUseGPU)
   {
      VIDEO_GPU_ResolvePalette(gpPalette->colors);
   }
#endif
   return gpPalette->colors;
}

VOID
VIDEO_ToggleScaleScreen(
   VOID
)
/*++
  Purpose:

    Toggle scalescreen mode, only used in some platforms.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   //
   // The SDL_GPU backend only shows the game at full height, which is why VIDEO_Startup()
   // does not use it for an unscaled screen either.
   //
   if (gfUseGPU)
   {
      return;
   }

   bScaleScreen = !bScaleScreen;
   VIDEO_Resize(PAL_DEFAULT_WINDOW_WIDTH, PAL_DEFAULT_WINDOW_HEIGHT);
   VIDEO_UpdateScreen(NULL);
}

VOID
VIDEO_ToggleFullscreen(
   VOID
)
/*++
  Purpose:

    Toggle fullscreen mode.

  Parameters:

    None.

  Return value:

    None.

--*/
{
#if SDL_VERSION_ATLEAST(2,0,0)
	if (gConfig.fFullScreen)
	{
		SDL_SetWindowFullscreen(gpWindow, 0);
		gConfig.fFullScreen = FALSE;
	}
	else
	{
		SDL_SetWindowFullscreen(gpWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
		gConfig.fFullScreen = TRUE;
	}
#else
   DWORD                    flags;
   PAL_LARGE SDL_Color      palette[256];
   int                      i, bpp;

   //
   // Get the original palette.
   //
   if (gpScreenReal->format->palette != NULL)
   {
      for (i = 0; i < gpScreenReal->format->palette->ncolors; i++)
      {
         palette[i] = gpScreenReal->format->palette->colors[i];
      }
   }

   //
   // Get the flags and bpp of the original screen surface
   //
   flags = gpScreenReal->flags;
   bpp = gpScreenReal->format->BitsPerPixel;

   if (flags & SDL_FULLSCREEN)
   {
      //
      // Already in fullscreen mode. Remove the fullscreen flag.
      //
      flags &= ~SDL_FULLSCREEN;
      flags |= SDL_RESIZABLE;
      SDL_ShowCursor(TRUE);
   }
   else
   {
      //
      // Not in fullscreen mode. Set the fullscreen flag.
      //
      flags |= SDL_FULLSCREEN;
      SDL_ShowCursor(FALSE);
   }

   //
   // Free the original screen surface
   //
   SDL_FreeSurface(gpScreenReal);

   //
   // ... and create a new one
   //
   if (gConfig.dwScreenWidth == 640 && gConfig.dwScreenHeight == 400 && (flags & SDL_FULLSCREEN))
   {
      gpScreenReal = SDL_SetVideoMode(640, 480, bpp, flags);
   }
   else if (gConfig.dwScreenWidth == 640 && gConfig.dwScreenHeight == 480 && !(flags & SDL_FULLSCREEN))
   {
      gpScreenReal = SDL_SetVideoMode(640, 400, bpp, flags);
   }
   else
   {
      gpScreenReal = SDL_SetVideoMode(gConfig.dwScreenWidth, gConfig.dwScreenHeight, bpp, flags);
   }

   VIDEO_SetPalette(palette);

   //
   // Update the screen
   //
   VIDEO_UpdateScreen(NULL);
#endif
}

VOID
VIDEO_ChangeDepth(
   INT             bpp
)
/*++
  Purpose:

    Change screen color depth.

  Parameters:

    [IN]  bpp - bits per pixel (0 = default).

  Return value:

    None.

--*/
{
#if !SDL_VERSION_ATLEAST(2,0,0)
   DWORD                    flags;
   int                      w, h;

   //
   // Get the flags and resolution of the original screen surface
   //
   flags = gpScreenReal->flags;
   w = gpScreenReal->w;
   h = gpScreenReal->h;

   //
   // Clear the screen surface.
   //
   SDL_FillRect(gpScreenReal, NULL, 0);

   //
   // Create the screen surface.
   //
   SDL_FreeSurface(gpScreenReal);
   gpScreenReal = SDL_SetVideoMode(w, h, (bpp == 0)?8:bpp, flags);

   if (gpScreenReal == NULL)
   {
      //
      // Fall back to software windowed mode in default size.
      //
      gpScreenReal = SDL_SetVideoMode(PAL_DEFAULT_WINDOW_WIDTH, PAL_DEFAULT_WINDOW_HEIGHT, (bpp == 0)?8:bpp, SDL_SWSURFACE);
   }

   gpPalette = gpScreenReal->format->palette;
#endif
}

VOID
VIDEO_SaveScreenshot(
   VOID
)
/*++
  Purpose:

    Save the screenshot of current screen to a BMP file.

  Parameters:

    None.

  Return value:

    None.

--*/
{
	char filename[80];
#ifdef _WIN32
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf(filename, "%04d%02d%02d%02d%02d%02d%03d.bmp", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#elif !defined( GEKKO )
	struct timeval tv;
	struct tm *ptm;
	gettimeofday(&tv, NULL);
	ptm = localtime(&tv.tv_sec);
	sprintf(filename, "%04d%02d%02d%02d%02d%02d%03d.bmp", ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, (int)(tv.tv_usec / 1000));
#endif

	//
	// Save the screenshot.
	//
#if PAL_HAS_SDL_GPU
	if (gfUseGPU)
	{
		//
		// The screen is on the GPU, so the file gets a copy of its own. Palette fades on the
		// GPU leave the palette behind, so this first has to work out where it is now.
		//
		SDL_Surface *pCopy = SDL_CreateSurface(320, 200, SDL_PIXELFORMAT_INDEX8);

		if (pCopy != NULL)
		{
			VIDEO_GPU_ResolvePalette(gpPalette->colors);
			VIDEO_UpdateSurfacePalette(pCopy);
			if (VIDEO_GPU_ReadSurface(gpScreen, (BYTE *)pCopy->pixels, pCopy->pitch))
			{
				SDL_SaveBMP(pCopy, PAL_CombinePath(0, gConfig.pszSavePath, filename));
			}
			SDL_FreeSurface(pCopy);
		}
		return;
	}
#endif
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_SaveBMP(gpScreen, PAL_CombinePath(0, gConfig.pszSavePath, filename));
#else
	SDL_SaveBMP(gpScreenReal, PAL_CombinePath(0, gConfig.pszSavePath, filename));
#endif
}

VOID
VIDEO_ShakeScreen(
   WORD           wShakeTime,
   WORD           wShakeLevel
)
/*++
  Purpose:

    Set the screen shake time and level.

  Parameters:

    [IN]  wShakeTime - how many times should we shake the screen.

    [IN]  wShakeLevel - level of shaking.

  Return value:

    None.

--*/
{
   g_wShakeTime = wShakeTime;
   g_wShakeLevel = wShakeLevel;
}

VOID
VIDEO_SwitchScreen(
   WORD           wSpeed
)
/*++
  Purpose:

    Switch the screen from the backup screen buffer to the current screen buffer.
    NOTE: This will destroy the backup buffer.

  Parameters:

    [IN]  wSpeed - speed of fading (the larger value, the slower).

  Return value:

    None.

--*/
{
   int               i, j;
   const int         rgIndex[6] = {0, 3, 1, 5, 2, 4};
   SDL_Rect          dstrect;

   short             offset = 240 - 200;
   short             screenRealHeight = gpScreenReal->h;
   short             screenRealY = 0;

   if (!bScaleScreen)
   {
      screenRealHeight -= offset;
      screenRealY = offset / 2;
   }

   wSpeed++;
   wSpeed *= 10;

   for (i = 0; i < 6; i++)
   {
      if (!VIDEO_GPU_SwitchStep(gpScreen, gpScreenBak, rgIndex[i]))
      {
         for (j = rgIndex[i]; j < gpScreen->pitch * gpScreen->h; j += 6)
         {
            ((LPBYTE)(gpScreenBak->pixels))[j] = ((LPBYTE)(gpScreen->pixels))[j];
         }
      }

      //
      // Draw the backup buffer to the screen
      //
      dstrect.x = 0;
      dstrect.y = screenRealY;
      dstrect.w = gpScreenReal->w;
      dstrect.h = screenRealHeight;

	  if (SDL_MUSTLOCK(gpScreenReal))
	  {
		  if (SDL_LockSurface(gpScreenReal) < 0)
			  return;
	  }

      VIDEO_BlitToScreenReal(gpScreenBak, NULL, &dstrect);
#if SDL_VERSION_ATLEAST(2, 0, 0)
      VIDEO_ShowFrame();
#else
      SDL_UpdateRect(gpScreenReal, 0, 0, gpScreenReal->w, gpScreenReal->h);
#endif

	  if (SDL_MUSTLOCK(gpScreenReal))
	  {
		  SDL_UnlockSurface(gpScreenReal);
	  }

      UTIL_Delay(wSpeed);
   }
}

VOID
VIDEO_CrossFadeStep(
   SDL_Surface    *lpFrom,
   SDL_Surface    *lpTo,
   INT             iPhase,
   BOOL            fStep
)
/*++
  Purpose:

    Blend every sixth pixel of one 320x200 surface into another.

  Parameters:

    [IN]  lpFrom - the surface to blend from.

    [OUT] lpTo - the surface to blend into.

    [IN]  iPhase - which of the six sets of pixels to blend.

    [IN]  fStep - whether the brightness moves one step closer, or stays as it is.

  Return value:

    None.

--*/
{
   int  k;
   BYTE a, b;

   if (VIDEO_GPU_CrossFadeStep(lpFrom, lpTo, iPhase, fStep))
   {
      return;
   }

   for (k = iPhase; k < lpTo->pitch * lpTo->h; k += 6)
   {
      a = ((LPBYTE)(lpFrom->pixels))[k];
      b = ((LPBYTE)(lpTo->pixels))[k];

      if (fStep)
      {
         if ((a & 0x0F) > (b & 0x0F))
         {
            b++;
         }
         else if ((a & 0x0F) < (b & 0x0F))
         {
            b--;
         }
      }

      ((LPBYTE)(lpTo->pixels))[k] = ((a & 0xF0) | (b & 0x0F));
   }
}

VOID
VIDEO_FadeScreen(
   WORD           wSpeed
)
/*++
  Purpose:

    Fade from the backup screen buffer to the current screen buffer.
    NOTE: This will destroy the backup buffer.

  Parameters:

    [IN]  wSpeed - speed of fading (the larger value, the slower).

  Return value:

    None.

--*/
{
   int               i, j;
   DWORD             time;
   const int         rgIndex[6] = {0, 3, 1, 5, 2, 4};
   SDL_Rect          dstrect;
   short             offset = 240 - 200;
   short             screenRealHeight = gpScreenReal->h;
   short             screenRealY = 0;

   //
   // Lock surface if needed
   //
   if (SDL_MUSTLOCK(gpScreenReal))
   {
      if (SDL_LockSurface(gpScreenReal) < 0)
         return;
   }

   if (!bScaleScreen)
   {
      screenRealHeight -= offset;
      screenRealY = offset / 2;
   }

   time = SDL_GetTicks();

   wSpeed++;
   wSpeed *= 10;

   for (i = 0; i < 12; i++)
   {
      for (j = 0; j < 6; j++)
      {
         PAL_ProcessEvent();
         while (!SDL_TICKS_PASSED(SDL_GetTicks(), time))
         {
            PAL_ProcessEvent();
            SDL_Delay(5);
         }
         time = SDL_GetTicks() + wSpeed;

         //
         // Blend the pixels in the 2 buffers, and put the result into the
         // backup buffer
         //
         VIDEO_CrossFadeStep(gpScreen, gpScreenBak, rgIndex[j], i > 0);

         //
         // Draw the backup buffer to the screen
         //
         if (g_wShakeTime != 0)
         {
            //
            // Shake the screen
            //
            SDL_Rect srcrect, dstrect;

            srcrect.x = 0;
            srcrect.y = 0;
            srcrect.w = 320;
            srcrect.h = 200 - g_wShakeLevel;

            dstrect.x = 0;
            dstrect.y = screenRealY;
            dstrect.w = 320 * gpScreenReal->w / gpScreen->w;
            dstrect.h = (200 - g_wShakeLevel) * screenRealHeight / gpScreen->h;

            if (g_wShakeTime & 1)
            {
               srcrect.y = g_wShakeLevel;
            }
            else
            {
               dstrect.y = (screenRealY + g_wShakeLevel) * screenRealHeight / gpScreen->h;
            }

            VIDEO_BlitToScreenReal(gpScreenBak, &srcrect, &dstrect);

            if (g_wShakeTime & 1)
            {
               dstrect.y = (screenRealY + screenRealHeight - g_wShakeLevel) * screenRealHeight / gpScreen->h;
            }
            else
            {
               dstrect.y = screenRealY;
            }

            dstrect.h = g_wShakeLevel * screenRealHeight / gpScreen->h;

            VIDEO_FillScreenRealBlack(&dstrect);
#if SDL_VERSION_ATLEAST(2, 0, 0)
            VIDEO_ShowFrame();
#else
			SDL_UpdateRect(gpScreenReal, 0, 0, gpScreenReal->w, gpScreenReal->h);
#endif
            g_wShakeTime--;
         }
         else
         {
            dstrect.x = 0;
            dstrect.y = screenRealY;
            dstrect.w = gpScreenReal->w;
            dstrect.h = screenRealHeight;

            VIDEO_BlitToScreenReal(gpScreenBak, NULL, &dstrect);
#if SDL_VERSION_ATLEAST(2, 0, 0)
            VIDEO_ShowFrame();
#else
            SDL_UpdateRect(gpScreenReal, 0, 0, gpScreenReal->w, gpScreenReal->h);
#endif
         }
      }
   }

   if (SDL_MUSTLOCK(gpScreenReal))
   {
      SDL_UnlockSurface(gpScreenReal);
   }

   //
   // Draw the result buffer to the screen as the final step
   //
   VIDEO_UpdateScreen(NULL);
}

const char *
VIDEO_GetGPUDriverName(
   VOID
)
/*++
  Purpose:

    Get the name of the SDL_GPU driver drawing the game, such as "vulkan".

  Parameters:

    None.

  Return value:

    The name, or NULL if the game draws through the SDL renderer or OpenGL.

--*/
{
#if PAL_HAS_SDL_GPU
   if (gfUseGPU)
   {
      return VIDEO_GPU_GetDriverName();
   }
#endif
   return NULL;
}

void
VIDEO_SetWindowTitle(
	const char*     pszTitle
)
/*++
  Purpose:

    Set the caption of the window.

  Parameters:

    [IN]  pszTitle - the new caption of the window.

  Return value:

    None.

--*/
{
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_SetWindowTitle(gpWindow, PAL_CONVERT_UTF8(pszTitle));
#else
	SDL_WM_SetCaption(pszTitle, NULL);
#endif
}

SDL_Surface *
VIDEO_CreateCompatibleSurface(
	SDL_Surface    *pSource
)
{
	return VIDEO_CreateCompatibleSizedSurface(pSource, NULL);
}

SDL_Surface *
VIDEO_CreateCompatibleSizedSurface(
	SDL_Surface    *pSource,
	const SDL_Rect *pSize
)
/*++
  Purpose:

    Create a surface that compatible with the source surface.

  Parameters:

    [IN]  pSource   - the source surface from which attributes are taken.
    [IN]  pSize     - the size (width & height) of the created surface.

  Return value:

    None.

--*/
{
	//
	// Create the surface
	//
#if SDL_VERSION_ATLEAST(3,0,0)
    int w = pSize ? pSize->w : pSource->w;
    int h = pSize ? pSize->h : pSource->h;
    SDL_Surface* dest = gfUseGPU ? VIDEO_GPU_CreateSurface(w, h)
        : SDL_CreateSurface(w, h, SDL_GetPixelFormatDetails(pSource->format)->format);

    //
    // The GPU holds the only copy of a surface, and without one the game would go on with
    // pictures quietly missing.
    //
    if (dest == NULL && gfUseGPU)
    {
        TerminateOnError("Could not create a surface on the GPU: %s\n", SDL_GetError());
    }
#else
	SDL_Surface *dest = SDL_CreateRGBSurface(pSource->flags,
		pSize ? pSize->w : pSource->w,
		pSize ? pSize->h : pSource->h,
		pSource->format->BitsPerPixel,
		pSource->format->Rmask, pSource->format->Gmask,
		pSource->format->Bmask, pSource->format->Amask);
#endif

	if (dest)
	{
		VIDEO_UpdateSurfacePalette(dest);
	}

	return dest;
}

SDL_Surface *
VIDEO_DuplicateSurface(
	SDL_Surface    *pSource,
	const SDL_Rect *pRect
)
/*++
  Purpose:

    Duplicate the selected area from the source surface into new surface.

  Parameters:

    [IN]  pSource - the source surface.
	[IN]  pRect   - the area to be duplicated, NULL for entire surface.

  Return value:

    None.

--*/
{
	SDL_Surface* dest = VIDEO_CreateCompatibleSizedSurface(pSource, pRect);

	if (dest)
	{
		VIDEO_CopySurface(pSource, pRect, dest, NULL);
	}

	return dest;
}

void
VIDEO_UpdateSurfacePalette(
	SDL_Surface    *pSurface
)
/*++
  Purpose:

    Use the global palette to update the palette of pSurface.

  Parameters:

    [IN]  pSurface - the surface whose palette should be updated.

  Return value:

    None.

--*/
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_SetSurfacePalette(pSurface, gpPalette);
#else
	if (gpPalette != NULL)
	{
		SDL_SetPalette(pSurface, SDL_PHYSPAL | SDL_LOGPAL, gpPalette->colors, 0, 256);
	}
#endif
}

VOID
VIDEO_DrawSurfaceToScreen(
    SDL_Surface    *pSurface
)
/*++
  Purpose:

    Draw a surface directly to screen.

  Parameters:

    [IN]  pSurface - the surface which needs to be drawn to screen.

  Return value:

    None.

--*/
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
   //
   // Draw the surface to screen.
   //
   if (g_bRenderPaused || g_bAppInBackground)
   {
      return;
   }
#if PAL_HAS_SDL_GPU
   if (gfUseGPU)
   {
      VIDEO_GPU_PresentSurface(pSurface);
      return;
   }
#endif
   SDL_BlitScaled(pSurface, NULL, gpScreenReal, NULL);
   VIDEO_ShowFrame();
#else
   SDL_Surface   *pCompatSurface;
   SDL_Rect       rect;

   rect.x = rect.y = 0;
   rect.w = pSurface->w;
   rect.h = pSurface->h;

   pCompatSurface = VIDEO_CreateCompatibleSizedSurface(gpScreenReal, &rect);

   //
   // First convert the surface to compatible format.
   //
   SDL_BlitSurface(pSurface, NULL, pCompatSurface, NULL);

   //
   // Draw the surface to screen.
   //
   SDL_SoftStretch(pCompatSurface, NULL, gpScreenReal, NULL);

   SDL_UpdateRect(gpScreenReal, 0, 0, gpScreenReal->w, gpScreenReal->h);
   SDL_FreeSurface(pCompatSurface);
#endif
}
