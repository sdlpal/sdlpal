/*
 * A small header-only library to load an image into a RGB(A) SDL_Surface*,
 * like a stripped down version of SDL_Image, but using stb_image.h to decode
 * images and thus without any further external dependencies.
 * Supports all filetypes supported by stb_image (JPEG, PNG, TGA, BMP, PSD, ...
 * See stb_image.h for details).
 *
 * (C) 2015 Daniel Gibson
 *
 * Homepage: https://github.com/DanielGibson/Snippets/
 *
 * Dependencies:
 *     libSDL2      http://www.libsdl.org
 *     stb_image.h  https://github.com/nothings/stb
 *
 * Usage:
 *   Put this file and stb_image.h somewhere in your project.
 *   In *one* of your .c/.cpp files, do
 *     #define SDL_STBIMAGE_IMPLEMENTATION
 *     #include "SDL_stbimage.h"
 *   to create the implementation of this library in that file.
 *   You can just #include "SDL_stbimage.h" (without the #define) in other source
 *   files to use it there. (See also below this comment for an usage example)
 *   This header implicitly #includes <SDL.h> and "stb_image.h".
 *
 *   You can #define SDL_STBIMG_DEF before including this header if you want to
 *   prepend anything to the function signatures (like "static", "inline",
 *   "__declspec(dllexport)", ...)
 *     Example: #define SDL_STBIMG_DEF static inline
 *
 *   By default, this deactivates stb_image's load from file functions via
 *   #define STBI_NO_STDIO, as they use stdio.h  and that adds a dependency to the
 *   CRT on windows and with SDL you're better off using SDL_RWops, incl. SDL_RWFromFile()
 *   If you wanna use stbi_load(), stbi_info(), stbi_load_from_file() etc anyway, do
 *     #define SDL_STBIMG_ALLOW_STDIO
 *   before including this header.
 *   (Note that all the STBIMG_* functions of this lib will work without it)
 *
 *   stb_image.h uses assert.h by default. You can #define STBI_ASSERT(x)
 *   before the implementation-#include of SDL_stbimage.h to avoid that.
 *   By default stb_image supports HDR images, for that it needs pow() from libm.
 *   If you don't need HDR (it can't be loaded into a SDL_Surface anyway),
 *   #define STBI_NO_LINEAR and #define STBI_NO_HDR before including this header.
 *
 * License:
 *   This software is dual-licensed to the public domain and under the following
 *   license: you are granted a perpetual, irrevocable license to copy, modify,
 *   publish, and distribute this file as you see fit.
 *   No warranty implied; use at your own risk.
 *
 * So you can do whatever you want with this code, including copying it
 * (or parts of it) into your own source.
 * No need to mention me or this "license" in your code or docs, even though
 * it would be appreciated, of course.
 */

#if 0 // Usage Example:
  #define SDL_STBIMAGE_IMPLEMENTATION
  #include "SDL_stbimage.h"

  void yourFunction(const char* imageFilePath)
  {
    SDL_Surface* surf = STBIMG_Load(imageFilePath);
    if(surf == NULL) {
      printf("ERROR: Couldn't load %s, reason: %s\n", imageFilePath, SDL_GetError());
      exit(1);
    }

    // ... do something with surf ...

    SDL_FreeSurface(surf);
  }
#endif // 0 (usage example)


#ifndef SDL__STBIMAGE_H
#define SDL__STBIMAGE_H

// if you really think you need <SDL2/SDL.h> here instead.. feel free to change it,
// but the cool kids have path/to/include/SDL2/ in their compilers include path.
#include <SDL.h>

#ifndef SDL_STBIMG_ALLOW_STDIO
  #define STBI_NO_STDIO // don't need STDIO, will use SDL_RWops to open files
#endif
#include "stb_image.h"

// this allows you to prepend stuff to function signatures, e.g. "static"
#ifndef SDL_STBIMG_DEF
  // by default it's empty
  #define SDL_STBIMG_DEF
#endif // DG_MISC_DEF


