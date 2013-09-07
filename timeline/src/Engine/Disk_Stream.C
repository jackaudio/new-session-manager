 
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

#include "../Track.H"
// #include "Audio_Sequence.H"
class Audio_Sequence;

// #include "Port.H"
#include "Engine.H" // for locking.

#include "Disk_Stream.H"
#include "dsp.h"

#include "const.h"
#include "debug.h"

#include <unistd.h>



/**********/
/* Engine */
/**********/

/* A Disk_Stream uses a separate I/O thread to stream a track's
   regions from disk into a ringbuffer to be processed by the RT
   thread (or vice-versa). The I/O thread syncronizes access with the
   user thread via the Timeline mutex. The size of the buffer (in
   seconds) must be set before any Disk_Stream objects are created;
   that is, at startup time. The default is 5 seconds, which may or
   may not be excessive depending on various external factors. */

float Disk_Stream::seconds_to_buffer = 2.0f;
/* this is really only a rough estimate. The actual amount of data
 read depends on many factors.  Overlapping regions, for example, will
 require more data to be read from disk, as will varying channel
 counts.*/
size_t Disk_Stream::disk_io_kbytes = 256;



Disk_Stream::Disk_Stream ( Track *track, float frame_rate, nframes_t nframes, int channels ) : _track( track )
{
    assert( channels );

    _frame = 0;
    _terminate = false;
    _pending_seek = false;
    _seek_frame = 0;
    _xruns = 0;
    _frame_rate = frame_rate;

    sem_init( &_blocks, 0, 0 );
        
    _resize_buffers( nframes, channels );
}

Disk_Stream::~Disk_Stream ( )
{
    /* it isn't safe to do all this with the RT thread running */

//    timeline->wrlock();

     _track = NULL;

    sem_destroy( &_blocks );

    for ( int i = channels(); i--; )
        jack_ringbuffer_free( _rb[ i ] );

//    timeline->unlock();
}



/** flush buffers and reset. Must only be called from the RT thread. */
void
Disk_Stream::base_flush ( bool is_output )
{
//    THREAD_ASSERT( RT );

    /* flush buffers */
    for ( unsigned int i = _rb.size(); i--; )
        jack_ringbuffer_reset( _rb[ i ] );

    sem_destroy( &_blocks );
    
    if ( is_output )
        sem_init( &_blocks, 0, _total_blocks );
    else
        sem_init( &_blocks, 0, 0 );
}

/** signal thread to terminate, then detach it */
void
Disk_Stream::detach ( void )
{
    _terminate = true;

    block_processed();

    _thread.detach();
}

/** stop the IO thread. */
void
Disk_Stream::shutdown ( void )
{
    if ( _thread.running() )
    {
        DMESSAGE( "Sending terminate signal to diskthread." );

        _terminate = true;
        
        /* try to wake the thread so it'll see that it's time to die */
        while ( _terminate )
        {
            block_processed();
            usleep( 10 * 1000 );
        }
        
        _thread.join();
    }
}

Track *
Disk_Stream::track ( void ) const
{
    return _track;
}

Audio_Sequence *
Disk_Stream::sequence ( void ) const
{
    return (Audio_Sequence*)_track->sequence();
}

/** start Disk_Stream thread */
void
Disk_Stream::run ( void )
{
    ASSERT( ! _thread.running(), "Thread is already running" );

    if ( ! _thread.clone( &Disk_Stream::disk_thread, this ) )
        FATAL( "Could not create IO thread!" );
}

void
Disk_Stream::_resize_buffers ( nframes_t nframes, int channels )
{
    for ( int i = _rb.size(); i--; )
        jack_ringbuffer_free( _rb[ i ] );

    _rb.clear();

    _nframes = nframes;

    _total_blocks = ( _frame_rate * seconds_to_buffer ) / nframes;

    size_t bufsize = _total_blocks * nframes * sizeof( sample_t );

    if ( disk_io_kbytes )
        _disk_io_blocks = ( bufsize * channels ) / ( disk_io_kbytes * 1024 );
    else
        _disk_io_blocks = 1;

    for ( int i = channels; i--; )
        _rb.push_back( jack_ringbuffer_create( bufsize ) );
}

/* THREAD: RT (non-RT)  */
/* to be called when the JACK buffer size changes. */
void
Disk_Stream::resize_buffers ( nframes_t nframes )
{
    if ( nframes != _nframes )
    {
        DMESSAGE( "resizing buffers" );

        const bool was_running = _thread.running();

        if ( was_running )
            shutdown();

        _resize_buffers( nframes, channels() );

        flush();

        if ( was_running )
            run();
    }
}


/* static wrapper */
void *
Disk_Stream::disk_thread ( void *arg )
{
    ((Disk_Stream*)arg)->disk_thread();

    return NULL;
}

int
Disk_Stream::buffer_percent ( void )
{
    int n;

    sem_getvalue( &_blocks, &n );

    return 100 - (n * 100 / _total_blocks);
}
