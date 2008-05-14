
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

#pragma once

#include "Mutex.H"

#include <jack/jack.h>

typedef jack_nframes_t nframes_t;

class Port;

class Engine : public Mutex
{
    jack_client_t *_client;

    /* I know locking out the process callback is cheating, even
       though we use trylock... The thing is, every other DAW does
       this too and you can hear it in the glitches Ardour and friends
       produce when reconfiguring I/O... Working out a message queue
       system would obviously be better, but a DAW isn't a performance
       instrument anyway, so I think these drop-outs are a reasonable
       compromise. Obviously, this lock should never be held during
       blocking operations. */

    int _buffers_dropped;                                       /* buffers dropped because of locking */
    nframes_t _sample_rate;
    volatile int _xruns;

    static int process ( nframes_t nframes, void *arg );
    int process ( nframes_t nframes );
    static int sync ( jack_transport_state_t state, jack_position_t *pos, void *arg );
    int sync ( jack_transport_state_t state, jack_position_t *pos );
    static int xrun ( void *arg );
    int xrun ( void );
    static void timebase ( jack_transport_state_t state, jack_nframes_t nframes, jack_position_t *pos, int new_pos, void *arg );
    void timebase ( jack_transport_state_t state, jack_nframes_t nframes, jack_position_t *pos, int new_pos );

    Engine ( const Engine &rhs );
    Engine & operator = ( const Engine &rhs );

private:

    friend class Port;
    friend class Transport;
    jack_client_t * client ( void ) { return _client; }


public:

    Engine ( );

    int init ( void );

    void request_locate ( nframes_t frame );

    nframes_t nframes ( void ) const { return jack_get_buffer_size( _client ); }
    float frame_rate ( void ) const { return jack_get_sample_rate( _client ); }
    nframes_t sample_rate ( void ) const { return _sample_rate; }
    int xruns ( void ) const { return _xruns; };
    int dropped ( void ) const { return _buffers_dropped; }

    float cpu_load ( void ) const { return jack_cpu_load( _client ); }
};


extern Engine * engine;