#ifdef __cplusplus
extern "C" {
#endif

// loads the image file at the given path into a RGB(A) SDL_Surface
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Surface* STBIMG_Load(const char* file);

// loads the image file in the given memory buffer into a RGB(A) SDL_Surface
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Surface* STBIMG_LoadFromMemory(const unsigned char* buffer, int length);

// loads an image file into a RGB(A) SDL_Surface from a seekable SDL_RWops (src)
// if you set freesrc to non-zero, SDL_RWclose(src) will be executed after reading.
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Surface* STBIMG_Load_RW(SDL_RWops* src, int freesrc);


// Creates an SDL_Surface* using the raw RGB(A) pixelData with given width/height
// (this doesn't use stb_image and is just a simple SDL_CreateSurfaceFrom()-wrapper)
// ! It must be byte-wise 24bit RGB ("888", bytesPerPixel=3) !
// !  or byte-wise 32bit RGBA ("8888", bytesPerPixel=4) data !
// If freeWithSurface is SDL_TRUE, SDL_FreeSurface() will free the pixelData
//  you passed with SDL_free() - NOTE that you should only do that if pixelData
//  was allocated with SDL_malloc(), SDL_calloc() or SDL_realloc()!
// Returns NULL on error (in that case pixelData won't be freed!),
//  use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Surface* STBIMG_CreateSurface(unsigned char* pixelData, int width, int height,
                                                 int bytesPerPixel, SDL_bool freeWithSurface);


#if SDL_MAJOR_VERSION > 1
// loads the image file at the given path into a RGB(A) SDL_Texture
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Texture*
STBIMG_LoadTexture(SDL_Renderer* renderer, const char* file);

// loads the image file in the given memory buffer into a RGB(A) SDL_Texture
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Texture*
STBIMG_LoadTextureFromMemory(SDL_Renderer* renderer, const unsigned char* buffer, int length);

// loads an image file into a RGB(A) SDL_Texture from a seekable SDL_RWops (src)
// if you set freesrc to non-zero, SDL_RWclose(src) will be executed after reading.
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Texture*
STBIMG_LoadTexture_RW(SDL_Renderer* renderer, SDL_RWops* src, int freesrc);

// Creates an SDL_Texture* using the raw RGB(A) pixelData with given width/height
// (this doesn't use stb_image and is just a simple SDL_CreateSurfaceFrom()-wrapper)
// ! It must be byte-wise 24bit RGB ("888", bytesPerPixel=3) !
// !  or byte-wise 32bit RGBA ("8888", bytesPerPixel=4) data !
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Texture*
STBIMG_CreateTexture(SDL_Renderer* renderer, const unsigned char* pixelData,
                     int width, int height, int bytesPerPixel);
#endif // SDL_MAJOR_VERSION > 1


typedef struct {
	SDL_RWops* src;
	stbi_io_callbacks stb_cbs;
	int atEOF; // defaults to 0; 1: reached EOF or error on read, 2: error on seek
} STBIMG_stbio_RWops;

// creates stbi_io_callbacks and userdata to use stbi_*_from_callbacks() directly,
//  especially useful to use SDL_RWops with stb_image, without using SDL_Surface
// src must be readable and seekable!
// Returns SDL_FALSE on error (SDL_GetError() will give you info), else SDL_TRUE
// NOTE: If you want to use src twice (e.g. for info and load), remember to rewind
//       it by seeking back to its initial position and resetting out->atEOF to 0
//       inbetween the uses!
SDL_STBIMG_DEF SDL_bool STBIMG_stbi_callback_from_RW(SDL_RWops* src, STBIMG_stbio_RWops* out);

