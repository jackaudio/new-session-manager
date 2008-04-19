
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

#include "Disk_Stream.H"
#include "Track.H"
#include "Audio_Sequence.H"
#include "Port.H"
#include "Engine.H" // for locking.

#include "dsp.h"

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

/* FIXME: handle termination of IO thread in destructor */
/* FIXME: deal with (jack) buffer size changes */
/* FIXME: needs error handling everywhere! */
/* TODO: handle capture too. For this to work with some kind of
 * punch-in/out system, I believe we'll have to always keep at least
 * one buffer's worth of input. We would need this anyway in order to
 * pass input through to output (software monitoring). What about
 * looped recording? */
/* TODO: latency compensation? Does this really apply to us? (we're
 * not hosting plugins here) */
/* TODO: read/write data from/to disk in larger chunks to avoid
 * excessive seeking. 256k is supposedly the sweetspot. */

float Disk_Stream::seconds_to_buffer = 5.0f;
// size_t Disk_Stream::disk_block_frames = 2048;

Disk_Stream::Disk_Stream ( Track *th, float frame_rate, nframes_t nframes, int channels ) : _th( th )
{
    _frame = 0;
    _thread = 0;
    _terminate = false;
    _pending_seek = -1;

    printf( "nframes %lu\n", nframes );

    _total_blocks = frame_rate * seconds_to_buffer / nframes;

    _nframes = nframes;

    size_t bufsize = _total_blocks * nframes * sizeof( sample_t );

    for ( int i = channels; i--; )
        _rb.push_back( jack_ringbuffer_create( bufsize ) );

    sem_init( &_blocks, 0, _total_blocks );

//    run();
}

Disk_Stream::~Disk_Stream ( )
{
    /* it isn't safe to do all this with the RT thread running */
    engine->lock();

    _th = NULL;

    sem_destroy( &_blocks );

    for ( int i = channels(); i--; )
        jack_ringbuffer_free( _rb[ i ] );

    engine->unlock();
}

/** stop the IO thread, block until it finishes. */
void
Disk_Stream::shutdown ( void )
{
    _terminate = true;
    pthread_join( _thread, NULL );
    _terminate = false;
}

Audio_Sequence *
Disk_Stream::track ( void )
{
    return (Audio_Sequence*)_th->track();
}

/** start Disk_Stream thread */
void
Disk_Stream::run ( void )
{
    if ( pthread_create( &_thread, NULL, &Disk_Stream::disk_thread, this ) != 0 )
        /* error */;
}

/* to be called when the JACK buffer size changes. */
void
Disk_Stream::resize ( nframes_t nframes )
{
    if ( nframes != _nframes )
        /* FIXME: to something here! */;
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
