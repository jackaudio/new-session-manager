
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

#include "Audio_File_SF.H"
// #include "Timeline.H"

#include <sndfile.h>

#include <stdlib.h>
#include <string.h>

#include <assert.h>

Audio_File_SF *
Audio_File_SF::from_file ( const char *filename )
{
    SNDFILE *in;
    SF_INFO si;

    Audio_File_SF *c = NULL;

    memset( &si, 0, sizeof( si ) );

    if ( ! ( in = sf_open( filename, SFM_READ, &si ) ) )
    {
        printf( "couldn't open file\n" );
        return NULL;
    }

/*     if ( si.samplerate != timeline->sample_rate() ) */
/*     { */
/*         printf( "error: samplerate mismatch!\n" ); */
/*         goto invalid; */
/*     } */

    c = new Audio_File_SF;

    c->_filename = strdup( filename );
    c->_length = si.frames;
    c->_channels = si.channels;

    c->_in = in;
//    sf_close( in );

    return c;

invalid:

    sf_close( in );
    return NULL;
}

bool
Audio_File_SF::open ( void )
{

/*     SF_INFO si; */

/*     memset( &si, 0, sizeof( si ) ); */

/*     if ( ! ( _in = sf_open( _filename, SFM_READ, &si ) ) ) */
/*         return false; */

    seek( 0 );
    return true;
}

void
Audio_File_SF::close ( void )
{
//    sf_close( _in );
}

void
Audio_File_SF::seek ( nframes_t offset )
{
    sf_seek( _in, offset, SEEK_SET );
}

/* if channels is -1, then all channels are read into buffer
 (interleaved).  buf should be big enough to hold them all */
nframes_t
Audio_File_SF::read ( sample_t *buf, int channel, nframes_t len )
{
    if ( _channels == 1 || channel == -1 )
        return sf_readf_float( _in, buf, len );
    else
    {
        if ( len > 256 * 100 )
            printf( "warning: attempt to read an insane number of frames (%lu) from soundfile\n", len );

        printf( "len = %lu, channels = %d\n", len, _channels );

        sample_t *tmp = new sample_t[ len * _channels ];

        nframes_t rlen = sf_readf_float( _in, tmp, len );

        /* extract the requested channel */
        for ( int i = channel; i < rlen; i += _channels )
            *(buf++) = tmp[ i ];

        delete tmp;

        return rlen;
    }
}

/** read samples from /start/ to /end/ into /buf/ */
nframes_t
Audio_File_SF::read ( sample_t *buf, int channel, nframes_t start, nframes_t end )
{
    assert( end > start );

    open();

    seek( start );

    nframes_t len = read( buf, channel, end - start );

    close();

    return len;
}