#if 0 //  Use STBIMG_stbi_callback_from_RW() like this:
  SDL_RWops* src = ...; // wherever it's from
  STBIMG_stbio_RWops io;
  if(!STBIMG_stbi_callback_from_RW(src, &io)) {
    printf("ERROR creating stbio callbacks: %s\n", SDL_GetError());
    exit(1);
  }
  Sint64 origSrcPosition = SDL_RWtell(src);
  int w, h, fmt;
  if(!stbi_info_from_callbacks(&io.stb_cbs, &io, &w, &h, &fmt)) {
     printf("stbi_info_from_callbacks() failed, reason: %s\n", stbi_failure_reason());
     exit(1);
  }
  printf("image is %d x %d pixels with %d bytes per pixel\n", w, h, fmt);

  // rewind src before using it again in stbi_load_from_callbacks()
  if(SDL_RWseek(src, origSrcPosition, RW_SEEK_SET) < 0)
  {
    printf("ERROR: src not be seekable!\n");
    exit(1);
  }
  io.atEOF = 0; // remember to reset atEOF, too!

  unsigned char* data;
  data = stbi_load_from_callbacks(&io.stb_cbs, &io, &w, &h, &fmt, 0);
  if(data == NULL) {
    printf("stbi_load_from_callbacks() failed, reason: %s\n", stbi_failure_reason());
    exit(1);
  }
  // ... do something with data ...
  stbi_image_free(data);
#endif // 0 (STBIMG_stbi_callback_from_RW() example)


#if SDL_MAJOR_VERSION > 1
// loads an image file into a RGB(A) SDL_Surface from a SDL_RWops (src)
// - without using SDL_RWseek(), for streams that don't support or are slow
//   at seeking. It reads everything into a buffer and calls STBIMG_LoadFromMemory()
// You should probably only use this if you *really* have performance problems
//  because of seeking or your src doesn't support  SDL_RWseek(), but SDL_RWsize()
// src must at least support SDL_RWread() and SDL_RWsize()
// if you set freesrc to non-zero, SDL_RWclose(src) will be executed after reading.
// Returns NULL on error, use SDL_GetError() to get more information.
SDL_STBIMG_DEF SDL_Surface* STBIMG_Load_RW_noSeek(SDL_RWops* src, int freesrc);

// the same for textures (you should probably not use this one, either..)
SDL_STBIMG_DEF SDL_Texture* STBIMG_LoadTexture_RW_noSeek(SDL_Renderer* renderer, SDL_RWops* src, int freesrc);
#endif // SDL_MAJOR_VERSION > 1

#ifdef __cplusplus
} // extern "C"
#endif

#endif // SDL__STBIMAGE_H


// ############# Below: Implementation ###############


#ifdef SDL_STBIMAGE_IMPLEMENTATION

// make stb_image use SDL_malloc etc, so SDL_FreeSurface() can SDL_free()
// the data allocated by stb_image
#define STBI_MALLOC SDL_malloc
#define STBI_REALLOC SDL_realloc
#define STBI_FREE SDL_free
#define STB_IMAGE_IMPLEMENTATION
#ifndef SDL_STBIMG_ALLOW_STDIO
  #define STBI_NO_STDIO // don't need STDIO, will use SDL_RWops to open files
#endif
#include "stb_image.h"

typedef struct {
	unsigned char* data;
	int w;
	int h;
	int format; // 3: RGB, 4: RGBA
} STBIMG__image;

static SDL_Surface* STBIMG__CreateSurfaceImpl(STBIMG__image img, int freeWithSurface)
{
	SDL_Surface* surf = NULL;
	Uint32 rmask, gmask, bmask, amask;
	// ok, the following is pretty stupid.. SDL_CreateRGBSurfaceFrom() pretends to use
	// a void* for the data, but it's really treated as endian-specific Uint32*
	// and there isn't even an SDL_PIXELFORMAT_* for 32bit byte-wise RGBA
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	int shift = (img.format == STBI_rgb) ? 8 : 0;
	rmask = 0xff000000 >> shift;
	gmask = 0x00ff0000 >> shift;
	bmask = 0x0000ff00 >> shift;
	amask = 0x000000ff >> shift;
#else // little endian, like x86
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = (img.format == STBI_rgb) ? 0 : 0xff000000;
#endif

	surf = SDL_CreateRGBSurfaceFrom((void*)img.data, img.w, img.h,
	                                img.format*8, img.format*img.w,
	                                rmask, gmask, bmask, amask);

	if(surf == NULL)
	{
		// hopefully SDL_CreateRGBSurfaceFrom() has set an sdl error
		return NULL;
	}

	if(freeWithSurface)
	{
		// SDL_Surface::flags is documented to be read-only.. but if the pixeldata
		// has been allocated with SDL_malloc()/SDL_calloc()/SDL_realloc() this
		// should work (and it currently does) + @icculus said it's reasonably safe:
		//  https://twitter.com/icculus/status/667036586610139137 :-)
		// clear the SDL_PREALLOC flag, so SDL_FreeSurface() free()s the data passed from img.data
		surf->flags &= ~SDL_PREALLOC;
	}

	return surf;
}


