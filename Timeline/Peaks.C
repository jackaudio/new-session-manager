
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

#include <FL/Fl.H> // for Fl::check();

Peaks::peakbuffer Peaks::_peakbuf;


static
const char *
peakname ( const char *filename, int chunksize )
{
    static char file[512];

    snprintf( file, 512, "%s-[%d].peak", filename, chunksize );

    return (const char*)&file;
}

/** Prepare a buffer of peaks from /s/ to /e/ for reading. Must be
 * called before any calls to operator[] */
int
Peaks::fill_buffer ( float fpp, nframes_t s, nframes_t e ) const
{
    _fpp = fpp;

    return read_peaks( s, e, (e - s) / fpp, fpp );
}

static int
nearest_power_of_two ( int v )
{
    int p = 1;
    while ( 1 << p < v )
        ++p;

    return 1 << p;
}

const int MAX_CHUNKSIZE = 4096;
const int MIN_CHUNKSIZE = 256;

int
Peaks::read_peakfile_peaks ( Peak *peaks, nframes_t s, int npeaks, int chunksize ) const
{
    FILE *fp;

    int best_fit = nearest_power_of_two( chunksize );

    int pfchunksize;

//    for ( pfchunksize = best_fit; pfchunksize < MAX_CHUNKSIZE; pfchunksize <<= 1 )
    for ( pfchunksize = best_fit; pfchunksize >= MIN_CHUNKSIZE; pfchunksize >>= 1 )
        if ( ( fp = fopen( peakname( _clip->name(), pfchunksize ), "r" ) ) )
            break;

    if ( ! fp )
    {
        printf( "failed to open peak file!\n" );

        return 0;
    }

    int channels = _clip->channels();
    const int ratio = chunksize / pfchunksize;

    /* locate to start position */
    if ( fseek( fp, (s * channels / pfchunksize) * sizeof( Peak ), SEEK_CUR ) )
        /* failed to seek... peaks not ready? */
        return 0;

    if ( ratio == 1 )
    {
        int len = fread( peaks, sizeof( Peak ) * channels, npeaks, fp );
        fclose( fp );
        return len;
    }

    Peak *pbuf = new Peak[ ratio * channels ];

    size_t len = 0;

    int i;
    for ( i = 0; i < npeaks; ++i )
    {
        /* read in a buffer */
        len = fread( pbuf, sizeof( Peak ) * channels, ratio, fp );

        Peak *pk = peaks + (i * channels);

        /* get the peak for each channel */
        for ( int j = 0; j < channels; ++j )
        {
            Peak *p = &pk[ j ];

            p->min = 0;
            p->max = 0;

            const Peak *pb = pbuf + j;

            for ( int k = len; k--; pb += channels )
            {
                if ( pb->max > p->max )
                    p->max = pb->max;
                if ( pb->min < p->min )
                    p->min = pb->min;
            }

        }

        if ( len < ratio )
            break;
    }

    delete[] pbuf;

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

    delete[] fbuf;

    return i;
}

int
Peaks::read_source_peaks ( Peak *peaks, nframes_t s, int npeaks, int chunksize ) const
{
//    _clip->open();
    _clip->seek( s );

    int i = read_source_peaks( peaks, npeaks, chunksize );

//    _clip->close();

    return i;
}

int
Peaks::read_peaks ( nframes_t s, nframes_t e, int npeaks, int chunksize ) const
{
    printf( "reading peaks %d @ %d\n", npeaks, chunksize );

    if ( _peakbuf.size < npeaks * _clip->channels() )
    {
        _peakbuf.size = npeaks * _clip->channels();
//        printf( "reallocating peak buffer %li\n", _peakbuf.size );
        _peakbuf.buf = (peakdata*)realloc( _peakbuf.buf, sizeof( peakdata ) + (_peakbuf.size * sizeof( Peak )) );
    }

    assert( s >= 0 );

    _peakbuf.offset = s;
    _peakbuf.buf->chunksize = chunksize;

    /* FIXME: compart to (minimum) peakfile chunk size */
    if ( chunksize < 256 )
        _peakbuf.len = read_source_peaks( _peakbuf.buf->data, s, npeaks, chunksize );
    else
        _peakbuf.len = read_peakfile_peaks( _peakbuf.buf->data, s, npeaks, chunksize );

    return _peakbuf.len;
}

bool
Peaks::open ( void )
{
    const char *filename = _clip->name();

    int fd;

    if ( ! current() )
        /* Build peaks asyncronously */
        if ( ! fork() )
            exit( make_peaks( 256 ) );

    /* FIXME: 256 == bogus */
    if ( ( fd = ::open( peakname( filename, 256 ), O_RDONLY ) ) < 0 )
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

    /* FIXME: 256 == bogus  */
    if ( ( pfd = ::open( peakname( _clip->name(), 256 ), O_RDONLY ) ) < 0 )
        return false;

    struct stat sst, pst;

    fstat( sfd, &sst );
    fstat( pfd, &pst );

    close( sfd );
    close( pfd );

    return sst.st_mtime <= pst.st_mtime;
}


/* FIXME: we need to work out a way to run this in another thread and
   possibly stream back the data to the GUI */
/** build peaks file for /filename/ if necessary */
bool
Peaks::make_peaks ( int chunksize )
{
    const char *filename = _clip->name();

    if ( current() )
        return true;

    _clip->seek( 0 );

    FILE *fp = fopen( peakname( filename, chunksize ), "w" );

    if ( ! fp )
        return false;

    Peak peaks[ _clip->channels() ];

    size_t len;
    do {
        len = read_source_peaks( peaks, 1, chunksize );
        fwrite( peaks, sizeof( peaks ), 1, fp );
    }
    while ( len );

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



Peak_Writer::Peak_Writer ( const char *filename, int chunksize, int channels )
{

    _channels  = channels;
    _chunksize = chunksize;
    _index     = 0;

    _peak = new Peak[ channels ];
    memset( _peak, 0, sizeof( Peak ) * channels );

    if ( ! ( _fp = fopen( peakname( filename, chunksize ), "w" ) ) )
        /* error! */;
}

Peak_Writer::~Peak_Writer ( )
{
    fclose( _fp );
    delete _peak;
}

/** append peaks for samples in /buf/ to peakfile */
void
Peak_Writer::write ( sample_t *buf, nframes_t nframes )
{
    for ( ; nframes--; ++_index, buf += _channels )
    {
        for ( int j = 0; j < _channels; ++j )
        {
            Peak *p = _peak + j;

            if ( *buf > p->max )
                p->max = *buf;
            if ( *buf < p->min )
                p->min = *buf;
        }

        if ( _index == _chunksize - 1 )
        {
            fwrite( _peak, sizeof( Peak ), _channels, _fp );
            memset( _peak, 0, sizeof( Peak ) * _channels );
            _index = 0;
        }

    }
}
