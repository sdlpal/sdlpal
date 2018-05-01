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

#include "native_midi/native_midi.h"
#include "native_midi/native_midi_common.h"
#include "palcfg.h"

#include <SDL_thread.h>
#include <alsa/asoundlib.h>


typedef struct {
    uint32_t tick;
    uint8_t type, channel, data1, data2;
    union {
        int32_t pitch;
        int32_t tempo;
        uint32_t len;
    };
    uint8_t *sysex;
    uint64_t time;
} midi_event_info;

struct _NativeMidiSong {
    snd_seq_t          *sequencer;
    int                 src_client_id;
    int                 src_port_id;
    int                 dst_client_id;
    int                 dst_port_id;
    int                 queue;

    midi_event_info    *events;
    unsigned int        timediv;

    volatile int        playing;
    int                 current_volume;
    volatile int        new_volume;
    int                 looping;

    SDL_Thread         *thread;
};


static const uint8_t alsa_event_types[8] = {
    SND_SEQ_EVENT_NOTEOFF,
    SND_SEQ_EVENT_NOTEON,
    SND_SEQ_EVENT_KEYPRESS,
    SND_SEQ_EVENT_CONTROLLER,
    SND_SEQ_EVENT_PGMCHANGE,
    SND_SEQ_EVENT_CHANPRESS,
    SND_SEQ_EVENT_PITCHBEND,
    SND_SEQ_EVENT_NONE
};


static int native_midi_available = -1;
static int dst_client_id = 0;
static int dst_port_id = 0;


static void error_handler(const char *file, int line, const char *function, int err, const char *fmt, ...)
{
    // do nothing
}

static int create_src_port(NativeMidiSong *song)
{
    snd_seq_port_info_t *pinfo;

    snd_seq_port_info_alloca(&pinfo);

    snd_seq_port_info_set_name(pinfo, "PAL-midi");

    snd_seq_port_info_set_capability(pinfo, 0);
    snd_seq_port_info_set_type(pinfo, SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);

    if (snd_seq_create_port(song->sequencer, pinfo) < 0) return -1;

    song->src_port_id = snd_seq_port_info_get_port(pinfo);

    return 0;
}

static int find_dst_port(NativeMidiSong *song, const char *midi_client)
{
    snd_seq_client_info_t *cinfo;
    snd_seq_port_info_t *pinfo;
    int client_id, port_id;

    if ((midi_client != NULL) && (*midi_client != 0))
    {
        snd_seq_addr_t addr;
        if (snd_seq_parse_address(song->sequencer, &addr, midi_client) < 0) return -1;

        song->dst_client_id = addr.client;
        song->dst_port_id = addr.port;

        snd_seq_port_info_alloca(&pinfo);

        if (snd_seq_get_any_port_info(song->sequencer, song->dst_client_id, song->dst_port_id, pinfo) < 0) return -2;

        if ((snd_seq_port_info_get_capability(pinfo) & (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE | SND_SEQ_PORT_CAP_NO_EXPORT)) != (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE))
        {
            return -3;
        }
        else
        {
            return 0;
        }
    }
    else
    {
        client_id = -1;

        snd_seq_client_info_alloca(&cinfo);
        snd_seq_port_info_alloca(&pinfo);

        snd_seq_client_info_set_client(cinfo, -1);
        while (snd_seq_query_next_client(song->sequencer, cinfo) >= 0)
        {
            snd_seq_port_info_set_client(pinfo, snd_seq_client_info_get_client(cinfo));
            snd_seq_port_info_set_port(pinfo, -1);

            while (snd_seq_query_next_port(song->sequencer, pinfo) >= 0)
            {
                if ( ((snd_seq_port_info_get_capability(pinfo) & (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE | SND_SEQ_PORT_CAP_NO_EXPORT)) == (SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE)) &&
                     (snd_seq_port_info_get_type(pinfo) & (SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_MIDI_GM))
                   )
                {
                    if (snd_seq_port_info_get_midi_channels(pinfo))
                    {
                        song->dst_client_id = snd_seq_client_info_get_client(cinfo);
                        song->dst_port_id = snd_seq_port_info_get_port(pinfo);
                        return 0;
                    }
                    else if (client_id == -1)
                    {
                        client_id = snd_seq_client_info_get_client(cinfo);
                        port_id = snd_seq_port_info_get_port(pinfo);
                    }
                }
            }
        }

        if (client_id != -1)
        {
            song->dst_client_id = client_id;
            song->dst_port_id = port_id;
            return 0;
        }
        else
        {
            return -4;
        }
    }
}

