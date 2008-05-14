
/*******************************************************************************/
/* Copyright (C) 2008 Jonathan Moore Liles                                     */
/*                                                                             */
/* This program is free software; you can redistribute it and/or modify it     */
/* under the terms of the GNU General Public License as published by the       */
/* Free Software Foundation; either version 2 of the License, or (at your      */
/* option) any later version.                                                  */
/*                                                                             */
/* This program is distributed in the hope that it will be useful, but WITHOUT */
/* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       */
/* FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for   */
/* more details.                                                               */
/*                                                                             */
/* You should have received a copy of the GNU General Public License along     */
/* with This program; see the file COPYING.  If not,write to the Free Software */
/* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */
/*******************************************************************************/

#include "Engine.H"
#include "Transport.H"

#include "Timeline.H" // for process()
#include "Sequence_Widget.H" // for BBT and position info.

#define APP_NAME "Non-DAW" // FIXME: wrong place for this!

/* This is the home of the JACK process callback (does this *really*
   need to be a class?) */

Engine::Engine ( )
{
    _client = NULL;
    _buffers_dropped = 0;
    _xruns = 0;
}

/*******************/
/* Static Wrappers */
/*******************/

int
Engine::process ( nframes_t nframes, void *arg )
{
    return ((Engine*)arg)->process( nframes );
}

int
Engine::sync ( jack_transport_state_t state, jack_position_t *pos, void *arg )
{
    return ((Engine*)arg)->sync( state, pos );
}

int
Engine::xrun ( void *arg )
{
    return ((Engine*)arg)->xrun();
}

void
Engine::timebase ( jack_transport_state_t state, jack_nframes_t nframes, jack_position_t *pos, int new_pos, void *arg )
{
    ((Engine*)arg)->timebase( state, nframes, pos, new_pos );
}



void
Engine::request_locate ( nframes_t frame )
{
    if ( timeline )
        timeline->seek( frame );
}

/* THREAD: RT */
/** This is the jack xrun callback */
int
Engine::xrun ( void )
{
    ++_xruns;

    return 0;
}

/* THREAD: RT */
/** This is the jack slow-sync callback. */
int
Engine::sync ( jack_transport_state_t state, jack_position_t *pos )
{
    static bool seeking = false;

    switch ( state )
    {
        case JackTransportStopped:           /* new position requested */
            /* JACK docs lie. This is only called when the transport
               is *really* stopped, not when starting a slow-sync
               cycle */
            request_locate( pos->frame );
            return 1;
        case JackTransportStarting:          /* this means JACK is polling slow-sync clients */
        {
            if ( ! seeking )
            {
                request_locate( pos->frame );
                seeking = true;
            }

            int r = true;

            if ( timeline )
                r = timeline->seek_pending();

            if ( ! r )
                seeking = false;

            return ! seeking;
        }
        case JackTransportRolling:           /* JACK's timeout has expired */
            /* FIXME: what's the right thing to do here? */
//            request_locate( pos->frame );
            return 1;
//            return transport->frame == pos->frame;
            break;
        default:
            printf( "unknown transport state.\n" );
    }

    return 0;
}


/* THREAD: RT */
void
Engine::timebase ( jack_transport_state_t state, jack_nframes_t nframes, jack_position_t *pos, int new_pos )
{
    position_info pi = timeline->solve_tempomap( pos->frame );

    pos->valid = JackPositionBBT;
    pos->beats_per_bar = pi.beats_per_bar;
    pos->beat_type = pi.beat_type;
    pos->beats_per_minute = pi.tempo;

    pos->bar = pi.bbt.bar + 1;
    pos->beat = pi.bbt.beat + 1;
    pos->tick = pi.bbt.tick;
    pos->ticks_per_beat = 1920.0;                               /* FIXME: wrong place for this */

    /* FIXME: fill this in */
    pos->bar_start_tick = 0;

}


/* THREAD: RT */
int
Engine::process ( nframes_t nframes )
{
    transport->poll();

    if ( ! trylock() )
    {
        /* the data structures we need to access here (tracks and
         * their ports, but not track contents) may be in an
         * inconsistent state at the moment. Just punt and drop this
         * buffer. */
        ++_buffers_dropped;
        return 0;
    }

/*     if ( ! transport->rolling ) */
/*         timeline->silence( nframes ); */
/*         return 0; */


    /* handle chicken/egg problem */
    if ( timeline )
        /* this will initiate the process() call graph for the various
         * number and types of tracks, which will in turn send data out
         * the appropriate ports.  */
        timeline->process( nframes );

    unlock();

    return 0;
}


int
Engine::init ( void )
{
    if (( _client = jack_client_open ( APP_NAME, (jack_options_t)0, NULL )) == 0 )
        return 0;

#define set_callback( name ) jack_set_ ## name ## _callback( _client, &Engine:: name , this )

    set_callback( process );
    set_callback( xrun );

    /* FIXME: should we wait to register this until after the project
     has been loaded (and we have disk threads running)? */
    set_callback( sync );

    jack_set_timebase_callback( _client, 0, &Engine::timebase, this );

    jack_activate( _client );

    _sample_rate = frame_rate();

    /* we don't need to create any ports until tracks are created */
    return 1;
}
