
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

#define APP_NAME "Non-DAW" // FIXME: wrong place for this!

/* This is the home of the JACK process callback (does this *really*
   need to be a class?) */

Engine::Engine ( )
{
    _client = NULL;
    _buffers_dropped = 0;
}


/* static wrapper */
int
Engine::process ( nframes_t nframes, void *arg )
{
    return ((Engine*)arg)->process( nframes );
}

/* static wrapper */
int
Engine::sync ( jack_transport_state_t state, jack_position_t *pos, void *arg )
{
    return ((Engine*)arg)->sync( state, pos );
}

void
Engine::request_locate ( nframes_t frame )
{
    if ( timeline )
        timeline->seek( frame );
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
//            return transport.frame == pos->frame;
            break;
        default:
            printf( "unknown transport state.\n" );
    }

    return 0;
}

/* THREAD: RT */
int
Engine::process ( nframes_t nframes )
{
    transport.poll();

    if ( ! transport.rolling )
        /* FIXME: fill all ports with silence */
        return 0;

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

    return 0;
}


int
Engine::init ( void )
{
    if (( _client = jack_client_open ( APP_NAME, (jack_options_t)0, NULL )) == 0 )
        return 0;

    jack_set_process_callback( _client, &Engine::process, this );

    /* FIXME: should we wait to register this until after the session
     has been loaded (and we have disk threads running)? */
    jack_set_sync_callback( _client, &Engine::sync, this );

    jack_activate( _client );

    /* we don't need to create any ports until tracks are created */
    return 1;
}
