/*
    SDL_mixer:  An audio mixer library based on the SDL library
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "music_mad.h"
#include "../resampler.h" /* SDLPAL */

#ifndef SDL_AUDIO_BITSIZE
# define SDL_AUDIO_BITSIZE(x)         (x & 0xFF)
#endif

mad_data *
mad_openFile(const char *filename, SDL_AudioSpec *mixer, int resampler_quality) {
  SDL_RWops *rw;

  rw = SDL_RWFromFile(filename, "rb");
  if (rw == NULL) {
	return NULL;
  }

  return mad_openFileRW(rw, mixer, resampler_quality);
}

mad_data *
mad_openFileRW(SDL_RWops *rw, SDL_AudioSpec *mixer, int resampler_quality) {
  mad_data *mp3_mad;

  mp3_mad = (mad_data *)malloc(sizeof(mad_data));
  mp3_mad->rw = rw;
  mad_stream_init(&mp3_mad->stream);
  mad_frame_init(&mp3_mad->frame);
  mad_synth_init(&mp3_mad->synth);
  mp3_mad->frames_read = 0;
  mad_timer_reset(&mp3_mad->next_frame_start);
  mp3_mad->status = 0;
  mp3_mad->output_begin = 0;
  mp3_mad->output_end = 0;
  mp3_mad->mixer = *mixer;
  mp3_mad->upsample = 0; /* SDLPAL */
  mp3_mad->resampler_quality = resampler_quality; /* SDLPAL */
  memset(mp3_mad->resampler, 0, sizeof(mp3_mad->resampler)); /* SDLPAL */

  return mp3_mad;
}

void
mad_closeFile(mad_data *mp3_mad) {
  SDL_FreeRW(mp3_mad->rw);
  mad_stream_finish(&mp3_mad->stream);
  mad_frame_finish(&mp3_mad->frame);
  mad_synth_finish(&mp3_mad->synth);

  free(mp3_mad);
}

/* Starts the playback. */
void
mad_start(mad_data *mp3_mad) {
  mp3_mad->status |= MS_playing;
}

/* Stops the playback. */
void 
mad_stop(mad_data *mp3_mad) {
  mp3_mad->status &= ~MS_playing;
}

/* Returns true if the playing is engaged, false otherwise. */
int
mad_isPlaying(mad_data *mp3_mad) {
  return ((mp3_mad->status & MS_playing) != 0);
}

static void
convert_mono(signed short *srcdst, int samples)
{
	signed short *left = srcdst, *right = srcdst + 1;
	while (samples > 0)
	{
		*srcdst++ = (signed short)(((signed int)(*left) + (signed int)(*right)) >> 1);
		samples--; left += 2; right += 2;
	}
}

static void
convert_stereo(signed short *srcdst, int samples)
{
	signed short *left = srcdst + (samples - 1) * 2, *right = srcdst + samples * 2 - 1;
	srcdst += samples - 1;
	while (samples > 0)
	{
		*left = *right = *srcdst--;
		samples--; left -= 2; right -= 2;
	}
}

/* Reads the next frame from the file.  Returns true on success or
   false on failure. */
static int
read_next_frame(mad_data *mp3_mad) {
  if (mp3_mad->stream.buffer == NULL || 
	  mp3_mad->stream.error == MAD_ERROR_BUFLEN) {
	size_t read_size;
	size_t remaining;
	unsigned char *read_start;
	
	/* There might be some bytes in the buffer left over from last
	   time.  If so, move them down and read more bytes following
	   them. */
	if (mp3_mad->stream.next_frame != NULL) {
	  remaining = mp3_mad->stream.bufend - mp3_mad->stream.next_frame;
	  memmove(mp3_mad->input_buffer, mp3_mad->stream.next_frame, remaining);
	  read_start = mp3_mad->input_buffer + remaining;
	  read_size = MAD_INPUT_BUFFER_SIZE - remaining;
	  
	} else {
	  read_size = MAD_INPUT_BUFFER_SIZE;
	  read_start = mp3_mad->input_buffer;
	  remaining = 0;
	}

	/* Now read additional bytes from the input file. */
	read_size = SDL_RWread(mp3_mad->rw, read_start, 1, read_size);
	
	if (read_size <= 0) {
	  if ((mp3_mad->status & (MS_input_eof | MS_input_error)) == 0) {
		if (read_size == 0) {
		  mp3_mad->status |= MS_input_eof;
		} else {
		  mp3_mad->status |= MS_input_error;
		}
		
		/* At the end of the file, we must stuff MAD_BUFFER_GUARD
		   number of 0 bytes. */
		memset(read_start + read_size, 0, MAD_BUFFER_GUARD);
		read_size += MAD_BUFFER_GUARD;
	  }
	}
	
	/* Now feed those bytes into the libmad stream. */
	mad_stream_buffer(&mp3_mad->stream, mp3_mad->input_buffer,
					  read_size + remaining);
	mp3_mad->stream.error = MAD_ERROR_NONE;
  }
  
  /* Now ask libmad to extract a frame from the data we just put in
	 its buffer. */
  if (mad_frame_decode(&mp3_mad->frame, &mp3_mad->stream)) {
	if (MAD_RECOVERABLE(mp3_mad->stream.error)) {
	  return 0;
	  
	} else if (mp3_mad->stream.error == MAD_ERROR_BUFLEN) {
	  return 0;
	  
	} else {
	  mp3_mad->status |= MS_decode_error;
	  return 0;
	}
  }
  
  mp3_mad->frames_read++;
  mad_timer_add(&mp3_mad->next_frame_start, mp3_mad->frame.header.duration);

  return 1;
}