static int open_alsa_port(NativeMidiSong *song)
{
    if (song == NULL) return -1;
    if (song->sequencer != NULL) return -2;

    if (snd_seq_open(&(song->sequencer), "default", SND_SEQ_OPEN_DUPLEX, 0) < 0)
    {
        song->sequencer = NULL;
        return -3;
    }

    if (snd_seq_set_client_name(song->sequencer, "PAL-midi") < 0)
    {
        snd_seq_close(song->sequencer);
        song->sequencer = NULL;
        return -4;
    }

    song->src_client_id = snd_seq_client_id(song->sequencer);
    if (song->src_client_id < 0)
    {
        snd_seq_close(song->sequencer);
        song->sequencer = NULL;
        return -5;
    }

    if (dst_client_id == 0)
    {
        if (find_dst_port(song, gConfig.pszMIDIClient) < 0)
        {
            snd_seq_close(song->sequencer);
            song->sequencer = NULL;
            return -6;
        }
    }
    else
    {
        song->dst_client_id = dst_client_id;
        song->dst_port_id = dst_port_id;
    }

    if (create_src_port(song) < 0)
    {
        snd_seq_close(song->sequencer);
        song->sequencer = NULL;
        return -7;
    }

    song->queue = snd_seq_alloc_named_queue(song->sequencer, "PAL-midi");
    if (song->queue < 0)
    {
        snd_seq_delete_port(song->sequencer, song->src_port_id);
        snd_seq_close(song->sequencer);
        song->sequencer = NULL;
        return -8;
    }

    if (snd_seq_connect_to(song->sequencer, song->src_port_id, song->dst_client_id, song->dst_port_id) < 0)
    {
        snd_seq_free_queue(song->sequencer, song->queue);
        snd_seq_delete_port(song->sequencer, song->src_port_id);
        snd_seq_close(song->sequencer);
        song->sequencer = NULL;
        return -9;
    }

    return 0;
}

static void close_alsa_port(NativeMidiSong *song)
{
    if (song->sequencer != NULL)
    {
        snd_seq_disconnect_to(song->sequencer, song->src_port_id, song->dst_client_id, song->dst_port_id);
        snd_seq_free_queue(song->sequencer, song->queue);
        snd_seq_delete_port(song->sequencer, song->src_port_id);
        snd_seq_close(song->sequencer);

        song->sequencer = NULL;
    }
}

