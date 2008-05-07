
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
#include "Timeline.H"
#include "Engine.H"
#include "Audio_Sequence.H"
#include "Track.H"
#include "Port.H"
#include "Playback_DS.H"


#include "dsp.h"

#include "debug.h"

bool
Playback_DS::seek_pending ( void )
{
    return _pending_seek != (nframes_t)-1;
}

/* THREAD: RT */
/** request that the IO thread perform a seek and rebuffer.  This is
 called for each Disk_Stream whenever the RT thread determines that
 the transport has jumped to a new position. This is called *before*
 process. */
void
Playback_DS::seek ( nframes_t frame )
{
    DMESSAGE( "requesting seek" );

    if ( seek_pending() )
        printf( "seek error, attempt to seek while seek is pending\n" );

    _pending_seek = frame;

    flush( true );
}

/* THREAD: IO */
/** read /nframes/ from the attached track into /buf/ */
void
Playback_DS::read_block ( sample_t *buf, nframes_t nframes )
{

    memset( buf, 0, nframes * sizeof( sample_t ) * channels() );

    /* stupid chicken/egg */
    if ( ! timeline )
        return;

//    printf( "IO: attempting to read block @ %lu\n", _frame );

    if ( ! sequence() )
    {
//        _frame += _nframes;
        return;
    }

    timeline->rdlock();

    if ( sequence()->play( buf, _frame, nframes, channels() ) )
        _frame += nframes;
    else
        /* error */;

    timeline->unlock();
}

#define AVOID_UNNECESSARY_COPYING 1

/* THREAD: IO */
void
Playback_DS::disk_thread ( void )
{

    DMESSAGE( "playback thread running" );

    /* buffer to hold the interleaved data returned by the track reader */
    sample_t *buf = new sample_t[ _nframes * channels() * _disk_io_blocks ];
#ifndef AVOID_UNNECESSARY_COPYING
    sample_t *cbuf = new sample_t[ _nframes * _disk_io_blocks ];
#endif

    int blocks_ready = 1;

    const nframes_t nframes = _nframes * _disk_io_blocks;

    while ( wait_for_block() )
    {

//        lock(); // for seeking

        if ( seek_pending() )
        {
            DMESSAGE( "performing seek" );

            _frame = _pending_seek;
            _pending_seek = -1;
            blocks_ready = 1;
            /* finish flushing the buffer */

/*             for ( int i = channels(); i-- ) */
/*                 jack_ringbuffer_write_advance( _rb[ i ], jack_ringbuffer_write_space( _rb[ i ] ) ); */

        }

        if ( blocks_ready < _disk_io_blocks )
        {
            ++blocks_ready;
            /* wait for more space */
            continue;
        }

        /* reset */
        blocks_ready = 1;

        read_block( buf, nframes );

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

    DMESSAGE( "playback thread terminating" );

    delete[] buf;
#ifndef AVOID_UNNECESSARY_COPYING
    delete[] cbuf;
#endif
}

/* THREAD: RT */
/** take a single block from the ringbuffers and send it out the
 *  attached track's ports */
nframes_t
Playback_DS::process ( nframes_t nframes )
{
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
