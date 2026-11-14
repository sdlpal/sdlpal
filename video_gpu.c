/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
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
// video_gpu.c: Renders the game through SDL_GPU, which means Vulkan or Direct3D 12 on
//              Windows, Vulkan on Linux and Android and Metal on macOS and iOS.
//
// The game is still 8-bit, but its surfaces live on the GPU: each one is an R8 texture
// holding palette indices, and its SDL_Surface has no pixels at all, so there is only one
// copy of its contents. Each software drawing routine has an exact GPU version here.
// Sprites and glyphs are decoded once into an atlas and drawn as instances in batches.
// Reads of the destination, which blending cannot express (shadows, the wave, cross-fades),
// go through a copy of the surface. FBP pictures and RNG frames are decoded on the CPU and
// only ever uploaded. The one thing read back is a screenshot being saved.
//
// Setting the environment variable SDLPAL_GPU_VERIFY=1 runs every drawing operation both
// ways. Only in this mode do surfaces get pixels, which hold the software result. Each
// presented frame is compared with it, as is the palette worked out during a fade, and any
// difference is logged as [GPUVERIFY]. Use it after changing any drawing code.
//
// The shaders are compiled to SPIR-V, MSL and, on Windows, DXIL, so SDL_GPU can pick any of
// its backends; on Windows it tries Vulkan before Direct3D 12. When none is usable
// VIDEO_GPU_Init() fails and video.c falls back to software drawing and the SDL renderer.
//

#include "main.h"

#if PAL_HAS_SDL_GPU

#include <stddef.h>

#include "video_gpu.h"
#include "video_gpu_shaders.h"

#ifdef VIDEO_GPU_HAS_DXIL
# define SHADER_FORMATS  (SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL)
# define DXIL_BLOB(name) name##_dxil, name##_dxil_len
#else
# define SHADER_FORMATS  (SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_MSL)
# define DXIL_BLOB(name) NULL, 0
#endif

#define SCREEN_WIDTH     320
#define SCREEN_HEIGHT    200
#define SCREEN_PIXELS    (SCREEN_WIDTH * SCREEN_HEIGHT)
#define PALETTE_ENTRIES  256
#define PALETTE_BYTES    (PALETTE_ENTRIES * 4)

#define MAX_SHADOW_AREAS 256
#define MAX_INSTANCES    8192
#define MAX_UPLOADS      1024
#define ATLAS_SIZE       2048
#define ATLAS_SLOTS      16384
#define STAGE_BYTES      (2 * 1024 * 1024)
#define RING_BYTES       (4 * 1024 * 1024)
#define VERTEX_RING      (MAX_INSTANCES * 8 * (int)sizeof(GPUINSTANCE))
#define MAX_BAND_RECTS   16
#define MAX_SHOWN_RECTS  (1 + 3 * MAX_BAND_RECTS)
#define PARTIAL_BYTES    (SCREEN_PIXELS * 2)

#define FNV_OFFSET       0xCBF29CE484222325ULL
#define FNV_PRIME        0x100000001B3ULL

enum { kFilterNone, kFilterReverse, kFilterCartoon, kFilterCrt };
enum { kScaleNearest = 0, kScaleLinear = 1, kScaleSharp = 2 };

// These values are shared with shaders/gpu/effect.frag.
enum { kEffectShiftBackground, kEffectWave, kEffectCrossFade, kEffectSwitch, kEffectReplace };

typedef struct tagGPUSURFACE
{
   SDL_Surface          *pSurface;
   SDL_GPUTexture       *pTexture;
   struct tagGPUSURFACE *pNext;
} GPUSURFACE;

typedef struct tagGPUINSTANCE
{
   Sint16 dstX, dstY;
   Uint16 w, h;
   Uint16 srcX, srcY;
   Uint8  mode, shift, color, unused;
} GPUINSTANCE;

typedef struct tagATLASENTRY
{
   Uint64 hash;  // 0 = free slot
   Uint16 x, y, w, h;
} ATLASENTRY;

typedef struct tagATLASUPLOAD
{
   Uint32 offset;
   Uint16 x, y, w, h;
} ATLASUPLOAD;

//
// Images go side by side on shelves, each as high as its highest image, and the shelves
// are stacked downwards in one part of the atlas.
//
typedef struct tagATLASSHELVES
{
   int left, bottom;   // the part: from column left to the right edge, down to row bottom
   int x, y, height;   // where the next image goes on the current shelf, and its height
} ATLASSHELVES;

typedef struct tagEFFECTPARAMS
{
   Sint32 mode, phase, pitch, shift, from, to, step, waveIndex;
   Sint32 wave[32];
} EFFECTPARAMS;

typedef struct tagFADEPARAMS
{
   Sint32 mode, step, color, unused;
} FADEPARAMS;

static struct
{
   SDL_GPUDevice           *pDevice;
   SDL_Window              *pWindow;
   SDL_GPUCommandBuffer    *pCommands;       // recording; submitted when a frame is presented
   BOOL                     fVerify;

   SDL_GPUGraphicsPipeline *pSpritePipeline;
   SDL_GPUGraphicsPipeline *pEffectPipeline;
   SDL_GPUGraphicsPipeline *pPalettePipeline;
   SDL_GPUGraphicsPipeline *pOverlayPipeline;
   SDL_GPUGraphicsPipeline *pSharpPipeline;
   SDL_GPUGraphicsPipeline *pFilterPipeline;
   SDL_GPUGraphicsPipeline *pFilterPrePipeline;
   SDL_GPUSampler          *pNearestSampler;
   SDL_GPUSampler          *pLinearSampler;

   SDL_GPUTexture          *pAtlas;          // RG8: palette index, opaque
   SDL_GPUTexture          *pScratch;        // copy of a surface, for reading while drawing to it
   SDL_GPUTexture          *pIndexTexture;   // what the player should see, as indices
   SDL_GPUTexture          *pPaletteTexture;
   SDL_GPUTexture          *pPaletteToTexture;
   SDL_GPUTexture          *pScreenTexture;  // the same after palette lookup
   SDL_GPUTexture          *pRGBTexture;     // a direct-color frame (AVI playback)
   SDL_GPUTexture          *pFilterTexture;  // output of a filter's first pass
   SDL_GPUTexture          *pOverlayTexture;
   Uint32                   uiRGBWidth, uiRGBHeight;
   Uint32                   uiFilterWidth, uiFilterHeight;

   SDL_GPUBuffer           *pVertexBuffer;
   Uint32                   uiVertexOffset;
   SDL_GPUTransferBuffer   *pRing;
   Uint32                   uiRingOffset;
   SDL_GPUTransferBuffer   *pDownload;

   GPUSURFACE              *pSurfaces;

   // The current batch: instances for one surface and the atlas updates they need.
   GPUSURFACE              *pBatchTarget;
   GPUINSTANCE             *pInstances;
   int                      nInstances;
   SDL_Rect                 batchBounds;     // what those instances cover
   SDL_Rect                 rgShadowAreas[MAX_SHADOW_AREAS];  // to copy for them to read
   int                      nShadowAreas;
   BYTE                    *pStage;
   Uint32                   uiStageUsed;
   ATLASUPLOAD             *pUploads;
   int                      nUploads;

   ATLASENTRY              *pAtlasEntries;
   int                      nAtlasEntries;
   ATLASSHELVES             rgShelves[2];    // beside the partial image, then below it
   Uint64                   qwOneOffImages;

   int                      iFilter;
   int                      iScale;
   SDL_GPUFilter            filter;          // for plain scaling
   float                    flOverlayAlpha;

   SDL_Color                rgPalette[PALETTE_ENTRIES];
   SDL_Color                rgPaletteTo[PALETTE_ENTRIES];
   FADEPARAMS               fade;
   BOOL                     fFadeArmed;      // keep the fade through the next palette change

   // What a shake left uncovered, not overlapping. It stays black until written over.
   SDL_Rect                 rgBlackBand[MAX_BAND_RECTS];
   int                      nBlackBand;

   // Verification only.
   BYTE                    *pExpectedIndex;
   BYTE                    *pExpectedBlack;  // nonzero where the software result is the black band
   SDL_Color                rgExpectedPalette[PALETTE_ENTRIES];
   DWORD                    dwFrames;
   DWORD                    dwIndexMismatches;
   DWORD                    dwColorMismatches;
   DWORD                    dwPaletteMismatches;
   DWORD                    dwDownloads;
   DWORD                    dwDownloadMismatches;
} gGPU;

static SDL_GPUCommandBuffer *
VIDEO_GPU_TryCommands(
   VOID
)
{
   if (gGPU.pCommands == NULL)
   {
      gGPU.pCommands = SDL_AcquireGPUCommandBuffer(gGPU.pDevice);
   }
   return gGPU.pCommands;
}

static SDL_GPUCommandBuffer *
VIDEO_GPU_Commands(
   VOID
)
{
   SDL_GPUCommandBuffer *pCommands = VIDEO_GPU_TryCommands();

   if (pCommands == NULL)
   {
      TerminateOnError("SDL_AcquireGPUCommandBuffer failed: %s\n", SDL_GetError());
   }
   return pCommands;
}

static VOID
VIDEO_GPU_Submit(
   VOID
)
/*++
  Purpose:

    Submit the recorded work. If that fails the device is most likely lost, and with it
    the surfaces, as their textures are the only copy. The game could only go on unseen.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   BOOL fSubmitted = SDL_SubmitGPUCommandBuffer(gGPU.pCommands);

   gGPU.pCommands = NULL;
   if (!fSubmitted)
   {
      TerminateOnError("SDL_SubmitGPUCommandBuffer failed: %s\n", SDL_GetError());
   }
}

static BOOL
VIDEO_GPU_SubmitAndWait(
   VOID
)
/*++
  Purpose:

    Submit the recorded work and wait for it, to read the results.

  Parameters:

    None.

  Return value:

    TRUE if the work is done, FALSE if it could not be submitted.

--*/
{
   SDL_GPUFence *pFence = SDL_SubmitGPUCommandBufferAndAcquireFence(gGPU.pCommands);

   gGPU.pCommands = NULL;
   if (pFence == NULL)
   {
      UTIL_LogOutput(LOGLEVEL_ERROR, "SDL_SubmitGPUCommandBufferAndAcquireFence failed: %s\n", SDL_GetError());
      return FALSE;
   }

   SDL_WaitForGPUFences(gGPU.pDevice, true, &pFence, 1);
   SDL_ReleaseGPUFence(gGPU.pDevice, pFence);
   return TRUE;
}

static SDL_GPUTexture *
VIDEO_GPU_CreateTexture(
   SDL_GPUTextureFormat     format,
   SDL_GPUTextureUsageFlags usage,
   Uint32                   uiWidth,
   Uint32                   uiHeight
)
{
   SDL_GPUTextureCreateInfo info;

   SDL_zero(info);
   info.type = SDL_GPU_TEXTURETYPE_2D;
   info.format = format;
   info.usage = usage;
   info.width = uiWidth;
   info.height = uiHeight;
   info.layer_count_or_depth = 1;
   info.num_levels = 1;
   info.sample_count = SDL_GPU_SAMPLECOUNT_1;

   return SDL_CreateGPUTexture(gGPU.pDevice, &info);
}

static BYTE *
VIDEO_GPU_RingMap(
   Uint32  uiSize,
   Uint32 *puiOffset
)
/*++
  Purpose:

    Reserve space in the upload ring. Regions still in use by the GPU are never touched:
    the ring only moves forward, and SDL_GPU replaces the buffer when it wraps around.

  Parameters:

    [IN]  uiSize - the number of bytes needed; must not exceed RING_BYTES.

    [OUT] puiOffset - offset of the reserved space within the ring.

  Return value:

    Where to write. The caller must call SDL_UnmapGPUTransferBuffer().

--*/
{
   BYTE *pMapped;
   BOOL  fWrap = (gGPU.uiRingOffset + uiSize > RING_BYTES);

   if (fWrap)
   {
      gGPU.uiRingOffset = 0;
   }

   pMapped = (BYTE *)SDL_MapGPUTransferBuffer(gGPU.pDevice, gGPU.pRing, fWrap);
   if (pMapped == NULL)
   {
      TerminateOnError("SDL_MapGPUTransferBuffer failed: %s\n", SDL_GetError());
   }

   *puiOffset = gGPU.uiRingOffset;
   gGPU.uiRingOffset = (gGPU.uiRingOffset + uiSize + 15) & ~15u;

   return pMapped + *puiOffset;
}

static VOID
VIDEO_GPU_UploadRegion(
   SDL_GPUCopyPass       *pCopyPass,
   SDL_GPUTransferBuffer *pBuffer,
   Uint32                 uiOffset,
   SDL_GPUTexture        *pTexture,
   Uint32                 x,
   Uint32                 y,
   Uint32                 w,
   Uint32                 h
)
{
   SDL_GPUTextureTransferInfo source;
   SDL_GPUTextureRegion       destination;

   SDL_zero(source);
   source.transfer_buffer = pBuffer;
   source.offset = uiOffset;
   source.pixels_per_row = w;
   source.rows_per_layer = h;

   SDL_zero(destination);
   destination.texture = pTexture;
   destination.x = x;
   destination.y = y;
   destination.w = w;
   destination.h = h;
   destination.d = 1;

   SDL_UploadToGPUTexture(pCopyPass, &source, &destination, false);
}