SDL_STBIMG_DEF SDL_Surface* STBIMG_LoadFromMemory(const unsigned char* buffer, int length)
{
	STBIMG__image img = {0};
	int bppToUse = 0;
	int inforet = 0;
	SDL_Surface* ret = NULL;

	if(buffer == NULL)
	{
		SDL_SetError("STBIMG_LoadFromMemory(): passed buffer was NULL!");
		return NULL;
	}
	if(length <= 0)
	{
		SDL_SetError("STBIMG_LoadFromMemory(): passed invalid length: %d!", length);
		return NULL;
	}

	inforet = stbi_info_from_memory(buffer, length, &img.w, &img.h, &img.format);
	if(!inforet)
	{
		SDL_SetError("STBIMG_LoadFromMemory(): Couldn't get image info: %s!\n", stbi_failure_reason());
		return NULL;
	}

	// no alpha => use RGB, else use RGBA
	bppToUse = (img.format == STBI_grey || img.format == STBI_rgb) ? STBI_rgb : STBI_rgb_alpha;

	img.data = stbi_load_from_memory(buffer, length, &img.w, &img.h, &img.format, bppToUse);
	if(img.data == NULL)
	{
		SDL_SetError("STBIMG_LoadFromMemory(): Couldn't load image: %s!\n", stbi_failure_reason());
		return NULL;
	}
	img.format = bppToUse;

	ret = STBIMG__CreateSurfaceImpl(img, 1);

	if(ret == NULL)
	{
		// no need to log an error here, it was an SDL error which should still be available through SDL_GetError()
		SDL_free(img.data);
		return NULL;
	}

	return ret;
}


// fill 'data' with 'size' bytes.  return number of bytes actually read
static int STBIMG__io_read(void* user, char* data, int size)
{
	STBIMG_stbio_RWops* io = (STBIMG_stbio_RWops*)user;

	int ret = SDL_RWread(io->src, data, sizeof(char), size);
	if(ret == 0)
	{
		// we're at EOF or some error happend
		io->atEOF = 1;
	}
	return (int)ret*sizeof(char);
}

// skip the next 'n' bytes, or 'unget' the last -n bytes if negative
static void STBIMG__io_skip(void* user, int n)
{
	STBIMG_stbio_RWops* io = (STBIMG_stbio_RWops*)user;

	if(SDL_RWseek(io->src, n, RW_SEEK_CUR) == -1)
	{
		// an error happened during seeking, hopefully setting EOF will make stb_image abort
		io->atEOF = 2; // set this to 2 for "aborting because seeking failed" (stb_image only cares about != 0)
	}
}

// returns nonzero if we are at end of file/data
static int STBIMG__io_eof(void* user)
{
	STBIMG_stbio_RWops* io = (STBIMG_stbio_RWops*)user;
	return io->atEOF;
}


SDL_STBIMG_DEF SDL_bool STBIMG_stbi_callback_from_RW(SDL_RWops* src, STBIMG_stbio_RWops* out)
{
	if(out == NULL)
	{
		SDL_SetError("STBIMG_stbi_callback_from_RW(): out must not be NULL!");
		return SDL_FALSE;
	}

	// make sure out is at least initialized to something deterministic
	memset(out, 0, sizeof(*out));

	if(src == NULL)
	{
		SDL_SetError("STBIMG_stbi_callback_from_RW(): src must not be NULL!");
		return SDL_FALSE;
	}

	out->src = src;
	out->atEOF = 0;
	out->stb_cbs.read = STBIMG__io_read;
	out->stb_cbs.skip = STBIMG__io_skip;
	out->stb_cbs.eof  = STBIMG__io_eof;

	return SDL_TRUE;
}


