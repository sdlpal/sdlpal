/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2018, SDLPAL development team.
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
// native_midi_android:  Native Midi support on iOS for SDLPal
// Copyright (C) 2017  SDLPal team
//

#include "SDL_config.h"

#include "SDL.h"
#include "SDL_endian.h"
#include "native_midi/native_midi.h"

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

#include "palcommon.h"
#include "util.h"

/* Native Midi song */
struct _NativeMidiSong
{
    int _placeholder;
    int playing;
};

static NativeMidiSong *currentsong = NULL;
static int latched_volume = 128;

static AVMIDIPlayer *midiPlayer;

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
    NativeMidiSong *retval = (NativeMidiSong *)malloc(sizeof(NativeMidiSong));
    char midiInterFile[PATH_MAX];
    snprintf(midiInterFile, PATH_MAX, "%s%s", [NSTemporaryDirectory() UTF8String], "inter.mid");
    FILE *fp = fopen(midiInterFile, "wb+");
    if (fp)
    {
        char buf[4096];
        size_t bytes;
        while((bytes = SDL_RWread(rw, buf, sizeof(char), sizeof(buf)))!=0)
            fwrite(buf, sizeof(char), bytes, fp);
        fclose(fp);
        
        memset(retval, 0, sizeof(NativeMidiSong));
        NSURL *midiFileURL = [NSURL URLWithString:[NSString stringWithUTF8String:midiInterFile]];
        NSURL *bankURL = [[NSBundle mainBundle] URLForResource:@"gs_instruments" withExtension: @"dls"];
        if( midiPlayer ) {
            midiPlayer = nil;
        }
        NSError *err=nil;
        midiPlayer = [[AVMIDIPlayer new] initWithContentsOfURL:midiFileURL soundBankURL:bankURL error:&err];
        [midiPlayer prepareToPlay];
    }
    return retval;
}

void native_midi_freesong(NativeMidiSong *song)
{
    if (song != NULL)
    {
        native_midi_stop(song);
        if (currentsong == song)
            currentsong = NULL;
        free(song);
        if( midiPlayer ) {
            midiPlayer = nil;
        }
    }
}

void native_midi_start(NativeMidiSong *song, int looping)
{
    native_midi_stop(song);
    if (song != NULL)
    {
        currentsong = song;
        currentsong->playing = 1;
        [midiPlayer play:^(){
            if( currentsong ) {
                midiPlayer.currentPosition = 0;
                native_midi_start(currentsong,looping);
            }
        }];
    }
}

void native_midi_stop()
{
    if (currentsong) {
        currentsong->playing = 0;
        [midiPlayer stop];
    }
}

int native_midi_active()
{
    return currentsong ? currentsong->playing : 0;
}

void native_midi_setvolume(NativeMidiSong *song, int volume)
{
}

const char *native_midi_error(NativeMidiSong *song)
{
    return "";  /* !!! FIXME */
}
