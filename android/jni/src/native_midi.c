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
    void *player;
    int   volume;
};

int native_midi_detect()
{
    return 1;  /* always available. */
}

NativeMidiSong *native_midi_loadsong(const char *midifile)
{
    NativeMidiSong *song = (NativeMidiSong *)malloc(sizeof(NativeMidiSong));
    if (song)
    {
        song->volume = 127;
        song->player = JNI_mediaplayer_load(midifile);
    }
    return song;
}

NativeMidiSong *native_midi_loadsong_RW(SDL_RWops *rw)
{
    FILE *fp = fopen(midiInterFile, "wb+");
    if (fp)
    {
        char buf[4096];
        size_t bytes;
        while(bytes = SDL_RWread(rw, buf, sizeof(char), sizeof(buf)))
            fwrite(buf, sizeof(char), bytes, fp);
        fclose(fp);

        return native_midi_loadsong(midiInterFile);
    }
    return NULL;
}

void native_midi_freesong(NativeMidiSong *song)
{
    if (song != NULL)
    {
        JNI_mediaplayer_free(song->player);
        free(song);
    }
}

void native_midi_start(NativeMidiSong *song, int looping)
{
    native_midi_stop(song);
    if (song != NULL)
    {
        JNI_mediaplayer_play(song->player, looping);
    }
}

void native_midi_stop(NativeMidiSong *song)
{
    if (song)
    {
        JNI_mediaplayer_stop(song->player);
    }
}

int native_midi_active(NativeMidiSong *song)
{
    return song ? JNI_mediaplayer_isplaying(song->player) : 0;
}

void native_midi_setvolume(NativeMidiSong *song, int volume)
{
    if (song)
    {
        JNI_mediaplayer_setvolume(song->player, song->volume = volume);
    }
}

const char *native_midi_error(NativeMidiSong *song)
{
    return "";  /* !!! FIXME */
}
