
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

#include <pthread.h>

const pthread_mutex_t _mutex_initializer = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;

class Mutex
{

    pthread_mutex_t _lock;

public:

    Mutex ( )
        {
//            pthread_mutex_init( &_lock, NULL );
            _lock = _mutex_initializer;
        }

    virtual ~Mutex ( )
        {
            pthread_mutex_destroy( &_lock );
        }

    void
    lock ( void )
        {
            pthread_mutex_lock( &_lock );
        }

    void
    unlock ( void )
        {
            pthread_mutex_unlock( &_lock );
        }

    bool
    trylock ( void )
        {
            return pthread_mutex_trylock( &_lock ) == 0;
        }

};


class Locker
{

    Mutex &_lock;

public:

    Locker ( Mutex & lock ) : _lock( lock )
        {
            _lock.lock();
        }

    ~Locker ( )
        {
            _lock.unlock();
        }
};
