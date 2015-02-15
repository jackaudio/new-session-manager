
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

    if ( ! _capture )
    {
        _capture = new Track::Capture;

    /* if ( ! _capture->audio_file ) */
        /* create the file */
        track()->record( _capture, _frame );
    }

    track()->write( _capture, buf, nframes );

    _frames_written += nframes;
}

void
Record_DS::disk_thread ( void )
{
    _thread.name( "Capture" );

    DMESSAGE( "capture thread running..." );

    const nframes_t nframes = _nframes;

    _disk_io_blocks = 1;

    /* buffer to hold the interleaved data returned by the track reader */
    sample_t *buf = buffer_alloc( nframes * channels() * _disk_io_blocks );
    sample_t *cbuf = buffer_alloc( nframes );

    _recording = true;

//    const size_t block_size = nframes * sizeof( sample_t );

    nframes_t frames_read = 0;

    bool punching_in = false;
    bool punching_out = false;
    bool punched_in = false;

    nframes_t bS = 0;
    nframes_t bE = 0;

again:

    _capture = NULL;

    punched_in = false;
    punching_out = false;

    nframes_t pS = _frame;
    nframes_t pE = _stop_frame;

    if ( punching_in )
    {
        /* write remainder of buffer */
        write_block( buf + ((pS - bS) * channels()),
                         bE - pS );        

        punching_in = false;
        punched_in = true;
    }
    
    while ( wait_for_block() )
    {
        /* pull data from the per-channel ringbuffers and interlace it */
        size_t frames_to_read = nframes;
        
        /* we read the entire block if a partial... */
        for ( int i = 0; i < channels(); i++ )
        {
            while ( jack_ringbuffer_read_space( _rb[ i ] ) < frames_to_read * sizeof( sample_t ) )
                usleep( 10 * 1000 );
            
            jack_ringbuffer_read( _rb[ i ], ((char*)cbuf), frames_to_read * sizeof( sample_t ) );
            
            buffer_interleave_one_channel( buf,
                                           cbuf, 
                                           i,
                                           channels(),
                                           frames_to_read);
        }
       
        bS = _first_frame + frames_read;

        frames_read += frames_to_read;

        bE = _first_frame + frames_read;

        punching_in = ! punched_in && bE > pS;
        punching_out = punched_in && pE < bE;
        
        if ( punching_out )
        {
            write_block( buf,
                         pE - bS );

            break;
        }
        else
        if ( punching_in )
        {
            write_block( buf + ((pS - bS) * channels()),
                         bE - pS );

            punching_in = false;
            punched_in = true;
        }
        else if ( punched_in )
        {
            write_block( buf, bE - bS );
        }
    }

//    DMESSAGE( "capture thread terminating" );

    /* flush what remains in the buffer out to disk */

    /* { */
    /*     while ( blocks_read-- > 0 || ( ! sem_trywait( &_blocks ) && errno != EAGAIN ) ) */
    /*     { */
    /*         for ( int i = channels(); i--; ) */
    /*         { */
    /*             jack_ringbuffer_read( _rb[ i ], (char*)cbuf, block_size ); */

    /*             buffer_interleave_one_channel( buf, cbuf, i, channels(), nframes ); */
    /*         } */

    /*         const nframes_t frames_remaining = (_stop_frame - _frame ) - _frames_written; */

    /*         if ( frames_remaining < nframes ) */
    /*         { */
    /*             /\* this is the last block, might be partial  *\/ */
    /*             write_block( buf, frames_remaining ); */
    /*             break; */
    /*         } */
    /*         else */
    /*             write_block( buf, nframes ); */
    /*     } */
    /* } */

  
   
    if ( _capture )
    {
        DMESSAGE( "finalzing capture" );
        Track::Capture *c = _capture;
        
        _capture = NULL;
        
        /* now finalize the recording */
        
//        if ( c->audio_file )
        track()->finalize( c, _stop_frame );

        delete c;
    }

    if ( ! _terminate ) 
    {
        nframes_t in, out;
        
        if ( timeline->next_punch( _stop_frame, &in, &out ) )
        {
            _frame = in;
            _stop_frame = out;
            _frames_written = 0;

            punched_in = false;
            punching_out = false;
            
            punching_in = bE > in;

            DMESSAGE( "Next punch: %lu:%lu", (unsigned long)in,(unsigned long)out );

            goto again;
        }
    }

    free(buf);
    free(cbuf);

    flush();

    _terminate = false;
    _recording = false;

    DMESSAGE( "capture thread gone" );
}


/** begin recording */
void
Record_DS::start ( nframes_t frame, nframes_t start_frame, nframes_t stop_frame )
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

    _frame = start_frame;
    _stop_frame = stop_frame ? stop_frame : JACK_MAX_FRAMES;

    _first_frame = frame;

    run();
}

/** finalize the recording process. */
void
Record_DS::stop ( nframes_t frame )
{
    THREAD_ASSERT( UI );

    _stop_frame = frame;

    DMESSAGE( "recording stop scheduled" );
}


#include "../Transport.H"
extern Transport *transport;

/** read from the attached track's ports and stuff the ringbuffers */
nframes_t
Record_DS::process ( nframes_t nframes )
{
    THREAD_ASSERT( RT );

    if ( ! ( _recording && _thread.running() ) )
        return 0;

     /* if ( transport->frame < _frame  ) */
     /*     return 0; */

/*    DMESSAGE( "recording actually happening at %lu (start frame %lu)", (unsigned long)transport->frame, (unsigned long)_frame); */

    nframes_t offset = 0;

/*     if ( _frame > transport->frame && */
/*          _frame < transport->frame + nframes ) */
/*     { */
/*         /\* The record start frame falls somewhere within the current */
/*            buffer.  We must discard the unneeded portion and only */
/*            stuff the part requested into the ringbuffer. *\/ */

/*         offset = _frame - transport->frame; */

/* /\*         DMESSAGE( "offset = %lu", (unsigned long)offset ); *\/ */
/*     } */

    const size_t offset_size = offset * sizeof( sample_t );
    const size_t block_size = ( nframes * sizeof( sample_t ) ) - offset_size;

    for ( int i = 0; i < channels(); i++ )
    {
        /* read the entire input buffer */
        void *buf = track()->input[ i ].buffer( nframes );

        if ( engine->freewheeling() )
        {
            while ( _thread.running() && jack_ringbuffer_write_space( _rb[i] ) < block_size )
                usleep( 10 * 1000 );

            if ( ! _thread.running() )
                return 0;

            jack_ringbuffer_write( _rb[ i ], ((char*)buf) + offset_size, block_size );
        }
        else
        {
            if ( ! _thread.running() )
                return 0;

            if ( jack_ringbuffer_write_space( _rb[i] ) < block_size )
            {
                memset( buf, 0, block_size );
                /* FIXME: we need to resync somehow */
                WARNING( "xrun" );
                ++_xruns;
            }

            jack_ringbuffer_write( _rb[ i ], ((char*)buf) + offset_size, block_size );

//            DMESSAGE( "wrote %lu", (unsigned long) nframes );

        }
    }

    block_processed();

    /* FIXME: bogus */
    return nframes;
}
