
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


/*
   peakfile reading/writing.

   Here's how it works:

   Peakfiles are of the form 'name-[chunksize].peak', and contain
   floating point min/max pairs for an entire soundfile at a certain
   chunksize.

   It gets a little complicated because peakfiles are generated
   asynchronously and streamed to disk when capturing.

   When the GUI requests the peaks for a range of samples at a certain
   chunksize, a search is performed for a peakfile--starting with the
   requested chunksize, and then moving to smaller chunksizes--that
   exists and contains the peaks for the requested range (which it may
   not--if the peaks are in the process of being written). The peaks,
   if available, are then read from disk, downsampled to the exact
   chunksize requested, and displayed.

   On the generation end, peaks are either streamed to disk while
   recording (and therefore more or less in real time), or a process
   is forked to generate the entire peak file (either directly from
   the source, or from a higher-resolution peak file that exists and
   is complete).

   The end result is that peaks are:

  * read synchronously
  * generated asynchronously
  * cached at many resolutions

 */


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

#include "Audio_File.H"

#include "assert.h"

#include <math.h>

#include <FL/Fl.H> // for Fl::check();

#include "debug.h"

Peaks::peakbuffer Peaks::_peakbuf;

/* chunksizes at which to generate peakfiles (on demand). This should
 pretty much cover the usable range.  Better performance can be
 achieved at high zoom-levels and for compressed sources with a
 minimum of 64, but those files are up into the megabytes. */
const int Peaks::cache_minimum = 256;                           /* minimum chunksize to build peakfiles for */
const int Peaks::cache_levels  = 8;                             /* number of sampling levels in peak cache */
// const int Peaks::cache_step    = 2;                             /* powers of two between each level. 4 == 256, 2048, 16384, ... */

const int Peaks::cache_step    = 1;                             /* powers of two between each level. 4 == 256, 2048, 16384, ... */

static
const char *
peakname ( const char *filename, nframes_t chunksize )
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

static nframes_t
nearest_cached_chunksize ( nframes_t chunksize )
{
    nframes_t r = nearest_power_of_two( chunksize );

    for ( int i = Peaks::cache_levels; i--; r >>= Peaks::cache_step )
        if ( chunksize >= r )
            return r;

    return 0;
}

class Peakfile
{

    FILE *_fp;
    nframes_t _chunksize;
    int _channels;
    const char *_name;

public:

    Peakfile ( )
        {
            _fp = NULL;
            _chunksize = 0;
            _channels = 0;
            _name =NULL;
        }

    ~Peakfile ( )
        {
            if ( _fp )
                close();
        }

    /** convert frame number of peak number */
    nframes_t frame_to_peak ( nframes_t frame )
        {
            /* how many powers of two is chunksize > cache_minimum?
             skip clip size of peaks at increasing powers of two until
             we reach the closest one less than chunksize. Then
             address normally.*/

            return frame * _channels / _chunksize;
        }

    /** return the number of peaks in already open peakfile /fp/ */
    nframes_t
    npeaks ( void ) const
        {
            struct stat st;

            fstat( fileno( _fp ), &st );

            return st.st_size / sizeof( Peak );
        }

    /** returns true if the peakfile contains /npeaks/ peaks starting at sample /s/ */
    bool
    contains ( nframes_t start, nframes_t npeaks )
        {
            return frame_to_peak( start ) + npeaks <= this->npeaks();
        }

    /** given soundfile name /name/, try to open the best peakfile for /chunksize/ */
    bool
    open ( const char *name, nframes_t chunksize, int channels )
        {
            _channels = channels;
            _name = name;

            for ( _chunksize = nearest_cached_chunksize( chunksize );
                  _chunksize >= Peaks::cache_minimum; _chunksize >>= Peaks::cache_step )
                if ( ( _fp = fopen( peakname( name, _chunksize ), "r" ) ) )
                    break;

            return _fp != NULL;
        }

    bool
    open ( FILE *fp, int channels, nframes_t chunksize )
        {
            _fp = fp;
            _chunksize = chunksize;
            _channels = channels;
        }

    void
    leave_open ( void )
        {
            _fp = NULL;
        }

    void
    close ( void )
        {
            fclose( _fp );
            _fp = NULL;
        }

