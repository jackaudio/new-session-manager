
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

#include "../Mixer.H" // for process()

/* This is the home of the JACK process callback */

// #include "const.h"
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
Engine::buffer_size ( nframes_t )
{
    // timeline->resize_buffers( nframes );

    return 0;
}

int Engine::sync ( jack_transport_state_t, jack_position_t * )
{
    return 0;
}

void
Engine::timebase ( jack_transport_state_t, jack_nframes_t, jack_position_t *, int )
{


}

void
Engine::timebase ( jack_transport_state_t, jack_nframes_t, jack_position_t * )
{

}

/* THREAD: RT */
int
Engine::process ( nframes_t nframes )
{
    /* FIXME: wrong place for this */
    _thread.set( "RT" );

    if ( freewheeling() )
    {
/*         /\* freewheeling mode/export. We're actually running */
/*            non-RT. Assume that everything is quiescent, locking is */
/*            unecessary and do I/O synchronously *\/ */
/*         if ( timeline ) */
/*             timeline->process( nframes ); */

/*         /\* because we're going faster than realtime. *\/ */
/*         timeline->wait_for_buffers(); */
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
        if ( mixer )
            /* this will initiate the process() call graph for the various
             * number and types of tracks, which will in turn send data out
             * the appropriate ports.  */
            mixer->process( nframes );

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