static VOID
VIDEO_GPU_FlushBatch(
   VOID
)
/*++
  Purpose:

    Record the pending atlas updates and instances. Everything that does not go through
    the batch must call this first, so that operations stay in drawing order.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   SDL_GPUCommandBuffer *pCommands;
   SDL_GPUCopyPass      *pCopyPass;
   BYTE                 *pMapped;
   Uint32                uiOffset, uiInstanceBytes;
   int                   i;

   if (gGPU.nInstances == 0 && gGPU.nUploads == 0 && gGPU.nShadowAreas == 0)
   {
      return;
   }

   pCommands = VIDEO_GPU_Commands();
   uiInstanceBytes = gGPU.nInstances * sizeof(GPUINSTANCE);

   pMapped = VIDEO_GPU_RingMap(gGPU.uiStageUsed + uiInstanceBytes, &uiOffset);
   memcpy(pMapped, gGPU.pStage, gGPU.uiStageUsed);
   memcpy(pMapped + gGPU.uiStageUsed, gGPU.pInstances, uiInstanceBytes);
   SDL_UnmapGPUTransferBuffer(gGPU.pDevice, gGPU.pRing);

   pCopyPass = SDL_BeginGPUCopyPass(pCommands);
   for (i = 0; i < gGPU.nUploads; i++)
   {
      const ATLASUPLOAD *pUpload = &gGPU.pUploads[i];
      VIDEO_GPU_UploadRegion(pCopyPass, gGPU.pRing, uiOffset + pUpload->offset, gGPU.pAtlas,
         pUpload->x, pUpload->y, pUpload->w, pUpload->h);
   }

   if (gGPU.nInstances > 0)
   {
      SDL_GPUTransferBufferLocation location;
      SDL_GPUBufferRegion           region;
      BOOL                          fWrap = (gGPU.uiVertexOffset + uiInstanceBytes > VERTEX_RING);

      if (fWrap)
      {
         gGPU.uiVertexOffset = 0;
      }

      location.transfer_buffer = gGPU.pRing;
      location.offset = uiOffset + gGPU.uiStageUsed;
      region.buffer = gGPU.pVertexBuffer;
      region.offset = gGPU.uiVertexOffset;
      region.size = uiInstanceBytes;

      SDL_UploadToGPUBuffer(pCopyPass, &location, &region, fWrap);
   }

   for (i = 0; i < gGPU.nShadowAreas; i++)
   {
      const SDL_Rect        *pArea = &gGPU.rgShadowAreas[i];
      SDL_GPUTextureLocation source, destination;

      SDL_zero(source);
      source.texture = gGPU.pBatchTarget->pTexture;
      source.x = pArea->x;
      source.y = pArea->y;

      destination = source;
      destination.texture = gGPU.pScratch;

      SDL_CopyGPUTextureToTexture(pCopyPass, &source, &destination, pArea->w, pArea->h, 1, false);
   }
   SDL_EndGPUCopyPass(pCopyPass);

   if (gGPU.nInstances > 0)
   {
      SDL_GPUColorTargetInfo        target;
      SDL_GPUBufferBinding          vertices;
      SDL_GPUTextureSamplerBinding  samplers[2];
      SDL_GPURenderPass            *pPass;
      float                         rgSize[2];

      SDL_zero(target);
      target.texture = gGPU.pBatchTarget->pTexture;
      target.load_op = SDL_GPU_LOADOP_LOAD;
      target.store_op = SDL_GPU_STOREOP_STORE;

      vertices.buffer = gGPU.pVertexBuffer;
      vertices.offset = gGPU.uiVertexOffset;

      samplers[0].texture = gGPU.pAtlas;
      samplers[0].sampler = gGPU.pNearestSampler;
      samplers[1].texture = gGPU.pScratch;
      samplers[1].sampler = gGPU.pNearestSampler;

      rgSize[0] = (float)gGPU.pBatchTarget->pSurface->w;
      rgSize[1] = (float)gGPU.pBatchTarget->pSurface->h;
      SDL_PushGPUVertexUniformData(pCommands, 0, rgSize, sizeof(rgSize));

      pPass = SDL_BeginGPURenderPass(pCommands, &target, 1, NULL);
      SDL_BindGPUGraphicsPipeline(pPass, gGPU.pSpritePipeline);
      SDL_BindGPUVertexBuffers(pPass, 0, &vertices, 1);
      SDL_BindGPUFragmentSamplers(pPass, 0, samplers, 2);
      SDL_DrawGPUPrimitives(pPass, 4, gGPU.nInstances, 0, 0);
      SDL_EndGPURenderPass(pPass);

      gGPU.uiVertexOffset += uiInstanceBytes;
   }

   gGPU.nInstances = 0;
   gGPU.nUploads = 0;
   gGPU.nShadowAreas = 0;
   gGPU.uiStageUsed = 0;
}

static GPUSURFACE *
VIDEO_GPU_FindSurface(
   SDL_Surface *pSurface
)
{
   GPUSURFACE *pEntry;

   for (pEntry = gGPU.pSurfaces; pEntry != NULL; pEntry = pEntry->pNext)
   {
      if (pEntry->pSurface == pSurface)
      {
         return pEntry;
      }
   }
   return NULL;
}

static VOID
VIDEO_GPU_CopyRegion(
   SDL_GPUTexture  *pSrc,
   const SDL_Rect  *pSrcRect,
   SDL_GPUTexture  *pDst,
   const SDL_Point *pDstPos
)
{
   SDL_GPUTextureLocation source, destination;
   SDL_GPUCopyPass       *pCopyPass;

   SDL_zero(source);
   source.texture = pSrc;
   source.x = pSrcRect->x;
   source.y = pSrcRect->y;

   SDL_zero(destination);
   destination.texture = pDst;
   destination.x = pDstPos->x;
   destination.y = pDstPos->y;

   pCopyPass = SDL_BeginGPUCopyPass(VIDEO_GPU_Commands());
   SDL_CopyGPUTextureToTexture(pCopyPass, &source, &destination, pSrcRect->w, pSrcRect->h, 1, false);
   SDL_EndGPUCopyPass(pCopyPass);
}

static VOID
VIDEO_GPU_CopyToScratch(
   GPUSURFACE     *pEntry,
   const SDL_Rect *pArea
)
/*++
  Purpose:

    Copy a screen-sized surface to the scratch texture, keeping everything in place.

  Parameters:

    [IN]  pEntry - the surface.

    [IN]  pArea - the area to copy, NULL for the whole surface.

  Return value:

    None.

--*/
{
   SDL_Rect  area;
   SDL_Point position;

   area.x = pArea ? pArea->x : 0;
   area.y = pArea ? pArea->y : 0;
   area.w = pArea ? pArea->w : SCREEN_WIDTH;
   area.h = pArea ? pArea->h : SCREEN_HEIGHT;
   position.x = area.x;
   position.y = area.y;

   VIDEO_GPU_CopyRegion(pEntry->pTexture, &area, gGPU.pScratch, &position);
}

static BOOL
VIDEO_GPU_IsScreenSized(
   const GPUSURFACE *pEntry
)
{
   return pEntry->pSurface->w == SCREEN_WIDTH && pEntry->pSurface->h == SCREEN_HEIGHT;
}

static BOOL
VIDEO_GPU_NotOnGPU(
   VOID
)
/*++
  Purpose:

    Work out what a drawing function returns when a surface it was given is not on the
    GPU. With the GPU in use every 8-bit surface is on it, so this can only be a NULL
    surface, which there is nothing to do with. Otherwise everything is on the CPU.

  Parameters:

    None.

  Return value:

    TRUE if there is nothing left to do. FALSE if the caller has to do it on the CPU.

--*/
{
   return gGPU.pDevice != NULL;
}

static BOOL
VIDEO_GPU_WrongSize(
   VOID
)
/*++
  Purpose:

    Give up on an effect because of the size of its surface. These work on a copy of the
    surface, which holds 320x200, and the game has no larger surfaces.

  Parameters:

    None.

  Return value:

    What the drawing function should return.

--*/
{
   UTIL_LogOutput(LOGLEVEL_ERROR, "This effect does not work on a surface of this size\n");
   return !gGPU.fVerify;
}

SDL_Surface *
VIDEO_GPU_CreateSurface(
   int             iWidth,
   int             iHeight
)
/*++
  Purpose:

    Create an 8-bit surface on the GPU. It has no pixels, unless every operation is being
    verified: its texture is the only copy of its contents.

  Parameters:

    [IN]  iWidth - width of the surface.

    [IN]  iHeight - height of the surface.

  Return value:

    The surface, which starts out as all zeros. NULL if it could not be created.

--*/
{
   GPUSURFACE            *pEntry;
   GPUSURFACE           **ppLast = &gGPU.pSurfaces;
   SDL_Surface           *pSurface;
   SDL_GPUCommandBuffer  *pCommands = VIDEO_GPU_TryCommands();
   SDL_GPUColorTargetInfo target;

   if (pCommands == NULL)
   {
      return NULL;
   }

   pEntry = (GPUSURFACE *)SDL_calloc(1, sizeof(GPUSURFACE));
   if (pEntry == NULL)
   {
      return NULL;
   }

   pSurface = gGPU.fVerify ? SDL_CreateSurface(iWidth, iHeight, SDL_PIXELFORMAT_INDEX8)
      : SDL_CreateSurfaceFrom(iWidth, iHeight, SDL_PIXELFORMAT_INDEX8, NULL, iWidth);
   if (pSurface == NULL)
   {
      SDL_free(pEntry);
      return NULL;
   }

   pEntry->pTexture = VIDEO_GPU_CreateTexture(SDL_GPU_TEXTUREFORMAT_R8_UNORM,
      SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET, iWidth, iHeight);
   if (pEntry->pTexture == NULL)
   {
      UTIL_LogOutput(LOGLEVEL_WARNING, "Surface texture could not be created: %s\n", SDL_GetError());
      SDL_DestroySurface(pSurface);
      SDL_free(pEntry);
      return NULL;
   }
   pEntry->pSurface = pSurface;

   while (*ppLast != NULL)
   {
      ppLast = &(*ppLast)->pNext;
   }
   *ppLast = pEntry;

   SDL_zero(target);
   target.texture = pEntry->pTexture;
   target.load_op = SDL_GPU_LOADOP_CLEAR;
   target.store_op = SDL_GPU_STOREOP_STORE;
   SDL_EndGPURenderPass(SDL_BeginGPURenderPass(pCommands, &target, 1, NULL));

   return pSurface;
}

VOID
VIDEO_GPU_UnregisterSurface(
   SDL_Surface    *pSurface
)
{
   GPUSURFACE **ppEntry = &gGPU.pSurfaces;

   while (*ppEntry != NULL && (*ppEntry)->pSurface != pSurface)
   {
      ppEntry = &(*ppEntry)->pNext;
   }

   if (*ppEntry != NULL)
   {
      GPUSURFACE *pEntry = *ppEntry;

      if (gGPU.pBatchTarget == pEntry)
      {
         VIDEO_GPU_FlushBatch();
         gGPU.pBatchTarget = NULL;
      }
      SDL_ReleaseGPUTexture(gGPU.pDevice, pEntry->pTexture);
      *ppEntry = pEntry->pNext;
      SDL_free(pEntry);
   }
}

BOOL
VIDEO_GPU_IsTarget(
   SDL_Surface    *pSurface
)
{
   return VIDEO_GPU_FindSurface(pSurface) != NULL;
}

static VOID
VIDEO_GPU_WaitForForeground(
   VOID
)
/*++
  Purpose:

    Wait until the app is back in the foreground, for work on the GPU that the caller
    cannot go on without. Using the GPU from the background gets an iOS app terminated,
    and the system suspends the app in here before long anyway.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   while (g_bAppInBackground)
   {
      SDL_PumpEvents();
      SDL_Delay(50);
   }
}

static VOID
VIDEO_GPU_CheckDownload(
   SDL_Surface    *pSurface,
   const BYTE     *pMapped
)
/*++
  Purpose:

    Verification: compare a surface downloaded from the GPU with its pixels, which hold
    the software result because the CPU drew everything as well.

  Parameters:

    [IN]  pSurface - the surface.

    [IN]  pMapped - what the GPU has for it.

  Return value:

    None.

--*/
{
   DWORD dwBad = 0;
   int   x, y, iFirstX = -1, iFirstY = -1;

   for (y = 0; y < pSurface->h; y++)
   {
      const BYTE *pRow = (const BYTE *)pSurface->pixels + y * pSurface->pitch;

      for (x = 0; x < pSurface->w; x++)
      {
         if (pRow[x] != pMapped[y * pSurface->w + x])
         {
            if (dwBad++ == 0)
            {
               iFirstX = x;
               iFirstY = y;
            }
         }
      }
   }

   gGPU.dwDownloads++;
   if (dwBad > 0 && ++gGPU.dwDownloadMismatches <= 40)
   {
      UTIL_LogOutput(LOGLEVEL_WARNING, "[GPUVERIFY] download %u: %u wrong pixels; first at (%d,%d): gpu index %d, cpu index %d\n",
         gGPU.dwDownloads, dwBad, iFirstX, iFirstY, pMapped[iFirstY * pSurface->w + iFirstX],
         ((const BYTE *)pSurface->pixels)[iFirstY * pSurface->pitch + iFirstX]);
   }
}

BOOL
VIDEO_GPU_ReadSurface(
   SDL_Surface    *pSurface,
   BYTE           *pDst,
   int             iPitch
)
/*++
  Purpose:

    Read a surface from the GPU. This has to wait for the GPU, so it is only for things
    like saving a screenshot. Nothing that happens during play needs it.

  Parameters:

    [IN]  pSurface - the surface.

    [OUT] pDst - receives the palette indices.

    [IN]  iPitch - bytes per row of pDst.

  Return value:

    TRUE if done. FALSE if the surface is not on the GPU.

--*/
{
   GPUSURFACE                *pEntry = VIDEO_GPU_FindSurface(pSurface);
   SDL_GPUTextureRegion       source;
   SDL_GPUTextureTransferInfo destination;
   SDL_GPUCopyPass           *pCopyPass;
   const BYTE                *pMapped;
   int                        y;

   if (pEntry == NULL)
   {
      return FALSE;
   }

   VIDEO_GPU_WaitForForeground();

   VIDEO_GPU_FlushBatch();

   SDL_zero(source);
   source.texture = pEntry->pTexture;
   source.w = pSurface->w;
   source.h = pSurface->h;
   source.d = 1;

   SDL_zero(destination);
   destination.transfer_buffer = gGPU.pDownload;
   destination.pixels_per_row = pSurface->w;
   destination.rows_per_layer = pSurface->h;

   pCopyPass = SDL_BeginGPUCopyPass(VIDEO_GPU_Commands());
   SDL_DownloadFromGPUTexture(pCopyPass, &source, &destination);
   SDL_EndGPUCopyPass(pCopyPass);

   if (!VIDEO_GPU_SubmitAndWait())
   {
      return FALSE;
   }

   pMapped = (const BYTE *)SDL_MapGPUTransferBuffer(gGPU.pDevice, gGPU.pDownload, false);
   if (pMapped == NULL)
   {
      UTIL_LogOutput(LOGLEVEL_ERROR, "SDL_MapGPUTransferBuffer failed: %s\n", SDL_GetError());
      return FALSE;
   }

   if (gGPU.fVerify)
   {
      VIDEO_GPU_CheckDownload(pSurface, pMapped);
   }
   for (y = 0; y < pSurface->h; y++)
   {
      memcpy(pDst + y * iPitch, pMapped + y * pSurface->w, pSurface->w);
   }
   SDL_UnmapGPUTransferBuffer(gGPU.pDevice, gGPU.pDownload);

   return TRUE;
}

