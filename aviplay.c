/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2020, SDLPAL development team.
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

#include "util.h"
#include "audio.h"
#include "aviplay.h"
#include "input.h"
#include "video.h"
#include "riff.h"
#include "palcfg.h"

#if SDL_BYTEORDER == SDL_BIG_ENDIAN

# define SwapStruct32(v, s) \
	for(int s##_i = 0; s##_i < sizeof(s) / sizeof(uint32_t); s##_i++) \
		((uint32_t *)&v)[s##_i] = SDL_Swap32(((uint32_t *)&v)[s##_i])

# define SwapStructFields(v, f1, f2) v.f1 ^= v.f2, v.f2 ^= v.f1, v.f1 ^= v.f2


#else

# define SwapStruct32(...)
# define SwapStructFields(...)

#endif

#define HAS_FLAG(v, f) (((v) & (f)) == (f))

#define MAX_AVI_BLOCK_LEVELS 3

#define FLAGS_AVI_MAIN_HEADER  0x01
#define FLAGS_AVI_VIDEO_FORMAT 0x02
#define FLAGS_AVI_AUDIO_FORMAT 0x04
#define FLAGS_AVI_ALL_HEADERS  0x07

typedef struct AVIPlayState
{
	SDL_mutex     *selfMutex;
    volatile FILE *fp;                 // pointer to the AVI file
    SDL_Surface   *surface;            // video buffer

    long           lVideoEndPos;
	uint32_t       dwMicroSecPerFrame;       // microseconds per frame
	uint32_t       dwBufferSize;
    SDL_AudioCVT   cvt;

	uint8_t       *pChunkBuffer;
	uint8_t       *pbAudioBuf;  // ring buffer for audio data
	uint32_t       dwAudBufLen;
	uint32_t       dwAudioReadPos;
	uint32_t       dwAudioWritePos;

	BOOL          fInterleaved;
} AVIPlayState;

static AVIPlayState gAVIPlayState;

static AVIPlayState *
PAL_ReadAVIInfo(
	FILE         *fp,
	AVIPlayState *avi
)
{
	RIFFHeader hdr;
	AVIMainHeader aviHeader;
	AVIStreamHeader streamHeader = { 0 };
	BitmapInfoHeader bih;
	WAVEFormatEx wfe;
	uint32_t   block_type[MAX_AVI_BLOCK_LEVELS];
	long       next_pos[MAX_AVI_BLOCK_LEVELS];
	long       file_length = (fseek(fp, 0, SEEK_END), ftell(fp)), pos = 0;
	int        current_level = 0, flags = 0;

    //
    // Check RIFF file header
    //
	fseek(fp, 0, SEEK_SET);
	if(fread(&hdr, sizeof(RIFFHeader), 1, fp) != 1)
	{
		UTIL_LogOutput(LOGLEVEL_WARNING, "No RIFF header!");
		return NULL;
	}
	hdr.signature = SDL_SwapLE32(hdr.signature);
	hdr.type      = SDL_SwapLE32(hdr.type);
	hdr.length    = SDL_SwapLE32(hdr.length);
	if (hdr.signature != RIFF_RIFF || hdr.type != RIFF_AVI ||
		hdr.length > (uint32_t)(file_length - sizeof(RIFFHeader) + sizeof(uint32_t)))
	{
		UTIL_LogOutput(LOGLEVEL_WARNING, "Illegal AVI RIFF header!");
		return NULL;
	}
	else
	{
		next_pos[current_level] = (pos += sizeof(RIFFHeader)) + hdr.length;
		block_type[current_level++] = hdr.type;
	}
    
    while (!feof(fp) && current_level > 0)
    {
		RIFFBlockHeader block;
		fseek(fp, pos, SEEK_SET);
		if (fread(&block.type, sizeof(RIFFChunkHeader), 1, fp) != 1)
		{
			UTIL_LogOutput(LOGLEVEL_WARNING, "Illegal AVI RIFF LIST/Chunk header!");
			return NULL;
		}
		else
		{
			block.type = SDL_SwapLE32(block.type);
			block.length = SDL_SwapLE32(block.length);
			pos += sizeof(RIFFChunkHeader);
		}

		//
		// Read further if current block is a 'LIST'
		//
		if (block.type == AVI_LIST)
		{
			if (fread(&block.list.type, sizeof(RIFFListHeader) - sizeof(RIFFChunkHeader), 1, fp) != 1)
			{
				UTIL_LogOutput(LOGLEVEL_WARNING, "Illegal AVI RIFF LIST header!");
				return NULL;
			}
			else
			{
				block.list.type = SDL_SwapLE32(block.list.type);
			}
		}

		switch (block_type[current_level - 1])
		{
		case RIFF_AVI:
			//
			// RIFF_AVI only appears at top-level
			//
			if (current_level != 1)
			{
				UTIL_LogOutput(LOGLEVEL_WARNING, "RIFF 'AVI ' block appears at non-top level!");
				return NULL;
			}
			//
			// For 'LIST' block, should read its contents
			//
			if (block.type == AVI_LIST)
			{
				next_pos[current_level] = pos + block.length;
				block_type[current_level++] = block.list.type;
				pos += sizeof(RIFFListHeader) - sizeof(RIFFChunkHeader);
				continue;
			}
			//
			// Ignore any block types other than 'LIST'
			//
			break;

		case AVI_hdrl:
			//
			// AVI_hdrl only appears at second-level
			//
			if (current_level != 2)
			{
				UTIL_LogOutput(LOGLEVEL_WARNING, "RIFF 'hdrl' block does not appear at second level!");
				return NULL;
			}
			switch (block.type)
			{
			case AVI_avih:
				//
				// The main header should only appear once
				//
				if (HAS_FLAG(flags, FLAGS_AVI_MAIN_HEADER))
				{
					UTIL_LogOutput(LOGLEVEL_WARNING, "More than one RIFF 'avih' blocks appear!");
					return NULL;
				}
				if (fread(&aviHeader, sizeof(AVIMainHeader), 1, fp) != 1)
				{
					UTIL_LogOutput(LOGLEVEL_WARNING, "RIFF 'avih' blocks corrupted!");
					return NULL;
				}
				SwapStruct32(aviHeader, AVIMainHeader);
				flags |= FLAGS_AVI_MAIN_HEADER;
				if (aviHeader.dwWidth == 0 || aviHeader.dwHeight == 0)
				{
					UTIL_LogOutput(LOGLEVEL_WARNING, "Invalid AVI frame size!");
					return NULL;
				}
				if (HAS_FLAG(aviHeader.dwFlags, AVIF_MUSTUSEINDEX))
				{
					UTIL_LogOutput(LOGLEVEL_WARNING, "No built-in support for index-based AVI!");
					return NULL;
				}
				break;
			case AVI_LIST:
				if (block.list.type == AVI_strl)
				{
					next_pos[current_level] = pos + block.length;
					block_type[current_level++] = block.list.type;
					pos += sizeof(RIFFListHeader) - sizeof(RIFFChunkHeader);
					continue;
				}
				break;
			}
			break;

		case AVI_movi:
			//
			// AVI_movi only appears at second-level and all headers should be read before
			//
			if (current_level != 2 || !HAS_FLAG(flags, FLAGS_AVI_ALL_HEADERS))
			{
				UTIL_LogOutput(LOGLEVEL_WARNING, "RIFF 'movi' block does not appear at second level or the AVI does not contain both video & audio!");
				return NULL;
			}
			//
			// Stop parsing here as actual movie data starts
			//
			fseek(fp, pos - sizeof(RIFFChunkHeader), SEEK_SET);
			avi->lVideoEndPos = next_pos[current_level - 1];
			avi->dwMicroSecPerFrame = aviHeader.dwMicroSecPerFrame;
			//
			// Create surface
			//
			avi->surface = SDL_CreateRGBSurface(SDL_SWSURFACE,
				bih.biWidth, bih.biHeight, bih.biBitCount,
				0x7C00, 0x03E0, 0x001F, 0x0000);
			//
			// Build SDL audio conversion info
			//
			SDL_BuildAudioCVT(&avi->cvt,
				(wfe.format.wBitsPerSample == 8) ? AUDIO_U8 : AUDIO_S16LSB,
				wfe.format.nChannels, wfe.format.nSamplesPerSec,
				AUDIO_S16SYS,
				AUDIO_GetDeviceSpec()->channels,
				AUDIO_GetDeviceSpec()->freq);
			//
			// Allocate chunk buffer
			// Since SDL converts audio in-place, we need to make the buffer large enough to hold converted data
			//
			avi->dwBufferSize = aviHeader.dwSuggestedBufferSize * avi->cvt.len_mult + sizeof(RIFFChunkHeader);
			if (avi->dwBufferSize > 0)
				avi->pChunkBuffer = UTIL_malloc(avi->dwBufferSize);
			else
				avi->pChunkBuffer = NULL;
			//
			// Allocate audio buffer, the buffer size is large enough to hold two-second audio data
			//
			avi->dwAudBufLen = max(wfe.format.nAvgBytesPerSec * 2, aviHeader.dwSuggestedBufferSize) * avi->cvt.len_mult;
			avi->pbAudioBuf = (uint8_t *)UTIL_malloc(avi->dwAudBufLen);
			avi->dwAudioReadPos = avi->dwAudioWritePos = 0;
			return avi;

		case AVI_strl:
			//
			// AVI_strl only appears at third-level
			//
			if (current_level != 3)
			{
				UTIL_LogOutput(LOGLEVEL_WARNING, "RIFF 'hdrl' block does not appear at third level!");
				return NULL;
			}
			switch (block.type)
			{
			case AVI_strh:
				// strh should be the first block of the list
				if (streamHeader.fccType != 0)
				{
					UTIL_LogOutput(LOGLEVEL_WARNING, "RIFF 'strh' block does not appear at first!");
					return NULL;
				}
				if (fread(&streamHeader, sizeof(AVIStreamHeader), 1, fp) != 1)
				{
					UTIL_LogOutput(LOGLEVEL_WARNING, "RIFF 'hdrl' block data corrupted!");
					return NULL;
				}
				SwapStruct32(streamHeader, AVIStreamHeader);
				SwapStructFields(streamHeader, wLanguage, wPriority);
				SwapStructFields(streamHeader, rcFrame[0], rcFrame[1]);
				SwapStructFields(streamHeader, rcFrame[2], rcFrame[3]);
				break;
			case AVI_strf:
				//
				// AVI_strf should follow AVI_strh
				// Accept only one video stream & one audio stream
				//
				switch (streamHeader.fccType)
				{
				case AVI_vids:
					if (HAS_FLAG(flags, FLAGS_AVI_VIDEO_FORMAT) || (streamHeader.fccHandler != VIDS_MSVC && streamHeader.fccHandler != VIDS_msvc))
					{
						UTIL_LogOutput(LOGLEVEL_WARNING, "The AVI uses video codec with no built-in support, or video codec appeared before!");
						return NULL;
					}
					if (fread(&bih, sizeof(BitmapInfoHeader), 1, fp) != 1)
					{
						UTIL_LogOutput(LOGLEVEL_WARNING, "Video codec information corrupted!");
						return NULL;
					}
					SwapStruct32(bih, BitmapInfoHeader);
					SwapStructFields(bih, biPlanes, biBitCount);
					if (bih.biBitCount != 16)
					{
						UTIL_LogOutput(LOGLEVEL_WARNING, "Built-in AVI playing support only 16-bit video!");
						return NULL;
					}
					flags |= FLAGS_AVI_VIDEO_FORMAT;
					break;
				case AVI_auds:
					if (HAS_FLAG(flags, FLAGS_AVI_AUDIO_FORMAT) || streamHeader.fccHandler != 0)
					{
						UTIL_LogOutput(LOGLEVEL_WARNING, "The AVI uses audio codec with no built-in support, or audio codec appeared before!");
						return NULL;
					}
					if (fread(&wfe, sizeof(WAVEFormatPCM) + sizeof(uint16_t), 1, fp) != 1)
					{
						UTIL_LogOutput(LOGLEVEL_WARNING, "Audio codec information corrupted!");
						return NULL;
					}
					SwapStruct32(wfe, WAVEFormatPCM);
					SwapStructFields(wfe.format, wFormatTag, nChannels);
					SwapStructFields(wfe.format, nBlockAlign, wBitsPerSample);
					flags |= FLAGS_AVI_AUDIO_FORMAT;
					break;
				}
				//
				// One strf per strh, reset the fccType here to prepare for next strh
				//
				streamHeader.fccType = 0;
				break;
			}
		}

		//
		// Goto next block
		//
		pos += block.length;

		//
		// Check if it is the end of the parent block
		//
		while (current_level > 0 && pos == next_pos[current_level - 1])
		{
			current_level--;
		}
		//
		// Returns NULL if block is illegaly formed
		//
		if (current_level > 0 && pos > next_pos[current_level - 1])
		{
			return NULL;
		}
    }

	return NULL;
}


static RIFFChunk *
PAL_ReadDataChunk(
	FILE     *fp,
	long      endPos,
	void     *userbuf,
	uint32_t  buflen,
	int       mult
)
{
	RIFFBlockHeader  hdr;
	RIFFChunk       *chunk = NULL;
	long             pos = feof(fp) ? endPos : ftell(fp);

	while (chunk == NULL && pos < endPos)
	{
		if (fread(&hdr, sizeof(RIFFChunkHeader), 1, fp) != 1) return NULL;

		hdr.type = SDL_SwapLE32(hdr.type);
		hdr.length = SDL_SwapLE32(hdr.length);
		pos += sizeof(RIFFChunkHeader);

		switch (hdr.type)
		{
		case AVI_01wb:
		case AVI_00db:
		case AVI_00dc:
			//
			// got actual audio/video frame
			//
			if (userbuf && buflen >= sizeof(RIFFChunkHeader) + hdr.length)
				chunk = (RIFFChunk *)userbuf;
			else
				chunk = (RIFFChunk *)UTIL_malloc(sizeof(RIFFChunkHeader) + hdr.length * (hdr.type == AVI_01wb ? mult : 1));
			if (fread(chunk->data, hdr.length, 1, fp) != 1)
			{
				free(chunk);
				return NULL;
			}
			chunk->header = hdr.chunk;
			break;

		case AVI_LIST:
			//
			// Only 'rec ' LIST is allowed here, if not, skip it completely
			//
			if (fread(&hdr.list.type, sizeof(uint32_t), 1, fp) != 1) return NULL;
			hdr.list.type = SDL_SwapLE32(hdr.list.type);
			if (hdr.list.type == AVI_rec) break;
		case AVI_JUNK:
		default:
			//
			// Ignore unrecognized chunks
			//
			fseek(fp, pos += hdr.length, SEEK_SET);
		}
	}

    return chunk;
}

static void
PAL_AVIFeedAudio(
    AVIPlayState   *avi,
    uint8_t        *buffer,
    uint32_t        size
)
{
    //
    // Convert audio in-place at the original buffer
    // This makes filling process much more simpler
    //
    avi->cvt.buf = buffer;
    avi->cvt.len = size;
    SDL_ConvertAudio(&avi->cvt);
    size = avi->cvt.len_cvt;

    SDL_mutexP(avi->selfMutex);
    while (size > 0)
    {
        uint32_t feed_size = (avi->dwAudioWritePos + size > avi->dwAudBufLen) ? avi->dwAudBufLen - avi->dwAudioWritePos : size;

        memcpy(avi->pbAudioBuf + avi->dwAudioWritePos, buffer, feed_size);

        avi->dwAudioWritePos = (avi->dwAudioWritePos + feed_size) % avi->dwAudBufLen;

        buffer += feed_size;
        size -= feed_size;
    }
    SDL_mutexV(avi->selfMutex);
}

void
PAL_AVIInit(
	void
)
{
    gAVIPlayState.selfMutex = SDL_CreateMutex();
}

void
PAL_AVIShutdown(
	void
)
{
    SDL_DestroyMutex(gAVIPlayState.selfMutex);
}

static void
PAL_RenderAVIFrameToSurface(
    SDL_Surface      *lpSurface,
    const RIFFChunk  *lpChunk
)
{
#define AV_RL16(x) ((((const uint8_t *)(x))[1] << 8) | ((const uint8_t *)(x))[0])
#define CHECK_STREAM_PTR(n) if ((stream_ptr + n) > lpChunk->header.length) { return; }

    /* decoding parameters */
	uint16_t *pixels = (unsigned short *)lpSurface->pixels;
	uint32_t  stream_ptr = 0, skip_blocks = 0;
	uint32_t  stride = lpSurface->pitch >> 1;
	const int block_inc = 4;
	const int row_dec = stride + 4;
	const int blocks_wide = lpSurface->w >> 2; // width in 4x4 blocks
	const int blocks_high = lpSurface->h >> 2; // height in 4x4 blocks
	uint32_t  total_blocks = blocks_wide * blocks_high;

    for (int block_y = blocks_high; block_y > 0; block_y--)
    {
        int block_ptr = ((block_y * 4) - 1) * stride;
        for (int block_x = blocks_wide; block_x > 0; block_x--)
        {
            // check if this block should be skipped
            if (skip_blocks)
            {
                block_ptr += block_inc;
                skip_blocks--;
                total_blocks--;
                continue;
            }
            
            int pixel_ptr = block_ptr;
            
            // get the next two bytes in the encoded data stream
            CHECK_STREAM_PTR(2);
            uint8_t byte_a = lpChunk->data[stream_ptr++];
			uint8_t byte_b = lpChunk->data[stream_ptr++];
            
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
                uint16_t flags = (byte_b << 8) | byte_a;
				uint16_t colors[8];
                
                CHECK_STREAM_PTR(4);
                colors[0] = AV_RL16(&lpChunk->data[stream_ptr]);
                stream_ptr += 2;
                colors[1] = AV_RL16(&lpChunk->data[stream_ptr]);
                stream_ptr += 2;
                
                if (colors[0] & 0x8000)
                {
                    // 8-color encoding
                    CHECK_STREAM_PTR(12);
                    colors[2] = AV_RL16(&lpChunk->data[stream_ptr]);
                    stream_ptr += 2;
                    colors[3] = AV_RL16(&lpChunk->data[stream_ptr]);
                    stream_ptr += 2;
                    colors[4] = AV_RL16(&lpChunk->data[stream_ptr]);
                    stream_ptr += 2;
                    colors[5] = AV_RL16(&lpChunk->data[stream_ptr]);
                    stream_ptr += 2;
                    colors[6] = AV_RL16(&lpChunk->data[stream_ptr]);
                    stream_ptr += 2;
                    colors[7] = AV_RL16(&lpChunk->data[stream_ptr]);
                    stream_ptr += 2;
                    
                    for (int pixel_y = 0; pixel_y < 4; pixel_y++)
                    {
                        for (int pixel_x = 0; pixel_x < 4; pixel_x++, flags >>= 1)
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
                    for (int pixel_y = 0; pixel_y < 4; pixel_y++)
                    {
                        for (int pixel_x = 0; pixel_x < 4; pixel_x++, flags >>= 1)
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
				uint16_t color = (byte_b << 8) | byte_a;

                for (int pixel_y = 0; pixel_y < 4; pixel_y++)
                {
                    for (int pixel_x = 0; pixel_x < 4; pixel_x++)
                    {
                        pixels[pixel_ptr++] = color;
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
	if (!gConfig.fEnableAviPlay) return FALSE;

	//
	// Open the file
	//
	FILE *fp = UTIL_OpenFile(lpszPath);
	if (fp == NULL)
	{
		UTIL_LogOutput(LOGLEVEL_WARNING, "Cannot open AVI file: %s!\n", lpszPath);
		return FALSE;
	}

	AVIPlayState *avi = PAL_ReadAVIInfo(fp, &gAVIPlayState);
	if (avi == NULL)
	{
		UTIL_LogOutput(LOGLEVEL_WARNING, "Failed to parse AVI file or its format not supported!\n");
		fclose(fp);
		return FALSE;
	}

    PAL_ClearKeyState();

    VIDEO_ChangeDepth(avi->surface->format->BitsPerPixel);

	BOOL       fEndPlay = FALSE;
	RIFFChunk *buf = (RIFFChunk *)avi->pChunkBuffer;
	uint32_t   len = avi->dwBufferSize;
	uint32_t   dwMicroSecChange = 0;
	uint32_t   dwCurrentTime = SDL_GetTicks();
	uint32_t   dwNextFrameTime;
	uint32_t   dwFrameStartTime = dwCurrentTime;

    while (!fEndPlay)
    {
		RIFFChunk *chunk = PAL_ReadDataChunk(fp, avi->lVideoEndPos, buf, len, avi->cvt.len_mult);

		if (chunk == NULL) break;

        switch (chunk->header.type)
        {
        case AVI_00dc:
        case AVI_00db:
            //
            // Video frame
            //
			dwNextFrameTime = dwFrameStartTime + (avi->dwMicroSecPerFrame / 1000);

			dwMicroSecChange += avi->dwMicroSecPerFrame % 1000;
			dwNextFrameTime += dwMicroSecChange / 1000;
			dwMicroSecChange %= 1000;

			PAL_RenderAVIFrameToSurface(avi->surface, chunk);
            VIDEO_DrawSurfaceToScreen(avi->surface);

            dwCurrentTime = SDL_GetTicks();

            // Check input states here
            UTIL_Delay(dwCurrentTime >= dwNextFrameTime ? 1 : dwNextFrameTime - dwCurrentTime);
            dwFrameStartTime = SDL_GetTicks();

            if (g_InputState.dwKeyPress & (kKeyMenu | kKeySearch))
            {
                fEndPlay = TRUE;
            }
            break;

        case AVI_01wb:
            //
            // Audio data, just convert it & feed into buffer
            //
            PAL_AVIFeedAudio(avi, chunk->data, chunk->header.length);
			//
			// Only enable AVI audio when data are available
			// We do not lock on the 'if' because only this function changes 'avi->fp'
			//
			if (!avi->fp)
			{
				SDL_mutexP(avi->selfMutex);
				avi->fp = fp;
				SDL_mutexV(avi->selfMutex);
			}
            break;
        }

        if (chunk != buf) free(chunk);
    }

	SDL_mutexP(avi->selfMutex);
	avi->fp = NULL;
	SDL_mutexV(avi->selfMutex);

    if (fEndPlay)
    {
        //
        // Simulate a short delay (like the original game)
        //
        UTIL_Delay(500);
    }

    VIDEO_ChangeDepth(0);

	if (avi->surface != NULL)
	{
		SDL_FreeSurface(avi->surface);
		avi->surface = NULL;
	}

	if (avi->pChunkBuffer)
	{
		free(avi->pChunkBuffer);
		avi->pChunkBuffer = NULL;
	}

	if (avi->pbAudioBuf)
	{
		free(avi->pbAudioBuf);
		avi->pbAudioBuf = NULL;
	}

	fclose(fp);

	return TRUE;
}

VOID SDLCALL
AVI_FillAudioBuffer(
	void       *udata,
	uint8_t    *stream,
	int         len
)
{
    AVIPlayState *avi = (AVIPlayState *)udata;

    SDL_mutexP(avi->selfMutex);
    while (avi->fp != NULL && len > 0 && avi->dwAudioWritePos != avi->dwAudioReadPos)
    {
        uint32_t fill_size = (avi->dwAudioReadPos + len > avi->dwAudBufLen) ? avi->dwAudBufLen - avi->dwAudioReadPos : len;

        if (avi->dwAudioWritePos > avi->dwAudioReadPos &&
            fill_size > avi->dwAudioWritePos - avi->dwAudioReadPos)
        {
            fill_size = avi->dwAudioWritePos - avi->dwAudioReadPos;
        }

        memcpy(stream, avi->pbAudioBuf + avi->dwAudioReadPos, fill_size);

        avi->dwAudioReadPos = (avi->dwAudioReadPos + fill_size) % avi->dwAudBufLen;

        stream += fill_size;
        len -= fill_size;
    }
    SDL_mutexV(avi->selfMutex);
}

void *
AVI_GetPlayState(
	void
)
{
	return &gAVIPlayState;
}
