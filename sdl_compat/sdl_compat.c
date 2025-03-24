#include "sdl_compat.h"

#include "video.h"
#include "audio.h"

#if SDL_VERSION_ATLEAST(3,0,0)

extern SDL_Window* gpWindow;
extern SDL_Surface* gpScreenReal;
extern SDL_Renderer* gpRenderer;
extern SDL_Texture* gpTexture;
extern SDL_Texture* gpTouchOverlay;
extern SDL_Rect           gOverlayRect;
extern SDL_Rect           gTextureRect;

SDL_Surface* SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
    return SDL_CreateSurface(width, height,
        SDL_GetPixelFormatForMasks(depth, Rmask, Gmask, Bmask, Amask));
}

SDL_Surface* SDL_CreateRGBSurfaceWithFormat(Uint32 flags, int width, int height, int depth, Uint32 format)
{
    return SDL_CreateSurface(width, height, format);
}

SDL_Surface* SDL_CreateRGBSurfaceFrom(void* pixels, int width, int height, int depth, int pitch, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
    return SDL_CreateSurfaceFrom(width, height,
        SDL_GetPixelFormatForMasks(depth, Rmask, Gmask, Bmask, Amask),
        pixels, pitch);
}

SDL_Surface* SDL_CreateRGBSurfaceWithFormatFrom(void* pixels, int width, int height, int depth, int pitch, Uint32 format)
{
    return SDL_CreateSurfaceFrom(width, height, format, pixels, pitch);
}

SDL_DECLSPEC int SDLCALL
SDL_SoftStretch(SDL_Surface* src, const SDL_Rect* srcrect, SDL_Surface* dst, const SDL_Rect* dstrect)
{
    return SDL_BlitSurface(src, srcrect, dst, dstrect) ? 0 : -1;
}

SDL_DECLSPEC int SDLCALL
SDL_BlitScaled(SDL_Surface* src, const SDL_Rect* srcrect, SDL_Surface* dst, const SDL_Rect* dstrect)
{
    return SDL_BlitSurfaceScaled(src, srcrect, dst, dstrect, VIDEO_GetScaleMode()) ? 0 : -1;
}

size_t SDL_RWread(SDL_IOStream* stream, void* ptr, size_t size, size_t nitems)
{
    if (size > 0 && nitems > 0) {
        return SDL_ReadIO(stream, ptr, size * nitems) / size;
    }
    return 0;
}

void SDL_PauseAudio(bool pause_on)
{
    if(pause_on)
        SDL_PauseAudioStreamDevice(gAudioDevice.stream);
    else
        SDL_ResumeAudioStreamDevice(gAudioDevice.stream);
}

#if PAL_HAS_GLSL
#include "video_glsl.h"
#include "glslp.h"

void SetupScaleMode(GLenum target)
{
    // hack for GLSL; since SDL3 defaults batching, and then 3.2.10 removes default 
#if SDL_VERSION_ATLEAST(3,2,10)
    glTexParameteri((GLenum)target, GL_TEXTURE_MIN_FILTER, VIDEO_GLSL_GetScaleMode());
    glTexParameteri((GLenum)target, GL_TEXTURE_MAG_FILTER, VIDEO_GLSL_GetScaleMode());
#endif
}

int SDL_GL_BindTexture(SDL_Texture* texture, float* texw, float* texh)
{
    SDL_PropertiesID props;
    SDL_Renderer* renderer;
    Sint64 tex;

    renderer = gpRenderer;
    if (!renderer) {
        return -1;
    }

    props = SDL_GetTextureProperties(texture);
    if (!props) {
        return -1;
    }

    /* always flush the renderer here; good enough. SDL only flushed if the texture might have changed, but we'll be conservative. */
    SDL_FlushRenderer(renderer);

#if !GLES
    if ((tex = SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_OPENGL_TEXTURE_NUMBER, -1)) != -1) {  // opengl renderer.
        const Sint64 target = SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_OPENGL_TEXTURE_TARGET_NUMBER, 0);
        const Sint64 uv = SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_OPENGL_TEXTURE_UV_NUMBER, 0);
        const Sint64 u = SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_OPENGL_TEXTURE_U_NUMBER, 0);
        const Sint64 v = SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_OPENGL_TEXTURE_V_NUMBER, 0);
        
        glEnable((GLenum)target);
        
        if (u && v) {
            glActiveTextureARB(GL_TEXTURE2_ARB);
            glBindTexture((GLenum)target, (GLuint)v);
            glActiveTextureARB(GL_TEXTURE1_ARB);
            glBindTexture((GLenum)target, (GLuint)u);
            glActiveTextureARB(GL_TEXTURE0_ARB);
        }
        else if (uv) {
            glActiveTextureARB(GL_TEXTURE1_ARB);
            glBindTexture((GLenum)target, (GLuint)uv);
            glActiveTextureARB(GL_TEXTURE0_ARB);
        }
        glBindTexture((GLenum)target, (GLuint)tex);

        SetupScaleMode(target);

        if (texw) {
            *texw = SDL_GetFloatProperty(props, SDL_PROP_TEXTURE_OPENGL_TEX_W_FLOAT, 1.0f);
        }
        if (texh) {
            *texh = SDL_GetFloatProperty(props, SDL_PROP_TEXTURE_OPENGL_TEX_H_FLOAT, 1.0f);
        }
    }