BOOL
VIDEO_GPU_WriteImage(
   SDL_Surface    *pSurface,
   const BYTE     *pIndices
)
/*++
  Purpose:

    Replace all of a surface with an image decoded on the CPU.

  Parameters:

    [OUT] pSurface - the surface.

    [IN]  pIndices - the palette indices of the image, with no gaps between the rows.

  Return value:

    TRUE if done. FALSE if the caller still has to write them to the pixels of the surface.

--*/
{
   GPUSURFACE      *pEntry = VIDEO_GPU_FindSurface(pSurface);
   SDL_GPUCopyPass *pCopyPass;
   BYTE            *pMapped;
   Uint32           uiOffset, uiBytes;

   if (pEntry == NULL)
   {
      return FALSE;
   }

   VIDEO_GPU_FlushBatch();

   uiBytes = pSurface->w * pSurface->h;
   pMapped = VIDEO_GPU_RingMap(uiBytes, &uiOffset);
   memcpy(pMapped, pIndices, uiBytes);
   SDL_UnmapGPUTransferBuffer(gGPU.pDevice, gGPU.pRing);

   pCopyPass = SDL_BeginGPUCopyPass(VIDEO_GPU_Commands());
   VIDEO_GPU_UploadRegion(pCopyPass, gGPU.pRing, uiOffset, pEntry->pTexture, 0, 0, pSurface->w, pSurface->h);
   SDL_EndGPUCopyPass(pCopyPass);

   return !gGPU.fVerify;
}

BYTE *
VIDEO_GPU_BeginPartialImage(
   SDL_Surface    *pSurface
)
/*++
  Purpose:

    Start an image that changes only some pixels of a 320x200 surface, decoded on the CPU.
    The pixels it leaves alone keep what they have, like the transparent parts of a sprite,
    so this needs no knowledge of what the surface holds. It takes the top left 320x200
    of the atlas, which is kept free for it.

  Parameters:

    [IN]  pSurface - the surface.

  Return value:

    Where to write the image, cleared: 320x200 with two bytes per pixel, the palette index
    and then 0xFF for a pixel to change. NULL if the surface is not on the GPU.

--*/
{
   ATLASUPLOAD *pUpload;
   BYTE        *pPixels;

   if (VIDEO_GPU_FindSurface(pSurface) == NULL)
   {
      return NULL;
   }

   if (gGPU.uiStageUsed + PARTIAL_BYTES > STAGE_BYTES || gGPU.nUploads == MAX_UPLOADS)
   {
      VIDEO_GPU_FlushBatch();
   }

   pPixels = gGPU.pStage + gGPU.uiStageUsed;
   memset(pPixels, 0, PARTIAL_BYTES);

   pUpload = &gGPU.pUploads[gGPU.nUploads++];
   pUpload->offset = gGPU.uiStageUsed;
   pUpload->x = 0;
   pUpload->y = 0;
   pUpload->w = SCREEN_WIDTH;
   pUpload->h = SCREEN_HEIGHT;

   gGPU.uiStageUsed += PARTIAL_BYTES;

   return pPixels;
}

static VOID
VIDEO_GPU_AddInstance(
   GPUSURFACE *pTarget,
   int         x,
   int         y,
   int         w,
   int         h,
   int         iSrcX,
   int         iSrcY,
   int         iMode,
   int         iShift,
   int         iColor
)
{
   GPUINSTANCE *pInstance;

   if (gGPU.pBatchTarget != pTarget || gGPU.nInstances == MAX_INSTANCES)
   {
      VIDEO_GPU_FlushBatch();
      gGPU.pBatchTarget = pTarget;
   }

   if (gGPU.nInstances == 0)
   {
      gGPU.batchBounds.x = x;
      gGPU.batchBounds.y = y;
      gGPU.batchBounds.w = w;
      gGPU.batchBounds.h = h;
   }
   else
   {
      SDL_Rect rect;

      rect.x = x;
      rect.y = y;
      rect.w = w;
      rect.h = h;
      SDL_GetRectUnion(&gGPU.batchBounds, &rect, &gGPU.batchBounds);
   }

   pInstance = &gGPU.pInstances[gGPU.nInstances++];
   pInstance->dstX = (Sint16)x;
   pInstance->dstY = (Sint16)y;
   pInstance->w = (Uint16)w;
   pInstance->h = (Uint16)h;
   pInstance->srcX = (Uint16)iSrcX;
   pInstance->srcY = (Uint16)iSrcY;
   pInstance->mode = (Uint8)iMode;
   pInstance->shift = (Uint8)(iShift + 128);
   pInstance->color = (Uint8)iColor;
   pInstance->unused = 0;
}

VOID
VIDEO_GPU_EndPartialImage(
   SDL_Surface    *pSurface
)
/*++
  Purpose:

    Draw the image started by VIDEO_GPU_BeginPartialImage().

  Parameters:

    [OUT] pSurface - the surface.

  Return value:

    None.

--*/
{
   VIDEO_GPU_AddInstance(VIDEO_GPU_FindSurface(pSurface), 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, kGPUDrawNormal, 0, 0);

   //
   // The next image goes to the same part of the atlas.
   //
   VIDEO_GPU_FlushBatch();
}

static VOID
VIDEO_GPU_AtlasRewind(
   VOID
)
/*++
  Purpose:

    Make the whole atlas free for images again, except its top left 320x200, which a
    partial image takes. The rows to the right of it and the rows below it are filled
    with shelves of their own.

--*/
{
   static const ATLASSHELVES rgEmpty[2] =
   {
      { SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH, 0,             0 },
      { 0,            ATLAS_SIZE,    0,            SCREEN_HEIGHT, 0 },
   };

   memcpy(gGPU.rgShelves, rgEmpty, sizeof(rgEmpty));
}

static BOOL
VIDEO_GPU_AtlasPlace(
   int  w,
   int  h,
   int *px,
   int *py
)
/*++
  Purpose:

    Find room for an image in the atlas: on the current shelf of a part, or on a new
    shelf below it. The rows beside the partial image are tried first.

  Parameters:

    [IN]  w - width of the image.

    [IN]  h - height of the image.

    [OUT] px, py - where the image goes.

  Return value:

    TRUE if it was placed, FALSE if the atlas is full.

--*/
{
   int i;

   for (i = 0; i < 2; i++)
   {
      ATLASSHELVES *p = &gGPU.rgShelves[i];

      if (p->x + w > ATLAS_SIZE)
      {
         if (p->left + w > ATLAS_SIZE || p->y + p->height + h > p->bottom)
         {
            continue;
         }
         p->x = p->left;
         p->y += p->height;
         p->height = 0;
      }
      else if (p->y + h > p->bottom)
      {
         continue;
      }

      *px = p->x;
      *py = p->y;
      p->x += w;
      if (h > p->height)
      {
         p->height = h;
      }
      return TRUE;
   }

   return FALSE;
}

static VOID
VIDEO_GPU_AtlasReset(
   VOID
)
{
   //
   // Drawing already recorded keeps its pixels: uploads run in command order.
   //
   VIDEO_GPU_FlushBatch();
   memset(gGPU.pAtlasEntries, 0, ATLAS_SLOTS * sizeof(ATLASENTRY));
   gGPU.nAtlasEntries = 0;
   VIDEO_GPU_AtlasRewind();
}

static ATLASENTRY *
VIDEO_GPU_AtlasFind(
   Uint64 hash
)
{
   int i = (int)(hash & (ATLAS_SLOTS - 1));

   while (gGPU.pAtlasEntries[i].hash != 0 && gGPU.pAtlasEntries[i].hash != hash)
   {
      i = (i + 1) & (ATLAS_SLOTS - 1);
   }
   return &gGPU.pAtlasEntries[i];
}

static BYTE *
VIDEO_GPU_AtlasAdd(
   Uint64        hash,
   int           w,
   int           h,
   ATLASENTRY  **ppEntry
)
/*++
  Purpose:

    Reserve room for a new image in the atlas, clearing the atlas if it is full.

  Parameters:

    [IN]  hash - identifies the image.

    [IN]  w - width of the image, at most ATLAS_SIZE. All its pixels must fit in STAGE_BYTES.

    [IN]  h - height of the image, at most ATLAS_SIZE - SCREEN_HEIGHT.

    [OUT] ppEntry - the new atlas entry.

  Return value:

    Where to write the 2-byte pixels of the image, already zeroed (transparent).

--*/
{
   ATLASUPLOAD *pUpload;
   ATLASENTRY  *pEntry;
   Uint32       uiBytes = w * h * 2;
   BYTE        *pPixels;
   int          x, y;

   if (gGPU.nAtlasEntries >= ATLAS_SLOTS / 2 || !VIDEO_GPU_AtlasPlace(w, h, &x, &y))
   {
      //
      // An empty atlas always has room: below the partial image it is ATLAS_SIZE wide and
      // ATLAS_SIZE - SCREEN_HEIGHT high.
      //
      VIDEO_GPU_AtlasReset();
      VIDEO_GPU_AtlasPlace(w, h, &x, &y);
   }

   if (gGPU.uiStageUsed + uiBytes > STAGE_BYTES || gGPU.nUploads == MAX_UPLOADS)
   {
      VIDEO_GPU_FlushBatch();
   }

   pEntry = VIDEO_GPU_AtlasFind(hash);
   pEntry->hash = hash;
   pEntry->x = (Uint16)x;
   pEntry->y = (Uint16)y;
   pEntry->w = (Uint16)w;
   pEntry->h = (Uint16)h;
   gGPU.nAtlasEntries++;

   pPixels = gGPU.pStage + gGPU.uiStageUsed;
   memset(pPixels, 0, uiBytes);

   pUpload = &gGPU.pUploads[gGPU.nUploads++];
   pUpload->offset = gGPU.uiStageUsed;
   pUpload->x = pEntry->x;
   pUpload->y = pEntry->y;
   pUpload->w = pEntry->w;
   pUpload->h = pEntry->h;

   gGPU.uiStageUsed += uiBytes;

   *ppEntry = pEntry;
   return pPixels;
}

static const ATLASENTRY *
VIDEO_GPU_CacheRLE(
   LPCBITMAPRLE lpBitmapRLE,
   const BYTE  *pData,
   UINT         uiWidth,
   UINT         uiRows
)
/*++
  Purpose:

    Find an RLE bitmap in the atlas, decoding it first if needed. Bitmaps are identified
    by their content, because the buffers holding them get reused for other sprites.

  Parameters:

    [IN]  lpBitmapRLE - the bitmap.

    [IN]  pData - where its pixel data starts.

    [IN]  uiWidth - its width, not zero.

    [IN]  uiRows - how many of its rows to use, not zero. Like the software routines, this
          reads no further than the bottom edge of the surface.

  Return value:

    The atlas entry. NULL if the bitmap cannot fit.

--*/
{
   const BYTE *p;
   ATLASENTRY *pEntry;
   BYTE       *pPixels;
   Uint64      hash = (FNV_OFFSET ^ uiRows) * FNV_PRIME;
   UINT        uiLen = uiWidth * uiRows, i, j, n;
   BYTE        T;

   if (uiWidth > ATLAS_SIZE || uiRows > ATLAS_SIZE - SCREEN_HEIGHT || uiLen * 2 > STAGE_BYTES)
   {
      return NULL;
   }

   for (p = lpBitmapRLE; p < pData; p++)
   {
      hash = (hash ^ *p) * FNV_PRIME;
   }

   for (i = 0; i < uiLen;)
   {
      T = *p++;
      hash = (hash ^ T) * FNV_PRIME;
      if ((T & 0x80) && T <= 0x80 + uiWidth)
      {
         i += T - 0x80;
      }
      else
      {
         n = SDL_min((UINT)T, uiLen - i);
         for (j = 0; j < n; j++)
         {
            hash = (hash ^ p[j]) * FNV_PRIME;
         }
         p += T;
         i += T;
      }
   }
   hash |= 1;

   pEntry = VIDEO_GPU_AtlasFind(hash);
   if (pEntry->hash == hash)
   {
      return pEntry;
   }

   pPixels = VIDEO_GPU_AtlasAdd(hash, uiWidth, uiRows, &pEntry);

   p = pData;
   for (i = 0; i < uiLen;)
   {
      T = *p++;
      if ((T & 0x80) && T <= 0x80 + uiWidth)
      {
         i += T - 0x80;
      }
      else
      {
         for (j = 0; j < T; j++)
         {
            if (i + j < uiLen)
            {
               pPixels[(i + j) * 2] = p[j];
               pPixels[(i + j) * 2 + 1] = 0xFF;
            }
         }
         p += T;
         i += T;
      }
   }

   return pEntry;
}

static const ATLASENTRY *
VIDEO_GPU_DecodeVisiblePart(
   const BYTE     *pData,
   UINT            uiWidth,
   UINT            uiHeight,
   const SDL_Rect *pVisible
)
/*++
  Purpose:

    Decode the part of a bitmap that shows, for a bitmap too big for the atlas. Surfaces
    are small, so that part always fits. It is good for one draw and is not looked up again.

  Parameters:

    [IN]  pData - where the pixel data of the bitmap starts.

    [IN]  uiWidth - width of the bitmap.

    [IN]  uiHeight - height of the bitmap.

    [IN]  pVisible - the part to decode, in pixels of the bitmap.

  Return value:

    The atlas entry.

--*/
{
   const BYTE *p = pData;
   ATLASENTRY *pEntry;
   BYTE       *pPixels;
   Uint64      hash = ((++gGPU.qwOneOffImages * FNV_PRIME) ^ 0x1C0DEC0DE0000ULL) | 1;
   UINT        uiEnd = uiWidth * (UINT)(pVisible->y + pVisible->h);
   UINT        uiLen = uiWidth * uiHeight, i, j;
   BYTE        T;

   pPixels = VIDEO_GPU_AtlasAdd(hash, pVisible->w, pVisible->h, &pEntry);

   //
   // Like the software routines, this stops at the bottom edge of the surface.
   //
   for (i = 0; i < uiLen && i < uiEnd;)
   {
      T = *p++;
      if ((T & 0x80) && T <= 0x80 + uiWidth)
      {
         i += T - 0x80;
         continue;
      }

      for (j = 0; j < T; j++)
      {
         int x = (int)((i + j) % uiWidth) - pVisible->x;
         int y = (int)((i + j) / uiWidth) - pVisible->y;

         if (i + j < uiLen && x >= 0 && x < pVisible->w && y >= 0 && y < pVisible->h)
         {
            pPixels[(y * pVisible->w + x) * 2] = p[j];
            pPixels[(y * pVisible->w + x) * 2 + 1] = 0xFF;
         }
      }
      p += T;
      i += T;
   }

   return pEntry;
}