    /** read /npeaks/ peaks at /chunksize/ starting at sample /s/
     * assuming the peakfile contains data for /channels/
     * channels. Place the result in buffer /peaks/, which must be
     * large enough to fit the entire request. Returns the number of
     * peaks actually read, which may be fewer than were requested. */
    int
    read_peaks ( Peak *peaks, nframes_t s, int npeaks, nframes_t chunksize )
        {
            if ( ! _fp )
                return 0;

            const unsigned int ratio = chunksize / _chunksize;

            /* locate to start position */
            if ( fseek( _fp, frame_to_peak( s ) * sizeof( Peak ), SEEK_SET ) )
                /* failed to seek... peaks not ready? */
                return 0;

            if ( ratio == 1 )
            {
                int len = fread( peaks, sizeof( Peak ) * _channels, npeaks, _fp );
                // close;
                return len;
            }

            Peak *pbuf = new Peak[ ratio * _channels ];

            size_t len = 0;

            int i;
            for ( i = 0; i < npeaks; ++i )
            {
                /* read in a buffer */
                len = fread( pbuf, sizeof( Peak ) * _channels, ratio, _fp );

                Peak *pk = peaks + (i * _channels);

                /* get the peak for each channel */
                for ( int j = 0; j < _channels; ++j )
                {
                    Peak *p = &pk[ j ];

                    p->min = 0;
                    p->max = 0;

                    const Peak *pb = pbuf + j;

                    for ( int k = len; k--; pb += _channels )
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

//          close();

            return i;
        }

};


int
Peaks::read_peakfile_peaks ( Peak *peaks, nframes_t s, int npeaks, nframes_t chunksize ) const
{

    nframes_t ncc = nearest_cached_chunksize( chunksize );

    if ( ! current( cache_minimum ) )
        /* Build peaks asyncronously */
        if ( ! fork() )
            exit( make_peaks( ) );
        else
            return 0;

    Peakfile _peakfile;

    if ( ! _peakfile.open( _clip->name(), chunksize, _clip->channels() ) )
        return 0;
    else if ( ! _peakfile.contains( s, npeaks ) )
    {
        /* the best peakfile for this chunksize doesn't have the
         * peaks we need. Perhaps it's still being constructed,
         * try the next best, then give up. */
        if ( ! _peakfile.open( _clip->name(), chunksize >> 1, _clip->channels() ) )
            return 0;
    }

    return _peakfile.read_peaks( peaks, s, npeaks, chunksize );

    // _peakfile.close();

}


int
Peaks::read_source_peaks ( Peak *peaks, int npeaks, nframes_t chunksize ) const
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

            for ( nframes_t k = j; k < len * channels; k += channels )
            {
                if ( fbuf[ k ] > p.max )
                    p.max = fbuf[ k ];
                if ( fbuf[ k ] < p.min )
                    p.min = fbuf[ k ];
            }

        }

        if ( len < (nframes_t)chunksize )
            break;
    }

    delete[] fbuf;

    return i;
}

int
Peaks::read_source_peaks ( Peak *peaks, nframes_t s, int npeaks, nframes_t chunksize ) const
{
//    _clip->open();
    _clip->seek( s );

    int i = read_source_peaks( peaks, npeaks, chunksize );

//    _clip->close();

    return i;
}

int
Peaks::read_peaks ( nframes_t s, nframes_t e, int npeaks, nframes_t chunksize ) const
{
//    printf( "reading peaks %d @ %d\n", npeaks, chunksize );

    if ( _peakbuf.size < (nframes_t)( npeaks * _clip->channels() ) )
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

    return _peakbuf.len;
}

/* FIXME: what purpose does this serve now? */
bool
Peaks::open ( void )
{

/*     const char *filename = _clip->name(); */

/*     /\* FIXME: determine this based on zoom level *\/ */
/*     const nframes_t chunksize = 256; */

/* /\*     if ( ! current( chunksize ) ) *\/ */
/* /\*         /\\* Build peaks asyncronously *\\/ *\/ */
/* /\*         if ( ! fork() ) *\/ */
/* /\*             exit( make_peaks( chunksize ) ); *\/ */

/*     return true; */

}


/** returns false if peak file for /filename/ is out of date  */
bool
Peaks::current ( nframes_t chunksize ) const
{
    int sfd, pfd;

    if ( ( sfd = ::open( _clip->name(), O_RDONLY ) ) < 0 )
        return true;

    if ( ( pfd = ::open( peakname( _clip->name(), chunksize ), O_RDONLY ) ) < 0 )
        return false;

    struct stat sst, pst;

    fstat( sfd, &sst );
    fstat( pfd, &pst );

    close( sfd );
    close( pfd );

    return sst.st_mtime <= pst.st_mtime;
}


/** build peaks file at /chunksize/. If higher-resolution peaks
    already exist, downsample those rather than building from
    scratch */
bool
Peaks::make_peaks ( void ) const
{
    const char *filename = _clip->name();

    _clip->seek( 0 );

    FILE *fp[ cache_levels ];

    Peak buf[ _clip->channels() ];

    if ( ! ( fp[ 0 ]  = fopen( peakname( filename, cache_minimum ), "w+" ) ) )
        return false;

    DMESSAGE( "building level 1 peak cache" );

    /* build first level from source */
    size_t len;
    do {
        len = read_source_peaks( buf, 1, cache_minimum );

        fwrite( buf, sizeof( buf ), len, fp[ 0 ] );
    }
    while ( len );

    /* now build the remaining peak levels, each based on the
     * preceding level */

    nframes_t cs = cache_minimum << cache_step;
    for ( int i = 1; i < cache_levels; ++i, cs <<= cache_step )
    {
        DMESSAGE( "building level %d peak cache", i + 1 );

        Peakfile pf;

        if ( ! ( fp[ i ] = fopen( peakname( filename, cs ), "w+" ) ) )
        {
            DWARNING( "could not open peakfile for writing" );
            return false;
        }

        /* open the peakfile for the previous cache level */
        pf.open( fp[ i - 1 ], _clip->channels(), cs >> cache_step );

        size_t len;
        nframes_t s = 0;
        do {
            len = pf.read_peaks( buf, s, 1, cs );
            s += cs;

            fwrite( buf, sizeof( buf ), len, fp[ i ] );
        }
        while ( len );

        pf.leave_open();
    }

    /* all done */
    for ( int i = cache_levels; i--; )
        fclose( fp[ i ] );

    return true;
}


/** return normalization factor for a single peak, assuming the peak
 * represents a downsampling of the entire range to be normalized. */
float
Peak::normalization_factor( void ) const
{
    float s;

    s = 1.0f / fabs( this->max );

    if ( s * this->min < -1.0 )
        s = 1.0f / fabs( this->min );

    return s;
}

Peak_Writer::Peak_Writer ( const char *filename, nframes_t chunksize, int channels )
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