#else
    if ((tex = SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_OPENGLES2_TEXTURE_NUMBER, -1)) != -1) {  // opengles2 renderer.
        const Sint64 target = SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_OPENGLES2_TEXTURE_TARGET_NUMBER, 0);
        const Sint64 uv = SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_OPENGLES2_TEXTURE_UV_NUMBER, 0);
        const Sint64 u = SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_OPENGLES2_TEXTURE_U_NUMBER, 0);
        const Sint64 v = SDL_GetNumberProperty(props, SDL_PROP_TEXTURE_OPENGLES2_TEXTURE_V_NUMBER, 0);

        if (u && v) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture((GLenum) target, (GLuint) v);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture((GLenum) target, (GLuint) u);
            glActiveTexture(GL_TEXTURE0);
        } else if (uv) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture((GLenum) target, (GLuint) uv);
            glActiveTexture(GL_TEXTURE0);
        }
        glBindTexture((GLenum) target, (GLuint) tex);

        SetupScaleMode(target);

        if (texw) {
            *texw = 1.0f;
        }
        if (texh) {
            *texh = 1.0f;
        }
    }
#endif
    return 0;
}
#endif

typedef struct {
    /* src_format is read directly from the AudioCVT in real SDL */
    Uint8 src_channels;
    int src_rate;
    SDL_AudioFormat dst_format;
    Uint8 dst_channels;
    int dst_rate;
} AudioParam;

#define RESAMPLER_BITS_PER_SAMPLE           16
#define RESAMPLER_SAMPLES_PER_ZERO_CROSSING (1 << ((RESAMPLER_BITS_PER_SAMPLE / 2) + 1))

static void SDLCALL AudioCVTFilter(SDL_AudioCVT* cvt, SDL_AudioFormat src_format)
{
    SDL_AudioStream* stream2;
    int src_len, dst_len, real_dst_len;
    int src_samplesize;
    SDL_AudioSpec src_spec, dst_spec;

    { /* Fetch from the end of filters[], aligned */
        AudioParam ap;

        SDL_memcpy(
            &ap,
            (Uint8*)&cvt->filters[SDL_AUDIOCVT_MAX_FILTERS + 1] - (sizeof(AudioParam) & ~3),
            sizeof(ap));

        src_spec.format = src_format;
        src_spec.channels = ap.src_channels;
        src_spec.freq = ap.src_rate;
        dst_spec.format = ap.dst_format;
        dst_spec.channels = ap.dst_channels;
        dst_spec.freq = ap.dst_rate;
    }

    /* don't use the SDL stream directly or even SDL_ConvertAudioSamples; we want the U16 support in the SDL-compat layer */
    stream2 = SDL_CreateAudioStream(&src_spec, &dst_spec);
    if (stream2 == NULL) {
        goto exit;
    }

    src_samplesize = (SDL_AUDIO_BITSIZE(src_format) / 8) * src_spec.channels;

    src_len = cvt->len_cvt & ~(src_samplesize - 1);
    dst_len = cvt->len * cvt->len_mult;

    /* Run the audio converter */
    if (SDL_PutAudioStreamData(stream2, cvt->buf, src_len) != SDL_OK ||
        SDL_FlushAudioStream(stream2) != SDL_OK) {
        goto exit;
    }

    /* Get back in the same buffer */
    real_dst_len = SDL_GetAudioStreamData(stream2, cvt->buf, dst_len);
    if (real_dst_len < 0) {
        goto exit;
    }

    cvt->len_cvt = real_dst_len;

exit:
    SDL_DestroyAudioStream(stream2);

    /* Call the next filter in the chain */
    if (cvt->filters[++cvt->filter_index]) {
        cvt->filters[cvt->filter_index](cvt, dst_spec.format);
    }
}
static bool
SDL_IsSupportedAudioFormat(const SDL_AudioFormat fmt)
{
    switch (fmt) {
    case SDL_AUDIO_U8:
    case SDL_AUDIO_S8:
    case SDL_AUDIO_S16LE:
    case SDL_AUDIO_S16BE:
    case SDL_AUDIO_S32LE:
    case SDL_AUDIO_S32BE:
    case SDL_AUDIO_F32LE:
    case SDL_AUDIO_F32BE:
        return true; /* supported. */

    default:
        break;
    }

    return false; /* unsupported. */
}
static bool SDL_IsSupportedChannelCount(const int channels)
{
    return ((channels >= 1) && (channels <= 8));
}

