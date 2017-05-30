#ifndef PAL_RIFF_H
#define PAL_RIFF_H

#include <stdint.h>

typedef struct RIFFHeader
{
	uint32_t signature;         /* 'RIFF' */
	uint32_t length;            /* Total length minus eight, little-endian */
	uint32_t type;              /* 'WAVE', 'AVI ', ... */
} RIFFHeader;

typedef struct RIFFChunkHeader
{
	uint32_t type;             /* 'fmt ', 'hdrl', 'movi' and so on */
	uint32_t length;           /* Total chunk length minus eight, little-endian */
} RIFFChunkHeader;

typedef struct RIFFChunk
{
	RIFFChunkHeader header;
	uint8_t         data[1];
} RIFFChunk;

typedef struct RIFFListHeader
{
	uint32_t signature;        /* 'LIST' */
	uint32_t length;           /* Total list length minus eight, little-endian */
	uint32_t type;             /* 'fmt ', 'hdrl', 'movi' and so on */
} RIFFListHeader;

typedef union RIFFBlockHeader
{
	struct {
		uint32_t  type;
		uint32_t  length;
	};
	RIFFChunkHeader chunk;
	RIFFListHeader  list;
} RIFFBlockHeader;

typedef struct WAVEFormatPCM
{
	uint16_t wFormatTag;      /* format type */
	uint16_t nChannels;       /* number of channels (i.e. mono, stereo, etc.) */
	uint32_t nSamplesPerSec;  /* sample rate */
	uint32_t nAvgBytesPerSec; /* for buffer estimation */
	uint16_t nBlockAlign;     /* block size of data */
	uint16_t wBitsPerSample;
} WAVEFormatPCM;

typedef struct WAVEFormatEx
{
	WAVEFormatPCM format;
	uint16_t      cbSize;
} WAVEFormatEx;

typedef struct AVIMainHeader
{
	uint32_t      dwMicroSecPerFrame; // frame display rate (or 0)
	uint32_t      dwMaxBytesPerSec; // max. transfer rate
	uint32_t      dwPaddingGranularity; // pad to multiples of this size
#define AVIF_HASINDEX        0x00000010 // Index at end of file?
#define AVIF_MUSTUSEINDEX    0x00000020
#define AVIF_ISINTERLEAVED   0x00000100
	uint32_t      dwFlags; // the ever-present flags
	uint32_t      dwTotalFrames; // # frames in file
	uint32_t      dwInitialFrames;
	uint32_t      dwStreams;
	uint32_t      dwSuggestedBufferSize;
	uint32_t      dwWidth;
	uint32_t      dwHeight;
	uint32_t      dwReserved[4];
} AVIMainHeader;

typedef struct AVIStreamHeader
{
	uint32_t      fccType;
	uint32_t      fccHandler;
	uint32_t      dwFlags;
	uint16_t      wPriority;
	uint16_t      wLanguage;
	uint32_t      dwInitialFrames;
	uint32_t      dwScale;
	uint32_t      dwRate; /* dwRate / dwScale == samples/second */
	uint32_t      dwStart;
	uint32_t      dwLength; /* In units above... */
	uint32_t      dwSuggestedBufferSize;
	uint32_t      dwQuality;
	uint32_t      dwSampleSize;
	uint16_t      rcFrame[4];
} AVIStreamHeader;

typedef struct BitmapInfoHeader
{
	uint32_t      biSize;
	uint32_t      biWidth;
	uint32_t      biHeight;
	uint16_t      biPlanes;
	uint16_t      biBitCount;
	uint32_t      biCompression;
	uint32_t      biSizeImage;
	uint32_t      biXPelsPerMeter;
	uint32_t      biYPelsPerMeter;
	uint32_t      biClrUsed;
	uint32_t      biClrImportant;
} BitmapInfoHeader;

#define RIFF_RIFF (((uint32_t)'R') | (((uint32_t)'I') << 8) | (((uint32_t)'F') << 16) | (((uint32_t)'F') << 24))

#define RIFF_WAVE (((uint32_t)'W') | (((uint32_t)'A') << 8) | (((uint32_t)'V') << 16) | (((uint32_t)'E') << 24))
#define WAVE_fmt  (((uint32_t)'f') | (((uint32_t)'m') << 8) | (((uint32_t)'t') << 16) | (((uint32_t)' ') << 24))
#define WAVE_data (((uint32_t)'d') | (((uint32_t)'a') << 8) | (((uint32_t)'t') << 16) | (((uint32_t)'a') << 24))

#define RIFF_AVI  (((uint32_t)'A') | (((uint32_t)'V') << 8) | (((uint32_t)'I') << 16) | (((uint32_t)' ') << 24))
#define AVI_hdrl  (((uint32_t)'h') | (((uint32_t)'d') << 8) | (((uint32_t)'r') << 16) | (((uint32_t)'l') << 24))
#define AVI_strl  (((uint32_t)'s') | (((uint32_t)'t') << 8) | (((uint32_t)'r') << 16) | (((uint32_t)'l') << 24))
#define AVI_strh  (((uint32_t)'s') | (((uint32_t)'t') << 8) | (((uint32_t)'r') << 16) | (((uint32_t)'h') << 24))
#define AVI_strf  (((uint32_t)'s') | (((uint32_t)'t') << 8) | (((uint32_t)'r') << 16) | (((uint32_t)'f') << 24))
#define AVI_avih  (((uint32_t)'a') | (((uint32_t)'v') << 8) | (((uint32_t)'i') << 16) | (((uint32_t)'h') << 24))
#define AVI_LIST  (((uint32_t)'L') | (((uint32_t)'I') << 8) | (((uint32_t)'S') << 16) | (((uint32_t)'T') << 24))
#define AVI_movi  (((uint32_t)'m') | (((uint32_t)'o') << 8) | (((uint32_t)'v') << 16) | (((uint32_t)'i') << 24))
#define AVI_01wb  (((uint32_t)'0') | (((uint32_t)'1') << 8) | (((uint32_t)'w') << 16) | (((uint32_t)'b') << 24))
#define AVI_00dc  (((uint32_t)'0') | (((uint32_t)'0') << 8) | (((uint32_t)'d') << 16) | (((uint32_t)'c') << 24))
#define AVI_00db  (((uint32_t)'0') | (((uint32_t)'0') << 8) | (((uint32_t)'d') << 16) | (((uint32_t)'b') << 24))
#define AVI_rec   (((uint32_t)'r') | (((uint32_t)'e') << 8) | (((uint32_t)'c') << 16) | (((uint32_t)' ') << 24))
#define AVI_JUNK  (((uint32_t)'J') | (((uint32_t)'U') << 8) | (((uint32_t)'N') << 16) | (((uint32_t)'K') << 24))
#define AVI_vids  (((uint32_t)'v') | (((uint32_t)'i') << 8) | (((uint32_t)'d') << 16) | (((uint32_t)'s') << 24))
#define AVI_auds  (((uint32_t)'a') | (((uint32_t)'u') << 8) | (((uint32_t)'d') << 16) | (((uint32_t)'s') << 24))
#define VIDS_MSVC (((uint32_t)'M') | (((uint32_t)'S') << 8) | (((uint32_t)'V') << 16) | (((uint32_t)'C') << 24))
#define VIDS_msvc (((uint32_t)'m') | (((uint32_t)'s') << 8) | (((uint32_t)'v') << 16) | (((uint32_t)'c') << 24))

#endif
