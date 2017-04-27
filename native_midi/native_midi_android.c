/*
    native_midi_android:  Native Midi support on Android for SDLPal
    Copyright (C) 2017  Pal Lockheart

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

    Pal Lockheart
*/

/* This is Android only, using MediaPlayer ( need java part work together ) */

#ifdef __ANDROID__
#include "android_jni.h"

#include "SDL_config.h"

#include "SDL.h"
#include "SDL_endian.h"
#include "native_midi.h"

/* Native Midi song */
struct _NativeMidiSong
{
    int _placeholder;
    int playing;
};

static NativeMidiSong *currentsong = NULL;
static int latched_volume = 128;

int native_midi_detect()
{
    return 1;  /* always available. */
}

NativeMidiSong *native_midi_loadsong(const char *midifile)
{
    NativeMidiSong *retval = NULL;
    SDL_RWops *rw = SDL_RWFromFile(midifile, "rb");
    if (rw != NULL) {
        retval = native_midi_loadsong_RW(rw);
        SDL_RWclose(rw);
    }

    return retval;
}

NativeMidiSong *native_midi_loadsong_RW(SDL_RWops *rw)
{
    NativeMidiSong *retval = NULL;
    void *buf = NULL;
    int len = 0;

    if (SDL_RWseek(rw, 0, RW_SEEK_END) < 0)
        goto fail;
    len = SDL_RWtell(rw);
    if (len < 0)
        goto fail;
    if (SDL_RWseek(rw, 0, RW_SEEK_SET) < 0)
        goto fail;

    buf = malloc(len);
    if (buf == NULL)
        goto fail;

    if (SDL_RWread(rw, buf, len, 1) != 1)
        goto fail;

    retval = malloc(sizeof(NativeMidiSong));
    if (retval == NULL)
        goto fail;

    memset(retval, '\0', sizeof (*retval));

    FILE *fp = fopen(midiInterFile,"wb+");
    fwrite(buf,len,1,fp);
    fclose(fp);
    JNI_mediaplayer_load();
    return retval;

fail:
    return NULL;
}

void native_midi_freesong(NativeMidiSong *song)
{
    if (song != NULL) {
        if (currentsong == song)
            currentsong = NULL;
        free(song);
    }
}

void native_midi_start(NativeMidiSong *song)
{
    int vol;

    if (song == NULL)
        return;
    
    currentsong = song;
    currentsong->playing = 1;
    JNI_mediaplayer_play();
}

void native_midi_stop()
{
    if (currentsong) {
        currentsong->playing = 0;
        JNI_mediaplayer_stop();
    }
}

int native_midi_active()
{
    if (currentsong == NULL)
        return 0;

    return currentsong->playing;
}

void native_midi_setvolume(int volume)
{
    if (latched_volume == volume)
        return;

    latched_volume = volume;
    JNI_mediaplayer_setvolume(volume);
}

const char *native_midi_error(void)
{
    return "";  /* !!! FIXME */
}

#endif
