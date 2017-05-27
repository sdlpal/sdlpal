/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2017, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

//
// aviplay.c
//
// Simple quick and dirty AVI player specially designed for PAL Win95.
//

/*
 * Portions based on:
 *
 * Microsoft Video-1 Decoder
 * Copyright (C) 2003 The FFmpeg project
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 * Microsoft Video-1 Decoder by Mike Melanson (melanson@pcisys.net)
 * For more information about the MS Video-1 format, visit:
 *   http://www.pcisys.net/~melanson/codecs/
 */

#include "main.h"

typedef struct
{
    FILE         *fp;                 // pointer to the AVI file
    SDL_Surface  *surface;            // video buffer

    DWORD         dwVideoEndOffset;
    WORD          wWidth, wHeight;    // width and height of video
    WORD          wMsPerFrame;        // milliseconds per frame
    DWORD         dwAudioSamplesPerSec;
    DWORD         dwAudioBitsPerSample;
    DWORD         dwAudioChannels;
    SDL_AudioCVT  cvt;

    SDL_mutex    *mtxAudioData;
    BYTE          bAudioBuf[256000];  // ring buffer for audio data
    DWORD         dwAudioReadPos;
    DWORD         dwAudioWritePos;
} AVIPlayState;

static SDL_mutex *gpAVIPlayStateMutex = NULL;
static AVIPlayState *gpAVIPlayState = NULL;

typedef struct
{
    DWORD   dwFourCC;
    DWORD   dwSize;
    BYTE    bData[1];
} AVIChunk;

typedef struct
{
    DWORD         dwMicroSecPerFrame; // frame display rate (or 0)
    DWORD         dwMaxBytesPerSec; // max. transfer rate
    DWORD         dwPaddingGranularity; // pad to multiples of this size
    DWORD         dwFlags; // the ever-present flags
    DWORD         dwTotalFrames; // # frames in file
    DWORD         dwInitialFrames;
    DWORD         dwStreams;
    DWORD         dwSuggestedBufferSize;
    DWORD         dwWidth;
    DWORD         dwHeight;
    DWORD         dwReserved[4];
} MainAVIHeader;

typedef struct
{
    DWORD         fccType;
    DWORD         fccHandler;
    DWORD         dwFlags;
    WORD          wPriority;
    WORD          wLanguage;
    DWORD         dwInitialFrames;
    DWORD         dwScale;
    DWORD         dwRate; /* dwRate / dwScale == samples/second */
    DWORD         dwStart;
    DWORD         dwLength; /* In units above... */
    DWORD         dwSuggestedBufferSize;
    DWORD         dwQuality;
    DWORD         dwSampleSize;
    DWORD         rcFrame[4];
} AVIStreamHeader;

typedef struct
{
    DWORD         biSize;
    DWORD         biWidth;
    DWORD         biHeight;
    WORD          biPlanes;
    WORD          biBitCount;
    DWORD         biCompression;
    DWORD         biSizeImage;
    DWORD         biXPelsPerMeter;
    DWORD         biYPelsPerMeter;
    DWORD         biClrUsed;
    DWORD         biClrImportant;
} BitmapInfoHeader;

typedef struct
{
    WORD          wFormatTag;
    WORD          nChannels;
    DWORD         nSamplesPerSec;
    DWORD         nAvgBytesPerSec;
    WORD          nBlockAlign;
    WORD          wBitsPerSample;
} WaveFormat;

