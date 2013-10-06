
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

// #include "Port.H"
#include "Record_DS.H"
#include "Engine.H"
#include "dsp.h"

#include "const.h"
#include "debug.h"
#include "Thread.H"

#include <unistd.h>

const Audio_Region *
Record_DS::capture_region ( void ) const
{
    if ( _capture )
        return _capture->region;
    else
        return NULL;
}

Track::Capture *
Record_DS::capture ( void )
{
    return _capture;
}

/** write /nframes/ from buf to the capture file of the attached track */
void
Record_DS::write_block ( sample_t *buf, nframes_t nframes )
{
    THREAD_ASSERT( Capture );

    /* stupid chicken/egg */
    if ( ! ( timeline && sequence() ) )
        return;


    if ( ! _capture->audio_file )
        /* create the file */
        track()->record( _capture, _frame );

    track()->write( _capture, buf, nframes );

    _frames_written += nframes;
}

void
Record_DS::disk_thread ( void )
{
    _thread.name( "Capture" );

    DMESSAGE( "capture thread running..." );

    const nframes_t nframes = _nframes;

    /* buffer to hold the interleaved data returned by the track reader */
    sample_t *buf = buffer_alloc( nframes * channels() * _disk_io_blocks );
    sample_t *cbuf = buffer_alloc( nframes );

    const size_t block_size = nframes * sizeof( sample_t );

    nframes_t blocks_read = 0;

    while ( wait_for_block() )
    {
        /* pull data from the per-channel ringbuffers and interlace it */
        for ( int i = channels(); i--; )
        {
            while ( jack_ringbuffer_read_space( _rb[ i ] ) < block_size )
                usleep( 10 * 1000 );

            jack_ringbuffer_read( _rb[ i ], ((char*)cbuf), block_size );
            
            buffer_interleave_one_channel( buf + ( blocks_read * nframes * channels() ),
                                           cbuf, 
                                           i,
                                           channels(),
                                           nframes );
        }

        blocks_read++;

        if ( blocks_read == _disk_io_blocks )
        {
            write_block( buf, nframes * _disk_io_blocks );
            blocks_read = 0;
        }
    }

    DMESSAGE( "capture thread terminating" );

    /* flush what remains in the buffer out to disk */

    {
        while ( blocks_read-- > 0 || ( ! sem_trywait( &_blocks ) && errno != EAGAIN ) )
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
    }

    free(buf);
    free(cbuf);

    DMESSAGE( "finalzing capture" );

    Track::Capture *c = _capture;

    _capture = NULL;

    /* now finalize the recording */

    if ( c->audio_file )
        track()->finalize( c, _stop_frame );

    delete c;

    flush();

    _terminate = false;

    DMESSAGE( "capture thread gone" );

    _thread.exit();
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

//    detach();

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

        /* FIXME: this results in a ringbuffer size that is no longer
         necessarily a multiple of nframes...  how will the other side
         handle that? */

        if ( engine->freewheeling() )
        {
            while ( jack_ringbuffer_write_space( _rb[i] ) < block_size )
                usleep( 10 * 1000 );

            jack_ringbuffer_write( _rb[ i ], ((char*)buf) + offset_size, block_size );
        }
        else
        {
            if ( jack_ringbuffer_write_space( _rb[i] ) < block_size )
            {
                memset( buf, 0, block_size );
                /* FIXME: we need to resync somehow */
                ++_xruns;
            }
            else
            {
                jack_ringbuffer_write( _rb[ i ], ((char*)buf) + offset_size, block_size );
            }
        }
    }

    block_processed();

    /* FIXME: bogus */
    return nframes;
}