BOOL
VIDEO_GPU_DrawRLE(
   LPCBITMAPRLE    lpBitmapRLE,
   SDL_Surface    *pDst,
   PAL_POS         pos,
   INT             iMode,
   INT             iColorShift,
   BYTE            bColor
)
/*++
  Purpose:

    Draw an RLE bitmap on a surface.

  Parameters:

    [IN]  lpBitmapRLE - the bitmap.

    [OUT] pDst - the surface.

    [IN]  pos - where to draw it.

    [IN]  iMode - one of the kGPUDraw values.

    [IN]  iColorShift - brightness change, for kGPUDrawColorShift and kGPUDrawMonoColor.

    [IN]  bColor - the color to use, for kGPUDrawMonoColor.

  Return value:

    TRUE if done. FALSE if the caller still has to draw it on the CPU.

--*/
{
   GPUSURFACE       *pTarget = VIDEO_GPU_FindSurface(pDst);
   const ATLASENTRY *pEntry;
   const BYTE       *pData = lpBitmapRLE;
   UINT              uiWidth = PAL_RLEGetWidth(lpBitmapRLE);
   UINT              uiHeight = PAL_RLEGetHeight(lpBitmapRLE);
   int               x = PAL_X(pos), y = PAL_Y(pos);

   if (pTarget == NULL)
   {
      return VIDEO_GPU_NotOnGPU();
   }

   //
   // Nothing to draw. Like the software routines, this does not read the pixel data of a
   // bitmap that misses the surface, which also keeps it out of the atlas.
   //
   if (uiWidth == 0 || uiHeight == 0 || x + (int)uiWidth <= 0 || x >= pDst->w ||
       y + (int)uiHeight <= 0 || y >= pDst->h)
   {
      return !gGPU.fVerify;
   }

   if (iMode == kGPUDrawShadow && (pDst->w > SCREEN_WIDTH || pDst->h > SCREEN_HEIGHT))
   {
      return VIDEO_GPU_WrongSize();
   }

   //
   // The pixel data comes after the size, and after 0x00000002 in the bitmaps that start
   // with that.
   //
   pData += (pData[0] == 0x02 && pData[1] == 0x00 && pData[2] == 0x00 && pData[3] == 0x00) ? 8 : 4;

   pEntry = VIDEO_GPU_CacheRLE(lpBitmapRLE, pData, uiWidth, SDL_min(uiHeight, (UINT)(pDst->h - y)));
   if (pEntry == NULL)
   {
      SDL_Rect visible;

      visible.x = SDL_max(-x, 0);
      visible.y = SDL_max(-y, 0);
      visible.w = SDL_min((int)uiWidth - visible.x, pDst->w - SDL_max(x, 0));
      visible.h = SDL_min((int)uiHeight - visible.y, pDst->h - SDL_max(y, 0));

      pEntry = VIDEO_GPU_DecodeVisiblePart(pData, uiWidth, uiHeight, &visible);
      x = SDL_max(x, 0);
      y = SDL_max(y, 0);
   }

   //
   // The result stays the same past this range: it is clamped to 0..15 in the end.
   //
   iColorShift = SDL_clamp(iColorShift, -16, 16);

   if (iMode == kGPUDrawShadow)
   {
      //
      // A shadow darkens what is already there, so it reads a copy of the area it covers.
      // That copy gets taken when the batch is recorded, before any of the drawing in the
      // batch, so it has to be a new batch if that drawing touches this area.
      //
      SDL_Rect area;

      area.x = SDL_max(x, 0);
      area.y = SDL_max(y, 0);
      area.w = SDL_min(x + pEntry->w, pDst->w) - area.x;
      area.h = SDL_min(y + pEntry->h, pDst->h) - area.y;

      if (gGPU.pBatchTarget != pTarget || gGPU.nShadowAreas == MAX_SHADOW_AREAS ||
          (gGPU.nInstances > 0 && SDL_HasRectIntersection(&area, &gGPU.batchBounds)))
      {
         VIDEO_GPU_FlushBatch();
         gGPU.pBatchTarget = pTarget;
      }
      gGPU.rgShadowAreas[gGPU.nShadowAreas++] = area;
   }

   VIDEO_GPU_AddInstance(pTarget, x, y, pEntry->w, pEntry->h,
      pEntry->x, pEntry->y, iMode, iColorShift, bColor);

   return !gGPU.fVerify;
}

static Uint64
VIDEO_GPU_GlyphHash(
   DWORD           dwKey,
   INT             iWidth,
   INT             iHeight
)
{
   //
   // Glyphs share the table with bitmaps. An odd hash of their own keeps them apart.
   //
   return ((((Uint64)dwKey + 1) * FNV_PRIME) ^ ((Uint64)iWidth << 48) ^ ((Uint64)iHeight << 56) ^ 0x5A5A5A5A0000ULL) | 1;
}

BOOL
VIDEO_GPU_IsGlyphCached(
   DWORD           dwKey,
   INT             iWidth,
   INT             iHeight
)
{
   Uint64 hash = VIDEO_GPU_GlyphHash(dwKey, iWidth, iHeight);

   return VIDEO_GPU_AtlasFind(hash)->hash == hash;
}

BOOL
VIDEO_GPU_DrawGlyph(
   SDL_Surface    *pDst,
   DWORD           dwKey,
   const BYTE     *pMask,
   INT             iWidth,
   INT             iHeight,
   INT             x,
   INT             y,
   INT             iSrcX,
   INT             iSrcWidth,
   BYTE            bColor
)
/*++
  Purpose:

    Draw part of a font glyph on a surface.

  Parameters:

    [OUT] pDst - the surface.

    [IN]  dwKey - identifies the glyph.

    [IN]  pMask - the glyph, one byte per pixel: nonzero where it is solid. Only read if
          VIDEO_GPU_IsGlyphCached() says the glyph is not in the atlas yet.

    [IN]  iWidth - width of the glyph.

    [IN]  iHeight - height of the glyph.

    [IN]  x - where to draw.

    [IN]  y - where to draw.

    [IN]  iSrcX - leftmost column of the glyph to draw.

    [IN]  iSrcWidth - how many columns to draw.

    [IN]  bColor - the color.

  Return value:

    TRUE if done. FALSE if the caller still has to draw it on the CPU.

--*/
{
   GPUSURFACE       *pTarget = VIDEO_GPU_FindSurface(pDst);
   const ATLASENTRY *pEntry;
   ATLASENTRY       *pSlot;
   Uint64            hash = VIDEO_GPU_GlyphHash(dwKey, iWidth, iHeight);

   if (pTarget == NULL)
   {
      return VIDEO_GPU_NotOnGPU();
   }

   pSlot = VIDEO_GPU_AtlasFind(hash);
   if (pSlot->hash != hash)
   {
      BYTE *pPixels = VIDEO_GPU_AtlasAdd(hash, iWidth, iHeight, &pSlot);
      int   i;

      for (i = 0; i < iWidth * iHeight; i++)
      {
         pPixels[i * 2 + 1] = pMask[i] ? 0xFF : 0;
      }
   }
   pEntry = pSlot;

   VIDEO_GPU_AddInstance(pTarget, x, y, iSrcWidth, pEntry->h, pEntry->x + iSrcX, pEntry->y,
      kGPUDrawGlyph, 0, bColor);

   return !gGPU.fVerify;
}

BOOL
VIDEO_GPU_Fill(
   SDL_Surface    *pDst,
   const SDL_Rect *pRect,
   BYTE            bColor
)
{
   GPUSURFACE *pTarget = VIDEO_GPU_FindSurface(pDst);

   if (pTarget == NULL)
   {
      return VIDEO_GPU_NotOnGPU();
   }

   if (pRect == NULL)
   {
      VIDEO_GPU_AddInstance(pTarget, 0, 0, pDst->w, pDst->h, 0, 0, kGPUDrawFill, 0, bColor);
   }
   else if (pRect->w > 0 && pRect->h > 0)
   {
      VIDEO_GPU_AddInstance(pTarget, pRect->x, pRect->y, pRect->w, pRect->h, 0, 0, kGPUDrawFill, 0, bColor);
   }

   return !gGPU.fVerify;
}

static BOOL
VIDEO_GPU_ClipCopy(
   int             iSrcW,
   int             iSrcH,
   const SDL_Rect *pSrcRect,
   int             iDstW,
   int             iDstH,
   const SDL_Rect *pDstRect,
   SDL_Rect       *pSrcOut,
   SDL_Point      *pDstOut
)
/*++
  Purpose:

    Clip a copy between two surfaces in the same way that SDL_BlitSurface() does.

  Parameters:

    [IN]  iSrcW - width of the source.

    [IN]  iSrcH - height of the source.

    [IN]  pSrcRect - the area to copy, NULL for the whole source.

    [IN]  iDstW - width of the destination.

    [IN]  iDstH - height of the destination.

    [IN]  pDstRect - where to copy to, only the position is used; NULL for the top left.

    [OUT] pSrcOut - the clipped area to copy.

    [OUT] pDstOut - the clipped position to copy to.

  Return value:

    TRUE if anything is left to copy.

--*/
{
   int sx = pSrcRect ? pSrcRect->x : 0;
   int sy = pSrcRect ? pSrcRect->y : 0;
   int w  = pSrcRect ? pSrcRect->w : iSrcW;
   int h  = pSrcRect ? pSrcRect->h : iSrcH;
   int dx = pDstRect ? pDstRect->x : 0;
   int dy = pDstRect ? pDstRect->y : 0;

   if (sx < 0) { w += sx; dx -= sx; sx = 0; }
   if (sy < 0) { h += sy; dy -= sy; sy = 0; }
   if (dx < 0) { w += dx; sx -= dx; dx = 0; }
   if (dy < 0) { h += dy; sy -= dy; dy = 0; }
   if (w > iSrcW - sx) w = iSrcW - sx;
   if (h > iSrcH - sy) h = iSrcH - sy;
   if (w > iDstW - dx) w = iDstW - dx;
   if (h > iDstH - dy) h = iDstH - dy;

   pSrcOut->x = sx;
   pSrcOut->y = sy;
   pSrcOut->w = w;
   pSrcOut->h = h;
   pDstOut->x = dx;
   pDstOut->y = dy;

   return w > 0 && h > 0;
}

BOOL
VIDEO_GPU_Blit(
   SDL_Surface    *pSrc,
   const SDL_Rect *pSrcRect,
   SDL_Surface    *pDst,
   const SDL_Rect *pDstRect
)
{
   GPUSURFACE *pSource = VIDEO_GPU_FindSurface(pSrc);
   GPUSURFACE *pTarget = VIDEO_GPU_FindSurface(pDst);
   SDL_Rect    area;
   SDL_Point   position;

   if (pSource == NULL || pTarget == NULL)
   {
      return VIDEO_GPU_NotOnGPU();
   }

   if (VIDEO_GPU_ClipCopy(pSrc->w, pSrc->h, pSrcRect, pDst->w, pDst->h, pDstRect, &area, &position))
   {
      VIDEO_GPU_FlushBatch();
      VIDEO_GPU_CopyRegion(pSource->pTexture, &area, pTarget->pTexture, &position);
   }

   return !gGPU.fVerify;
}

static VOID
VIDEO_GPU_RunEffect(
   GPUSURFACE         *pTarget,
   SDL_GPUTexture     *pFirst,
   SDL_GPUTexture     *pSecond,
   const EFFECTPARAMS *pParams
)
{
   SDL_GPUCommandBuffer         *pCommands = VIDEO_GPU_Commands();
   SDL_GPUColorTargetInfo        target;
   SDL_GPUTextureSamplerBinding  samplers[2];
   SDL_GPURenderPass            *pPass;

   SDL_zero(target);
   target.texture = pTarget->pTexture;
   target.load_op = SDL_GPU_LOADOP_LOAD;
   target.store_op = SDL_GPU_STOREOP_STORE;

   samplers[0].texture = pFirst;
   samplers[0].sampler = gGPU.pNearestSampler;
   samplers[1].texture = pSecond;
   samplers[1].sampler = gGPU.pNearestSampler;

   SDL_PushGPUFragmentUniformData(pCommands, 0, pParams, sizeof(*pParams));

   pPass = SDL_BeginGPURenderPass(pCommands, &target, 1, NULL);
   SDL_BindGPUGraphicsPipeline(pPass, gGPU.pEffectPipeline);
   SDL_BindGPUFragmentSamplers(pPass, 0, samplers, 2);
   SDL_DrawGPUPrimitives(pPass, 3, 1, 0, 0);
   SDL_EndGPURenderPass(pPass);
}

BOOL
VIDEO_GPU_ShiftBackground(
   SDL_Surface    *pSrc,
   SDL_Surface    *pDst,
   INT             iShift
)
{
   GPUSURFACE  *pSource = VIDEO_GPU_FindSurface(pSrc);
   GPUSURFACE  *pTarget = VIDEO_GPU_FindSurface(pDst);
   EFFECTPARAMS params;

   if (pSource == NULL || pTarget == NULL)
   {
      return VIDEO_GPU_NotOnGPU();
   }

   VIDEO_GPU_FlushBatch();

   SDL_zero(params);
   params.mode = kEffectShiftBackground;
   params.shift = iShift;
   VIDEO_GPU_RunEffect(pTarget, pSource->pTexture, pSource->pTexture, &params);

   return !gGPU.fVerify;
}

BOOL
VIDEO_GPU_ApplyWave(
   SDL_Surface    *pSurface,
   const int       rgWave[32],
   INT             iIndex
)
{
   GPUSURFACE  *pTarget = VIDEO_GPU_FindSurface(pSurface);
   EFFECTPARAMS params;
   int          i;

   if (pTarget == NULL)
   {
      return VIDEO_GPU_NotOnGPU();
   }

   if (!VIDEO_GPU_IsScreenSized(pTarget))
   {
      return VIDEO_GPU_WrongSize();
   }

   VIDEO_GPU_FlushBatch();
   VIDEO_GPU_CopyToScratch(pTarget, NULL);

   SDL_zero(params);
   params.mode = kEffectWave;
   params.waveIndex = iIndex;
   for (i = 0; i < 32; i++)
   {
      params.wave[i] = rgWave[i];
   }
   VIDEO_GPU_RunEffect(pTarget, gGPU.pScratch, gGPU.pScratch, &params);

   return !gGPU.fVerify;
}

BOOL
VIDEO_GPU_CrossFadeStep(
   SDL_Surface    *pFrom,
   SDL_Surface    *pDst,
   INT             iPhase,
   BOOL            fStep
)
{
   GPUSURFACE  *pSource = VIDEO_GPU_FindSurface(pFrom);
   GPUSURFACE  *pTarget = VIDEO_GPU_FindSurface(pDst);
   EFFECTPARAMS params;

   if (pSource == NULL || pTarget == NULL)
   {
      return VIDEO_GPU_NotOnGPU();
   }

   if (!VIDEO_GPU_IsScreenSized(pTarget))
   {
      return VIDEO_GPU_WrongSize();
   }

   VIDEO_GPU_FlushBatch();
   VIDEO_GPU_CopyToScratch(pTarget, NULL);

   SDL_zero(params);
   params.mode = kEffectCrossFade;
   params.phase = iPhase;
   params.pitch = pDst->pitch;
   params.step = fStep ? 1 : 0;
   VIDEO_GPU_RunEffect(pTarget, pSource->pTexture, gGPU.pScratch, &params);

   return !gGPU.fVerify;
}