#define AVI_RIFF (((DWORD)'R') | (((DWORD)'I') << 8) | (((DWORD)'F') << 16) | (((DWORD)'F') << 24))
#define AVI_hdrl (((DWORD)'h') | (((DWORD)'d') << 8) | (((DWORD)'r') << 16) | (((DWORD)'l') << 24))
#define AVI_strl (((DWORD)'s') | (((DWORD)'t') << 8) | (((DWORD)'r') << 16) | (((DWORD)'l') << 24))
#define AVI_strh (((DWORD)'s') | (((DWORD)'t') << 8) | (((DWORD)'r') << 16) | (((DWORD)'h') << 24))
#define AVI_strf (((DWORD)'s') | (((DWORD)'t') << 8) | (((DWORD)'r') << 16) | (((DWORD)'f') << 24))
#define AVI_avih (((DWORD)'a') | (((DWORD)'v') << 8) | (((DWORD)'i') << 16) | (((DWORD)'h') << 24))
#define AVI_LIST (((DWORD)'L') | (((DWORD)'I') << 8) | (((DWORD)'S') << 16) | (((DWORD)'T') << 24))
#define AVI_movi (((DWORD)'m') | (((DWORD)'o') << 8) | (((DWORD)'v') << 16) | (((DWORD)'i') << 24))
#define AVI_01wb (((DWORD)'0') | (((DWORD)'1') << 8) | (((DWORD)'w') << 16) | (((DWORD)'b') << 24))
#define AVI_00dc (((DWORD)'0') | (((DWORD)'0') << 8) | (((DWORD)'d') << 16) | (((DWORD)'c') << 24))
#define AVI_00db (((DWORD)'0') | (((DWORD)'0') << 8) | (((DWORD)'d') << 16) | (((DWORD)'b') << 24))
#define AVI_rec  (((DWORD)'r') | (((DWORD)'e') << 8) | (((DWORD)'c') << 16) | (((DWORD)' ') << 24))
#define AVI_JUNK (((DWORD)'J') | (((DWORD)'U') << 8) | (((DWORD)'N') << 16) | (((DWORD)'K') << 24))
#define AVI_vids (((DWORD)'v') | (((DWORD)'i') << 8) | (((DWORD)'d') << 16) | (((DWORD)'s') << 24))
#define AVI_auds (((DWORD)'a') | (((DWORD)'u') << 8) | (((DWORD)'d') << 16) | (((DWORD)'s') << 24))

static VOID
PAL_ParseAVInfoList(
    AVIPlayState    *lpAVIPlayState,
    DWORD            dwEndOffset
)
{
    AVIChunk         hdr;
    DWORD            dwNextOffset;
    MainAVIHeader    aviHeader;
    AVIStreamHeader  streamHdr;
    BitmapInfoHeader bitmapHdr;
    WaveFormat       waveFormat;
    DWORD            dwInfoType = 0;

    while (ftell(lpAVIPlayState->fp) < dwEndOffset)
    {
        if (feof(lpAVIPlayState->fp))
        {
            return; // end of file reached
        }

        fread(&hdr, sizeof(DWORD) * 2, 1, lpAVIPlayState->fp);
        hdr.dwFourCC = SDL_SwapLE32(hdr.dwFourCC);
        hdr.dwSize = SDL_SwapLE32(hdr.dwSize);

        dwNextOffset = ftell(lpAVIPlayState->fp) + hdr.dwSize;

        switch (hdr.dwFourCC)
        {
        case AVI_strh:
            fread(&streamHdr, sizeof(AVIStreamHeader), 1, lpAVIPlayState->fp);
            dwInfoType = SDL_SwapLE32(streamHdr.fccType);
            break;

        case AVI_strf:
            if (dwInfoType == AVI_vids)
            {
                fread(&bitmapHdr, sizeof(bitmapHdr), 1, lpAVIPlayState->fp);
            }
            else if (dwInfoType == AVI_auds)
            {
                fread(&waveFormat, sizeof(waveFormat), 1, lpAVIPlayState->fp);
                lpAVIPlayState->dwAudioChannels = SDL_SwapLE16(waveFormat.nChannels);
                lpAVIPlayState->dwAudioSamplesPerSec = SDL_SwapLE16(waveFormat.nSamplesPerSec);
                lpAVIPlayState->dwAudioBitsPerSample = SDL_SwapLE16(waveFormat.wBitsPerSample);
            }
            break;

        default:
            break;
        }
        
        fseek(lpAVIPlayState->fp, dwNextOffset, SEEK_SET);
    }

}

