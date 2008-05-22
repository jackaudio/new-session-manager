
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
*/

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../Transport.H" // for .recording

#include "Audio_File.H"
#include "Peaks.H"

#include "assert.h"
#include "util/debug.h"
#include <errno.h>

#include <list>
#include <algorithm>
using std::min;
using std::max;


/* whether to cache peaks at multiple resolutions on disk to
 * drastically improve performance */
bool Peaks::mipmapped_peakfiles = true;

const int Peaks::cache_minimum = 256;          /* minimum chunksize to build peakfiles for */
const int Peaks::cache_levels  = 8;           /* number of sampling levels in peak cache */
const int Peaks::cache_step    = 1;            /* powers of two between each level. 4 == 256, 2048, 16384, ... */


Peaks::peakbuffer Peaks::_peakbuf;



static
const char *
peakname ( const char *filename )
{
    static char file[512];

    snprintf( file, 512, "%s.peak", filename );

    return (const char*)&file;
}

/** update the modification time of file referred to by /fd/ */
static void
touch ( int fd )
{
    struct stat st;

    fstat( fd, &st );

    fchmod( fd, st.st_mode );
}



Peaks::Peaks ( Audio_File *c )
{
    _clip = c;
    _peak_writer = NULL;
}

Peaks::~Peaks ( )
{
    if ( _peak_writer )
        delete _peak_writer;
}

/** Prepare a buffer of peaks from /s/ to /e/ for reading. Must be
 * called before any calls to operator[] */
int
Peaks::fill_buffer ( float fpp, nframes_t s, nframes_t e ) const
{
    _fpp = fpp;

    return read_peaks( s, (e - s) / fpp, fpp );
}

struct peakfile_block_header
{
    unsigned long chunksize;
    unsigned long skip;
};

class Peakfile
{

    FILE *_fp;
    nframes_t _chunksize;
    int _channels;   /* number of channels this peakfile represents */
    nframes_t _length; /* length, in frames, of the clip this peakfile represents */
    size_t _offset;
    int _blocks;

    struct block_descriptor
    {
        nframes_t chunksize;
        size_t pos;

        block_descriptor ( nframes_t chunksize, size_t pos ) : chunksize( chunksize ), pos( pos )
            {
            }

        bool operator< ( const block_descriptor &rhs )
            {
                return chunksize < rhs.chunksize;
            }
    };


public:

    Peakfile ( )
        {
            _blocks = 0;
            _fp = NULL;
            _offset = 0;
            _chunksize = 0;
            _channels = 0;
        }

    ~Peakfile ( )
        {
            if ( _fp )
                close();
        }

    int blocks ( void ) const { return _blocks; }
    /** find the best block for /chunksize/ */
    void
    scan ( nframes_t chunksize )
        {
            rewind( _fp );
            clearerr( _fp );

            std::list <block_descriptor> blocks;

            /* scan all blocks */
            for ( ;; )
            {
                peakfile_block_header bh;

                fread( &bh, sizeof( bh ), 1, _fp );

                if ( feof( _fp ) )
                    break;

//                printf( "chunksize=%lu, skip=%lu\n", (unsigned long)bh.chunksize, (unsigned long) bh.skip );

                ASSERT( bh.chunksize, "Invalid peak file structure!" );

                blocks.push_back( block_descriptor( bh.chunksize, ftell( _fp ) ) );

                if ( ! bh.skip )
                    /* last block */
                    break;

                if ( fseek( _fp, bh.skip, SEEK_CUR ) )
                {
                    WARNING( "seek failed: %s (%lu)", strerror( errno ), bh.skip );
                    break;
                }
            }

            if ( ! blocks.size() )
                FATAL( "invalid peak file?" );

//            DMESSAGE( "peakfile has %d blocks.", blocks.size() );

            blocks.sort();

            /* fall back on the smallest chunksize */
            fseek( _fp, blocks.front().pos, SEEK_SET );
            _chunksize = blocks.front().chunksize;

            /* search for the best-fit chunksize */
            for ( std::list <block_descriptor>::const_reverse_iterator i = blocks.rbegin();
                  i != blocks.rend(); ++i )
                if ( chunksize >= i->chunksize )
                {
                    _chunksize = i->chunksize;
                    fseek( _fp, i->pos, SEEK_SET );
                    break;
                }

//            DMESSAGE( "using peakfile block for chunksize %lu", _chunksize );
            _blocks = blocks.size();
            _offset = ftell( _fp );
        }

