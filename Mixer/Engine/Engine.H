
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

#include "util/Mutex.H"

class Port;

#include "JACK/Client.H"

#include "Thread.H"

class Engine : public JACK::Client, public Mutex
{
    Thread _thread;                                            /* only used for thread checking */

    int _buffers_dropped;                                       /* buffers dropped because of locking */
/*     int _buffers_dropped;                                       /\* buffers dropped because of locking *\/ */

    void ( * _process_callback ) ( nframes_t, void * );
    void *_process_callback_user_data;

    void ( * _buffer_size_callback ) ( nframes_t, void * );
    void *_buffer_size_callback_user_data;

    void shutdown ( void );
    int process ( nframes_t nframes );
    int xrun ( void );
    void freewheel ( bool yes );
    int buffer_size ( nframes_t nframes );
    void thread_init ( void );

    Engine ( const Engine &rhs );
    Engine & operator = ( const Engine &rhs );

    void request_locate ( nframes_t frame );

private:

    friend class Port;
    friend class Transport;

public:

    Engine ( void (*process_callback) (nframes_t, void *), void *user_data );
    ~Engine ( );

    int dropped ( void ) const { return _buffers_dropped; }
    void buffer_size_callback ( void ( *buffer_size_callback ) ( nframes_t, void * ), void *user_data );

};
