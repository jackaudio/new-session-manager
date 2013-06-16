
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

// #include "../Mixer.H" // for process()

/* This is the home of the JACK process callback */

// #include "const.h"
#include "debug.h"
#include "Thread.H"



Engine::Engine ( void (*process_callback)(nframes_t nframes, void *), void *user_data  ) : _thread( "RT" )
{
    _process_callback = process_callback;
    _process_callback_user_data = user_data;
    _buffer_size_callback = 0;
    _buffers_dropped = 0;
    _port_connect_callback = 0;
    _sample_rate_changed_callback = 0;
}

Engine::~Engine ( )
{
    deactivate();
}



void
Engine::buffer_size_callback (  void ( *buffer_size_callback ) ( nframes_t, void * ), void *user_data )
{
    _buffer_size_callback = buffer_size_callback;
    _buffer_size_callback_user_data = user_data;
}

void
Engine::sample_rate_changed_callback ( int ( *sample_rate_changed_callback ) ( nframes_t, void * ), void *user_data )
{
    _sample_rate_changed_callback = sample_rate_changed_callback;
    _sample_rate_changed_callback_user_data = user_data;
}

void
Engine::port_connect_callback (  void ( *port_connect_callback ) ( jack_port_id_t a, jack_port_id_t b, int connect, void *arg), void *user_data )
{
    _port_connect_callback = port_connect_callback;
    _port_connect_callback_user_data = user_data;
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
    /* JACK calls this in the RT thread, even though it's a
     * non-realtime operation. This mucks up our ability to do
     * THREAD_ASSERT, so just lie and say this is the UI thread... */

    _thread.set( "UI" );

    _buffer_size_callback( nframes, _buffer_size_callback_user_data );

    _thread.set( "RT" );

    return 0;
}

/* THREAD: ?? */
void
Engine::port_connect( jack_port_id_t a, jack_port_id_t b, int connect )
{
    if ( _port_connect_callback )
        _port_connect_callback( a, b, connect, _port_connect_callback_user_data );
}


/* THREAD: RT */
int
Engine::process ( nframes_t nframes )
{
    /* FIXME: wrong place for this */
    _thread.set( "RT" );

    if ( ! trylock() )
    {
        /* the data structures we need to access here (tracks and
         * their ports, but not track contents) may be in an
         * inconsistent state at the moment. Just punt and drop this
         * buffer. */
        ++_buffers_dropped;
        return 0;
    }

    _process_callback(nframes, _process_callback_user_data);

    unlock();

    return 0;
}

int
Engine::sample_rate_changed ( nframes_t srate )
{
    if ( _sample_rate_changed_callback )
        return _sample_rate_changed_callback( srate, _sample_rate_changed_callback_user_data );
    
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