SDL_DECLSPEC int SDLCALL
SDL_BuildAudioCVT(SDL_AudioCVT* cvt,
    SDL_AudioFormat src_format,
    Uint8 src_channels,
    int src_rate,
    SDL_AudioFormat dst_format,
    Uint8 dst_channels,
    int dst_rate)
{
    /* Sanity check target pointer */
    if (cvt == NULL) {
        SDL_InvalidParamError("cvt");
        return -1;
    }

    /* Make sure we zero out the audio conversion before error checking */
    SDL_zerop(cvt);

    if (!SDL_IsSupportedAudioFormat(src_format)) {
        SDL_SetError("Invalid source format");
        return -1;
    }
    if (!SDL_IsSupportedAudioFormat(dst_format)) {
        SDL_SetError("Invalid destination format");
        return -1;
    }
    if (!SDL_IsSupportedChannelCount(src_channels)) {
        SDL_SetError("Invalid source channels");
        return -1;
    }
    if (!SDL_IsSupportedChannelCount(dst_channels)) {
        SDL_SetError("Invalid destination channels");
        return -1;
    }
    if (src_rate <= 0) {
        SDL_SetError("Source rate is equal to or less than zero");
        return -1;
    }
    if (dst_rate <= 0) {
        SDL_SetError("Destination rate is equal to or less than zero");
        return -1;
    }
    if (src_rate >= SDL_MAX_SINT32 / RESAMPLER_SAMPLES_PER_ZERO_CROSSING) {
        SDL_SetError("Source rate is too high");
        return -1;
    }
    if (dst_rate >= SDL_MAX_SINT32 / RESAMPLER_SAMPLES_PER_ZERO_CROSSING) {
        SDL_SetError("Destination rate is too high");
        return -1;
    }

#ifdef DEBUG_CONVERT
    SDL_Log("SDL_AUDIO_CONVERT: Build format %04x->%04x, channels %u->%u, rate %d->%d\n",
        src_format, dst_format, src_channels, dst_channels, src_rate, dst_rate);
#endif

    /* Start off with no conversion necessary */
    cvt->src_format = src_format;
    cvt->dst_format = dst_format;
    cvt->needed = 0;
    cvt->filter_index = 0;
    SDL_zeroa(cvt->filters);
    cvt->len_mult = 1;
    cvt->len_ratio = 1.0;
    cvt->rate_incr = ((double)dst_rate) / ((double)src_rate);

    { /* Use the filters[] to store some data ... */
        AudioParam ap;
        ap.src_channels = src_channels;
        ap.src_rate = src_rate;
        ap.dst_format = dst_format;
        ap.dst_channels = dst_channels;
        ap.dst_rate = dst_rate;

        /* Store at the end of filters[], aligned */
        SDL_memcpy(
            (Uint8*)&cvt->filters[SDL_AUDIOCVT_MAX_FILTERS + 1] - (sizeof(AudioParam) & ~3),
            &ap,
            sizeof(ap));

        cvt->needed = 1;
        if (src_format == dst_format && src_rate == dst_rate && src_channels == dst_channels) {
            cvt->needed = 0;
        }

        if (src_format != dst_format) {
            const Uint16 src_bitsize = SDL_AUDIO_BITSIZE(src_format);
            const Uint16 dst_bitsize = SDL_AUDIO_BITSIZE(dst_format);

            if (src_bitsize < dst_bitsize) {
                const int mult = (dst_bitsize / src_bitsize);
                cvt->len_mult *= mult;
                cvt->len_ratio *= mult;
            }
            else if (src_bitsize > dst_bitsize) {
                const int div = (src_bitsize / dst_bitsize);
                cvt->len_ratio /= div;
            }
        }

        if (src_channels < dst_channels) {
            cvt->len_mult = ((cvt->len_mult * dst_channels) + (src_channels - 1)) / src_channels;
        }

        if (src_rate < dst_rate) {
            const double mult = ((double)dst_rate / (double)src_rate);
            cvt->len_mult *= (int)SDL_ceil(mult);
            cvt->len_ratio *= mult;
        }
        else {
            const double divisor = ((double)src_rate / (double)dst_rate);
            cvt->len_ratio /= divisor;
        }
    }

    if (cvt->needed) {
        /* Insert a single filter to perform all necessary audio conversion.
         * Some apps may examine or modify the filter chain, so we use a real
         * SDL-style audio filter function to keep those apps happy. */
        cvt->filters[0] = AudioCVTFilter;
        cvt->filters[1] = NULL;
        cvt->filter_index = 1;
    }

    return cvt->needed;
}

SDL_DECLSPEC int SDLCALL
SDL_ConvertAudio(SDL_AudioCVT* cvt)
{
    /* Make sure there's data to convert */
    if (!cvt->buf) {
        return SDL_SetError("No buffer allocated for conversion");
    }

    /* Return okay if no conversion is necessary */
    cvt->len_cvt = cvt->len;
    if (cvt->filters[0] == NULL) {
        return 0;
    }

    /* Set up the conversion and go! */
    cvt->filter_index = 0;
    cvt->filters[0](cvt, cvt->src_format);
    return 0;
}
#endif
