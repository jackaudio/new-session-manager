
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


/* Handles streaming regions from disk to track outputs. */

/* FIXME: we shouldn't depend on these */
#include "../Timeline.H" // for locking
#include "../Audio_Sequence.H"
#include "../Track.H"

// #include "Port.H"
#include "Playback_DS.H"
#include "Engine.H"
#include "dsp.h"

#include "const.h"
#include "debug.h"
#include "Thread.H"
#include <unistd.h>

bool
Playback_DS::seek_pending ( void )
{
    return _pending_seek != (nframes_t)-1;
}

/** request that the IO thread perform a seek and rebuffer.  This is
 called for each Disk_Stream whenever the RT thread determines that
 the transport has jumped to a new position. This is called *before*
 process. */
void
Playback_DS::seek ( nframes_t frame )
{
    THREAD_ASSERT( RT );

    /* FIXME: non-RT-safe IO */
    DMESSAGE( "requesting seek to frame %lu", (unsigned long)frame );

    if ( seek_pending() )
        printf( "seek error, attempt to seek while seek is pending\n" );

    _pending_seek = frame;

    flush();
}

/** set the playback delay to /frames/ frames. This be called prior to
a seek. */
void
Playback_DS::delay ( nframes_t frames )
{
    _delay = frames;
}

/** read /nframes/ from the attached track into /buf/ */
void
Playback_DS::read_block ( sample_t *buf, nframes_t nframes )
{
    THREAD_ASSERT( Playback );

    memset( buf, 0, nframes * sizeof( sample_t ) * channels() );

//    printf( "IO: attempting to read block @ %lu\n", _frame );

    if ( !timeline )
        return;

    while ( ! _terminate )
    {
        if ( ! timeline->tryrdlock() )
        {
            if ( sequence() )
            {
                
                /* FIXME: how does this work if _delay is not a multiple of bufsize? */
                
                if ( _frame >= _delay )
                {
                    if ( ! sequence()->play( buf, _frame - _delay, nframes, channels() ) )
                        WARNING( "Programming error?" );
                }
                
                _frame += nframes;
            }
            
            timeline->unlock();
            
            return;
        }

        usleep( 1000 * 10 );
    }
}

#define AVOID_UNNECESSARY_COPYING 1

void
Playback_DS::disk_thread ( void )
{
    _thread.name( "Playback" );

    DMESSAGE( "playback thread running" );

    /* buffer to hold the interleaved data returned by the track reader */
    sample_t *buf = buffer_alloc( _nframes * channels() * _disk_io_blocks );
#ifndef AVOID_UNNECESSARY_COPYING
    sample_t *cbuf = buffer_alloc( _nframes * _disk_io_blocks );
#endif

    int blocks_ready = 0;

    const nframes_t nframes = _nframes * _disk_io_blocks;

    while ( wait_for_block() )
    {

//        lock(); // for seeking

        if ( seek_pending() )
        {
            /* FIXME: non-RT-safe IO */
            DMESSAGE( "performing seek to frame %lu", (unsigned long)_pending_seek );

            _frame = _pending_seek;
            _pending_seek = -1;
            blocks_ready = 0;
        }

        if ( ++blocks_ready < _disk_io_blocks )
        {
            /* wait for more space */
            continue;
        }

        /* reset */
        blocks_ready = 0;

        read_block( buf, nframes );

        /* might have received terminate signal while waiting for block */
        if ( _terminate )
            goto done;

//        unlock(); // for seeking

        /* deinterleave the buffer and stuff it into the per-channel ringbuffers */

        const size_t block_size = nframes * sizeof( sample_t );

        for ( int i = channels(); i--; )
        {

#ifdef AVOID_UNNECESSARY_COPYING

            /* deinterleave direcectly into the ringbuffer to avoid
             * unnecessary copying */

            jack_ringbuffer_data_t rbd[2];

            memset( rbd, 0, sizeof( rbd ) );

            jack_ringbuffer_get_write_vector( _rb[ i ], rbd );

            if ( rbd[ 0 ].len >= block_size )
            {
                /* it'll all fit in one go */
                buffer_deinterleave_one_channel( (sample_t*)rbd[ 0 ].buf, buf, i, channels(), nframes );
            }
            else if ( rbd[ 0 ].len + rbd[ 1 ].len >= block_size )
            {
                /* there's enough space in the ringbuffer, but it's not contiguous */

                assert( ! ( rbd[ 0 ].len % sizeof( sample_t )  ) );
//                assert( ! ( rbd[ 1 ].len % sizeof( sample_t )  ) );

                const nframes_t f = rbd[ 0 ].len / sizeof( sample_t );

                /* do the first half */
                buffer_deinterleave_one_channel( (sample_t*)rbd[ 0 ].buf, buf, i, channels(), f );

                assert( rbd[ 1 ].len >= ( nframes - f ) * sizeof( sample_t ) );

                /* do the second half */
                buffer_deinterleave_one_channel( (sample_t*)rbd[ 1 ].buf, buf + f, i, channels(), nframes - f );
            }
            else
                ++_xruns;

            jack_ringbuffer_write_advance( _rb[ i ], block_size );
#else
            buffer_deinterleave_one_channel( cbuf, buf, i, channels(), nframes );

            if ( jack_ringbuffer_write( _rb[ i ], (char*)cbuf, block_size ) < block_size )
                ++_xruns;
#endif
        }

    }

done:

    DMESSAGE( "playback thread terminating" );

    free(buf);
#ifndef AVOID_UNNECESSARY_COPYING
    free(cbuf);
#endif

    _terminate = false;

    _thread.exit();
}

/** take a single block from the ringbuffers and send it out the
 *  attached track's ports */
nframes_t
Playback_DS::process ( nframes_t nframes )
{
    THREAD_ASSERT( RT );


    const size_t block_size = nframes * sizeof( sample_t );

//    printf( "process: %lu %lu %lu\n", _frame, _frame + nframes, nframes );

    for ( int i = channels(); i--;  )
    {

        void *buf = track()->output[ i ].buffer( nframes );

        if ( jack_ringbuffer_read( _rb[ i ], (char*)buf, block_size ) < block_size )
        {
            ++_xruns;
            memset( buf, 0, block_size );
            /* FIXME: we need to resync somehow */
        }

        /* TODO: figure out a way to stop IO while muted without losing sync */
        if ( track()->mute() || ( Track::soloing() && ! track()->solo() ) )
            buffer_fill_with_silence( (sample_t*)buf, nframes );
    }

    block_processed();

    /* FIXME: bogus */
    return nframes;
}
