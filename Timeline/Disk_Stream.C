
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
#include "Track_Header.H"
#include "Audio_Track.H"
#include "Port.H"
#include "Engine.H" // for locking.

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

Disk_Stream::Disk_Stream ( Track_Header *th, float frame_rate, nframes_t nframes, int channels ) : _th( th )
{
    _frame = 0;
    _thread = 0;
    _terminate = false;
    _pending_seek = -1;

    printf( "nframes %lu\n", nframes );

    _total_blocks = frame_rate * seconds_to_buffer / nframes;

    _nframes = nframes;

    size_t bufsize = _total_blocks * nframes * sizeof( sample_t );

/*     const int blocks = 64; */
/*     const size_t bufsize = (blocks * (nframes * sizeof( sample_t ))) + sizeof( sample_t ); */

    for ( int i = channels; i--; )
        _rb.push_back( jack_ringbuffer_create( bufsize ) );

    sem_init( &_blocks, 0, _total_blocks );

    run();
}

Disk_Stream::~Disk_Stream ( )
{

    /* stop the IO thread */
    _terminate = true;
    pthread_join( _thread, NULL );

    /* it isn't safe to do all this with the RT thread running */
    engine->lock();

    _th = NULL;

    sem_destroy( &_blocks );

    for ( int i = channels(); i--; )
        jack_ringbuffer_free( _rb[ i ] );

    engine->unlock();
}

Audio_Track *
Disk_Stream::track ( void )
{
    return (Audio_Track*)_th->track();
}

/** start Disk_Stream thread */
void
Disk_Stream::run ( void )
{
    if ( pthread_create( &_thread, NULL, &Disk_Stream::io_thread, this ) != 0 )
        /* error */;
}

/* to be called when the JACK buffer size changes. */
void
Disk_Stream::resize ( nframes_t nframes )
{
    if ( nframes != _nframes )
        /* FIXME: to something here! */;
}

bool
Disk_Stream::seek_pending ( void )
{
    return _pending_seek != (nframes_t)-1;
}

/* THREAD: RT */
/** request that the IO thread perform a seek and rebuffer.  This is
 called for each Disk_Stream whenever the RT thread determines that
 the transport has jumped to a new position. This is called *before*
 process. */
void
Disk_Stream::seek ( nframes_t frame )
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



/* void */
/* DIsk_Stream::shutdown ( void ) */
/* { */
/*     pthread_join( &_thread, NULL ); */
/* } */

/* static wrapper */
void *
Disk_Stream::io_thread ( void *arg )
{
    ((Disk_Stream*)arg)->io_thread();

    return NULL;
}

/* THREAD: IO */
/** read /nframes/ from the attached track into /buf/ */
void
Disk_Stream::read_block ( sample_t *buf, nframes_t nframes )
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

int
Disk_Stream::output_buffer_percent ( void )
{
    int n;

    sem_getvalue( &_blocks, &n );

    return 100 - (n * 100 / _total_blocks);
}

/* THREAD: IO */
void
Disk_Stream::io_thread ( void )
{

    printf( "IO thread running...\n" );

    /* buffer to hold the interleaved data returned by the track reader */
    sample_t *buf = new sample_t[ _nframes * channels() ];
    /* buffer for a single channel */
    sample_t *cbuf = new sample_t[ _nframes ];

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
            int k = 0;
            for ( unsigned int j = i; k < _nframes; j += channels() )
                cbuf[ k++ ] = buf[ j ];

            while ( jack_ringbuffer_write_space( _rb[ i ] ) < block_size )
            {
                printf( "IO: disk buffer overrun!\n" );
                /* FIXME: is this *really* the right thing to do?  */
                usleep( 2000 );
            }

            jack_ringbuffer_write( _rb[ i ], (char*)cbuf, block_size );
        }
    }

    printf( "IO thread terminating.\n" );

    delete[] buf;
    delete[] cbuf;
}

/* THREAD: RT */
/** take a single block from the ringbuffers and send it out the
 *  attached track's ports */
nframes_t
Disk_Stream::process ( nframes_t nframes )
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

/*             /\* testing. *\/ */
/*         FILE *fp = fopen( "testing.au", "a" ); */
/*         fwrite( buf, block_size, 1, fp ); */
/*         fclose( fp ); */

    }

    block_processed();

    /* FIXME: bogus */
    return nframes;
}
