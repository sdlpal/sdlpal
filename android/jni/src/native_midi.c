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

#include "android_jni.h"

#include "SDL_config.h"

#include "SDL.h"
#include "SDL_endian.h"
#include "native_midi/native_midi.h"

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
    NativeMidiSong *retval = (NativeMidiSong *)malloc(sizeof(NativeMidiSong));
    if (retval)
    {
        memset(retval, 0, sizeof(NativeMidiSong));
        JNI_mediaplayer_load(midifile);
    }
    return retval;
}

NativeMidiSong *native_midi_loadsong_RW(SDL_RWops *rw)
{
    NativeMidiSong *retval = (NativeMidiSong *)malloc(sizeof(NativeMidiSong));
    if (retval)
    {
        FILE *fp = fopen(midiInterFile, "wb+");
        if (fp)
        {
            char buf[4096];
            size_t bytes;
            while(bytes = SDL_RWread(rw, buf, sizeof(char), sizeof(buf)))
                fwrite(buf, sizeof(char), bytes, fp);
            fclose(fp);

            memset(retval, 0, sizeof(NativeMidiSong));
            JNI_mediaplayer_load(midiInterFile);
        }
    }
    return retval;
}

void native_midi_freesong(NativeMidiSong *song)
{
    if (song != NULL)
    {
        if (currentsong == song)
            currentsong = NULL;
        free(song);
    }
}

void native_midi_start(NativeMidiSong *song)
{
    native_midi_stop();
    if (song != NULL)
    {
        currentsong = song;
        currentsong->playing = 1;
        JNI_mediaplayer_play();
    }
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
    return currentsong ? currentsong->playing : 0;
}

void native_midi_setvolume(int volume)
{
    if (latched_volume != volume)
    {
        JNI_mediaplayer_setvolume(latched_volume = volume);
    }
}

const char *native_midi_error(void)
{
    return "";  /* !!! FIXME */
}