/* Scale a MAD sample to 16 bits for output. */
static signed int
scale(mad_fixed_t sample) {
  /* round */
  sample += (1L << (MAD_F_FRACBITS - 16));

  /* clip */
  if (sample >= MAD_F_ONE)
    sample = MAD_F_ONE - 1;
  else if (sample < -MAD_F_ONE)
    sample = -MAD_F_ONE;

  /* quantize */
  return sample >> (MAD_F_FRACBITS + 1 - 16);
}

/* Once the frame has been read, copies its samples into the output
   buffer. */
static void
decode_frame(mad_data *mp3_mad) {
  struct mad_pcm *pcm;
  unsigned int nchannels, nsamples;
  mad_fixed_t const *left_ch, *right_ch;
  unsigned char *out;

  mad_synth_frame(&mp3_mad->synth, &mp3_mad->frame);
  pcm = &mp3_mad->synth.pcm;
  out = mp3_mad->output_buffer + mp3_mad->output_end;

  if ((mp3_mad->status & MS_cvt_decoded) == 0) {
    int hi, lo; /* SDLPAL */
	mp3_mad->status |= MS_cvt_decoded;

	/* The first frame determines some key properties of the stream.
	   In particular, it tells us enough to set up the convert
	   structure now. */
    /* ------------------------------- SDLPAL start ------------------------------- */
    hi = (int)mp3_mad->frame.header.samplerate > mp3_mad->mixer.freq ? mp3_mad->frame.header.samplerate : mp3_mad->mixer.freq;
    lo = (int)mp3_mad->frame.header.samplerate < mp3_mad->mixer.freq ? mp3_mad->frame.header.samplerate : mp3_mad->mixer.freq;
	if (hi != lo) {
      /* Need sample rate conversion, resampler should be used. Try to create resamplers. */
      if ((mp3_mad->resampler[0] = resampler_create()) != NULL) {
        if (mp3_mad->mixer.channels == 2) {
          if ((mp3_mad->resampler[1] = resampler_create())) {
            resampler_set_quality(mp3_mad->resampler[1], (hi % lo == 0) ? RESAMPLER_QUALITY_MIN : mp3_mad->resampler_quality);
            resampler_set_rate(mp3_mad->resampler[1], (double)mp3_mad->frame.header.samplerate / (double)mp3_mad->mixer.freq);
          }
		  else {
            resampler_delete(mp3_mad->resampler[0]);
            mp3_mad->resampler[0] = NULL;
          }
        }
      }
      if (mp3_mad->resampler[0]) {
        resampler_set_quality(mp3_mad->resampler[0], mp3_mad->resampler_quality);
        resampler_set_rate(mp3_mad->resampler[0], (double)mp3_mad->frame.header.samplerate / (double)mp3_mad->mixer.freq);
        /* Resampler successfully created, cheat SDL for not converting sample rate */
        mp3_mad->upsample = mp3_mad->mixer.freq > (int)mp3_mad->frame.header.samplerate;
        mp3_mad->mixer.freq = mp3_mad->frame.header.samplerate;
      }
    }
	if (pcm->channels == 1 && mp3_mad->mixer.channels == 2)
		mp3_mad->converter = convert_stereo;
	else if (pcm->channels == 2 && mp3_mad->mixer.channels == 1)
		mp3_mad->converter = convert_mono;
	else
		mp3_mad->converter = NULL;
	/* ------------------------------- SDLPAL end ------------------------------- */
  }

  /* pcm->samplerate contains the sampling frequency */

  nchannels = pcm->channels;
  nsamples  = pcm->length;
  left_ch   = pcm->samples[0];
  right_ch  = pcm->samples[1];

  while (nsamples--) {
    /* output sample(s) in 16-bit signed native-endian PCM */

	*((signed short*)out) = (signed short)scale(*left_ch++); out += 2;

    if (nchannels == 2) {
      *((signed short*)out) = scale(*right_ch++); out += 2;
    }
  }

  mp3_mad->output_end = out - mp3_mad->output_buffer;
  /*assert(mp3_mad->output_end <= MAD_OUTPUT_BUFFER_SIZE);*/
}

