
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

/* simple wrapper for pthreads with thread role checking */
#include <pthread.h>

#define THREAD_ASSERT( n ) ASSERT( Thread::is( #n ), "Function called from wrong thread! (is %s, should be %s)", Thread::current()->name(), #n )

class Thread
{
    static pthread_key_t _current;

    pthread_t _thread;
    const char * _name;

    volatile bool _running;

    static void * run_thread ( void *arg );

public:

    static bool is ( const char *name );

    static void init ( void );
    static Thread *current ( void );

    Thread ( );
    Thread ( const char *name );

    const char *name ( void ) const { return _name; }
    void name ( const char *name ) { _name = name; }

    bool running ( void ) const { return _running; }
    void set ( const char *name );
    void set ( void ) { set( _name ); }
    bool clone ( void *(*entry_point)(void *), void *arg );
    void detach ( void );
    void join ( void );
    void cancel ( void );
    void exit ( void *retval = 0 );

};