BOOL
VIDEO_GPU_SwitchStep(
   SDL_Surface    *pFrom,
   SDL_Surface    *pDst,
   INT             iPhase
)
{
   GPUSURFACE  *pSource = VIDEO_GPU_FindSurface(pFrom);
   GPUSURFACE  *pTarget = VIDEO_GPU_FindSurface(pDst);
   EFFECTPARAMS params;

   if (pSource == NULL || pTarget == NULL)
   {
      return VIDEO_GPU_NotOnGPU();
   }

   VIDEO_GPU_FlushBatch();

   SDL_zero(params);
   params.mode = kEffectSwitch;
   params.phase = iPhase;
   params.pitch = pDst->pitch;
   VIDEO_GPU_RunEffect(pTarget, pSource->pTexture, pSource->pTexture, &params);

   return !gGPU.fVerify;
}

BOOL
VIDEO_GPU_ReplaceIndex(
   SDL_Surface    *pSurface,
   BYTE            bFrom,
   BYTE            bTo
)
{
   GPUSURFACE  *pTarget = VIDEO_GPU_FindSurface(pSurface);
   EFFECTPARAMS params;

   if (pTarget == NULL)
   {
      return VIDEO_GPU_NotOnGPU();
   }

   if (!VIDEO_GPU_IsScreenSized(pTarget))
   {
      return VIDEO_GPU_WrongSize();
   }

   VIDEO_GPU_FlushBatch();
   VIDEO_GPU_CopyToScratch(pTarget, NULL);

   SDL_zero(params);
   params.mode = kEffectReplace;
   params.from = bFrom;
   params.to = bTo;
   VIDEO_GPU_RunEffect(pTarget, gGPU.pScratch, gGPU.pScratch, &params);

   return !gGPU.fVerify;
}

BOOL
VIDEO_GPU_OnSetPalette(
   const SDL_Color *rgPalette
)
/*++
  Purpose:

    Take over a palette that was just set, which also ends any fade.

  Parameters:

    [IN]  rgPalette - the 256 colors.

  Return value:

    TRUE to go on and set this palette. FALSE if it is only the software result of the
    fade step just set up on the GPU, kept to compare the GPU result with when verifying.
    A normal run leaves the palette of the screen alone during a fade, so this does too.

--*/
{
   if (gGPU.fVerify)
   {
      memcpy(gGPU.rgExpectedPalette, rgPalette, PALETTE_BYTES);
   }

   if (gGPU.fFadeArmed)
   {
      gGPU.fFadeArmed = FALSE;
      return FALSE;
   }

   memcpy(gGPU.rgPalette, rgPalette, PALETTE_BYTES);
   SDL_zero(gGPU.fade);
   return TRUE;
}

BOOL
VIDEO_GPU_SetPaletteFade(
   INT              iMode,
   const SDL_Color *rgPalette,
   const SDL_Color *rgPaletteTo,
   INT              iStep,
   BYTE             bColor
)
/*++
  Purpose:

    Set up one step of a palette fade, which the GPU works out during palette lookup.

  Parameters:

    [IN]  iMode - one of the kPaletteFade values.

    [IN]  rgPalette - the palette to fade.

    [IN]  rgPaletteTo - the palette to fade to, for kPaletteFadeBlend.

    [IN]  iStep - how far the fade has gone; the meaning depends on the mode.

    [IN]  bColor - the palette entry to fade from or to, where the mode has one.

  Return value:

    TRUE if done. FALSE if the caller still has to set the resulting palette, which is
    only for comparing it when verifying.

--*/
{
   memcpy(gGPU.rgPalette, rgPalette, PALETTE_BYTES);
   if (rgPaletteTo != NULL)
   {
      memcpy(gGPU.rgPaletteTo, rgPaletteTo, PALETTE_BYTES);
   }

   gGPU.fade.mode = iMode;
   gGPU.fade.step = iStep;
   gGPU.fade.color = bColor;
   gGPU.fFadeArmed = gGPU.fVerify;

   return !gGPU.fVerify;
}

static int
VIDEO_GPU_Toward(
   int iFrom,
   int iTo,
   int iLimit
)
{
   int iDelta = iTo - iFrom;
   return iFrom + ((iDelta > iLimit) ? iLimit : ((iDelta < -iLimit) ? -iLimit : iDelta));
}

VOID
VIDEO_GPU_ResolvePalette(
   SDL_Color       *rgPalette
)
/*++
  Purpose:

    Write out the palette in effect, when code on the CPU wants to know it during or
    right after a fade. Mirrors the calculation in shaders/gpu/palette.frag.

  Parameters:

    [OUT] rgPalette - receives the 256 colors, if a fade is in progress.

  Return value:

    None.

--*/
{
   const FADEPARAMS *f = &gGPU.fade;
   int               i;

   if (f->mode == kPaletteFadeNone)
   {
      return;
   }

   for (i = 0; i < PALETTE_ENTRIES; i++)
   {
      const SDL_Color *a = &gGPU.rgPalette[i];
      const SDL_Color *b = &gGPU.rgPaletteTo[i];
      const SDL_Color *c = &gGPU.rgPalette[f->color];
      int              red;

      switch (f->mode)
      {
      case kPaletteFadeScale:
         rgPalette[i].r = (Uint8)((a->r * f->step) >> 6);
         rgPalette[i].g = (Uint8)((a->g * f->step) >> 6);
         rgPalette[i].b = (Uint8)((a->b * f->step) >> 6);
         break;

      case kPaletteFadeBlend:
         rgPalette[i].r = (Uint8)((a->r * (31 - f->step) + b->r * f->step) / 31);
         rgPalette[i].g = (Uint8)((a->g * (31 - f->step) + b->g * f->step) / 31);
         rgPalette[i].b = (Uint8)((a->b * (31 - f->step) + b->b * f->step) / 31);
         break;

      case kPaletteFadeFromColor:
         rgPalette[i].r = (Uint8)VIDEO_GPU_Toward(c->r, a->r, 4 * f->step);
         rgPalette[i].g = (Uint8)VIDEO_GPU_Toward(c->g, a->g, 4 * f->step);
         rgPalette[i].b = (Uint8)VIDEO_GPU_Toward(c->b, a->b, 4 * f->step);
         break;

      case kPaletteFadeToColor:
         rgPalette[i].r = (Uint8)VIDEO_GPU_Toward(a->r, c->r, 4 * f->step);
         rgPalette[i].g = (Uint8)VIDEO_GPU_Toward(a->g, c->g, 4 * f->step);
         rgPalette[i].b = (Uint8)VIDEO_GPU_Toward(a->b, c->b, 4 * f->step);
         break;

      case kPaletteFadeToRed:
         if (i == 0x4F)
         {
            rgPalette[i] = *a;
         }
         else
         {
            red = (a->r + a->g + a->b) / 4 + 64;
            rgPalette[i].r = (Uint8)VIDEO_GPU_Toward(a->r, red, 8 * f->step);
            rgPalette[i].g = (Uint8)VIDEO_GPU_Toward(a->g, 0, 8 * f->step);
            rgPalette[i].b = (Uint8)VIDEO_GPU_Toward(a->b, 0, 8 * f->step);
         }
         break;
      }
      rgPalette[i].a = 255;
   }
}

static int
VIDEO_GPU_SubtractRect(
   SDL_Rect       *rgRects,
   int             nRects,
   int             nMaxRects,
   const SDL_Rect *pCut
)
/*++
  Purpose:

    Remove an area from a list of rectangles that do not overlap.

  Parameters:

    [IN/OUT] rgRects - the list.

    [IN]  nRects - how many rectangles it holds.

    [IN]  nMaxRects - how many rectangles it has room for. A rectangle that would need
          more stays whole.

    [IN]  pCut - the area to remove.

  Return value:

    How many rectangles it holds now.

--*/
{
   int i;

   for (i = nRects - 1; i >= 0; i--)
   {
      const SDL_Rect r = rgRects[i];
      SDL_Rect       rgParts[4];
      SDL_Rect       overlap;
      int            nParts = 0, iBelow, iRight, iPart;

      if (!SDL_GetRectIntersection(&r, pCut, &overlap))
      {
         continue;
      }

      iBelow = overlap.y + overlap.h;
      iRight = overlap.x + overlap.w;

      if (overlap.y > r.y)
      {
         rgParts[nParts].x = r.x;
         rgParts[nParts].y = r.y;
         rgParts[nParts].w = r.w;
         rgParts[nParts++].h = overlap.y - r.y;
      }
      if (iBelow < r.y + r.h)
      {
         rgParts[nParts].x = r.x;
         rgParts[nParts].y = iBelow;
         rgParts[nParts].w = r.w;
         rgParts[nParts++].h = r.y + r.h - iBelow;
      }
      if (overlap.x > r.x)
      {
         rgParts[nParts].x = r.x;
         rgParts[nParts].y = overlap.y;
         rgParts[nParts].w = overlap.x - r.x;
         rgParts[nParts++].h = overlap.h;
      }
      if (iRight < r.x + r.w)
      {
         rgParts[nParts].x = iRight;
         rgParts[nParts].y = overlap.y;
         rgParts[nParts].w = r.x + r.w - iRight;
         rgParts[nParts++].h = overlap.h;
      }

      if (nRects - 1 + nParts > nMaxRects)
      {
         continue;
      }

      //
      // Whatever moves into this place is either done with or a new part, so neither
      // needs another look.
      //
      rgRects[i] = rgRects[--nRects];
      for (iPart = 0; iPart < nParts; iPart++)
      {
         rgRects[nRects++] = rgParts[iPart];
      }
   }

   return nRects;
}

VOID
VIDEO_GPU_WriteScreen(
   SDL_Surface    *pSource,
   const SDL_Rect *pSourceRect,
   const SDL_Rect *pDestRect
)
/*++
  Purpose:

    Copy an area of an 8-bit surface to the image the player should see. Only this
    area changes, so anything else drawn on the source stays hidden until a later
    update shows it.

  Parameters:

    [IN]  pSource - the surface to copy from.

    [IN]  pSourceRect - the area to copy, NULL for the whole surface.

    [IN]  pDestRect - where to copy to; only its position is used.

  Return value:

    None.

--*/
{
   GPUSURFACE *pEntry = VIDEO_GPU_FindSurface(pSource);
   SDL_Rect    area, written;
   SDL_Point   position;

   if (pEntry == NULL ||
       !VIDEO_GPU_ClipCopy(pSource->w, pSource->h, pSourceRect, SCREEN_WIDTH, SCREEN_HEIGHT, pDestRect, &area, &position))
   {
      return;
   }

   VIDEO_GPU_FlushBatch();

   VIDEO_GPU_CopyRegion(pEntry->pTexture, &area, gGPU.pIndexTexture, &position);

   if (gGPU.fVerify)
   {
      int y;

      for (y = 0; y < area.h; y++)
      {
         memcpy(gGPU.pExpectedIndex + (position.y + y) * SCREEN_WIDTH + position.x,
            (const BYTE *)pSource->pixels + (area.y + y) * pSource->pitch + area.x, area.w);
         memset(gGPU.pExpectedBlack + (position.y + y) * SCREEN_WIDTH + position.x, 0, area.w);
      }
   }

   //
   // New pixels replace whatever part of the black band they cover.
   //
   written.x = position.x;
   written.y = position.y;
   written.w = area.w;
   written.h = area.h;
   gGPU.nBlackBand = VIDEO_GPU_SubtractRect(gGPU.rgBlackBand, gGPU.nBlackBand, MAX_BAND_RECTS, &written);
}

VOID
VIDEO_GPU_SetBlackBand(
   const SDL_Rect *pRect
)
/*++
  Purpose:

    Blacken the band at the top or bottom of the screen which a shake leaves uncovered.
    It has to be true black, while palette entry 0 is not black in every palette.

  Parameters:

    [IN]  pRect - the band.

  Return value:

    None.

--*/
{
   static const SDL_Rect screen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
   SDL_Rect              band;

   if (!SDL_GetRectIntersection(pRect, &screen, &band))
   {
      return;
   }

   gGPU.nBlackBand = VIDEO_GPU_SubtractRect(gGPU.rgBlackBand, gGPU.nBlackBand, MAX_BAND_RECTS, &band);
   if (gGPU.nBlackBand < MAX_BAND_RECTS)
   {
      gGPU.rgBlackBand[gGPU.nBlackBand++] = band;
   }

   if (gGPU.fVerify)
   {
      int y;

      for (y = 0; y < band.h; y++)
      {
         memset(gGPU.pExpectedBlack + (band.y + y) * SCREEN_WIDTH + band.x, 1, band.w);
      }
   }
}

SDL_Rect
VIDEO_GPU_CalcViewport(
   int             iWidth,
   int             iHeight
)
/*++
  Purpose:

    Calculate where the game appears in a window of the given size. As with the SDL
    renderer, TextureWidth and TextureHeight set the kept aspect ratio.

  Parameters:

    [IN]  iWidth - width of the window, in pixels.

    [IN]  iHeight - height of the window, in pixels.

  Return value:

    The area of the window that the game occupies.

--*/
{
   SDL_Rect rect;

   rect.w = iWidth;
   rect.h = iHeight;

   if (gConfig.fKeepAspectRatio)
   {
      double aspect = (double)gConfig.dwTextureWidth / (double)gConfig.dwTextureHeight;

      if (iWidth > iHeight * aspect)
      {
         rect.w = (int)(iHeight * aspect + 0.5);
      }
      else
      {
         rect.h = (int)(iWidth / aspect + 0.5);
      }

      if (rect.w < 1) rect.w = 1;
      if (rect.h < 1) rect.h = 1;
   }

   rect.x = (iWidth - rect.w) / 2;
   rect.y = (iHeight - rect.h) / 2;

   return rect;
}

static VOID
VIDEO_GPU_DrawFullscreen(
   SDL_GPUCommandBuffer    *pCommands,
   SDL_GPUTexture          *pTarget,
   const SDL_Rect          *pViewport,
   BOOL                     fClear,
   SDL_GPUGraphicsPipeline *pPipeline,
   SDL_GPUTexture          *pImage,
   SDL_GPUSampler          *pSampler
)
/*++
  Purpose:

    Run a one-texture fragment shader over an area of a render target. Its uniform data,
    if any, has to be pushed first.

  Parameters:

    [IN]  pCommands - the command buffer.

    [IN]  pTarget - the render target.

    [IN]  pViewport - the area, NULL for the whole target.

    [IN]  fClear - whether to clear the target to black first, instead of keeping it.

    [IN]  pPipeline - the shader.

    [IN]  pImage - the texture.

    [IN]  pSampler - how to sample the texture.

  Return value:

    None.

--*/
{
   SDL_GPUColorTargetInfo       target;
   SDL_GPUTextureSamplerBinding binding;
   SDL_GPURenderPass           *pPass;

   SDL_zero(target);
   target.texture = pTarget;
   target.clear_color.a = 1.0f;
   target.load_op = fClear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD;
   target.store_op = SDL_GPU_STOREOP_STORE;

   binding.texture = pImage;
   binding.sampler = pSampler;

   pPass = SDL_BeginGPURenderPass(pCommands, &target, 1, NULL);
   SDL_BindGPUGraphicsPipeline(pPass, pPipeline);
   if (pViewport != NULL)
   {
      SDL_GPUViewport area;

      SDL_zero(area);
      area.x = (float)pViewport->x;
      area.y = (float)pViewport->y;
      area.w = (float)pViewport->w;
      area.h = (float)pViewport->h;
      area.max_depth = 1.0f;
      SDL_SetGPUViewport(pPass, &area);
   }
   SDL_BindGPUFragmentSamplers(pPass, 0, &binding, 1);
   SDL_DrawGPUPrimitives(pPass, 3, 1, 0, 0);
   SDL_EndGPURenderPass(pPass);
}