void
mad_getSamples(mad_data *mp3_mad, Uint8 *stream, int len) {
  int bytes_remaining;
  int num_bytes;
  Uint8 *out;

  if ((mp3_mad->status & MS_playing) == 0) {
	/* We're not supposed to be playing, so send silence instead. */
	memset(stream, 0, len);
	return;
  }

  out = stream;
  bytes_remaining = len;
  while (bytes_remaining > 0) {
	if (mp3_mad->output_end == mp3_mad->output_begin) {
	  /* We need to get a new frame. */
	  mp3_mad->output_begin = 0;
	  mp3_mad->output_end = 0;
	  if (!read_next_frame(mp3_mad)) {
		if ((mp3_mad->status & MS_error_flags) != 0) {
		  /* Couldn't read a frame; either an error condition or
			 end-of-file.  Stop. */
		  memset(out, 0, bytes_remaining);
		  mp3_mad->status &= ~MS_playing;
		  return;
		}
	  } else {
		decode_frame(mp3_mad);

		/* Now convert the frame data to the appropriate format for
		   output. */
        /* ------------------------------- SDLPAL start ------------------------------- */
		if (mp3_mad->converter)
		{
			int nchannels = (mp3_mad->converter == convert_stereo) ? 1 : 2;
			mp3_mad->converter((signed short *)mp3_mad->output_buffer, mp3_mad->output_end / (nchannels * 2));
			if (nchannels == 1)
				mp3_mad->output_end <<= 1;
			else
				mp3_mad->output_end >>= 1;
		}

        if (mp3_mad->resampler[0]) {
		  int dst_samples = 0, pos = 0, i;
		  if (mp3_mad->upsample) {
            /* Upsample, should move memory blocks to avoid overwrite. MP3's lowest samplerate
               is 32KHz, while SDLPAL support up to 48KHz, so maximum upsample rate is 1.5.
               As one frame has 1152 samples, the maximum bytes used will be 6912 bytes. So
               it is safe by first moving memory blocks to the end of the buffer */
            pos = sizeof(mp3_mad->output_buffer) - mp3_mad->output_end;
            memmove(mp3_mad->output_buffer + pos, mp3_mad->output_buffer, mp3_mad->output_end);
          }
          
          for (i = 0; i < mp3_mad->mixer.channels; i++) {
            short *src = (short *)(mp3_mad->output_buffer + pos) + i;
            short *dst = (short *)mp3_mad->output_buffer + i;
            int src_samples = 1152; /* Defined by MP3 specification */
            while(src_samples > 0) {
              int to_write = resampler_get_free_count(mp3_mad->resampler[i]), j;
              for (j = 0; j < to_write; j++) {
                resampler_write_sample(mp3_mad->resampler[i], *src);
                src += mp3_mad->mixer.channels;
              }
			  src_samples -= to_write;
              while (resampler_get_sample_count(mp3_mad->resampler[i]) > 0) {
                *dst = resampler_get_and_remove_sample(mp3_mad->resampler[i]);
				dst += mp3_mad->mixer.channels; dst_samples++;
              }
            }
          }
          mp3_mad->output_end = dst_samples * (SDL_AUDIO_BITSIZE(mp3_mad->mixer.format) >> 3);
        }
        /* ------------------------------- SDLPAL end ------------------------------- */
	  }
	}

	num_bytes = mp3_mad->output_end - mp3_mad->output_begin;
	if (bytes_remaining < num_bytes) {
	  num_bytes = bytes_remaining;
	}

	memcpy(out, mp3_mad->output_buffer + mp3_mad->output_begin, num_bytes);
	out += num_bytes;
	mp3_mad->output_begin += num_bytes;
	bytes_remaining -= num_bytes;
  }
}

void
mad_seek(mad_data *mp3_mad, double position) {
  mad_timer_t target;
  int int_part;

  int_part = (int)position;
  mad_timer_set(&target, int_part, 
				(int)((position - int_part) * 1000000), 1000000);

  if (mad_timer_compare(mp3_mad->next_frame_start, target) > 0) {
	/* In order to seek backwards in a VBR file, we have to rewind and
	   start again from the beginning.  This isn't necessary if the
	   file happens to be CBR, of course; in that case we could seek
	   directly to the frame we want.  But I leave that little
	   optimization for the future developer who discovers she really
	   needs it. */
	mp3_mad->frames_read = 0;
	mad_timer_reset(&mp3_mad->next_frame_start);
	mp3_mad->status &= ~MS_error_flags;
	mp3_mad->output_begin = 0;
	mp3_mad->output_end = 0;

	SDL_RWseek(mp3_mad->rw, 0, SEEK_SET);
  }

  /* Now we have to skip frames until we come to the right one.
	 Again, only truly necessary if the file is VBR. */
  while (mad_timer_compare(mp3_mad->next_frame_start, target) < 0) {
	if (!read_next_frame(mp3_mad)) {
	  if ((mp3_mad->status & MS_error_flags) != 0) {
		/* Couldn't read a frame; either an error condition or
		   end-of-file.  Stop. */
		mp3_mad->status &= ~MS_playing;
		return;
	  }
	}
  }

  /* Here we are, at the beginning of the frame that contains the
	 target time.  Ehh, I say that's close enough.  If we wanted to,
	 we could get more precise by decoding the frame now and counting
	 the appropriate number of samples out of it. */
}