static int convert_event_list(NativeMidiSong *song, MIDIEvent *eventlist)
{
    int eventcount = 0;

    MIDIEvent* eventlist1;
    for (eventlist1 = eventlist; eventlist1 != NULL; eventlist1 = eventlist1->next)
    {
        eventcount++;
    }

    midi_event_info *events = calloc(eventcount + 1, sizeof(midi_event_info));
    if (events == NULL) return -1;

    events[0].tick = 0;
    events[0].type = SND_SEQ_EVENT_NONE;
    events[0].len = 0;
    events[0].sysex = NULL;
    events[0].time = 0;


    unsigned int tempo, tempo_tick;
    uint64_t tempo_time;

    tempo = 500000; // 500000 MPQN = 120 BPM
    tempo_tick = 0;
    tempo_time = 0;

    unsigned int time_division = song->timediv;

    eventcount = 0;
    for (; eventlist != NULL; eventlist = eventlist->next)
    {
        midi_event_info event;

        event.tick = eventlist->time;
        event.sysex = NULL;
        event.type = SND_SEQ_EVENT_NONE;

        // calculate event time in nanoseconds
        {
            div_t divres;

            divres = div(event.tick - tempo_tick, time_division);

            event.time = ( ((1000 * divres.rem) * (uint64_t)tempo) / time_division )
                       + ( (divres.quot * (uint64_t)tempo) * 1000 )
                       + tempo_time
                       ;

            //event.time = ( (((event.tick - tempo_tick) * (uint64_t) 1000) * tempo) / time_division ) + tempo_time;
        }


        int status = (eventlist->status & 0xf0) >> 4;
        switch (status)
        {
        case MIDI_STATUS_NOTE_OFF:
        case MIDI_STATUS_NOTE_ON:
        case MIDI_STATUS_AFTERTOUCH:
        case MIDI_STATUS_CONTROLLER:
        case MIDI_STATUS_PROG_CHANGE:
        case MIDI_STATUS_PRESSURE:
        case MIDI_STATUS_PITCH_WHEEL:
            event.type = alsa_event_types[status - 8];
            event.channel = eventlist->status & 0x0f;
            event.data1 = eventlist->data[0];
            event.data2 = eventlist->data[1];
            if (status == MIDI_STATUS_PITCH_WHEEL)
            {
                event.pitch = ( ((int32_t)event.data1) | (((int32_t)event.data2) << 7) ) - 0x2000;
            }
            break;

        case MIDI_STATUS_SYSEX:
            if (eventlist->status == 0xff) // meta events
            {
                if (eventlist->data[0] == 0x51) // set tempo
                {
                    event.type = SND_SEQ_EVENT_TEMPO;
                    event.channel = eventlist->extraData[0];
                    event.data1 = eventlist->extraData[1];
                    event.data2 = eventlist->extraData[2];
                    event.tempo = (((uint32_t)event.channel) << 16) | (((uint32_t)event.data1) << 8) | ((uint32_t)event.data2);

                    tempo = event.tempo;
                    tempo_tick = event.tick;
                    tempo_time = event.time;
                }
            }
            else if ((eventlist->status == 0xf0) || (eventlist->status == 0xf7)) // sysex
            {
                event.type = SND_SEQ_EVENT_SYSEX;
                event.len = eventlist->extraLen + (eventlist->status == 0xf0)?1:0;
                if (event.len)
                {
                    event.sysex = (uint8_t *) malloc(event.len);
                    if (event.sysex != NULL)
                    {
                        if (eventlist->status == 0xf0)
                        {
                            event.sysex[0] = 0xf0;
                            memcpy(event.sysex + 1, eventlist->extraData, eventlist->extraLen);
                        }
                        else
                        {
                            memcpy(event.sysex, eventlist->extraData, eventlist->extraLen);
                        }
                    }
                }
            }

            break;
        }

        if (event.type != SND_SEQ_EVENT_NONE)
        {
            eventcount++;
            events[eventcount] = event;
        }
    }

    events[0].len = eventcount;

    song->events = events;
    return 0;
}

static void free_midi_events(NativeMidiSong *song)
{
    unsigned int index;

    if (song->events != NULL)
    {
        for (index = song->events[0].len; index != 0; index--)
        {
            if (song->events[index].sysex != NULL)
            {
                free(song->events[index].sysex);
                song->events[index].sysex = NULL;
            }
        }

        free(song->events);
        song->events = NULL;
    }
}