static VOID
PAL_ParseHdrlList(
    AVIPlayState    *lpAVIPlayState,
    DWORD            dwEndOffset
)
{
    AVIChunk         hdr;
    DWORD            dwNextOffset, dwListType;
    MainAVIHeader    aviHeader;

    while (ftell(lpAVIPlayState->fp) < dwEndOffset)
    {
        if (feof(lpAVIPlayState->fp))
        {
            return; // end of file reached
        }
        
        fread(&hdr, sizeof(DWORD) * 2, 1, lpAVIPlayState->fp);
        hdr.dwFourCC = SDL_SwapLE32(hdr.dwFourCC);
        hdr.dwSize = SDL_SwapLE32(hdr.dwSize);

        dwNextOffset = ftell(lpAVIPlayState->fp) + hdr.dwSize;

        switch (hdr.dwFourCC)
        {
        case AVI_avih:
            fread(&aviHeader, sizeof(aviHeader), 1, lpAVIPlayState->fp);
            lpAVIPlayState->wWidth = aviHeader.dwWidth;
            lpAVIPlayState->wHeight = aviHeader.dwHeight;
            lpAVIPlayState->wMsPerFrame = aviHeader.dwMicroSecPerFrame / 1000;
            break;

        case AVI_LIST:
            fread(&dwListType, sizeof(dwListType), 1, lpAVIPlayState->fp);
            if (SDL_SwapLE32(dwListType) == AVI_strl)
            {
                PAL_ParseAVInfoList(lpAVIPlayState, dwNextOffset);
            }
            break;
        }

        fseek(lpAVIPlayState->fp, dwNextOffset, SEEK_SET);
    }
}

static VOID
PAL_ReadAVIInfo(
    AVIPlayState    *lpAVIPlayState
)
{
    AVIChunk   hdr;
    DWORD      dwListType = 0;
    DWORD      dwNextOffset = 0;

    //
    // Skip RIFF header
    //
    fseek(lpAVIPlayState->fp, 12, SEEK_SET);
    
    while (TRUE)
    {
        if (feof(lpAVIPlayState->fp))
        {
            return; // end of file reached
        }

        fread(&hdr, sizeof(DWORD) * 2, 1, lpAVIPlayState->fp);
        hdr.dwFourCC = SDL_SwapLE32(hdr.dwFourCC);
        hdr.dwSize = SDL_SwapLE32(hdr.dwSize);
        
        dwNextOffset = ftell(lpAVIPlayState->fp) + hdr.dwSize;

        switch (hdr.dwFourCC)
        {
        case AVI_LIST:
            fread(&dwListType, sizeof(DWORD), 1, lpAVIPlayState->fp);
            dwListType = SDL_SwapLE32(dwListType);

            switch (dwListType)
            {
            case AVI_hdrl:
                PAL_ParseHdrlList(lpAVIPlayState, dwNextOffset);
                break;

            case AVI_movi:
                //
                // Stop right here as the actual movie data starts
                //
                lpAVIPlayState->dwVideoEndOffset = dwNextOffset;
                return;
            }
            
            break;

        case AVI_JUNK:
        default:
            //
            // Ignore these chunks
            //
            break;
        }

        fseek(lpAVIPlayState->fp, dwNextOffset, SEEK_SET);
    }
}

static AVIPlayState *
PAL_OpenAVI(
    LPCSTR     lpszPath
)
{
    AVIPlayState *ret;

    ret = (AVIPlayState *)UTIL_calloc(1, sizeof(AVIPlayState));

    //
    // Open the file
    //
    ret->fp = UTIL_OpenFile(lpszPath);
    if (ret->fp == NULL)
    {
        fprintf(stderr, "Cannot open file: %s!\n", lpszPath);
        free(ret);
        return NULL;
    }

    PAL_ReadAVIInfo(ret);

    if (ret->wWidth == 0 || ret->wHeight == 0)
    {
        return NULL;
    }

    //
    // Create surface
    //
    ret->surface = SDL_CreateRGBSurface(SDL_SWSURFACE, ret->wWidth, ret->wHeight, 16,
                                        0x7C00, 0x03E0, 0x001F, 0x0000);

    //
    // Create mutex
    //
    ret->mtxAudioData = SDL_CreateMutex();

    //
    // Build SDL audio conversion info
    //
    SDL_BuildAudioCVT(&ret->cvt,
        (ret->dwAudioBitsPerSample == 8) ? AUDIO_U8 : AUDIO_S16LSB,
        ret->dwAudioChannels,
        ret->dwAudioSamplesPerSec,
        AUDIO_S16SYS,
        gConfig.iAudioChannels,
        gConfig.iSampleRate);

    return ret;
}

