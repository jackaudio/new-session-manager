
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

#include "Port.H" // for nframes_t

#include <jack/ringbuffer.h>
#include <semaphore.h>
#include <errno.h>

#include <pthread.h>

#include "Mutex.H"

#include <vector>
using std::vector;

class Track_Header;
class Audio_Track;

class Disk_Stream : public Mutex
{

    pthread_t _thread;

    Track_Header *_th;                                    /* Track_Header we whould be playing */

    nframes_t _nframes;
    nframes_t _frame;

    vector <jack_ringbuffer_t *> _rb;

    // jack_ringbuffer_t *_rb;                                     /* One interleaved ringbuffer for all channels */

    sem_t _blocks;                                                /* semaphore to wake the IO thread with */

    int _total_blocks;

    volatile nframes_t _pending_seek;                           /* absolute transport position to seek to */

    volatile int _terminate;

    int channels ( void ) const { return _rb.size(); }

    Audio_Track * track ( void );

    static void *io_thread ( void *arg );

protected:

    void block_processed ( void ) { sem_post( &_blocks ); }
    bool wait_for_block ( void )
        {
            if ( _terminate )
                return false;
            else
            {
                while ( sem_wait( &_blocks ) == EINTR );
                return true;
            }
        }

    void read_block ( sample_t *buf, nframes_t nframes );
    void io_thread ( void );

public:

    /* must be set before any Disk_Streams are created */
    static float seconds_to_buffer;

    Disk_Stream ( Track_Header *th, float frame_rate, nframes_t nframes, int channels );

    virtual ~Disk_Stream ( );

    void resize ( nframes_t nframes );
    void seek ( nframes_t frame );
    bool seek_pending ( void );

    void run ( void );
    nframes_t process ( nframes_t nframes );

    int output_buffer_percent ( void );
};