static VOID
VIDEO_GPU_Display(
   SDL_GPUCommandBuffer *pCommands,
   SDL_GPUTexture       *pSwapchain,
   Uint32                uiWidth,
   Uint32                uiHeight,
   SDL_GPUTexture       *pImage,
   Uint32                uiImageWidth,
   Uint32                uiImageHeight
)
/*++
  Purpose:

    Put a finished image on the swapchain image: filter, scale, overlay.

  Parameters:

    [IN]  pCommands - the command buffer of this frame.

    [IN]  pSwapchain - the swapchain image of this frame.

    [IN]  uiWidth - width of the swapchain image.

    [IN]  uiHeight - height of the swapchain image.

    [IN]  pImage - the image to show.

    [IN]  uiImageWidth - width of the image.

    [IN]  uiImageHeight - height of the image.

  Return value:

    None.

--*/
{
   SDL_Rect viewport = VIDEO_GPU_CalcViewport(uiWidth, uiHeight);

   if (gGPU.iFilter != kFilterNone)
   {
      //
      // Like the OpenGL filters, these sample according to ScaleQuality.
      //
      SDL_GPUSampler *pSampler = (gGPU.iScale == kScaleLinear) ? gGPU.pLinearSampler : gGPU.pNearestSampler;
      float           rgInputSize[2];

      rgInputSize[0] = (float)uiImageWidth;
      rgInputSize[1] = (float)uiImageHeight;
      SDL_PushGPUFragmentUniformData(pCommands, 0, rgInputSize, sizeof(rgInputSize));

      if (gGPU.iFilter == kFilterCartoon)
      {
         //
         // The OpenGL version of this filter renders its first pass to a texture of the
         // next power-of-two size, which affects how it looks, so this one does as well.
         //
         Uint32 uiWidth2 = UTIL_NextPowerOfTwo(uiImageWidth);
         Uint32 uiHeight2 = UTIL_NextPowerOfTwo(uiImageHeight);

         if (gGPU.uiFilterWidth != uiWidth2 || gGPU.uiFilterHeight != uiHeight2)
         {
            SDL_ReleaseGPUTexture(gGPU.pDevice, gGPU.pFilterTexture);

            gGPU.pFilterTexture = VIDEO_GPU_CreateTexture(SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
               SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET, uiWidth2, uiHeight2);
            gGPU.uiFilterWidth = uiWidth2;
            gGPU.uiFilterHeight = uiHeight2;
            if (gGPU.pFilterTexture == NULL)
            {
               gGPU.uiFilterWidth = gGPU.uiFilterHeight = 0;
               UTIL_LogOutput(LOGLEVEL_ERROR, "Filter texture could not be created: %s\n", SDL_GetError());
            }
         }

         //
         // Without the texture the first pass is left out, and the next frame tries again.
         //
         if (gGPU.pFilterTexture != NULL)
         {
            VIDEO_GPU_DrawFullscreen(pCommands, gGPU.pFilterTexture, NULL, TRUE, gGPU.pFilterPrePipeline,
               pImage, pSampler);
            pImage = gGPU.pFilterTexture;
         }
      }

      VIDEO_GPU_DrawFullscreen(pCommands, pSwapchain, &viewport, TRUE, gGPU.pFilterPipeline, pImage, pSampler);
   }
   else if (gGPU.iScale == kScaleSharp)
   {
      float rgSizes[4];

      rgSizes[0] = (float)uiImageWidth;
      rgSizes[1] = (float)uiImageHeight;
      rgSizes[2] = (float)viewport.w;
      rgSizes[3] = (float)viewport.h;
      SDL_PushGPUFragmentUniformData(pCommands, 0, rgSizes, sizeof(rgSizes));

      VIDEO_GPU_DrawFullscreen(pCommands, pSwapchain, &viewport, TRUE, gGPU.pSharpPipeline, pImage, gGPU.pLinearSampler);
   }
   else
   {
      SDL_GPUBlitInfo blit;

      SDL_zero(blit);
      blit.source.texture = pImage;
      blit.source.w = uiImageWidth;
      blit.source.h = uiImageHeight;
      blit.destination.texture = pSwapchain;
      blit.destination.x = viewport.x;
      blit.destination.y = viewport.y;
      blit.destination.w = viewport.w;
      blit.destination.h = viewport.h;
      blit.load_op = SDL_GPU_LOADOP_CLEAR;
      blit.clear_color.a = 1.0f;
      blit.filter = gGPU.filter;
      SDL_BlitGPUTexture(pCommands, &blit);
   }

   if (gGPU.pOverlayTexture != NULL)
   {
      SDL_PushGPUFragmentUniformData(pCommands, 0, &gGPU.flOverlayAlpha, sizeof(gGPU.flOverlayAlpha));
      VIDEO_GPU_DrawFullscreen(pCommands, pSwapchain, &viewport, FALSE, gGPU.pOverlayPipeline,
         gGPU.pOverlayTexture, (gGPU.iScale == kScaleLinear) ? gGPU.pLinearSampler : gGPU.pNearestSampler);
   }
}

static VOID
VIDEO_GPU_VerifyFrame(
   VOID
)
/*++
  Purpose:

    Submit the frame, read back what the GPU produced and compare it with the software
    result, logging any difference.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   SDL_GPUTextureRegion       source;
   SDL_GPUTextureTransferInfo destination;
   SDL_GPUCopyPass           *pCopyPass;
   const BYTE                *pMapped;
   const SDL_Color           *rgPalette = gGPU.rgExpectedPalette;
   SDL_Color                  rgResolved[PALETTE_ENTRIES];
   DWORD                      dwBadIndex = 0, dwBadColor = 0, dwBadPalette = 0;
   int                        i, iFirstBad = -1;

   //
   // What CPU code gets if it asks for the palette at this point.
   //
   memcpy(rgResolved, rgPalette, PALETTE_BYTES);
   VIDEO_GPU_ResolvePalette(rgResolved);
   for (i = 0; i < PALETTE_ENTRIES; i++)
   {
      if (rgResolved[i].r != rgPalette[i].r || rgResolved[i].g != rgPalette[i].g || rgResolved[i].b != rgPalette[i].b)
      {
         dwBadPalette++;
      }
   }

   SDL_zero(source);
   source.w = SCREEN_WIDTH;
   source.h = SCREEN_HEIGHT;
   source.d = 1;

   SDL_zero(destination);
   destination.transfer_buffer = gGPU.pDownload;
   destination.pixels_per_row = SCREEN_WIDTH;
   destination.rows_per_layer = SCREEN_HEIGHT;

   pCopyPass = SDL_BeginGPUCopyPass(VIDEO_GPU_Commands());
   source.texture = gGPU.pIndexTexture;
   SDL_DownloadFromGPUTexture(pCopyPass, &source, &destination);
   source.texture = gGPU.pScreenTexture;
   destination.offset = SCREEN_PIXELS;
   SDL_DownloadFromGPUTexture(pCopyPass, &source, &destination);
   SDL_EndGPUCopyPass(pCopyPass);

   if (!VIDEO_GPU_SubmitAndWait())
   {
      return;
   }

   pMapped = (const BYTE *)SDL_MapGPUTransferBuffer(gGPU.pDevice, gGPU.pDownload, false);
   if (pMapped == NULL)
   {
      return;
   }

   for (i = 0; i < SCREEN_PIXELS; i++)
   {
      static const SDL_Color black = { 0, 0, 0, 255 };
      const BYTE            *pRGB = pMapped + SCREEN_PIXELS + i * 4;
      const SDL_Color       *pWanted = gGPU.pExpectedBlack[i] ? &black : &rgPalette[gGPU.pExpectedIndex[i]];

      if (pMapped[i] != gGPU.pExpectedIndex[i])
      {
         dwBadIndex++;
         if (iFirstBad < 0)
         {
            iFirstBad = i;
         }
      }

      if (pRGB[0] != pWanted->r || pRGB[1] != pWanted->g || pRGB[2] != pWanted->b)
      {
         dwBadColor++;
         if (iFirstBad < 0)
         {
            iFirstBad = i;
         }
      }
   }

   gGPU.dwFrames++;
   if (dwBadIndex > 0)
   {
      gGPU.dwIndexMismatches++;
   }
   if (dwBadColor > 0)
   {
      gGPU.dwColorMismatches++;
   }
   if (dwBadPalette > 0 && ++gGPU.dwPaletteMismatches <= 40)
   {
      UTIL_LogOutput(LOGLEVEL_WARNING, "[GPUVERIFY] frame %u: %u palette entries worked out wrong; fade mode %d step %d\n",
         gGPU.dwFrames, dwBadPalette, gGPU.fade.mode, gGPU.fade.step);
   }

   if ((dwBadIndex > 0 || dwBadColor > 0) && gGPU.dwIndexMismatches + gGPU.dwColorMismatches <= 40)
   {
      UTIL_LogOutput(LOGLEVEL_WARNING,
         "[GPUVERIFY] frame %u: %u wrong indices, %u wrong colors; first at (%d,%d): gpu index %d, cpu index %d, fade mode %d step %d\n",
         gGPU.dwFrames, dwBadIndex, dwBadColor, iFirstBad % SCREEN_WIDTH, iFirstBad / SCREEN_WIDTH,
         pMapped[iFirstBad], gGPU.pExpectedIndex[iFirstBad], gGPU.fade.mode, gGPU.fade.step);
   }

   SDL_UnmapGPUTransferBuffer(gGPU.pDevice, gGPU.pDownload);
}

static VOID
VIDEO_GPU_EndFrame(
   SDL_GPUTexture *pImage,
   Uint32          uiImageWidth,
   Uint32          uiImageHeight,
   BOOL            fVerifiable
)
/*++
  Purpose:

    Show a finished image and submit all the work recorded for this frame.

  Parameters:

    [IN]  pImage - the image to show.

    [IN]  uiImageWidth - width of the image.

    [IN]  uiImageHeight - height of the image.

    [IN]  fVerifiable - whether the image can be compared with a software result.

  Return value:

    None.

--*/
{
   SDL_GPUCommandBuffer *pCommands = VIDEO_GPU_Commands();
   SDL_GPUTexture       *pSwapchain = NULL;
   Uint32                uiWidth = 0, uiHeight = 0;

   if (!SDL_WaitAndAcquireGPUSwapchainTexture(pCommands, gGPU.pWindow, &pSwapchain, &uiWidth, &uiHeight))
   {
      UTIL_LogOutput(LOGLEVEL_ERROR, "SDL_WaitAndAcquireGPUSwapchainTexture failed: %s\n", SDL_GetError());
   }

   //
   // There is no swapchain image while the window is minimized. The drawing recorded
   // still has to happen, so the frame is submitted either way.
   //
   if (pSwapchain != NULL)
   {
      VIDEO_GPU_Display(pCommands, pSwapchain, uiWidth, uiHeight, pImage, uiImageWidth, uiImageHeight);
   }

   if (gGPU.fVerify && fVerifiable)
   {
      VIDEO_GPU_VerifyFrame();
   }
   else
   {
      VIDEO_GPU_Submit();
   }
}