static AVIChunk *
PAL_ReadAVChunk(
    AVIPlayState  *lpAVIPlayState
)
{
    AVIChunk   hdr;
    AVIChunk  *ret = NULL;
    DWORD      dwNextOffset;

begin:
    if (feof(lpAVIPlayState->fp) || ftell(lpAVIPlayState->fp) >= lpAVIPlayState->dwVideoEndOffset)
    {
        return NULL; // end of file reached
    }

    fread(&hdr, sizeof(DWORD) * 2, 1, lpAVIPlayState->fp);
    hdr.dwFourCC = SDL_SwapLE32(hdr.dwFourCC);
    hdr.dwSize = SDL_SwapLE32(hdr.dwSize);

    dwNextOffset = ftell(lpAVIPlayState->fp) + hdr.dwSize;

    switch (hdr.dwFourCC)
    {
    case AVI_LIST:
        //
        // Just skip list header here
        //
        fseek(lpAVIPlayState->fp, sizeof(DWORD), SEEK_CUR);
        goto begin;

    case AVI_01wb:
    case AVI_00db:
    case AVI_00dc:
        //
        // got actual audio/video frame
        //
        ret = (AVIChunk *)UTIL_malloc(sizeof(DWORD) * 2 + hdr.dwSize);
        *ret = hdr;
        fread(ret->bData, hdr.dwSize, 1, lpAVIPlayState->fp);
        break;

    case AVI_JUNK:
    default:
        //
        // Ignore these chunks
        //
        fseek(lpAVIPlayState->fp, dwNextOffset, SEEK_SET);
        goto begin;
    }

    fseek(lpAVIPlayState->fp, dwNextOffset, SEEK_SET);
    return ret;
}

static VOID
PAL_CloseAVI(
    AVIPlayState  *lpAVIPlayState
)
{
    if (lpAVIPlayState->fp != NULL)
    {
        fclose(lpAVIPlayState->fp);
    }

    if (lpAVIPlayState->surface != NULL)
    {
        SDL_FreeSurface(lpAVIPlayState->surface);
    }

    if (lpAVIPlayState->mtxAudioData != NULL)
    {
        SDL_DestroyMutex(lpAVIPlayState->mtxAudioData);
    }

    free(lpAVIPlayState);
}

static VOID
PAL_AVIFeedAudio(
    AVIPlayState   *lpAVIPlayState,
    LPBYTE          lpBuffer,
    DWORD           dwSize
)
{
    SDL_mutexP(lpAVIPlayState->mtxAudioData);

    while (dwSize > 0)
    {
        DWORD dwFeedSize = dwSize;

        if (lpAVIPlayState->dwAudioWritePos + dwSize > sizeof(lpAVIPlayState->bAudioBuf))
        {
            dwFeedSize = sizeof(lpAVIPlayState->bAudioBuf) - lpAVIPlayState->dwAudioWritePos;
        }

        memcpy(&lpAVIPlayState->bAudioBuf[lpAVIPlayState->dwAudioWritePos], lpBuffer, dwFeedSize);

        lpAVIPlayState->dwAudioWritePos += dwFeedSize;
        lpAVIPlayState->dwAudioWritePos %= sizeof(lpAVIPlayState->bAudioBuf);

        lpBuffer += dwFeedSize;
        dwSize -= dwFeedSize;
    }

    SDL_mutexV(lpAVIPlayState->mtxAudioData);
}

VOID
PAL_AVIInit(
    VOID
)
{
    gpAVIPlayStateMutex = SDL_CreateMutex();
}

VOID
PAL_AVIShutdown(
    VOID
)
{
    SDL_DestroyMutex(gpAVIPlayStateMutex);
}

