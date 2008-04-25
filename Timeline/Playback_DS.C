
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
    printf( "requesting seek\n" );

    if ( seek_pending() )
        printf( "seek error, attempt to seek while seek is pending\n" );

    _pending_seek = frame;

    /* flush buffers */
    for ( int i = channels(); i--; )
        jack_ringbuffer_read_advance( _rb[ i ], jack_ringbuffer_read_space( _rb[ i ] ) );

    /* dirty hack... reset the semaphore. Should we just call sem_init
     * again instead? */

/*     sem_init( &_blocks, 0, _total_blocks ); */

    int n;
    sem_getvalue( &_blocks, &n );

    n = _total_blocks - n;

    while ( n-- )
        sem_post( &_blocks );

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

    if ( ! track() )
    {
//        _frame += _nframes;
        return;
    }

    timeline->rdlock();

    if ( track()->play( buf, _frame, nframes, channels() ) )
        _frame += nframes;
    else
        /* error */;

    timeline->unlock();
}

/* THREAD: IO */
void
Playback_DS::disk_thread ( void )
{

    printf( "IO thread running...\n" );

    /* buffer to hold the interleaved data returned by the track reader */
    sample_t *buf = new sample_t[ _nframes * channels() ];

    const size_t block_size = _nframes * sizeof( sample_t );

    while ( wait_for_block() )
    {
//        printf( "IO: RT thread is ready for more data...\n" );

//        printf( "IO: disk buffer is %3d%% full\r", output_buffer_percent() );

//        lock(); // for seeking

        if ( seek_pending() )
        {
            printf( "performing seek\n" );
            _frame = _pending_seek;
            _pending_seek = -1;
            /* finish flushing the buffer */

/*             for ( int i = channels(); i-- ) */
/*                 jack_ringbuffer_write_advance( _rb[ i ], jack_ringbuffer_write_space( _rb[ i ] ) ); */

        }

        /* FIXME: should we not read from disk in larger-than-JACK-buffer blocks? */
        read_block( buf, _nframes );

//        unlock(); // for seeking

        /* deinterleave the buffer and stuff it into the per-channel ringbuffers */

        for ( int i = channels(); i--; )
        {
            while ( jack_ringbuffer_write_space( _rb[ i ] ) < block_size )
            {
                printf( "IO: disk buffer overrun!\n" );
                /* FIXME: is this *really* the right thing to do?  */
                usleep( 2000 );
            }

            /* deinterleave direcectly into the ringbuffer to avoid
             * unnecessary copying */

            jack_ringbuffer_data_t rbd[2];

            jack_ringbuffer_get_write_vector( _rb[ i ], rbd );

            if ( rbd[ 0 ].len >= _nframes )
                /* it'll all fit in one go */
                buffer_deinterleave_one_channel( (sample_t*)rbd[ 0 ].buf, buf, i, channels(), _nframes );
            else if ( rbd[ 1 ].len )
            {
                /* there's enough space in the ringbuffer, but it's not contiguous */

                /* do the first half */
                const nframes_t f = rbd[ 1 ].len / sizeof( sample_t );

                buffer_deinterleave_one_channel( (sample_t*)rbd[ 0 ].buf, buf, i, channels(), f );

                assert( rbd[ 1 ].len >= (_nframes - f) * sizeof( sample_t ) );

                /* do the second half */
                buffer_deinterleave_one_channel( (sample_t*)rbd[ 1 ].buf, buf + f, i, channels(), _nframes - f );
            }
            else
                printf( "programming error: expected more space in ringbuffer\n" );

/*             buffer_deinterleave_one_channel( (sample_t*)rbd.buf, buf, i, channels(), _nframes ); */
/*             jack_ringbuffer_write( _rb[ i ], (char*)cbuf, block_size ); */

            jack_ringbuffer_write_advance( _rb[ i ], _nframes * sizeof( sample_t ) );



        }
    }

    printf( "IO thread terminating.\n" );

    delete[] buf;
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

        void *buf = _th->output[ i ].buffer( nframes );

        if ( jack_ringbuffer_read( _rb[ i ], (char*)buf, block_size ) < block_size )
        {
            printf( "RT: buffer underrun (disk can't keep up).\n" );
            memset( buf, 0, block_size );
            /* FIXME: we need to resync somehow */
        }

        /* TODO: figure out a way to stop IO while muted without losing sync */
        if ( _th->mute() || ( Track::soloing() && ! _th->solo() ) )
            buffer_fill_with_silence( (sample_t*)buf, nframes );
    }

    block_processed();

    /* FIXME: bogus */
    return nframes;
}
