
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

#include "Peaks.H"

// #include "Timeline.H"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sndfile.h>

#include "Audio_File.H"

#include "assert.h"

#include <math.h>

Peaks::peakbuffer Peaks::_peakbuf;


/** Prepare a buffer of peaks from /s/ to /e/ for reading. Must be
 * called before any calls to operator[] */
int
Peaks::fill_buffer ( float fpp, int s, int e ) const
{
    _fpp = fpp;

    printf( "fill_buffer\n" );

    /* FIXME: repair this */
//    if ( fpp < _peaks->chunksize )
    {
        /* looks like we're going to have to switch to a higher resolution peak file
         or read directly from the source */
        read_peaks( s, e, (e - s) / fpp, fpp );

        /* FIXME: are we *SURE* we got them all? */
        return (e - s) / fpp;
    }

/*     else */
/*     { */
/*         /\* we'll just downsample on the fly in this case--no need for extra copying into */
/*          the buffer *\/ */
/*     } */

}


void
Peaks::downsample ( Peak *peaks, int s, int e, float *mhi, float *mlo ) const
{
    *mhi = 0;
    *mlo = 0;

    if ( e > _len )
        e = _len;

    for ( int j = s; j < e; j++ )
    {
        const float lo = peaks[ j ].min;
        const float hi = peaks[ j ].max;

        if ( hi > *mhi )
            *mhi = hi;
        if ( lo < *mlo )
            *mlo = lo;
    }
}


int
Peaks::clip_read_peaks ( Peak *peaks, int npeaks, int chunksize ) const
{
    sample_t *fbuf = new sample_t[ chunksize ];

    size_t len;

    int i;
    for ( i = 0; i < npeaks; ++i )
    {
        /* read in a buffer */
        len = _clip->read( fbuf, _channel, chunksize );

        Peak &p = peaks[ i ];
        p.min = 0;
        p.max = 0;

        for ( int j = len; j--; )
        {
            if ( fbuf[j] > p.max )
                p.max = fbuf[j];
            if ( fbuf[j] < p.min )
                p.min = fbuf[j];
        }

        if ( len < chunksize )
            break;
    }

    delete fbuf;

    return i;
}

void
Peaks::read_peaks ( int s, int e, int npeaks, int chunksize ) const
{
    printf( "reading peaks %d @ %d\n", npeaks, chunksize );

    if ( _peakbuf.size < npeaks )
    {
        _peakbuf.size = npeaks;
//        printf( "reallocating peak buffer %li\n", _peakbuf.size );
        _peakbuf.buf = (peakdata*)realloc( _peakbuf.buf, sizeof( peakdata ) + (_peakbuf.size * sizeof( Peak )) );
    }

    _clip->open();
    _clip->seek( s );

    _peakbuf.offset = s;
    _peakbuf.buf->chunksize = chunksize;
    _peakbuf.len = clip_read_peaks( _peakbuf.buf->data, npeaks, chunksize );

    _clip->close();
}

/** Return the peak for the range of samples */
Peak &
Peaks::peak ( nframes_t start, nframes_t end ) const
{
    /* Is there a better way to return this?  */
    static Peak p;

    if ( _fpp < _peaks->chunksize )
    {
        assert( _fpp == _peakbuf.buf->chunksize );

        start = (start - _peakbuf.offset) / _peakbuf.buf->chunksize;
        end = (end - _peakbuf.offset) / _peakbuf.buf->chunksize;

        if ( end > _peakbuf.len )
            end = _peakbuf.len;

//        assert( _peakbuf.len > start );

        downsample( _peakbuf.buf->data, start, end, &p.max, &p.min );
    }
    else
    {
        start /= _peaks->chunksize;
        end /= _peaks->chunksize;

        downsample( _peaks->data, start, end, &p.max, &p.min );
    }

    return p;
}

/* virtual array. Index is a Pixel value, and it returns the
 * (resampled) peaks for that pixel based on the current timeline
 * zoom. */

/* Peak & */
/* Peaks::operator[] ( int X ) const */
/* { */

/*     Peak p; */
/*     p.min = 0; */
/*     p.max = 0; */
/*     return  p; */
/* //    return peak( timeline->x_to_ts( X ), timeline->x_to_ts( X + 1 ) ); */

/* } */


const char *
Peaks::peakname ( const char *filename ) const
{
    static char file[512];

    snprintf( file, 512, "%s.peak-%d", filename, _channel );

    return (const char*)&file;
}

bool
Peaks::open ( void )
{
    const char *filename = _clip->name();

    int fd;

    make_peaks( 256 );

    if ( ( fd = ::open( peakname( filename ), O_RDONLY ) ) < 0 )
        return false;

    {
        struct stat st;

        fstat( fd, &st );

        _len = st.st_size;
    }

    _peaks = (peakdata*)mmap( NULL, _len, PROT_READ, MAP_SHARED, fd, 0 );

    ::close( fd );

    if ( _peaks == MAP_FAILED )
        printf( "failed to create mapping!\n" );

    _len = (_len - sizeof( int )) / sizeof( Peak );

    return true;
}

/** returns false if peak file for /filename/ is out of date  */
bool
Peaks::current ( void ) const
{
    int sfd, pfd;

    if ( ( sfd = ::open( _clip->name(), O_RDONLY ) ) < 0 )
        return true;

    if ( ( pfd = ::open( peakname( _clip->name() ), O_RDONLY ) ) < 0 )
         return false;

    struct stat sst, pst;

    fstat( sfd, &sst );
    fstat( pfd, &pst );

    close( sfd );
    close( pfd );

    return sst.st_mtime <= pst.st_mtime;
}


/** build peaks file for /filename/ if necessary */
bool
Peaks::make_peaks ( int chunksize )
{
    const char *filename = _clip->name();

    if ( current() )
        return true;

    if ( ! _clip->open() )
        return false;

    FILE *fp = fopen( peakname( filename ), "w" );

    if ( fp == NULL )
    {
        _clip->close();
        return false;
    }

    /* write chunksize first */
    fwrite( &chunksize, sizeof( int ), 1, fp );

    size_t len;
    do {
        Peak p;
        len = clip_read_peaks( &p, 1, chunksize );
        fwrite(  &p, sizeof( Peak ), 1, fp );
    }
    while ( len );

    _clip->close();

    fclose( fp );

    return true;
}


/** return normalization factor for range of samples from /start/ to
    /end/ (uses known peak data if possible */

float
Peaks::normalization_factor( float fpp, nframes_t start, nframes_t end ) const
{
    float s;

//    fill_buffer( fpp, start, end );

    if ( end - start < _peaks->chunksize * 4 )
        fill_buffer( _clip->length() / 4, start, end );
    else
        fill_buffer( _clip->length(), start, end );

    Peak p = peak( start, end );

    s = 1.0f / fabs( p.max );

    if ( s * p.min < -1.0 )
        s = 1.0f / fabs( p.min );

    return s;
}
