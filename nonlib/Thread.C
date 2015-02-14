
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

#include "Thread.H"
#include <assert.h>
#include <string.h>



pthread_key_t Thread::_current = 0;



Thread::Thread ( )
{
    _thread = 0;
    _name = 0;
}

Thread::Thread ( const char *name )
{
    _thread = 0;
    _name = name;
}

void
Thread::init ( void )
{
    pthread_key_create( &_current, NULL );
}

bool
Thread::is ( const char *name )
{
    return ! strcmp( Thread::current()->name(), name );
}

/** to be used by existing threads (that won't call clone()) */
void
Thread::set ( const char *name )
{
    _thread = pthread_self();
    _name = name;

    pthread_setspecific( _current, (void*)this );
}

Thread *
Thread::current ( void )
{
    return (Thread*)pthread_getspecific( _current );
}


struct thread_data
{
    void *(*entry_point)(void *);
    void *arg;
    void *t;
};

void *
Thread::run_thread ( void *arg )
{
    thread_data td = *(thread_data *)arg;
    delete (thread_data*)arg;

    pthread_setspecific( _current, td.t );

    return td.entry_point( td.arg );
}


bool
Thread::clone ( void *(*entry_point)(void *), void *arg )
{
    assert( ! _thread );

    thread_data *td = new thread_data;
    td->entry_point = entry_point;
    td->arg = arg;
    td->t = this;

    if ( pthread_create( &_thread, NULL, run_thread, td ) != 0 )
        return false;

    return true;
}

void
Thread::detach ( void )
{
    pthread_detach( _thread );
    _thread = 0;
}

void
Thread::cancel ( void )
{
    pthread_cancel( _thread );
    _thread = 0;
}

void
Thread::join ( void )
{
    pthread_join( _thread, NULL );
    _thread = 0;
}

/* never call this unless some other thread will be calling 'join' on
 * this one, otherwise, running() will return true even though the
 * thread is dead */
void
Thread::exit ( void *retval )
{
    pthread_exit( retval );
}