static VOID
PAL_RenderAVIFrame(
    SDL_Surface      *lpSurface,
    AVIChunk         *lpChunk
)
{
#define AV_RL16(x) ((((LPCBYTE)(x))[1] << 8) | ((LPCBYTE)(x))[0])
#define CHECK_STREAM_PTR(n) if ((stream_ptr + n) > lpChunk->dwSize) { return; }
    int block_ptr, pixel_ptr;
    int total_blocks;
    int pixel_x, pixel_y;  // pixel width and height iterators
    int block_x, block_y;  // block width and height iterators
    int blocks_wide, blocks_high;  // width and height in 4x4 blocks
    int block_inc;
    int row_dec;

    /* decoding parameters */
    int stream_ptr;
    unsigned char byte_a, byte_b;
    unsigned short flags;
    int skip_blocks;
    unsigned short colors[8];
    unsigned short *pixels = (unsigned short *)lpSurface->pixels;
    int stride = lpSurface->pitch / 2;

    stream_ptr = 0;
    skip_blocks = 0;
    blocks_wide = lpSurface->w / 4;
    blocks_high = lpSurface->h / 4;
    total_blocks = blocks_wide * blocks_high;
    block_inc = 4;
    row_dec = stride + 4;

    for (block_y = blocks_high; block_y > 0; block_y--)
    {
        block_ptr = ((block_y * 4) - 1) * stride;
        for (block_x = blocks_wide; block_x > 0; block_x--)
        {
            // check if this block should be skipped
            if (skip_blocks)
            {
                block_ptr += block_inc;
                skip_blocks--;
                total_blocks--;
                continue;
            }
            
            pixel_ptr = block_ptr;
            
            // get the next two bytes in the encoded data stream
            CHECK_STREAM_PTR(2);
            byte_a = lpChunk->bData[stream_ptr++];
            byte_b = lpChunk->bData[stream_ptr++];
            
            // check if the decode is finished
            if ((byte_a == 0) && (byte_b == 0) && (total_blocks == 0))
            {
                return;
            }
            else if ((byte_b & 0xFC) == 0x84)
            {
                // skip code, but don't count the current block
                skip_blocks = ((byte_b - 0x84) << 8) + byte_a - 1;
            }
            else if (byte_b < 0x80)
            {
                // 2- or 8-color encoding modes
                flags = (byte_b << 8) | byte_a;
                
                CHECK_STREAM_PTR(4);
                colors[0] = AV_RL16(&lpChunk->bData[stream_ptr]);
                stream_ptr += 2;
                colors[1] = AV_RL16(&lpChunk->bData[stream_ptr]);
                stream_ptr += 2;
                
                if (colors[0] & 0x8000)
                {
                    // 8-color encoding
                    CHECK_STREAM_PTR(12);
                    colors[2] = AV_RL16(&lpChunk->bData[stream_ptr]);
                    stream_ptr += 2;
                    colors[3] = AV_RL16(&lpChunk->bData[stream_ptr]);
                    stream_ptr += 2;
                    colors[4] = AV_RL16(&lpChunk->bData[stream_ptr]);
                    stream_ptr += 2;
                    colors[5] = AV_RL16(&lpChunk->bData[stream_ptr]);
                    stream_ptr += 2;
                    colors[6] = AV_RL16(&lpChunk->bData[stream_ptr]);
                    stream_ptr += 2;
                    colors[7] = AV_RL16(&lpChunk->bData[stream_ptr]);
                    stream_ptr += 2;
                    
                    for (pixel_y = 0; pixel_y < 4; pixel_y++)
                    {
                        for (pixel_x = 0; pixel_x < 4; pixel_x++, flags >>= 1)
                        {
                            pixels[pixel_ptr++] =
                            colors[((pixel_y & 0x2) << 1) +
                                   (pixel_x & 0x2) + ((flags & 0x1) ^ 1)];
                        }
                        pixel_ptr -= row_dec;
                    }
                }
                else
                {
                    // 2-color encoding
                    for (pixel_y = 0; pixel_y < 4; pixel_y++)
                    {
                        for (pixel_x = 0; pixel_x < 4; pixel_x++, flags >>= 1)
                        {
                            pixels[pixel_ptr++] = colors[(flags & 0x1) ^ 1];
                        }
                        pixel_ptr -= row_dec;
                    }
                }
            }
            else
            {
                // otherwise, it's a 1-color block
                colors[0] = (byte_b << 8) | byte_a;

                for (pixel_y = 0; pixel_y < 4; pixel_y++)
                {
                    for (pixel_x = 0; pixel_x < 4; pixel_x++)
                    {
                        pixels[pixel_ptr++] = colors[0];
                    }
                    pixel_ptr -= row_dec;
                }
            }
            
            block_ptr += block_inc;
            total_blocks--;
        }
    }
}

