
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
#include "Audio_Track.H"
#include "Track_Header.H"
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

    _af->write( buf, nframes );

//    track()->record( buf, _frame, nframes, channels() );

//    timeline->unlock();
}

/* THREAD: IO */
void
Record_DS::disk_thread ( void )
{

    printf( "IO thread running...\n" );

    /* buffer to hold the interleaved data returned by the track reader */
    sample_t *buf = new sample_t[ _nframes * channels() ];
    sample_t *cbuf = new sample_t[ _nframes ];

    const size_t block_size = _nframes * sizeof( sample_t );

    while ( wait_for_block() )
    {
        /* pull data from the per-channel ringbuffers and interlace it */

        for ( int i = channels(); i--; )
        {
            while ( jack_ringbuffer_read_space( _rb[ i ] ) < block_size )
            {
                printf( "IO: disk buffer underrun!\n" );
                /* FIXME: is this *really* the right thing to do?  */
                usleep( 2000 );
            }

            /* FIXME: avoid this copy */

            jack_ringbuffer_read( _rb[ i ], (char*)cbuf, block_size );

            buffer_interleave_one_channel( buf, cbuf, i, channels(), _nframes );


/*             /\* deinterleave direcectly into the ringbuffer to avoid */
/*              * unnecessary copying *\/ */

/*             jack_ringbuffer_data_t rbd[2]; */

/*             jack_ringbuffer_get_write_vector( _rb[ i ], rbd ); */

/*             if ( rbd[ 0 ].len >= _nframes ) */
/*                 /\* it'll all fit in one go *\/ */
/*                 buffer_deinterleave_one_channel( (sample_t*)rbd[ 0 ].buf, buf, i, channels(), _nframes ); */
/*             else if ( rbd[ 1 ].len ) */
/*             { */
/*                 /\* there's enough space in the ringbuffer, but it's not contiguous *\/ */

/*                 /\* do the first half *\/ */
/*                 const nframes_t f = rbd[ 1 ].len / sizeof( sample_t ); */

/*                 buffer_deinterleave_one_channel( (sample_t*)rbd[ 0 ].buf, buf, i, channels(), f ); */

/*                 assert( rbd[ 1 ].len >= (_nframes - f) * sizeof( sample_t ) ); */

/*                 /\* do the second half *\/ */
/*                 buffer_deinterleave_one_channel( (sample_t*)rbd[ 1 ].buf, buf + f, i, channels(), _nframes - f ); */
/*             } */
/*             else */
/*                 printf( "programming error: expected more space in ringbuffer\n" ); */

/* /\*             buffer_deinterleave_one_channel( (sample_t*)rbd.buf, buf, i, channels(), _nframes ); *\/ */
/* /\*             jack_ringbuffer_write( _rb[ i ], (char*)cbuf, block_size ); *\/ */

/*             jack_ringbuffer_write_advance( _rb[ i ], _nframes * sizeof( sample_t ) ); */



        }

        write_block( buf, _nframes );

    }

    printf( "IO thread terminating.\n" );

    delete[] cbuf;
    delete[] buf;
}


/** begin recording */
/* FIXME: we need to make note of the exact frame we were on when recording began */
void
Record_DS::start ( nframes_t frame )
{
    /* FIXME: flush buffers here? */

    if ( _recording )
    {
        printf( "programming error: attempt to start recording while recording is still in progress\n" );
        return;
    }

    _af = Audio_File_SF::create( "testing.wav", 48000, channels(), "Wav/24" );

    _frame = frame;

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

    shutdown();

    /* FIXME: flush buffers here? */

    char *name = strdup( _af->name() );
    delete _af;
    _af = NULL;

    Audio_File *af = Audio_File::from_file( name );

    if ( ! af )
        printf( "impossible!\n" );

    new Region( af, track(), _frame );

    track()->redraw();

    _recording = false;

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
            printf( "RT: buffer overrun (disk can't keep up).\n" );
            memset( buf, 0, block_size );
            /* FIXME: we need to resync somehow */
        }
    }

    block_processed();

    /* FIXME: bogus */
    return nframes;
}
