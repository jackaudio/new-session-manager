
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
    return _pending_seek || buffer_percent() < 50;
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

    _seek_frame = frame;
    _pending_seek = true;

    /* wake the IO thread */
    block_processed();
}

/** set the playback delay to /frames/ frames. This be called prior to
a seek. */
void
Playback_DS::undelay ( nframes_t delay )
{
    _undelay = delay;
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

    while ( timeline->tryrdlock() )
    {
        if ( _terminate )
            return;
        
        usleep( 1000 * 10 );
    }
    
    if ( sequence() )
    {
        if ( ! sequence()->play( buf, _frame + _undelay, nframes, channels() ) )
            WARNING( "Programming error?" );
        
        _frame += nframes;
    }
    
    timeline->unlock();
}

void
Playback_DS::disk_thread ( void )
{
    _thread.name( "Playback" );

    DMESSAGE( "playback thread running" );

    /* buffer to hold the interleaved data returned by the track reader */
    sample_t *buf = buffer_alloc( _nframes * channels() * _disk_io_blocks );
    sample_t *cbuf = buffer_alloc( _nframes );

    const nframes_t nframes = _nframes;
    nframes_t blocks_written;

    while ( ! _terminate )
    {

    seek:

        blocks_written = 0;
        read_block( buf, nframes * _disk_io_blocks );

        while ( blocks_written < _disk_io_blocks &&
                wait_for_block() )
        {
//        lock(); // for seeking
            
            if ( _pending_seek )
            {
                /* FIXME: non-RT-safe IO */
                DMESSAGE( "performing seek to frame %lu", (unsigned long)_seek_frame );
                
                _frame = _seek_frame;
                _pending_seek = false;

                flush();
                
                goto seek;
            }

            /* might have received terminate signal while waiting for block */
            if ( _terminate )
                goto done;
        
//        unlock(); // for seeking
        
            /* deinterleave the buffer and stuff it into the per-channel ringbuffers */

            const size_t block_size = nframes * sizeof( sample_t );

            for ( int i = 0; i < channels(); i++ )
            {
                buffer_deinterleave_one_channel( cbuf,
                                                 buf + ( blocks_written * nframes * channels() ),
                                                 i,
                                                 channels(), 
                                                 nframes );

                while ( jack_ringbuffer_write_space( _rb[ i ] ) < block_size )
                    usleep( 100 * 1000 );

                jack_ringbuffer_write( _rb[ i ], ((char*)cbuf), block_size );
            }

            blocks_written++;
        }
    }

done:

    DMESSAGE( "playback thread terminating" );

    free(buf);
    free(cbuf);

//    flush();

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

        if ( engine->freewheeling() )
        {
            /* only ever read nframes at a time */
            while ( jack_ringbuffer_read_space( _rb[i] ) < block_size )
                usleep( 10 * 1000 );
            
            jack_ringbuffer_read( _rb[ i ], ((char*)buf), block_size );
        }
        else
       {
            /* only ever read nframes at a time */
            if ( jack_ringbuffer_read_space( _rb[i] ) < block_size )
            {
                ++_xruns;
                memset( buf, 0, block_size );
                /* FIXME: we need to resync somehow */
            }
            else
            {
                jack_ringbuffer_read( _rb[ i ], (char*)buf, block_size );
            }
        }

        /* TODO: figure out a way to stop IO while muted without losing sync */
        if ( track()->mute() || ( Track::soloing() && ! track()->solo() ) )
            buffer_fill_with_silence( (sample_t*)buf, nframes );
    }

    block_processed();

    /* FIXME: bogus */
    return nframes;
}