    /** convert frame number of peak number */
    nframes_t frame_to_peak ( nframes_t frame )
        {
            return frame * _channels / _chunksize;
        }

    /** return the number of peaks in already open peakfile /fp/ */
    nframes_t
    npeaks ( void ) const
        {
            struct stat st;

            fstat( fileno( _fp ), &st );

            return ( st.st_size - sizeof( peakfile_block_header ) ) / sizeof( Peak );
        }

    /** returns true if the peakfile contains /npeaks/ peaks starting at sample /s/ */
    bool
    ready ( nframes_t start, nframes_t npeaks )
        {
            if ( _blocks > 1 )
                return true;
            else
                return this->npeaks() > frame_to_peak( start ) + npeaks;
        }

    /** given soundfile name /name/, try to open the best peakfile for /chunksize/ */
    bool
    open ( const char *name, int channels, nframes_t chunksize )
        {
            _chunksize = 0;
            _channels = channels;

            if ( ! ( _fp = fopen( peakname( name ), "r" ) ) )
                return false;

            scan( chunksize );

            assert( _chunksize );

            return true;
        }

    bool
    open ( FILE *fp, int channels, nframes_t chunksize )
        {
            _fp = fp;
            _chunksize = 0;
            _channels = channels;

            scan( chunksize );

            assert( _chunksize );

            return true;
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
    nframes_t
    read_peaks ( Peak *peaks, nframes_t s, int npeaks, nframes_t chunksize )
        {
            if ( ! _fp )
                return 0;

            const unsigned int ratio = chunksize / _chunksize;

            /* locate to start position */
            if ( fseek( _fp, _offset + ( frame_to_peak( s ) * sizeof( Peak ) ), SEEK_SET ) )
                /* failed to seek... peaks not ready? */
                return 0;

            if ( ratio == 1 )
                return fread( peaks, sizeof( Peak ) * _channels, npeaks, _fp );

            Peak *pbuf = new Peak[ ratio * _channels ];

            nframes_t len = 0;

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

            return i;
        }
};

bool
Peaks::ready ( nframes_t s, int npeaks, nframes_t chunksize ) const
{
    Peakfile _peakfile;

    if ( ! _peakfile.open( _clip->name(), _clip->channels(), chunksize ) )
        return false;

    return _peakfile.ready( s, npeaks );
}

int
Peaks::read_peakfile_peaks ( Peak *peaks, nframes_t s, int npeaks, nframes_t chunksize ) const
{
    /* never try to build peaks while recording */
    if ( ! transport->recording )
    {
        if ( ! current() )
            /* Build peaks asyncronously */
            if ( ! fork() )
                exit( make_peaks() );
            else
                return 0;
    }

    Peakfile _peakfile;

    if ( ! _peakfile.open( _clip->name(),  _clip->channels(), chunksize ) )
        return 0;

    return _peakfile.read_peaks( peaks, s, npeaks, chunksize );
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
    _clip->seek( s );

    int i = read_source_peaks( peaks, npeaks, chunksize );

    return i;
}

int
Peaks::read_peaks ( nframes_t s, int npeaks, nframes_t chunksize ) const
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

    /* FIXME: use actual minimum chunksize from peakfile! */
    if ( chunksize < (nframes_t)cache_minimum )
        _peakbuf.len = read_source_peaks( _peakbuf.buf->data, s, npeaks, chunksize );
    else
        _peakbuf.len = read_peakfile_peaks( _peakbuf.buf->data, s, npeaks, chunksize );