SDL_STBIMG_DEF SDL_Surface* STBIMG_Load_RW(SDL_RWops* src, int freesrc)
{
	STBIMG__image img = {0};
	int bppToUse = 0;
	int inforet = 0;
	SDL_Surface* ret = NULL;
	Sint64 srcOffset = 0;

	STBIMG_stbio_RWops cbData;

	if(src == NULL)
	{
		SDL_SetError("STBIMG_Load_RW(): src was NULL!");
		return NULL;
	}

	srcOffset = SDL_RWtell(src);
	if(srcOffset < 0)
	{
		SDL_SetError("STBIMG_Load_RW(): src must be seekable, maybe use STBIMG_Load_RW_noSeek() instead!");
		// TODO: or do that automatically? but I think the user should be aware of what they're doing
		goto end;
	}

	if(!STBIMG_stbi_callback_from_RW(src, &cbData))
	{
		goto end;
	}

	inforet = stbi_info_from_callbacks(&cbData.stb_cbs, &cbData, &img.w, &img.h, &img.format);
	if(!inforet)
	{
		if(cbData.atEOF == 2) SDL_SetError("STBIMG_Load_RW(): src must be seekable!");
		else SDL_SetError("STBIMG_Load_RW(): Couldn't get image info: %s!\n", stbi_failure_reason());
		goto end;
	}

	// rewind src so stbi_load_from_callbacks() will start reading from the beginning again
	if(SDL_RWseek(src, srcOffset, RW_SEEK_SET) < 0)
	{
		SDL_SetError("STBIMG_Load_RW(): src must be seekable!");
		goto end;
	}

	cbData.atEOF = 0; // we've rewinded (rewound?)

	// no alpha => use RGB, else use RGBA
	bppToUse = (img.format == STBI_grey || img.format == STBI_rgb) ? STBI_rgb : STBI_rgb_alpha;

	img.data = stbi_load_from_callbacks(&cbData.stb_cbs, &cbData, &img.w, &img.h, &img.format, bppToUse);
	if(img.data == NULL)
	{
		SDL_SetError("STBIMG_Load_RW(): Couldn't load image: %s!\n", stbi_failure_reason());
		goto end;
	}
	img.format = bppToUse;

	ret = STBIMG__CreateSurfaceImpl(img, 1);

	if(ret == NULL)
	{
		// no need to log an error here, it was an SDL error which should still be available through SDL_GetError()
		SDL_free(img.data);
		img.data = NULL;
		goto  end;
	}

end:
	if(freesrc)
	{
		SDL_RWclose(src);
	}
	else if(img.data == NULL)
	{
		// if data is still NULL, there was an error and we should probably
		// seek src back to where it was when this function was called
		SDL_RWseek(src, srcOffset, RW_SEEK_SET);
	}

	return ret;
}

#if SDL_MAJOR_VERSION > 1
SDL_STBIMG_DEF SDL_Surface* STBIMG_Load_RW_noSeek(SDL_RWops* src, int freesrc)
{
	unsigned char* buf = NULL;
	Sint64 fileSize = 0;
	SDL_Surface* ret = NULL;

	if(src == NULL)
	{
		SDL_SetError("STBIMG_Load_RW_noSeek(): src was NULL!");
		return NULL;
	}

	fileSize = SDL_RWsize(src);
	if(fileSize < 0)
	{
		goto end; // SDL should have set an error already
	}
	else if(fileSize == 0)
	{
		SDL_SetError("STBIMG_Load_RW_noSeek(): SDL_RWsize(src) returned 0 => empty file/stream?!");
		goto end;
	}
	else if(fileSize > 0x7FFFFFFF)
	{
		// stb_image.h uses ints for all sizes, so we can't support more
		// (but >2GB images are insane anyway)
		SDL_SetError("STBIMG_Load_RW_noSeek(): SDL_RWsize(src) too big (> 2GB)!");
		goto end;
	}

	buf = (unsigned char*)SDL_malloc(fileSize);
	if(buf == NULL)
	{
		SDL_SetError("STBIMG_Load_RW_noSeek(): Couldn't allocate buffer to read src into!");
		goto end;
	}

	if(SDL_RWread(src, buf, fileSize, 1) > 0)
	{
		// if that fails, STBIMG_LoadFromMemory() has set an SDL error
		// and ret is NULL, so nothing special to do for us
		ret = STBIMG_LoadFromMemory(buf, fileSize);
	}

end:
	if(freesrc)
	{
		SDL_RWclose(src);
	}

	SDL_free(buf);
	return ret;
}
#endif // SDL_MAJOR_VERSION > 1


