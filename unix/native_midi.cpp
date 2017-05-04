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

// Warning: this is a unverified implementation and this comment should be removed if verified

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
char* timidity = nullptr;

extern "C" int native_midi_detect()
{
    // FIXME!!!
    if (timidity)
    {
        free(timidity);
        timidity = nullptr;
    }
    //system `timidity -v` will cause CLI blocked by the version info...
    if (access("/usr/bin/timidity",F_OK) == 0)
    {
        timidity = strdup("/usr/bin/timidity");
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
        while(bytes = SDL_RWread(rw, buf, sizeof(char), sizeof(buf)))
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
	//stop it first to prevent app terminated by destructing joinable thread destruction 
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
                char* args[] = { timidity, song->file, NULL };
                if (-1 == execv(timidity, args)) exit(-1);
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
