
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

#include "Clip.H"
#include "Timeline.H"

#include <sndfile.h>

#include <stdlib.h>
#include <string.h>

Clip::Clip ( const char *filename ) : _peaks( this )
{
    _filename = filename;

    SNDFILE *in;
    SF_INFO si;

    memset( &si, 0, sizeof( si ) );

    in = sf_open( filename, SFM_READ, &si );

    if ( si.channels != 1 )
        printf( "error: incompatible format" );

    if ( si.samplerate != timeline.sample_rate )
        printf( "error: samplerate mismatch!\n" );

    _length = si.frames;

    sf_close( in );

    _peaks.open();
}


bool
Clip::open ( void )
{
    SF_INFO si;

    memset( &si, 0, sizeof( si ) );

    if ( ! ( _in = sf_open( _filename, SFM_READ, &si ) ) )
        return false;

    return true;
}

void
Clip::close ( void )
{
    sf_close( _in );
}

void
Clip::seek ( nframes_t offset )
{
    sf_seek( _in, offset, SEEK_SET );
}


nframes_t
Clip::read ( sample_t *buf, nframes_t len )
{
    return sf_read_float ( _in, buf, len );
}

/** read samples from /start/ to /end/ into /buf/ */
nframes_t
Clip::read ( sample_t *buf, nframes_t start, nframes_t end )
{
    open();

    seek( start );

    nframes_t len = read( buf, end - start );

    close();

    return len;
}