VOID
VIDEO_GPU_Present(
   VOID
)
/*++
  Purpose:

    Show the image built by VIDEO_GPU_WriteScreen() using the current palette.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   SDL_GPUCommandBuffer         *pCommands;
   SDL_GPUCopyPass              *pCopyPass;
   SDL_GPURenderPass            *pPass;
   SDL_GPUColorTargetInfo        target;
   SDL_GPUTextureSamplerBinding  bindings[3];
   BYTE                         *pUpload;
   Uint32                        uiOffset;
   SDL_Rect                      rgShown[MAX_SHOWN_RECTS];
   int                           nShown = 1, i;

   VIDEO_GPU_FlushBatch();
   pCommands = VIDEO_GPU_Commands();

   pUpload = VIDEO_GPU_RingMap(PALETTE_BYTES * 2, &uiOffset);
   memcpy(pUpload, gGPU.rgPalette, PALETTE_BYTES);
   memcpy(pUpload + PALETTE_BYTES, gGPU.rgPaletteTo, PALETTE_BYTES);
   SDL_UnmapGPUTransferBuffer(gGPU.pDevice, gGPU.pRing);

   pCopyPass = SDL_BeginGPUCopyPass(pCommands);
   VIDEO_GPU_UploadRegion(pCopyPass, gGPU.pRing, uiOffset, gGPU.pPaletteTexture, 0, 0, PALETTE_ENTRIES, 1);
   VIDEO_GPU_UploadRegion(pCopyPass, gGPU.pRing, uiOffset + PALETTE_BYTES, gGPU.pPaletteToTexture, 0, 0, PALETTE_ENTRIES, 1);
   SDL_EndGPUCopyPass(pCopyPass);

   SDL_zero(target);
   target.texture = gGPU.pScreenTexture;
   target.clear_color.a = 1.0f;
   target.load_op = SDL_GPU_LOADOP_CLEAR;
   target.store_op = SDL_GPU_STOREOP_STORE;
   target.cycle = true;

   bindings[0].texture = gGPU.pIndexTexture;
   bindings[0].sampler = gGPU.pNearestSampler;
   bindings[1].texture = gGPU.pPaletteTexture;
   bindings[1].sampler = gGPU.pNearestSampler;
   bindings[2].texture = gGPU.pPaletteToTexture;
   bindings[2].sampler = gGPU.pNearestSampler;

   SDL_PushGPUFragmentUniformData(pCommands, 0, &gGPU.fade, sizeof(gGPU.fade));

   //
   // The black band simply keeps the color the target was cleared to.
   //
   rgShown[0].x = 0;
   rgShown[0].y = 0;
   rgShown[0].w = SCREEN_WIDTH;
   rgShown[0].h = SCREEN_HEIGHT;
   for (i = 0; i < gGPU.nBlackBand; i++)
   {
      nShown = VIDEO_GPU_SubtractRect(rgShown, nShown, MAX_SHOWN_RECTS, &gGPU.rgBlackBand[i]);
   }

   pPass = SDL_BeginGPURenderPass(pCommands, &target, 1, NULL);
   SDL_BindGPUGraphicsPipeline(pPass, gGPU.pPalettePipeline);
   SDL_BindGPUFragmentSamplers(pPass, 0, bindings, 3);
   for (i = 0; i < nShown; i++)
   {
      SDL_SetGPUScissor(pPass, &rgShown[i]);
      SDL_DrawGPUPrimitives(pPass, 3, 1, 0, 0);
   }
   SDL_EndGPURenderPass(pPass);

   VIDEO_GPU_EndFrame(gGPU.pScreenTexture, SCREEN_WIDTH, SCREEN_HEIGHT, TRUE);
}

VOID
VIDEO_GPU_PresentSurface(
   SDL_Surface    *pSurface
)
/*++
  Purpose:

    Show a direct-color frame at its own resolution; it has no palette to look up.

  Parameters:

    [IN]  pSurface - the frame.

  Return value:

    None.

--*/
{
   SDL_GPUTransferBuffer *pBuffer = gGPU.pRing;
   SDL_GPUCopyPass       *pCopyPass;
   BYTE                  *pMapped;
   Uint32                 uiOffset = 0;
   Uint32                 uiBytes = pSurface->w * pSurface->h * 4;

   VIDEO_GPU_FlushBatch();

   if (gGPU.uiRGBWidth != (Uint32)pSurface->w || gGPU.uiRGBHeight != (Uint32)pSurface->h)
   {
      SDL_ReleaseGPUTexture(gGPU.pDevice, gGPU.pRGBTexture);

      gGPU.pRGBTexture = VIDEO_GPU_CreateTexture(SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM,
         SDL_GPU_TEXTUREUSAGE_SAMPLER, pSurface->w, pSurface->h);
      gGPU.uiRGBWidth = pSurface->w;
      gGPU.uiRGBHeight = pSurface->h;
      if (gGPU.pRGBTexture == NULL)
      {
         gGPU.uiRGBWidth = gGPU.uiRGBHeight = 0;
         UTIL_LogOutput(LOGLEVEL_ERROR, "Video frame texture could not be created: %s\n", SDL_GetError());
         return;
      }
   }

   if (uiBytes <= RING_BYTES)
   {
      pMapped = VIDEO_GPU_RingMap(uiBytes, &uiOffset);
   }
   else
   {
      SDL_GPUTransferBufferCreateInfo upload;

      SDL_zero(upload);
      upload.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
      upload.size = uiBytes;
      pBuffer = SDL_CreateGPUTransferBuffer(gGPU.pDevice, &upload);
      pMapped = (pBuffer != NULL) ? (BYTE *)SDL_MapGPUTransferBuffer(gGPU.pDevice, pBuffer, false) : NULL;
      if (pMapped == NULL)
      {
         UTIL_LogOutput(LOGLEVEL_ERROR, "Video frame could not be uploaded: %s\n", SDL_GetError());
         return;
      }
   }

   SDL_ConvertPixels(pSurface->w, pSurface->h, pSurface->format, pSurface->pixels, pSurface->pitch,
      SDL_PIXELFORMAT_BGRA32, pMapped, pSurface->w * 4);
   SDL_UnmapGPUTransferBuffer(gGPU.pDevice, pBuffer);

   pCopyPass = SDL_BeginGPUCopyPass(VIDEO_GPU_Commands());
   VIDEO_GPU_UploadRegion(pCopyPass, pBuffer, uiOffset, gGPU.pRGBTexture, 0, 0, pSurface->w, pSurface->h);
   SDL_EndGPUCopyPass(pCopyPass);

   if (pBuffer != gGPU.pRing)
   {
      SDL_ReleaseGPUTransferBuffer(gGPU.pDevice, pBuffer);
   }

   VIDEO_GPU_EndFrame(gGPU.pRGBTexture, pSurface->w, pSurface->h, FALSE);
}

static SDL_GPUShader *
VIDEO_GPU_CreateShader(
   const unsigned char *pSPIRV,
   unsigned int         uiSPIRVLen,
   const unsigned char *pMSL,
   unsigned int         uiMSLLen,
   const unsigned char *pDXIL,
   unsigned int         uiDXILLen,
   SDL_GPUShaderStage   stage,
   Uint32               uiSamplers,
   Uint32               uiUniformBuffers
)
/*++
  Purpose:

    Create a shader from the blob the device's backend takes: SPIR-V for Vulkan, DXIL for
    Direct3D 12 and MSL for Metal. The device was created asking for only the formats that
    are compiled in, so one of them always fits.

--*/
{
   SDL_GPUShaderCreateInfo info;
   SDL_GPUShaderFormat     formats = SDL_GetGPUShaderFormats(gGPU.pDevice);

   SDL_zero(info);
   info.stage = stage;
   info.num_samplers = uiSamplers;
   info.num_uniform_buffers = uiUniformBuffers;

   if (formats & SDL_GPU_SHADERFORMAT_SPIRV)
   {
      info.format = SDL_GPU_SHADERFORMAT_SPIRV;
      info.code = pSPIRV;
      info.code_size = uiSPIRVLen;
      info.entrypoint = "main";
   }
   else if ((formats & SDL_GPU_SHADERFORMAT_DXIL) && pDXIL != NULL)
   {
      info.format = SDL_GPU_SHADERFORMAT_DXIL;
      info.code = pDXIL;
      info.code_size = uiDXILLen;
      info.entrypoint = "main";
   }
   else
   {
      //
      // spirv-cross has to rename main, which is not a legal name for an MSL function.
      //
      info.format = SDL_GPU_SHADERFORMAT_MSL;
      info.code = pMSL;
      info.code_size = uiMSLLen;
      info.entrypoint = "main0";
   }

   return SDL_CreateGPUShader(gGPU.pDevice, &info);
}

#define VIDEO_GPU_SHADER(name, stage, samplers, uniforms)  \
   VIDEO_GPU_CreateShader(name##_spv, name##_spv_len, name##_msl, name##_msl_len, DXIL_BLOB(name), \
      (stage), (samplers), (uniforms))

#define VIDEO_GPU_FRAGMENT(name, samplers, uniforms) \
   VIDEO_GPU_SHADER(name, SDL_GPU_SHADERSTAGE_FRAGMENT, (samplers), (uniforms))

static SDL_GPUGraphicsPipeline *
VIDEO_GPU_CreatePipeline(
   SDL_GPUShader        *pFragmentShader,
   SDL_GPUTextureFormat  format,
   BOOL                  fBlend
)
/*++
  Purpose:

    Create a pipeline drawing one triangle over the whole target.

  Parameters:

    [IN]  pFragmentShader - the fragment shader, which gets released.

    [IN]  format - format of the render target.

    [IN]  fBlend - whether to blend using the alpha of the shader's output.

  Return value:

    The pipeline, NULL if it failed.

--*/
{
   SDL_GPUGraphicsPipelineCreateInfo  info;
   SDL_GPUColorTargetDescription      target;
   SDL_GPUGraphicsPipeline           *pPipeline = NULL;
   SDL_GPUShader                     *pVertexShader =
      VIDEO_GPU_SHADER(fullscreen_vert, SDL_GPU_SHADERSTAGE_VERTEX, 0, 0);

   if (pVertexShader != NULL && pFragmentShader != NULL)
   {
      SDL_zero(target);
      target.format = format;
      if (fBlend)
      {
         target.blend_state.enable_blend = true;
         target.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
         target.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
         target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
         target.blend_state.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
         target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
         target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
      }

      SDL_zero(info);
      info.vertex_shader = pVertexShader;
      info.fragment_shader = pFragmentShader;
      info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
      info.target_info.color_target_descriptions = &target;
      info.target_info.num_color_targets = 1;

      pPipeline = SDL_CreateGPUGraphicsPipeline(gGPU.pDevice, &info);
   }

   //
   // A pipeline keeps what it needs, so the shaders are not used after this point.
   //
   SDL_ReleaseGPUShader(gGPU.pDevice, pVertexShader);
   SDL_ReleaseGPUShader(gGPU.pDevice, pFragmentShader);

   return pPipeline;
}