SDL_STBIMG_DEF SDL_Surface* STBIMG_Load(const char* file)
{
	SDL_RWops* src = SDL_RWFromFile(file, "rb");
	if(src == NULL) return NULL;
	return STBIMG_Load_RW(src, 1);
}


SDL_STBIMG_DEF SDL_Surface* STBIMG_CreateSurface(unsigned char* pixelData, int width, int height, int bytesPerPixel, SDL_bool freeWithSurface)
{
	STBIMG__image img;

	if(pixelData == NULL)
	{
		SDL_SetError("STBIMG_CreateSurface(): passed pixelData was NULL!");
		return NULL;
	}
	if(bytesPerPixel != 3 && bytesPerPixel != 4)
	{
		SDL_SetError("STBIMG_CreateSurface(): passed bytesPerPixel = %d, only 3 (24bit RGB) and 4 (32bit RGBA) are allowed!", bytesPerPixel);
		return NULL;
	}
	if(width <= 0 || height <= 0)
	{
		SDL_SetError("STBIMG_CreateSurface(): width and height must be > 0!");
		return NULL;
	}

	img.data = pixelData;
	img.w = width;
	img.h = height;
	img.format = bytesPerPixel;

	return STBIMG__CreateSurfaceImpl(img, freeWithSurface);
}

#if SDL_MAJOR_VERSION > 1
static SDL_Texture* STBIMG__SurfToTex(SDL_Renderer* renderer, SDL_Surface* surf)
{
	SDL_Texture* ret = NULL;
	if(surf != NULL)
	{
		ret = SDL_CreateTextureFromSurface(renderer, surf);
		SDL_FreeSurface(surf); // not needed anymore, it's copied into tex
	}
	// if surf is NULL, whatever tried to create it should have called SDL_SetError(),
	// if SDL_CreateTextureFromSurface() returned NULL it should have set an error
	// so whenever this returns NULL, the user should be able to get a useful
	// error-message with SDL_GetError().
	return ret;
}

SDL_STBIMG_DEF SDL_Texture*
STBIMG_LoadTexture(SDL_Renderer* renderer, const char* file)
{
	return STBIMG__SurfToTex(renderer, STBIMG_Load(file));
}

SDL_STBIMG_DEF SDL_Texture*
STBIMG_LoadTextureFromMemory(SDL_Renderer *renderer, const unsigned char* buffer, int length)
{
	return STBIMG__SurfToTex(renderer, STBIMG_LoadFromMemory(buffer, length));
}

SDL_STBIMG_DEF SDL_Texture*
STBIMG_LoadTexture_RW(SDL_Renderer* renderer, SDL_RWops* src, int freesrc)
{
	return STBIMG__SurfToTex(renderer, STBIMG_Load_RW(src, freesrc));
}

SDL_STBIMG_DEF SDL_Texture*
STBIMG_CreateTexture(SDL_Renderer* renderer, const unsigned char* pixelData,
                     int width, int height, int bytesPerPixel)
{
	SDL_Surface* surf = STBIMG_CreateSurface((unsigned char*)pixelData, width, height, bytesPerPixel, SDL_FALSE);
	return STBIMG__SurfToTex(renderer, surf);
}

SDL_STBIMG_DEF SDL_Texture*
STBIMG_LoadTexture_RW_noSeek(SDL_Renderer* renderer, SDL_RWops* src, int freesrc)
{
	return STBIMG__SurfToTex(renderer, STBIMG_Load_RW_noSeek(src, freesrc));
}
#endif // SDL_MAJOR_VERSION > 1

#endif // SDL_STBIMAGE_IMPLEMENTATION
