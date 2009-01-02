
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
#include "../Timeline.H" // for locking
#include "../Audio_Sequence.H"
#include "../Track.H"

#include "Port.H"
#include "Record_DS.H"
#include "Engine.H"
#include "dsp.h"

#include "const.h"
#include "util/debug.h"
#include "util/Thread.H"

const Audio_Region *
Record_DS::capture_region ( void ) const
{
    if ( _capture )
        return _capture->region;
    else
        return NULL;
}

/** write /nframes/ from buf to the capture file of the attached track */
void
Record_DS::write_block ( sample_t *buf, nframes_t nframes )
{
    THREAD_ASSERT( Capture );

    /* stupid chicken/egg */
    if ( ! ( timeline && sequence() ) )
        return;

//    timeline->wrlock();

    track()->write( _capture, buf, nframes );

    _frames_written += nframes;

//    timeline->unlock();
}

#define AVOID_UNNECESSARY_COPYING 1

void
Record_DS::disk_thread ( void )
{
    _thread.name( "Capture" );

    track()->record( _capture, _frame );

    DMESSAGE( "capture thread running..." );

    const nframes_t nframes = _nframes * _disk_io_blocks;

    /* buffer to hold the interleaved data returned by the track reader */
    sample_t *buf = new sample_t[ nframes * channels() ];
#ifndef AVOID_UNNECESSARY_COPYING
    sample_t *cbuf = new sample_t[ nframes ];
#endif

    const size_t block_size = nframes * sizeof( sample_t );

    int blocks_ready = 0;

    while ( wait_for_block() )
    {
        if ( ++blocks_ready < _disk_io_blocks )
            continue;
        else
            blocks_ready = 0;

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

    DMESSAGE( "capture thread terminating" );

    /* flush what remains in the buffer out to disk */

    {
        /* use JACk sized blocks for this last bit */
        const nframes_t nframes = _nframes;
        const size_t block_size = _nframes * sizeof( sample_t );

#ifdef AVOID_UNNECESSARY_COPYING
        sample_t *cbuf = new sample_t[ nframes ];
#endif

        while ( blocks_ready-- > 0 || ( ! sem_trywait( &_blocks ) && errno != EAGAIN ) )
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

    DMESSAGE( "finalzing capture" );

    Track::Capture *c = _capture;

    _capture = NULL;

    /* now finalize the recording */

    track()->finalize( c, _stop_frame );

    delete c;

    _terminate = false;
    DMESSAGE( "capture thread gone" );
}


/** begin recording */
void
Record_DS::start ( nframes_t frame )
{
    THREAD_ASSERT( UI );

    if ( _recording )
    {
        WARNING( "programming error: attempt to start recording while recording is still in progress" );
        return;
    }

/*     /\* FIXME: safe to do this here? *\/ */
/*     flush(); */

    DMESSAGE( "recording started at frame %lu", (unsigned long)frame);

    _frame = frame;

    _capture = new Track::Capture;

    run();

    _recording = true;

}

/** finalize the recording process. */
void
Record_DS::stop ( nframes_t frame )
{
    THREAD_ASSERT( UI );

    if ( ! _recording )
    {
        WARNING( "programming error: attempt to stop recording when no recording is being made" );
        return;
    }

    _recording = false;

    _stop_frame = frame;

    detach();

    DMESSAGE( "recording finished" );
}


#include "../Transport.H"
extern Transport *transport;

/** read from the attached track's ports and stuff the ringbuffers */
nframes_t
Record_DS::process ( nframes_t nframes )
{
    THREAD_ASSERT( RT );

    if ( ! _recording )
        return 0;

     if ( transport->frame < _frame  )
         return 0;

/*    DMESSAGE( "recording actually happening at %lu (start frame %lu)", (unsigned long)transport->frame, (unsigned long)_frame); */

    nframes_t offset = 0;

    if ( _frame > transport->frame &&
         _frame < transport->frame + nframes )
    {
        /* The record start frame falls somewhere within the current
           buffer.  We must discard the unneeded portion and only
           stuff the part requested into the ringbuffer. */

        offset = _frame - transport->frame;

/*         DMESSAGE( "offset = %lu", (unsigned long)offset ); */
    }

    const size_t offset_size = offset * sizeof( sample_t );
    const size_t block_size = ( nframes * sizeof( sample_t ) ) - offset_size;

    for ( int i = channels(); i--;  )
    {
        /* read the entire input buffer */
        void *buf = track()->input[ i ].buffer( nframes );

/*         if ( buffer_is_digital_black( (sample_t*)buf, nframes ) ) */
/*              DWARNING( "recording an entirely blank buffer" ); */

        /* FIXME: this results in a ringbuffer size that is no longer
         necessarily a multiple of nframes...  how will the other side
         handle that? */
        if ( jack_ringbuffer_write( _rb[ i ], (char*)buf + offset, block_size ) < block_size )
        {
            ++_xruns;
            memset( buf, 0, block_size );
            /* FIXME: we need to resync somehow */
        }
    }

    block_processed();

    /* FIXME: bogus */
    return nframes;
}
