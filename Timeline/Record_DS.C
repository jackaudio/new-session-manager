
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

/* Handles streaming from track inputs to disk */

/* FIXME: we shouldn't depend on these */
#include "Timeline.H"
#include "Engine.H"
#include "Audio_Sequence.H"
#include "Track.H"
#include "Port.H"
#include "Record_DS.H"

#include "dsp.h"

/* THREAD: IO */
/** write /nframes/ from buf to the capture file of the attached track */
void
Record_DS::write_block ( sample_t *buf, nframes_t nframes )
{

    /* stupid chicken/egg */
    if ( ! ( timeline && track() ) )
        return;

//    timeline->wrlock();

    _th->write( buf, nframes );

    _frames_written += nframes;

//    timeline->unlock();
}

#define AVOID_UNNECESSARY_COPYING 1

/* THREAD: IO */
void
Record_DS::disk_thread ( void )
{
    printf( "IO thread running...\n" );

    const nframes_t nframes = _nframes * _disk_io_blocks;

    /* buffer to hold the interleaved data returned by the track reader */
    sample_t *buf = new sample_t[ nframes * channels() ];
#ifndef AVOID_UNNECESSARY_COPYING
    sample_t *cbuf = new sample_t[ nframes ];
#endif

    const size_t block_size = nframes * sizeof( sample_t );

    int blocks_ready = 1;

    while ( wait_for_block() )
    {

        if ( blocks_ready < _disk_io_blocks )
        {
            ++blocks_ready;
            continue;
        }

        blocks_ready = 1;

        /* pull data from the per-channel ringbuffers and interlace it */
        for ( int i = channels(); i--; )
        {

#ifdef AVOID_UNNECESSARY_COPYING

            /* interleave direcectly from the ringbuffer to avoid
             * unnecessary copying */

            jack_ringbuffer_data_t rbd[2];

            memset( rbd, 0, sizeof( rbd ) );

            jack_ringbuffer_get_read_vector( _rb[ i ], rbd );

            if ( rbd[ 0 ].len >= block_size )
            {
                /* it'll all fit in one go */
                buffer_interleave_one_channel( buf, (sample_t*)rbd[ 0 ].buf, i, channels(), nframes );
            }
            else if ( rbd[ 0 ].len + rbd[ 1 ].len >= block_size )
            {
                /* there's enough space in the ringbuffer, but it's not contiguous */

                assert( ! ( rbd[ 0 ].len % sizeof( sample_t )  ) );

                const nframes_t f = rbd[ 0 ].len / sizeof( sample_t );

                /* do the first half */
                buffer_deinterleave_one_channel( (sample_t*)rbd[ 0 ].buf, buf, i, channels(), f );
                buffer_interleave_one_channel( buf, (sample_t*)rbd[ 0 ].buf, i, channels(), f );

                assert( rbd[ 1 ].len >= ( nframes - f ) * sizeof( sample_t ) );

                /* do the second half */
                buffer_interleave_one_channel( buf + f, (sample_t*)rbd[ 0 ].buf, i, channels(), nframes - f );

            }
            else
                ++_xruns;

            jack_ringbuffer_read_advance( _rb[ i ], block_size );
#else
            if ( jack_ringbuffer_read( _rb[ i ], (char*)cbuf, block_size ) < block_size )
                ++_xruns;

            buffer_interleave_one_channel( buf, cbuf, i, channels(), nframes );
#endif

        }

        write_block( buf, nframes );

    }

    printf( "IO thread terminating.\n" );


    /* flush what remains in the buffer out to disk */

    {
        /* use JACk sized blocks for this last bit */
        const nframes_t nframes = _nframes;
        const size_t block_size = _nframes * sizeof( sample_t );


#ifdef AVOID_UNNECESSARY_COPYING
        sample_t *cbuf = new sample_t[ nframes ];
#endif

        while ( blocks_ready-- > 0 || ! sem_trywait( &_blocks ) && errno != EAGAIN )
        {

                for ( int i = channels(); i--; )
                {
                    jack_ringbuffer_read( _rb[ i ], (char*)cbuf, block_size );

                    buffer_interleave_one_channel( buf, cbuf, i, channels(), nframes );
                }

                const nframes_t frames_remaining = (_stop_frame - _frame ) - _frames_written;

                if ( frames_remaining < nframes )
                {
                    /* this is the last block, might be partial  */
                    write_block( buf, frames_remaining );
                    break;
                }
                else
                    write_block( buf, nframes );
        }

#ifdef AVOID_UNNECESSARY_COPYING
        delete[] cbuf;
#endif

    }

    delete[] buf;
#ifndef AVOID_UNNECESSARY_COPYING
    delete[] cbuf;
#endif
}


/** begin recording */
void
Record_DS::start ( nframes_t frame )
{

    if ( _recording )
    {
        printf( "programming error: attempt to start recording while recording is still in progress\n" );
        return;
    }

    /* FIXME: safe to do this here? */
    flush( false );

    _frame = frame;

    _th->record( frame );

    run();

    _recording = true;

}

/** finalize the recording process. */
void
Record_DS::stop ( nframes_t frame )
{

    if ( ! _recording )
    {
        printf( "programming error: attempt to stop recording when no recording is being made\n" );
        return;
    }

    _recording = false;

    /* FIXME: we may still have data in the buffers waiting to be
     * written to disk... We should flush it out before stopping... */

    _stop_frame = frame;

    shutdown();

    /* FIXME: flush buffers here? */

    _th->stop( frame );

    printf( "recording finished\n" );
}


/* THREAD: RT */
/** read from the attached track's ports and stuff the ringbuffers */
nframes_t
Record_DS::process ( nframes_t nframes )
{

    if ( ! _recording )
        return 0;

    const size_t block_size = nframes * sizeof( sample_t );

//    printf( "process: %lu %lu %lu\n", _frame, _frame + nframes, nframes );

    for ( int i = channels(); i--;  )
    {
        void *buf = _th->input[ i ].buffer( nframes );

        if ( jack_ringbuffer_write( _rb[ i ], (char*)buf, block_size ) < block_size )
        {
            ++_xruns;
            printf( "RT: buffer overrun (disk can't keep up).\n" );
            memset( buf, 0, block_size );
            /* FIXME: we need to resync somehow */
        }
    }

    block_processed();

    /* FIXME: bogus */
    return nframes;
}