static int play_midi(void *_song)
{
    NativeMidiSong *song = (NativeMidiSong *)_song;

    midi_event_info *events;
    unsigned int current_event, base_tick, last_tick, num_events;
    uint64_t base_time;

    // set queue tempo
    {
        snd_seq_queue_tempo_t *queue_tempo;

        snd_seq_queue_tempo_alloca(&queue_tempo);

        snd_seq_queue_tempo_set_tempo(queue_tempo, 500000); // 120 BPM
        snd_seq_queue_tempo_set_ppq(queue_tempo, song->timediv);

        if (0 > snd_seq_set_queue_tempo(song->sequencer, song->queue, queue_tempo))
        {
            return 2;
        }
    }

    // start play
    if (0 > snd_seq_start_queue(song->sequencer, song->queue, NULL))
    {
        return 3;
    }
    if (0 > snd_seq_drain_output(song->sequencer))
    {
        return 4;
    }

    events = song->events;
    num_events = events[0].len;
    current_event = 1;
    base_tick = 0;
    base_time = 0;
    last_tick = 0;

    snd_seq_sync_output_queue(song->sequencer);

    int do_sleep;
    snd_seq_queue_status_t *queue_status;
    const snd_seq_real_time_t *real_time;
    int64_t time_diff, base_diff;
    snd_seq_event_t event;

    snd_seq_queue_status_alloca(&queue_status);

    snd_seq_ev_clear(&event);
    event.queue = song->queue;
    event.source.port = song->src_port_id;
    event.flags = SND_SEQ_TIME_STAMP_TICK | SND_SEQ_TIME_MODE_ABS;

    do_sleep = 0;
    while (1)
    {
        if (do_sleep)
        {
            do_sleep = 0;

            SDL_Delay(10);
        }

        if (!song->playing) break;

        if (current_event > num_events)
        {
            if (!song->looping)
            {
                song->playing = 0;
                break;
            }

            // looping
            base_tick += events[num_events].tick;
            base_time += events[num_events].time;

            current_event = 1;
        }

        if ((song->new_volume != song->current_volume) && (events[current_event].tick != 0))
        {
            int chan;

            song->current_volume = song->new_volume;

            snd_seq_ev_set_fixed(&event);
            event.type = SND_SEQ_EVENT_CONTROLLER;
            event.time.tick = base_tick + events[current_event - 1].tick;
            event.dest.client = song->dst_client_id;
            event.dest.port = song->dst_port_id;
            event.data.control.param = MIDI_CTL_MSB_MAIN_VOLUME;
            event.data.control.value = song->current_volume;

            for (chan = 0; chan < 16; chan++)
            {
                event.data.control.channel = chan;
                snd_seq_event_output(song->sequencer, &event);
            }

            snd_seq_drain_output(song->sequencer);
        }

        if (0 > snd_seq_get_queue_status(song->sequencer, song->queue, queue_status))
        {
            do_sleep = 1;
            continue;
        }

        real_time = snd_seq_queue_status_get_real_time(queue_status);

        base_diff = ((real_time->tv_sec * (uint64_t)1000000000) + real_time->tv_nsec) - base_time;

        time_diff = events[current_event].time - base_diff;

        if (time_diff >= 100000000) // 100ms
        {
            do_sleep = 1;
            continue;
        }

        do
        {
            // add events to queue
            event.type = events[current_event].type;
            event.time.tick = base_tick + events[current_event].tick;
            event.dest.client = song->dst_client_id;
            event.dest.port = song->dst_port_id;

            switch (event.type)
            {
                case SND_SEQ_EVENT_NOTEON:
                case SND_SEQ_EVENT_NOTEOFF:
                case SND_SEQ_EVENT_KEYPRESS:
                    snd_seq_ev_set_fixed(&event);
                    event.data.note.channel = events[current_event].channel;
                    event.data.note.note = events[current_event].data1;
                    event.data.note.velocity = events[current_event].data2;
                    break;
                case SND_SEQ_EVENT_CONTROLLER:
                    snd_seq_ev_set_fixed(&event);
                    event.data.control.channel = events[current_event].channel;
                    event.data.control.param = events[current_event].data1;
                    event.data.control.value = events[current_event].data2;
                    break;
                case SND_SEQ_EVENT_PGMCHANGE:
                case SND_SEQ_EVENT_CHANPRESS:
                    snd_seq_ev_set_fixed(&event);
                    event.data.control.channel = events[current_event].channel;
                    event.data.control.value = events[current_event].data1;
                    break;
                case SND_SEQ_EVENT_PITCHBEND:
                    snd_seq_ev_set_fixed(&event);
                    event.data.control.channel = events[current_event].channel;
                    event.data.control.value = events[current_event].pitch;
                    break;
                case SND_SEQ_EVENT_SYSEX:
                    snd_seq_ev_set_variable(&event, events[current_event].len, events[current_event].sysex);
                    break;
                case SND_SEQ_EVENT_TEMPO:
                    snd_seq_ev_set_fixed(&event);
                    event.dest.client = SND_SEQ_CLIENT_SYSTEM;
                    event.dest.port = SND_SEQ_PORT_SYSTEM_TIMER;
                    event.data.queue.queue = song->queue;
                    event.data.queue.param.value = events[current_event].tempo;
                    break;
            }

            snd_seq_event_output(song->sequencer, &event);

            current_event++;
            if (current_event > num_events) break;
            time_diff = events[current_event].time - base_diff;
        } while (time_diff < 100000000); // 100ms

        snd_seq_drain_output(song->sequencer);

        last_tick = event.time.tick;
    }

    // stop playing
    event.time.tick = last_tick;

    snd_seq_sync_output_queue(song->sequencer);

    event.type = SND_SEQ_EVENT_CONTROLLER;
    event.dest.client = song->dst_client_id;
    event.dest.port = song->dst_port_id;

    int chan;
    for (chan = 0; chan < 16; chan++)
    {
        snd_seq_ev_set_fixed(&event);
        event.data.control.channel = chan;
        event.data.control.param = MIDI_CTL_ALL_NOTES_OFF; // All notes off (this message stops all the notes that are currently playing)
        event.data.control.value = 0;

        snd_seq_event_output(song->sequencer, &event);

        snd_seq_ev_set_fixed(&event);
        event.data.control.channel = chan;
        event.data.control.param = MIDI_CTL_RESET_CONTROLLERS; // All controllers off (this message clears all the controller values for this channel, back to their default values)
        event.data.control.value = 0;

        snd_seq_event_output(song->sequencer, &event);
    }

    snd_seq_ev_set_fixed(&event);
    event.type = SND_SEQ_EVENT_STOP;
    event.dest.client = SND_SEQ_CLIENT_SYSTEM;
    event.dest.port = SND_SEQ_PORT_SYSTEM_TIMER;
    event.data.queue.queue = song->queue;

    snd_seq_event_output(song->sequencer, &event);

    snd_seq_drain_output(song->sequencer);

    snd_seq_sync_output_queue(song->sequencer);

    return 0;
}


