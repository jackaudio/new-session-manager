
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
#include "../Transport.H"

#include "../Timeline.H" // for process()
#include "../Sequence_Widget.H" // for BBT and position info.

/* This is the home of the JACK process callback */

#include "const.h"
#include "util/debug.h"
#include "util/Thread.H"



Engine::Engine ( ) : _thread( "RT" )
{
    _buffers_dropped = 0;
}

Engine::~Engine ( )
{
}



/*************/
/* Callbacks */
/*************/

/* THREAD: RT */
/** This is the jack xrun callback */
int
Engine::xrun ( void )
{
    return 0;
}

/* THREAD: RT */
void
Engine::freewheel ( bool starting )
{
    if ( starting )
        DMESSAGE( "entering freewheeling mode" );
    else
        DMESSAGE( "leaving freewheeling mode" );
}

/* THREAD: RT (non-RT) */
int
Engine::buffer_size ( nframes_t nframes )
{
    timeline->resize_buffers( nframes );

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
            transport->frame = pos->frame;
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
Engine::timebase ( jack_transport_state_t, jack_nframes_t, jack_position_t *pos, int )
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
    /* FIXME: wrong place for this */
    _thread.set( "RT" );

    transport->poll();

    if ( freewheeling() )
    {
        /* freewheeling mode/export. We're actually running
           non-RT. Assume that everything is quiescent, locking is
           unecessary and do I/O synchronously */
        if ( timeline )
            timeline->process( nframes );

        /* because we're going faster than realtime. */
        timeline->wait_for_buffers();
    }
    else
    {
        if ( ! trylock() )
        {
            /* the data structures we need to access here (tracks and
             * their ports, but not track contents) may be in an
             * inconsistent state at the moment. Just punt and drop this
             * buffer. */
            ++_buffers_dropped;
            return 0;
        }

        /* handle chicken/egg problem */
        if ( timeline )
            /* this will initiate the process() call graph for the various
             * number and types of tracks, which will in turn send data out
             * the appropriate ports.  */
            timeline->process( nframes );

        unlock();
    }

    return 0;
}


/* TRHEAD: RT */
void
Engine::thread_init ( void )
{
    _thread.set( "RT" );
}

/* THREAD: RT */
void
Engine::shutdown ( void )
{
}

void
Engine::request_locate ( nframes_t frame )
{
    if ( timeline )
        timeline->seek( frame );
}
