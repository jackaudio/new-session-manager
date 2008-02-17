
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
#include "Timeline.H"

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sndfile.h>


void
Peaks::downsample ( int s, int e, float *mhi, float *mlo ) const
{
    *mhi = -1.0;
    *mlo = 1.0;

    if ( e > _len )
        e = _len;

    for ( int j = s; j < e; j++ )
    {
        const float lo = _peaks->data[ j ].min;
        const float hi = _peaks->data[ j ].max;

        if ( hi > *mhi )
            *mhi = hi;
        if ( lo < *mlo )
            *mlo = lo;
    }
}


void
Peaks::read ( int X, float *hi, float *lo ) const
{
    int start = X * timeline.fpp;
    int end = (X + 1) * timeline.fpp;

    downsample( start, end, hi, lo );
}


/* virtual array. Index is a Pixel value, and it returns the
 * (resampled) peaks for that pixel based on the current timeline
 * zoom. */
Peak &
Peaks::operator[] ( int X ) const
{
    /* Is there a better way to return this?  */
    static Peak p;

    if ( timeline.fpp < _peaks->chunksize )
    {
        printf( "we need to a smaller chunksize! examine the source!\n" );
    }

    int start = timeline.x_to_ts( X ) / _peaks->chunksize;
    int end   = timeline.x_to_ts( X + 1 ) / _peaks->chunksize;

/*     int start = X * timeline.fpp; */
/*     int end = (X + 1) * timeline.fpp; */

    downsample( start, end, &p.max, &p.min );

    return p;
}

static
const char *
peakname ( const char *filename )
{
    static char file[512];

    snprintf( file, 512, "%s.peak", filename );

    return (const char*)&file;
}

bool
Peaks::open ( const char *filename )
{
    int fd;

    make_peaks( filename, 256 );

    if ( ( fd = ::open( peakname( filename ), O_RDONLY ) ) < 0 )
        return false;

    {
        struct stat st;

        fstat( fd, &st );

        _len = st.st_size;
    }


    _peaks = (peaks*)mmap( NULL, _len, PROT_READ, MAP_SHARED, fd, 0 );

    ::close( fd );

    if ( _peaks == MAP_FAILED )
        printf( "failed to create mapping!\n" );

    _len = (_len - sizeof( int )) / sizeof( Peak );

    return true;
}


void
long_to_float( long *buf, int len )
{
    for ( int i = len; i--; )
        *((float*)buf) = *buf / 32768;
}

/** returns false if peak file for /filename/ is out of date  */
bool
Peaks::current ( const char *filename )
{
    int sfd, pfd;

    if ( ( sfd = ::open( filename, O_RDONLY ) ) < 0 )
        return true;

    if ( ( pfd = ::open( peakname( filename ), O_RDONLY ) ) < 0 )
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
Peaks::make_peaks ( const char *filename, int chunksize )
{
    if ( current( filename ) )
        return true;

    SNDFILE *in;
    SF_INFO si;

    memset( &si, 0, sizeof( si ) );

    in = sf_open( filename, SFM_READ, &si );

    if ( si.channels != 1 )
        abort();

    FILE *fp = fopen( peakname( filename ), "w" );

    if ( fp == NULL )
    {
        sf_close( in );
        /* return fals */
        return false;
    }

    /* write chunksize first */
    fwrite( &chunksize, sizeof( int ), 1, fp );

    float *fbuf = new float[ chunksize ];

    size_t len;
    do {
        /* read in a buffer */
        len = sf_read_float( in, fbuf, chunksize );

        Peak p;
        p.max = -1.0;
        p.min = 1.0;

        for ( int i = 0; i < len; ++i )
        {
            if ( fbuf[i] > p.max )
                p.max = fbuf[i];
            if ( fbuf[i] < p.min )
                p.min = fbuf[i];
        }

        fwrite(  &p, sizeof( Peak ), 1, fp );
    }
    while ( len == chunksize );

    fclose( fp );

    sf_close( in );

}
