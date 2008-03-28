
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

    read_peaks( s, e, (e - s) / fpp, fpp );

    /* FIXME: are we *SURE* we got them all? */
    return (e - s) / fpp;
}


/* inline void */
/* Peaks::downsample ( Peak *peaks, int s, int e, float *mhi, float *mlo ) const */
/* { */
/*     *mhi = 0; */
/*     *mlo = 0; */

/*     if ( e > _len ) */
/*         e = _len; */

/*     for ( int j = s; j < e; j++ ) */
/*     { */
/*         const float lo = peaks[ j ].min; */
/*         const float hi = peaks[ j ].max; */

/*         if ( hi > *mhi ) */
/*             *mhi = hi; */
/*         if ( lo < *mlo ) */
/*             *mlo = lo; */
/*     } */
/* } */

int
Peaks::read_peakfile_peaks ( Peak *peaks, nframes_t s, int npeaks, int chunksize ) const
{
    FILE *fp;

    if ( ! ( fp = fopen( peakname( _clip->name() ), "r" ) ) )
    {
        printf( "failed to open peak file!" );
        return 0;
    }

    /* get chunk size of peak file */
    int pfchunksize;
    fread( &pfchunksize, sizeof( int ), 1, fp );

    int channels = _clip->channels();
    const int ratio = chunksize / pfchunksize;

    if ( ratio == 1 )
    {
        int len = fread( peaks, sizeof( Peak ) * channels, npeaks, fp );
        fclose( fp );
        return len;
    }

    Peak *pbuf = new Peak[ ratio * channels ];

    /* locate to start position */
    fseek( fp, (s * channels / pfchunksize) * sizeof( Peak ), SEEK_CUR );

    size_t len;


    int i;
    for ( i = 0; i < npeaks; ++i )
    {
        /* read in a buffer */
        len = fread( pbuf, sizeof( Peak ) * channels, ratio, fp );

        Peak *pk = peaks + (i * channels);

        /* get the peak for each channel */
        for ( int j = 0; j < channels; ++j )
        {
            Peak &p = pk[ j ];

            p.min = 0;
            p.max = 0;

            for ( int k = j; k < len * channels; k += channels )
            {
                if ( pbuf[ k ].max > p.max )
                    p.max = pbuf[ k ].max;
                if ( pbuf[ k ].min < p.min )
                    p.min = pbuf[ k ].min;
            }

        }

        if ( len < ratio )
            break;
    }

    delete pbuf;

    fclose( fp );

    return i;
}

int
Peaks::read_source_peaks ( Peak *peaks, int npeaks, int chunksize ) const
{
    int channels = _clip->channels();

    sample_t *fbuf = new sample_t[ chunksize * channels ];

    size_t len;

    int i;
    for ( i = 0; i < npeaks; ++i )
    {
        /* read in a buffer */
        len = _clip->read( fbuf, -1, chunksize );

        Peak *pk = peaks + (i * channels);

        /* get the peak for each channel */
        for ( int j = 0; j < channels; ++j )
        {
            Peak &p = pk[ j ];

            p.min = 0;
            p.max = 0;

            for ( int k = j; k < len * channels; k += channels )
            {
                if ( fbuf[ k ] > p.max )
                    p.max = fbuf[ k ];
                if ( fbuf[ k ] < p.min )
                    p.min = fbuf[ k ];
            }

        }

        if ( len < chunksize )
            break;
    }

    delete fbuf;

    return i;
}

int
Peaks::read_source_peaks ( Peak *peaks, nframes_t s, int npeaks, int chunksize ) const
{
    _clip->open();
    _clip->seek( s );

    int i = read_source_peaks( peaks, npeaks, chunksize );

    _clip->close();

    return i;
}

void
Peaks::read_peaks ( int s, int e, int npeaks, int chunksize ) const
{
    printf( "reading peaks %d @ %d\n", npeaks, chunksize );

    if ( _peakbuf.size < npeaks * _clip->channels() )
    {
        _peakbuf.size = npeaks * _clip->channels();
//        printf( "reallocating peak buffer %li\n", _peakbuf.size );
        _peakbuf.buf = (peakdata*)realloc( _peakbuf.buf, sizeof( peakdata ) + (_peakbuf.size * sizeof( Peak )) );
    }

    _peakbuf.offset = s;
    _peakbuf.buf->chunksize = chunksize;

    /* FIXME: compart to (minimum) peakfile chunk size */
    if ( chunksize < 256 )
        _peakbuf.len = read_source_peaks( _peakbuf.buf->data, s, npeaks, chunksize );
    else
        _peakbuf.len = read_peakfile_peaks( _peakbuf.buf->data, s, npeaks, chunksize );

}

/** Return the peak for the range of samples */
/* Peak & */
/* Peaks::peak ( nframes_t start, nframes_t end ) const */
/* { */
/*     /\* Is there a better way to return this?  *\/ */
/*     static Peak p; */

/*     start = (start - _peakbuf.offset) / _peakbuf.buf->chunksize; */
/*     end = (end - _peakbuf.offset) / _peakbuf.buf->chunksize; */

/*     if ( end > _peakbuf.len ) */
/*         end = _peakbuf.len; */

/*     downsample( _peakbuf.buf->data, start, end, &p.max, &p.min ); */

/*     return p; */
/* } */

const char *
Peaks::peakname ( const char *filename ) const
{
    static char file[512];

    snprintf( file, 512, "%s.peak", filename );

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

    ::close( fd );

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

    Peak peaks[ _clip->channels() ];

    size_t len;
    do {
        len = read_source_peaks( peaks, 1, chunksize );
        fwrite( peaks, sizeof( peaks ), 1, fp );
    }
    while ( len );

    _clip->close();

    fclose( fp );

    return true;
}


/** return normalization factor for range of samples from /start/ to
    /end/ (uses known peak data if possible */

/* float */
/* Peaks::normalization_factor( float fpp, nframes_t start, nframes_t end ) const */
/* { */
/*     float s; */

/* //    fill_buffer( fpp, start, end ); */

/* /\*     if ( end - start < _peaks->chunksize * 4 ) *\/ */
/* /\*         fill_buffer( _clip->length() / 4, start, end ); *\/ */
/* /\*     else *\/ */
/* /\*         fill_buffer( _clip->length(), start, end ); *\/ */

/*     Peak p = peak( start, end ); */

/*     s = 1.0f / fabs( p.max ); */

/*     if ( s * p.min < -1.0 ) */
/*         s = 1.0f / fabs( p.min ); */

/*     return s; */
/* } */
