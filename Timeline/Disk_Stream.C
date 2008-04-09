
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

float Disk_Stream::seconds_to_buffer = 5.0f;

/* A Disk_Stream uses a separate I/O thread to stream a track's
   regions from disk into a ringbuffer, to be processed by the RT
   thread (or vice-versa). */
/* FIXME: handle termination of IO thread in destructor */
/* FIXME: could all of this not simply be included in the Track_Header
   class? */
/* FIXME: deal with (jack) buffer size changes */
/* FIXME: can this be made to actually handle capture? */
/* FIXME: needs error handling everywhere! */

Disk_Stream::Disk_Stream ( Track_Header *th, float frame_rate, nframes_t nframes, int channels ) : _th( th )
{
    _frame = 0;
    _thread = 0;

    const int blocks = frame_rate * seconds_to_buffer / nframes;

    _nframes = nframes;

    size_t bufsize = blocks * nframes * sizeof( sample_t );

    for ( int i = channels; i--; )
        _rb.push_back( jack_ringbuffer_create( bufsize ) );

    sem_init( &_blocks, 0, blocks );

    run();
}

Disk_Stream::~Disk_Stream ( )
{
    _th = NULL;

    sem_destroy( &_blocks );

    for ( int i = channels(); i--; )
        jack_ringbuffer_free( _rb[ i ] );
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
/** read a block of data from the track into /buf/ */
void
Disk_Stream::read_block ( sample_t *buf )
{

    /* stupid chicken/egg */
    if ( ! timeline )
        return;

    printf( "IO: attempting to read block @ %lu\n", _frame );

    if ( ! track() )
    {
//        _frame += _nframes;
        return;
    }

    timeline->rdlock();

    if ( track()->play( buf, _frame, _nframes, channels() ) )
        _frame += _nframes;
    else
        /* error */;

    timeline->unlock();
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

        read_block( buf );

        /* deinterleave the buffer and stuff it into the per-channel ringbuffers */

        for ( int i = channels(); i--; )
        {
            int k = 0;
            for ( unsigned int j = i; j < _nframes; j += channels() )
                cbuf[ k++ ] = buf[ j ];

            jack_ringbuffer_write( _rb[ i ], (char*)cbuf, block_size );
        }
    }

    delete[] buf;
    delete[] cbuf;
}

/* THREAD: RT */
/** take a block from the ringbuffers and send it out the track's
 * ports */
nframes_t
Disk_Stream::process ( nframes_t nframes )
{
    const size_t block_size = _nframes * sizeof( sample_t );

    for ( int i = channels(); i--;  )
    {
        sample_t *buf = (_th->output)[ i ].buffer( _nframes );

        /* FIXME: handle underrun */
        jack_ringbuffer_read( _rb[ i ], (char*)buf, block_size );
    }

    block_processed();

    /* FIXME: bogus */
    return nframes;
}
