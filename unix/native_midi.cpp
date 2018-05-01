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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>

#include <future>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <atomic>
#include <vector>

#include "native_midi/native_midi.h"
#include "util.h"
#include "palcfg.h"

#define CLIPLAYER_EXECUTABLE (which("timidity"))

struct _NativeMidiSong
{
	std::thread   Thread;
	std::mutex    Mutex;

    char *file;
    int  pid;
    volatile bool playing;
    bool looping;
};

const char* midi_file = "/tmp/sdlpal.temp.mid";
char* cliplayer = nullptr;

static char *which(const char *cmd)
{
	static char path[PATH_MAX] = { '\0' };
    FILE *fp = popen(PAL_va(0, "which %s", cmd), "r");
    if (fp == NULL) {
        return NULL;
    }else{
        if (fgets(path, sizeof(path)-1, fp) != NULL)
		{
			if (path[strlen(path) - 1] == '\n') path[strlen(path) - 1] = '\0';
			if (path[strlen(path) - 1] == '\r') path[strlen(path) - 1] = '\0';
		}
        pclose(fp);
        return path;
    }
}

extern "C" int native_midi_detect()
{
    if (cliplayer)
    {
        free(cliplayer);
        cliplayer = nullptr;
    }
    char *path = (gConfig.pszMIDIClient ? gConfig.pszMIDIClient : CLIPLAYER_EXECUTABLE);
    if (path && access(path,F_OK) == 0)
    {
        cliplayer = strdup(path);
        return 1;
    }
    else
    {
        return 0;
    }
}

extern "C" NativeMidiSong *native_midi_loadsong(const char *midifile)
{
    struct stat st;
    if (0 != stat(midifile, &st)) return NULL;

    auto song = new NativeMidiSong;
    if (NULL == song) return NULL;
    if (NULL == (song->file = new char[strlen(midifile) + 1])) return NULL;

    song->pid = -1;
    song->playing = false;
    song->looping = false;
    strcpy(song->file, midifile);

    return song;
}

extern "C" NativeMidiSong *native_midi_loadsong_RW(SDL_RWops *rw)
{
    FILE *fp = fopen(midi_file, "wb+");
    if (fp)
    {
        char buf[4096];
        size_t bytes;
        while((bytes = SDL_RWread(rw, buf, sizeof(char), sizeof(buf))) > 0)
            fwrite(buf, sizeof(char), bytes, fp);
        fclose(fp);

        return native_midi_loadsong(midi_file);
    }
    return NULL;
}

extern "C" void native_midi_freesong(NativeMidiSong *song)
{
    if (song)
    {
	if (native_midi_active(song))
		native_midi_stop(song);
        if (song->file) delete []song->file;
        delete song;
    }
}

extern "C" void native_midi_start(NativeMidiSong *song, int looping)
{
	if (!song) return;

	native_midi_stop(song);

	song->playing = true;
	song->looping = looping ? true : false;
	song->Thread = std::move(std::thread([](NativeMidiSong *song)->void {
        while(song->looping)
        {
            auto pid = fork();
            int status;

            if (0 == pid)
            {
                char* args[] = { cliplayer, song->file, NULL };
                if (-1 == execv(cliplayer, args)) exit(-1);
            }
            else if (-1 == pid)
            {
                return;
            }
            song->Mutex.lock();
            song->pid = pid;
            song->Mutex.unlock();
            waitpid(pid, &status, 0);
        }
        song->playing = false;
	}, song));
}

extern "C" void native_midi_stop(NativeMidiSong *song)
{
	if (song)
	{
		song->looping = false;
		song->playing = false;
 		if (-1 != song->pid)
            	{
			song->Mutex.lock();
			kill(song->pid, SIGTERM);
			song->Mutex.unlock();
            	}
		if (song->Thread.joinable())
			song->Thread.join();
		song->Thread = std::move(std::thread());
	}
}

extern "C" int native_midi_active(NativeMidiSong *song)
{
	return (song && song->playing) ? 1 : 0;
}

extern "C" void native_midi_setvolume(NativeMidiSong *song, int volume)
{
    // TODO
}

extern "C" const char *native_midi_error(NativeMidiSong *song)
{
    return "";
}
