
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
#include <errno.h>

#include <vector>

#include "types.h"
#include "util/Mutex.H"
#include "const.h"
#include "util/debug.h"
#include "util/Thread.H"

class Track;
class Audio_Sequence;

class Disk_Stream : public Mutex
{

    /* not permitted */
    Disk_Stream ( const Disk_Stream &rhs );
    Disk_Stream & operator = ( const Disk_Stream &rhs );


protected:

    Thread _thread;                                    /* io thread */

    Track *_track;                               /* Track we belong to */

    nframes_t _nframes;                              /* buffer size */

    nframes_t _frame;                      /* location of disk read */

    std::vector < jack_ringbuffer_t * >_rb; /* one ringbuffer for each channel */

    sem_t _blocks;          /* semaphore to wake the IO thread with */

    int _total_blocks; /* total number of blocks that we can  buffer */
    int _disk_io_blocks; /* the number of blocks to read/write to/from disk at once */


    nframes_t _frame_rate;      /* used for buffer size calculations */

    volatile nframes_t _pending_seek; /* absolute transport position to seek to */
    volatile int _terminate;

    volatile int _xruns;

    int channels ( void ) const { return _rb.size(); }

    Audio_Sequence * sequence ( void ) const;
    Track * track ( void ) const;

    static void *disk_thread ( void *arg );

    void _resize_buffers ( nframes_t nframes, int channels );

protected:

    void block_processed ( void ) { sem_post( &_blocks ); }
    bool wait_for_block ( void )
        {
            while ( ! sem_wait( &_blocks ) && errno == EINTR )
            {}

            return ! _terminate;
        }

    virtual void disk_thread ( void ) = 0;

    void base_flush ( bool is_output );
    virtual void flush ( void ) = 0;

    void run ( void );
    void shutdown ( void );
    void detach ( void );

public:

    /* must be set before any Disk_Streams are created */
    static float seconds_to_buffer;
    static size_t disk_io_kbytes;

    int xruns ( void ) { return _xruns; }

    Disk_Stream ( Track *th, float frame_rate, nframes_t nframes, int channels );

    virtual ~Disk_Stream ( );

    void resize_buffers ( nframes_t nframes );

/*     void seek ( nframes_t frame ); */
/*     bool seek_pending ( void ); */

    virtual nframes_t process ( nframes_t nframes ) = 0;

    int buffer_percent ( void );

};