#ifdef __cplusplus
extern "C"
{
#endif

int native_midi_detect()
{
    if (native_midi_available != -1) return native_midi_available;

    NativeMidiSong *song = calloc(1, sizeof(NativeMidiSong));
    if (song == NULL) return 0;

    snd_lib_error_set_handler(error_handler);

    if (open_alsa_port(song) < 0)
    {
        free(song);
        native_midi_available = 0;
        return 0;
    }

    native_midi_available = 1;
    dst_client_id = song->dst_client_id;
    dst_port_id = song->dst_port_id;

    close_alsa_port(song);
    free(song);

    return 1;
}

NativeMidiSong *native_midi_loadsong(const char *midifile)
{
    // Attempt to load the midi file
    SDL_RWops *rw = SDL_RWFromFile(midifile, "rb");
    if (rw == NULL) return NULL;

    NativeMidiSong *song = native_midi_loadsong_RW(rw);
    SDL_RWclose(rw);
    return song;
}


NativeMidiSong *native_midi_loadsong_RW(SDL_RWops *rw)
{
    NativeMidiSong *song = calloc(1, sizeof(NativeMidiSong));
    if (song == NULL) return NULL;

    Uint16 division;
    MIDIEvent *eventlist = CreateMIDIEventList(rw, &division);
    if (eventlist == NULL)
    {
        free(song);
        return NULL;
    }

    song->current_volume = 128;
    song->new_volume = 128;
    song->timediv = division;

    if (convert_event_list(song, eventlist) < 0)
    {
        FreeMIDIEventList(eventlist);
        free(song);
        return NULL;
    }

    FreeMIDIEventList(eventlist);

    if (open_alsa_port(song) < 0)
    {
        free_midi_events(song);
        free(song);
        return NULL;
    }

    return song;
}


void native_midi_freesong(NativeMidiSong *song)
{
    if (song != NULL)
    {
        native_midi_stop(song);
        close_alsa_port(song);
        free_midi_events(song);
        free(song);
    }
}


void native_midi_start(NativeMidiSong *song, int looping)
{
    if (song != NULL)
    {
        native_midi_stop(song);

        song->playing = 1;
        song->looping = looping;

#if SDL_VERSION_ATLEAST(2,0,0)
        song->thread = SDL_CreateThread(&play_midi, "PAL-midi", (void *)song);
#else
        song->thread = SDL_CreateThread(&play_midi, (void *)song);
#endif
    }
}


void native_midi_stop(NativeMidiSong *song)
{
    if (song != NULL)
    {
        song->playing = 0;
        if (song->thread != NULL)
        {
            SDL_WaitThread(song->thread, NULL);
            song->thread = NULL;
        }
    }
}


int native_midi_active(NativeMidiSong *song)
{
    return (song && song->playing) ? 1 : 0;
}


void native_midi_setvolume(NativeMidiSong *song, int volume)
{
    if (song != NULL)
    {
        if (volume > 127) volume = 127;
        if (volume < 0) volume = 0;
        song->new_volume = volume;
    }
}


const char *native_midi_error(NativeMidiSong *song)
{
    return "";
}

#ifdef __cplusplus
}
#endif