BOOL
PAL_PlayAVI(
    LPCSTR     lpszPath
)
{
    AVIPlayState *avi;
    AVIChunk     *chunk;
    DWORD         dwNextFrameTime = 0, dwCurrentTime = 0;
    BOOL          fEndPlay = FALSE;

    //
    // Open AVI file
    //
    avi = PAL_OpenAVI(lpszPath);
    if (avi == NULL)
    {
        return FALSE;
    }

    SDL_mutexP(gpAVIPlayStateMutex);
    gpAVIPlayState = avi;
    SDL_mutexV(gpAVIPlayStateMutex);

    PAL_ClearKeyState();

    while (!fEndPlay && (chunk = PAL_ReadAVChunk(avi)) != NULL)
    {
        dwCurrentTime = SDL_GetTicks();

        switch (chunk->dwFourCC)
        {
        case AVI_00dc:
        case AVI_00db:
            //
            // Video frame
            //
            PAL_RenderAVIFrame(avi->surface, chunk);
            VIDEO_DrawSurfaceToScreen(avi->surface);

            dwNextFrameTime = dwCurrentTime + avi->wMsPerFrame;
            dwCurrentTime = SDL_GetTicks();
            if (dwCurrentTime >= dwNextFrameTime)
            {
                UTIL_Delay(1);
            }
            else
            {
                UTIL_Delay(dwNextFrameTime - dwCurrentTime);
            }

            if (g_InputState.dwKeyPress & (kKeyMenu | kKeySearch))
            {
                fEndPlay = TRUE;
            }
            break;

        case AVI_01wb:
            //
            // Audio data, just feed into buffer
            //
            PAL_AVIFeedAudio(avi, chunk->bData, chunk->dwSize);
            break;
        }

        free(chunk);
    }

    SDL_mutexP(gpAVIPlayStateMutex);
    gpAVIPlayState = NULL;
    SDL_mutexV(gpAVIPlayStateMutex);

    if (fEndPlay)
    {
        //
        // Simulate a short delay (like the original game)
        //
        UTIL_Delay(500);
    }

    PAL_CloseAVI(avi);
    return TRUE;
}

VOID SDLCALL
AVI_FillAudioBuffer(
    LPVOID          udata,
    LPBYTE          stream,
    INT             len
)
{
    SDL_mutexP(gpAVIPlayStateMutex);
    if (gpAVIPlayState != NULL)
    {
        FLOAT flRateScale = ((FLOAT)gConfig.iSampleRate / gpAVIPlayState->dwAudioSamplesPerSec);
        while (len > 0 && gpAVIPlayState->dwAudioReadPos != gpAVIPlayState->dwAudioWritePos)
        {
            INT remainingLen = gpAVIPlayState->dwAudioWritePos - gpAVIPlayState->dwAudioReadPos;
            INT samplesToRead;

            if (remainingLen < 0)
            {
                remainingLen = sizeof(gpAVIPlayState->bAudioBuf) - gpAVIPlayState->dwAudioReadPos;
            }

            samplesToRead = remainingLen / gpAVIPlayState->dwAudioChannels / (gpAVIPlayState->dwAudioBitsPerSample / 8);
            if (samplesToRead > len / 2 / gConfig.iAudioChannels / flRateScale)
            {
                samplesToRead = len / 2 / gConfig.iAudioChannels / flRateScale;
            }

            gpAVIPlayState->cvt.buf = stream;
            gpAVIPlayState->cvt.len = samplesToRead *
                gpAVIPlayState->dwAudioChannels * (gpAVIPlayState->dwAudioBitsPerSample / 8);

            memcpy(stream, &gpAVIPlayState->bAudioBuf[gpAVIPlayState->dwAudioReadPos],
                samplesToRead * gpAVIPlayState->dwAudioChannels * (gpAVIPlayState->dwAudioBitsPerSample / 8));

            SDL_ConvertAudio(&gpAVIPlayState->cvt);

            stream += (DWORD)(samplesToRead * 2 * gConfig.iAudioChannels * flRateScale);
            len -= samplesToRead * 2 * gConfig.iAudioChannels * flRateScale;

            gpAVIPlayState->dwAudioReadPos += samplesToRead * gpAVIPlayState->dwAudioChannels *
                (gpAVIPlayState->dwAudioBitsPerSample / 8);

            gpAVIPlayState->dwAudioReadPos %= sizeof(gpAVIPlayState->bAudioBuf);
        }
    }
    SDL_mutexV(gpAVIPlayStateMutex);
}
