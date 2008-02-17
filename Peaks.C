
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

#include "Clip.H"

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


static
int
sf_read_peaks ( SNDFILE *in, Peak *peaks, int npeaks, int chunksize )
{
    float *fbuf = new float[ chunksize ];

    size_t len;

    int i;
    for ( i = 0; i < npeaks; ++i )
    {
        /* read in a buffer */
        len = sf_read_float( in, fbuf, chunksize );

        float hi = -1.0;
        float lo = 1.0;

        for ( int j = len; j--; )
        {
            if ( fbuf[j] > hi )
                hi = fbuf[j];
            if ( fbuf[j] < lo )
                lo = fbuf[j];
        }

        peaks[ i ].max = hi;
        peaks[ i ].min = lo;

        if ( len < chunksize )
            break;
    }

    delete fbuf;

    return i;
}

void
Peaks::read_peaks ( int s, int e, float *mhi, float *mlo ) const
{
    static Peak * peaks_read = NULL;
    static nframes_t peaks_read_offset = 0;
    const int buffer_size = BUFSIZ;

    if ( ! peaks_read )
        peaks_read = new Peak[ buffer_size ];
    else
    {
        if ( s >= peaks_read_offset &&
             e - peaks_read_offset < buffer_size )
            goto done;

        if ( e > peaks_read_offset + buffer_size )
        {
            printf( "hit buffer boundardy!\n" );
            memmove( peaks_read, &peaks_read[ (s - peaks_read_offset) ], (buffer_size - (s - peaks_read_offset)) * sizeof( Peak ) );
            peaks_read_offset = s;
            goto done;
        }
    }

    /* this could be faster, but who cares. Don't zoom in so far! */

    {
        SNDFILE *in;
        SF_INFO si;

        memset( &si, 0, sizeof( si ) );

        in = sf_open( _clip->name(), SFM_READ, &si );

        sf_seek( in, s, SEEK_SET );
        peaks_read_offset = s;

        int chunksize = e - s;

        printf( "read %d peaks\n",  sf_read_peaks( in, peaks_read, buffer_size, chunksize ) );

        sf_close( in );
    }

done:

    // FIXME: should downsample here?

    Peak p = peaks_read[ s - peaks_read_offset ];

    *mhi = p.max;
    *mlo = p.min;
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
        int start = timeline.x_to_ts( X );
        int end   = timeline.x_to_ts( X + 1 );

        read_peaks( start, end, &p.max, &p.min );
    }
    else
    {
        int start = timeline.x_to_ts( X ) / _peaks->chunksize;
        int end   = timeline.x_to_ts( X + 1 ) / _peaks->chunksize;

        downsample( start, end, &p.max, &p.min );
    }

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
    if ( si.samplerate != timeline.sample_rate )
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

    delete fbuf;
}
