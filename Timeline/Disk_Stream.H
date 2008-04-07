
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

#include <jack/ringbuffer.h>
#include <semaphore.h>

class Disk_Stream
{

    const Track_Header *_th;                                    /* Track_Header we whould be playing */

    nframes_t _nframes;

    vector <jack_ringbuffer_t *> _rb;

    // jack_ringbuffer_t *_rb;                                     /* One interleaved ringbuffer for all channels */

    sem_t _blocks;                                                /* semaphore to wake the IO thread with */

    int channels ( void ) const { return _rb.size(); }

protected:

    void block_processed ( void ) { sem_post( &_blocks ); }
    bool wait_for_block ( void ) { while ( sem_wait( &_work ) == EINTR ); return true; }

public:

    static float seconds_to_buffer;

    Disk_Stream ( const Track_Header *th, float frame_rate, nframes_t nframes, int channels ) : _th( th )
        {
            _frame = 0;

            const int blocks = frame_rate * seconds_to_buffer / nframes;

            _nframes = nframes;

            size_t bufsize = blocks * nframes * sizeof( sample_t );

            for ( int i = channels(); i-- )
                _rb[ i ] = jack_ringbuffer_create( bufsize );

            sem_init( &_blocks, 0, blocks );

            run();
        }

    virtual ~Disk_Stream ( )
        {
            _th = NULL;

            sem_destroy( &_blocks );

            for ( int i = channels(); i-- )
                jack_ringbuffer_free( _rb[ i ] );
        }

    void
    resize_buffer ( void )
        {
        }


    void
    seek ( nframes_t frame )
        {
            _frame = frame;
        }

    void run ( void );

};