static SDL_GPUGraphicsPipeline *
VIDEO_GPU_CreateSpritePipeline(
   VOID
)
{
   SDL_GPUGraphicsPipelineCreateInfo info;
   SDL_GPUColorTargetDescription     target;
   SDL_GPUVertexBufferDescription    buffer;
   SDL_GPUVertexAttribute            attributes[4];
   SDL_GPUGraphicsPipeline          *pPipeline = NULL;
   SDL_GPUShader *pVertexShader = VIDEO_GPU_SHADER(sprite_vert, SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
   SDL_GPUShader *pFragmentShader = VIDEO_GPU_FRAGMENT(sprite_frag, 2, 0);

   if (pVertexShader != NULL && pFragmentShader != NULL)
   {
      SDL_zero(buffer);
      buffer.slot = 0;
      buffer.pitch = sizeof(GPUINSTANCE);
      buffer.input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;

      SDL_zero(attributes);
      attributes[0].location = 0;
      attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_SHORT2;
      attributes[0].offset = offsetof(GPUINSTANCE, dstX);
      attributes[1].location = 1;
      attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_USHORT2;
      attributes[1].offset = offsetof(GPUINSTANCE, w);
      attributes[2].location = 2;
      attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_USHORT2;
      attributes[2].offset = offsetof(GPUINSTANCE, srcX);
      attributes[3].location = 3;
      attributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4;
      attributes[3].offset = offsetof(GPUINSTANCE, mode);

      SDL_zero(target);
      target.format = SDL_GPU_TEXTUREFORMAT_R8_UNORM;

      SDL_zero(info);
      info.vertex_shader = pVertexShader;
      info.fragment_shader = pFragmentShader;
      info.vertex_input_state.vertex_buffer_descriptions = &buffer;
      info.vertex_input_state.num_vertex_buffers = 1;
      info.vertex_input_state.vertex_attributes = attributes;
      info.vertex_input_state.num_vertex_attributes = 4;
      info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLESTRIP;
      info.target_info.color_target_descriptions = &target;
      info.target_info.num_color_targets = 1;

      pPipeline = SDL_CreateGPUGraphicsPipeline(gGPU.pDevice, &info);
   }

   SDL_ReleaseGPUShader(gGPU.pDevice, pVertexShader);
   SDL_ReleaseGPUShader(gGPU.pDevice, pFragmentShader);

   return pPipeline;
}

static SDL_GPUSampler *
VIDEO_GPU_CreateSampler(
   SDL_GPUFilter filter
)
{
   SDL_GPUSamplerCreateInfo info;

   SDL_zero(info);
   info.min_filter = filter;
   info.mag_filter = filter;
   info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
   info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
   info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
   info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

   return SDL_CreateGPUSampler(gGPU.pDevice, &info);
}

BOOL
VIDEO_GPU_Init(
   VOID
)
/*++
  Purpose:

    Create the GPU device and everything that does not depend on the window. This can
    take a noticeable time, so it should happen before the window appears: until the
    first frame is presented the window shows whatever the system paints it with.

  Parameters:

    None.

  Return value:

    TRUE if succeeded. If FALSE the game has to draw in software.

--*/
{
   SDL_GPUTransferBufferCreateInfo transfer;
   SDL_GPUBufferCreateInfo         buffer;
   const char                     *pszVerify = SDL_getenv("SDLPAL_GPU_VERIFY");

#if defined(_DEBUG) || defined(DEBUG)
   const bool fDebug = true;
#else
   const bool fDebug = false;
#endif

   gGPU.pDevice = SDL_CreateGPUDevice(SHADER_FORMATS, fDebug, NULL);
   if (gGPU.pDevice == NULL)
   {
      UTIL_LogOutput(LOGLEVEL_WARNING, "SDL_GPU is not available: %s\n", SDL_GetError());
      return FALSE;
   }

   gGPU.fVerify = (pszVerify != NULL && pszVerify[0] != '\0' && pszVerify[0] != '0');

   //
   // ScaleQuality keeps the values of the SDL renderer, where 2 is SDL_SCALEMODE_PIXELART
   // (SDL 3.4 and later). Without a ScaleQuality line this backend uses sharp scaling, which
   // equals nearest at whole-number scales, and leaves the global default of 0 alone.
   //
   gGPU.iScale = (gConfig.pszScaleQuality != NULL) ? SDL_atoi(gConfig.pszScaleQuality) : kScaleSharp;
   gGPU.filter = (gGPU.iScale == kScaleLinear) ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST;

   if (gConfig.pszFilter != NULL && gConfig.pszFilter[0] != '\0' && SDL_strcasecmp(gConfig.pszFilter, "none") != 0)
   {
      if (SDL_strcasecmp(gConfig.pszFilter, "reverse") == 0)
      {
         gGPU.iFilter = kFilterReverse;
      }
      else if (SDL_strcasecmp(gConfig.pszFilter, "cartoon") == 0)
      {
         gGPU.iFilter = kFilterCartoon;
      }
      else if (SDL_strcasecmp(gConfig.pszFilter, "crt") == 0)
      {
         gGPU.iFilter = kFilterCrt;
      }
      else
      {
         UTIL_LogOutput(LOGLEVEL_WARNING, "Unknown filter: %s\n", gConfig.pszFilter);
      }
   }

   gGPU.pSpritePipeline = VIDEO_GPU_CreateSpritePipeline();
   gGPU.pEffectPipeline = VIDEO_GPU_CreatePipeline(VIDEO_GPU_FRAGMENT(effect_frag, 2, 1), SDL_GPU_TEXTUREFORMAT_R8_UNORM, FALSE);
   gGPU.pPalettePipeline = VIDEO_GPU_CreatePipeline(VIDEO_GPU_FRAGMENT(palette_frag, 3, 1), SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, FALSE);
   gGPU.pNearestSampler = VIDEO_GPU_CreateSampler(SDL_GPU_FILTER_NEAREST);
   gGPU.pLinearSampler = VIDEO_GPU_CreateSampler(SDL_GPU_FILTER_LINEAR);

   gGPU.pAtlas = VIDEO_GPU_CreateTexture(SDL_GPU_TEXTUREFORMAT_R8G8_UNORM,
      SDL_GPU_TEXTUREUSAGE_SAMPLER, ATLAS_SIZE, ATLAS_SIZE);
   gGPU.pScratch = VIDEO_GPU_CreateTexture(SDL_GPU_TEXTUREFORMAT_R8_UNORM,
      SDL_GPU_TEXTUREUSAGE_SAMPLER, SCREEN_WIDTH, SCREEN_HEIGHT);
   gGPU.pIndexTexture = VIDEO_GPU_CreateTexture(SDL_GPU_TEXTUREFORMAT_R8_UNORM,
      SDL_GPU_TEXTUREUSAGE_SAMPLER, SCREEN_WIDTH, SCREEN_HEIGHT);
   gGPU.pPaletteTexture = VIDEO_GPU_CreateTexture(SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
      SDL_GPU_TEXTUREUSAGE_SAMPLER, PALETTE_ENTRIES, 1);
   gGPU.pPaletteToTexture = VIDEO_GPU_CreateTexture(SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
      SDL_GPU_TEXTUREUSAGE_SAMPLER, PALETTE_ENTRIES, 1);
   gGPU.pScreenTexture = VIDEO_GPU_CreateTexture(SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
      SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET, SCREEN_WIDTH, SCREEN_HEIGHT);

   SDL_zero(buffer);
   buffer.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
   buffer.size = VERTEX_RING;
   gGPU.pVertexBuffer = SDL_CreateGPUBuffer(gGPU.pDevice, &buffer);

   SDL_zero(transfer);
   transfer.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
   transfer.size = RING_BYTES;
   gGPU.pRing = SDL_CreateGPUTransferBuffer(gGPU.pDevice, &transfer);

   transfer.usage = SDL_GPU_TRANSFERBUFFERUSAGE_DOWNLOAD;
   transfer.size = SCREEN_PIXELS + SCREEN_PIXELS * 4;
   gGPU.pDownload = SDL_CreateGPUTransferBuffer(gGPU.pDevice, &transfer);

   gGPU.pInstances = (GPUINSTANCE *)SDL_malloc(MAX_INSTANCES * sizeof(GPUINSTANCE));
   gGPU.pStage = (BYTE *)SDL_malloc(STAGE_BYTES);
   gGPU.pUploads = (ATLASUPLOAD *)SDL_malloc(MAX_UPLOADS * sizeof(ATLASUPLOAD));
   gGPU.pAtlasEntries = (ATLASENTRY *)SDL_calloc(ATLAS_SLOTS, sizeof(ATLASENTRY));
   gGPU.pExpectedIndex = (BYTE *)SDL_calloc(SCREEN_PIXELS, 1);
   gGPU.pExpectedBlack = (BYTE *)SDL_calloc(SCREEN_PIXELS, 1);
   VIDEO_GPU_AtlasRewind();

   if (gGPU.pSpritePipeline == NULL || gGPU.pEffectPipeline == NULL || gGPU.pPalettePipeline == NULL ||
       gGPU.pNearestSampler == NULL || gGPU.pLinearSampler == NULL || gGPU.pAtlas == NULL ||
       gGPU.pScratch == NULL || gGPU.pIndexTexture == NULL || gGPU.pPaletteTexture == NULL ||
       gGPU.pPaletteToTexture == NULL || gGPU.pScreenTexture == NULL || gGPU.pVertexBuffer == NULL ||
       gGPU.pRing == NULL || gGPU.pDownload == NULL || gGPU.pInstances == NULL || gGPU.pStage == NULL ||
       gGPU.pUploads == NULL || gGPU.pAtlasEntries == NULL || gGPU.pExpectedIndex == NULL ||
       gGPU.pExpectedBlack == NULL)
   {
      UTIL_LogOutput(LOGLEVEL_WARNING, "SDL_GPU resources could not be created: %s\n", SDL_GetError());
      VIDEO_GPU_Shutdown();
      return FALSE;
   }

   UTIL_LogOutput(LOGLEVEL_DEBUG, "SDL_GPU backend:%s\n", SDL_GetGPUDeviceDriver(gGPU.pDevice));
   if (gGPU.fVerify)
   {
      UTIL_LogOutput(LOGLEVEL_WARNING, "[GPUVERIFY] comparing every frame with software drawing\n");
   }

   return TRUE;
}

const char *
VIDEO_GPU_GetDriverName(
   VOID
)
/*++
  Purpose:

    Get the name of the SDL_GPU driver in use, such as "vulkan", "direct3d12" or "metal".

  Parameters:

    None.

  Return value:

    The name, or NULL if there is no device.

--*/
{
   return (gGPU.pDevice != NULL) ? SDL_GetGPUDeviceDriver(gGPU.pDevice) : NULL;
}

BOOL
VIDEO_GPU_ClaimWindow(
   SDL_Window     *pWindow
)
/*++
  Purpose:

    Start presenting to the window, and turn it black right away.

  Parameters:

    [IN]  pWindow - the window to present to.

  Return value:

    TRUE if succeeded. If FALSE the backend has been shut down and the window is left
    free for the SDL renderer.

--*/
{
   SDL_GPUCommandBuffer *pCommands;
   SDL_GPUTexture       *pSwapchain = NULL;
   SDL_GPUTextureFormat  format;
   BOOL                  fFailed = FALSE;

   if (!SDL_ClaimWindowForGPUDevice(gGPU.pDevice, pWindow))
   {
      UTIL_LogOutput(LOGLEVEL_WARNING, "SDL_GPU cannot use the window: %s\n", SDL_GetError());
      VIDEO_GPU_Shutdown();
      return FALSE;
   }
   gGPU.pWindow = pWindow;

   //
   // These draw straight to the swapchain, whose format is only known now.
   //
   format = SDL_GetGPUSwapchainTextureFormat(gGPU.pDevice, pWindow);

   if (gGPU.iScale == kScaleSharp)
   {
      gGPU.pSharpPipeline = VIDEO_GPU_CreatePipeline(VIDEO_GPU_FRAGMENT(sharp_frag, 1, 1), format, FALSE);
      fFailed = fFailed || gGPU.pSharpPipeline == NULL;
   }

   switch (gGPU.iFilter)
   {
   case kFilterReverse:
      gGPU.pFilterPipeline = VIDEO_GPU_CreatePipeline(VIDEO_GPU_FRAGMENT(filter_reverse_frag, 1, 0), format, FALSE);
      break;

   case kFilterCartoon:
      gGPU.pFilterPrePipeline = VIDEO_GPU_CreatePipeline(VIDEO_GPU_FRAGMENT(filter_aa_frag, 1, 1),
         SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM, FALSE);
      gGPU.pFilterPipeline = VIDEO_GPU_CreatePipeline(VIDEO_GPU_FRAGMENT(filter_cartoon_frag, 1, 1), format, FALSE);
      fFailed = fFailed || gGPU.pFilterPrePipeline == NULL;
      break;

   case kFilterCrt:
      gGPU.pFilterPipeline = VIDEO_GPU_CreatePipeline(VIDEO_GPU_FRAGMENT(filter_crt_frag, 1, 0), format, FALSE);
      break;
   }
   fFailed = fFailed || (gGPU.iFilter != kFilterNone && gGPU.pFilterPipeline == NULL);

   if (fFailed)
   {
      UTIL_LogOutput(LOGLEVEL_WARNING, "SDL_GPU pipelines could not be created: %s\n", SDL_GetError());
      VIDEO_GPU_Shutdown();
      return FALSE;
   }

   pCommands = VIDEO_GPU_TryCommands();
   if (pCommands == NULL || !SDL_WaitAndAcquireGPUSwapchainTexture(pCommands, pWindow, &pSwapchain, NULL, NULL))
   {
      UTIL_LogOutput(LOGLEVEL_WARNING, "SDL_GPU cannot draw to the window: %s\n", SDL_GetError());
      VIDEO_GPU_Shutdown();
      return FALSE;
   }

   if (pSwapchain != NULL)
   {
      SDL_GPUColorTargetInfo target;

      SDL_zero(target);
      target.texture = pSwapchain;
      target.clear_color.a = 1.0f;
      target.load_op = SDL_GPU_LOADOP_CLEAR;
      target.store_op = SDL_GPU_STOREOP_STORE;

      SDL_EndGPURenderPass(SDL_BeginGPURenderPass(pCommands, &target, 1, NULL));
   }

   fFailed = !SDL_SubmitGPUCommandBuffer(pCommands);
   gGPU.pCommands = NULL;
   if (fFailed)
   {
      UTIL_LogOutput(LOGLEVEL_WARNING, "SDL_GPU cannot draw to the window: %s\n", SDL_GetError());
      VIDEO_GPU_Shutdown();
      return FALSE;
   }

   return TRUE;
}

VOID
VIDEO_GPU_Shutdown(
   VOID
)
/*++
  Purpose:

    Release everything created by this backend, including the claim on the window.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   if (gGPU.pDevice == NULL)
   {
      return;
   }

   if (gGPU.fVerify)
   {
      UTIL_LogOutput(LOGLEVEL_WARNING, "[GPUVERIFY] done: %u frames, %u with wrong indices, %u with wrong colors, %u with a wrong palette; %u downloads, %u wrong\n",
         gGPU.dwFrames, gGPU.dwIndexMismatches, gGPU.dwColorMismatches, gGPU.dwPaletteMismatches,
         gGPU.dwDownloads, gGPU.dwDownloadMismatches);
   }

   if (gGPU.pCommands != NULL)
   {
      SDL_CancelGPUCommandBuffer(gGPU.pCommands);
      gGPU.pCommands = NULL;
   }

   SDL_WaitForGPUIdle(gGPU.pDevice);

   while (gGPU.pSurfaces != NULL)
   {
      GPUSURFACE *pEntry = gGPU.pSurfaces;

      SDL_ReleaseGPUTexture(gGPU.pDevice, pEntry->pTexture);
      gGPU.pSurfaces = pEntry->pNext;
      SDL_free(pEntry);
   }

   SDL_ReleaseGPUGraphicsPipeline(gGPU.pDevice, gGPU.pSpritePipeline);
   SDL_ReleaseGPUGraphicsPipeline(gGPU.pDevice, gGPU.pEffectPipeline);
   SDL_ReleaseGPUGraphicsPipeline(gGPU.pDevice, gGPU.pPalettePipeline);
   SDL_ReleaseGPUGraphicsPipeline(gGPU.pDevice, gGPU.pOverlayPipeline);
   SDL_ReleaseGPUGraphicsPipeline(gGPU.pDevice, gGPU.pSharpPipeline);
   SDL_ReleaseGPUGraphicsPipeline(gGPU.pDevice, gGPU.pFilterPipeline);
   SDL_ReleaseGPUGraphicsPipeline(gGPU.pDevice, gGPU.pFilterPrePipeline);
   SDL_ReleaseGPUSampler(gGPU.pDevice, gGPU.pNearestSampler);
   SDL_ReleaseGPUSampler(gGPU.pDevice, gGPU.pLinearSampler);
   SDL_ReleaseGPUTexture(gGPU.pDevice, gGPU.pAtlas);
   SDL_ReleaseGPUTexture(gGPU.pDevice, gGPU.pScratch);
   SDL_ReleaseGPUTexture(gGPU.pDevice, gGPU.pIndexTexture);
   SDL_ReleaseGPUTexture(gGPU.pDevice, gGPU.pPaletteTexture);
   SDL_ReleaseGPUTexture(gGPU.pDevice, gGPU.pPaletteToTexture);
   SDL_ReleaseGPUTexture(gGPU.pDevice, gGPU.pScreenTexture);
   SDL_ReleaseGPUTexture(gGPU.pDevice, gGPU.pRGBTexture);
   SDL_ReleaseGPUTexture(gGPU.pDevice, gGPU.pFilterTexture);
   SDL_ReleaseGPUTexture(gGPU.pDevice, gGPU.pOverlayTexture);
   SDL_ReleaseGPUBuffer(gGPU.pDevice, gGPU.pVertexBuffer);
   SDL_ReleaseGPUTransferBuffer(gGPU.pDevice, gGPU.pRing);
   SDL_ReleaseGPUTransferBuffer(gGPU.pDevice, gGPU.pDownload);

   if (gGPU.pWindow != NULL)
   {
      SDL_ReleaseWindowFromGPUDevice(gGPU.pDevice, gGPU.pWindow);
   }
   SDL_DestroyGPUDevice(gGPU.pDevice);

   SDL_free(gGPU.pInstances);
   SDL_free(gGPU.pStage);
   SDL_free(gGPU.pUploads);
   SDL_free(gGPU.pAtlasEntries);
   SDL_free(gGPU.pExpectedIndex);
   SDL_free(gGPU.pExpectedBlack);

   SDL_zero(gGPU);
}

VOID
VIDEO_GPU_SetOverlay(
   SDL_Surface    *pOverlay,
   BYTE            bAlpha
)
/*++
  Purpose:

    Set the image drawn on top of the game on touch devices.

  Parameters:

    [IN]  pOverlay - the image. A color key on it is converted to transparency.

    [IN]  bAlpha - opacity of the whole image, 255 being fully opaque.

  Return value:

    None. On failure the game simply runs without the overlay.

--*/
{
   SDL_GPUTransferBufferCreateInfo upload;
   SDL_GPUTransferBuffer          *pBuffer = NULL;
   SDL_GPUCopyPass                *pCopyPass;
   SDL_Surface                    *pRGBA;
   BYTE                           *pDst;
   int                             y;

   pRGBA = SDL_ConvertSurface(pOverlay, SDL_PIXELFORMAT_RGBA32);
   if (pRGBA == NULL)
   {
      UTIL_LogOutput(LOGLEVEL_WARNING, "Touch overlay could not be converted: %s\n", SDL_GetError());
      return;
   }

   gGPU.pOverlayPipeline = VIDEO_GPU_CreatePipeline(VIDEO_GPU_FRAGMENT(overlay_frag, 1, 1),
      SDL_GetGPUSwapchainTextureFormat(gGPU.pDevice, gGPU.pWindow), TRUE);
   gGPU.pOverlayTexture = VIDEO_GPU_CreateTexture(SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
      SDL_GPU_TEXTUREUSAGE_SAMPLER, pRGBA->w, pRGBA->h);
   gGPU.flOverlayAlpha = bAlpha / 255.0f;

   SDL_zero(upload);
   upload.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
   upload.size = pRGBA->w * pRGBA->h * 4;
   pBuffer = SDL_CreateGPUTransferBuffer(gGPU.pDevice, &upload);
   pDst = (pBuffer != NULL) ? (BYTE *)SDL_MapGPUTransferBuffer(gGPU.pDevice, pBuffer, false) : NULL;

   if (gGPU.pOverlayPipeline == NULL || gGPU.pOverlayTexture == NULL || pDst == NULL)
   {
      UTIL_LogOutput(LOGLEVEL_WARNING, "Touch overlay could not be created: %s\n", SDL_GetError());

      //
      // VIDEO_GPU_Display() draws the overlay whenever the texture exists.
      //
      SDL_ReleaseGPUTexture(gGPU.pDevice, gGPU.pOverlayTexture);
      gGPU.pOverlayTexture = NULL;
   }
   else
   {
      for (y = 0; y < pRGBA->h; y++)
      {
         memcpy(pDst + y * pRGBA->w * 4, (BYTE *)pRGBA->pixels + y * pRGBA->pitch, pRGBA->w * 4);
      }
      SDL_UnmapGPUTransferBuffer(gGPU.pDevice, pBuffer);

      pCopyPass = SDL_BeginGPUCopyPass(VIDEO_GPU_Commands());
      VIDEO_GPU_UploadRegion(pCopyPass, pBuffer, 0, gGPU.pOverlayTexture, 0, 0, pRGBA->w, pRGBA->h);
      SDL_EndGPUCopyPass(pCopyPass);
   }

   SDL_ReleaseGPUTransferBuffer(gGPU.pDevice, pBuffer);
   SDL_DestroySurface(pRGBA);
}

#endif