    return _peakbuf.len;
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



bool
Peaks::make_peaks ( void ) const
{
    Peaks::Builder pb( this );

    return pb.make_peaks();
}

bool
Peaks::make_peaks_mipmap ( void ) const
{
    Peaks::Builder pb( this );

    return pb.make_peaks_mipmap();
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

/* THREAD: IO */
/* wrapper for peak writer */
void
Peaks::prepare_for_writing ( void )
{
    assert( ! _peak_writer );

    _peak_writer = new Peaks::Streamer( _clip->name(), _clip->channels(), cache_minimum );
}

/* THREAD: IO */
void
Peaks::finish_writing ( void )
{
    assert( _peak_writer );

    delete _peak_writer;
    _peak_writer = NULL;

    /* now fill in the rest of the cache */
    make_peaks_mipmap();
/*     if ( ! fork() ) */
/*         exit( make_peaks_mipmap() ); */

}

/* THREAD: IO */
void
Peaks::write ( sample_t *buf, nframes_t nframes )
{
    _peak_writer->write( buf, nframes );
}



/*
  The Streamer is for streaming peaks from audio buffers to disk while
  capturing. It works by accumulating a peak value across write()
  calls. The Streamer can only generate peaks at a single
  chunksize--additional cache levels must be appended after the
  Streamer has finished.
*/

Peaks::Streamer::Streamer ( const char *filename, int channels, nframes_t chunksize )
{
    _channels  = channels;
    _chunksize = chunksize;
    _index     = 0;
    _fp = NULL;

    _peak = new Peak[ channels ];
    memset( _peak, 0, sizeof( Peak ) * channels );

    if ( ! ( _fp = fopen( peakname( filename ), "w" ) ) )
    {
        WARNING( "could not open peakfile for streaming." );
    }

    peakfile_block_header bh;

    bh.chunksize = chunksize;
    bh.skip = 0;

    fwrite( &bh, sizeof( bh ), 1, _fp );

    fflush( _fp );
}

Peaks::Streamer::~Streamer ( )
{
/*     fwrite( _peak, sizeof( Peak ) * _channels, 1, _fp ); */

    touch( fileno( _fp ) );

    fclose( _fp );

    delete[] _peak;
}

/** append peaks for samples in /buf/ to peakfile */
void
Peaks::Streamer::write ( const sample_t *buf, nframes_t nframes )
{
    while ( nframes )
    {
        const nframes_t remaining = _chunksize - _index;

        if ( ! remaining )
        {
            fwrite( _peak, sizeof( Peak ) * _channels, 1, _fp );

            /* FIXME: shouldn't we just use write() instead? */
            fflush( _fp );

            memset( _peak, 0, sizeof( Peak ) * _channels );

            _index = 0;
        }

        int processed = min( nframes, remaining );

        for ( int i = _channels; i--; )
        {
            Peak *p = _peak + i;

            const sample_t *f = buf + i;

            for ( int j = processed; j--; f += _channels )
            {
                if ( *f > p->max )
                    p->max = *f;
                if ( *f < p->min )
                    p->min = *f;
            }
        }

        _index  += processed;
        nframes -= processed;
    }
}



/*
  The Builder is for generating peaks from imported or updated
  sources, or when the peakfile is simply missing.
*/

void
Peaks::Builder::write_block_header ( nframes_t chunksize )
{
    if ( last_block_pos )
    {
        /* update previous block */
        size_t pos = ftell( fp );

        fseek( fp, last_block_pos - sizeof( peakfile_block_header ), SEEK_SET );

        peakfile_block_header bh;

        fread( &bh, sizeof( bh ), 1, fp );

        fseek( fp, last_block_pos - sizeof( peakfile_block_header ), SEEK_SET );
//                fseek( fp, 0 - sizeof( bh ), SEEK_CUR );

//        DMESSAGE( "old block header: chunksize=%lu, skip=%lu", bh.chunksize, bh.skip );

        bh.skip = pos - last_block_pos;

        ASSERT( bh.skip, "Attempt to create empty block. pos=%lu, last_block_pos=%lu", pos, last_block_pos );

//        DMESSAGE( "new block header: chunksize=%lu, skip=%lu", bh.chunksize, bh.skip );

        fwrite( &bh, sizeof( bh ), 1, fp );

        fseek( fp, pos, SEEK_SET );
    }

    peakfile_block_header bh;

    bh.chunksize = chunksize;
    bh.skip = 0;

    fwrite( &bh, sizeof( bh ), 1, fp );

    last_block_pos = ftell( fp );

    fflush( fp );
}

/** generate additional cache levels for a peakfile with only 1 block (ie. that of a new capture) */
bool
Peaks::Builder::make_peaks_mipmap ( void )
{
    if ( ! Peaks::mipmapped_peakfiles )
        return true;

    Audio_File *_clip = _peaks->_clip;

    const char *filename = _clip->name();

    FILE *rfp;

    rfp = fopen( peakname( filename ), "r" );

    last_block_pos = sizeof( peakfile_block_header );

    /* open for reading */
//    rfp = fopen( peakname( filename ), "r" );
    /* open the file again for appending */
    if ( ! ( fp = fopen( peakname( filename ), "r+" ) ) )
    {
        WARNING( "could not open peakfile for appending." );
        return false;
    }

    if ( fseek( fp, 0, SEEK_END ) )
        FATAL( "error performing seek: %s", strerror( errno ) );

    if ( ftell( fp ) == sizeof( peakfile_block_header ) )
    {
        DWARNING( "truncated peakfile. Programming error?" );
        return false;
    }

    Peak buf[ _clip->channels() ];

    /* now build the remaining peak levels, each based on the
     * preceding level */

    nframes_t cs = Peaks::cache_minimum << Peaks::cache_step;
    for ( int i = 1; i < Peaks::cache_levels; ++i, cs <<= Peaks::cache_step )
    {
        DMESSAGE( "building level %d peak cache", i + 1 );

        DMESSAGE( "%lu", _clip->length() / cs );

        if ( _clip->length() / cs < 1 )
        {
            DMESSAGE( "source not long enough for any peaks at chunksize %lu", cs );
            break;
        }


        Peakfile pf;

        /* open the peakfile for the previous cache level */
        pf.open( rfp, _clip->channels(), cs >> Peaks::cache_step );

//        pf.open( _clip->name(), _clip->channels(), cs >> Peaks::cache_step );

        write_block_header( cs );

        size_t len;
        nframes_t s = 0;
        do {
            len = pf.read_peaks( buf, s, 1, cs );
            s += cs;

            fwrite( buf, sizeof( buf ), len, fp );
        }
        while ( len );

        pf.leave_open();
    }

    fclose( rfp );
    fclose( fp );

    return true;
}

bool
Peaks::Builder::make_peaks ( void )
{
    Audio_File *_clip = _peaks->_clip;

    const char *filename = _clip->name();

    DMESSAGE( "building peaks for \"%s\"", filename );

    if ( ! ( fp  = fopen( peakname( filename ), "w+" ) ) )
        return false;

    _clip->seek( 0 );

    Peak buf[ _clip->channels() ];

    DMESSAGE( "building level 1 peak cache" );

    write_block_header( Peaks::cache_minimum );

    /* build first level from source */
    size_t len;
    do {
        len = _peaks->read_source_peaks( buf, 1, Peaks::cache_minimum );

        fwrite( buf, sizeof( buf ), len, fp );
    }
    while ( len );

    /* reopen for reading */
    fclose( fp );

    make_peaks_mipmap();

    DMESSAGE( "done building peaks" );

    return true;
}


Peaks::Builder::Builder ( const Peaks *peaks ) : _peaks( peaks )
{
    fp = NULL;
    last_block_pos = 0;
}


