
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

#pragma once

#include <stdlib.h>

#include "types.h"

#include "Peak.H"

#include <stdio.h>

#include "Thread.H"


class Audio_File;
class Peakfile;

class Peaks
{
    mutable volatile bool _pending;

    mutable Thread _make_peaks_thread;
    mutable Thread _make_peaks_mipmap_thread;

    static void * make_peaks_mipmap ( void *v );
    static void * make_peaks ( void *v );

    struct peakdata {

        nframes_t chunksize;       /* should always be a power of 2 */
        Peak data[];

    };

    struct peakbuffer {

        size_t size;                       /* total allocation size */
        size_t len;                        /* number of peaks  */
        nframes_t offset;                  /* starting sample */

        peakdata *buf;

        peakbuffer ( )
            {
                size = len = 0;
            }
    };
    
    Peakfile *_peakfile;

    class Streamer
    {
        FILE *_fp;
        Peak *_peak;
        int _chunksize;
        int _channels;
        int _index;

        /* not permitted */
        Streamer ( const Streamer &rhs );
        const Streamer &operator= ( const Streamer &rhs );

    public:

        Streamer ( const char *filename, int channels, nframes_t chunksize );
        ~Streamer ( );

        void write ( const sample_t *buf, nframes_t nframes );

    };

    class Builder
    {
        FILE *fp;
        off_t last_block_pos;
        const Peaks *_peaks;

        void write_block_header ( nframes_t chunksize );

    public:

        bool make_peaks_mipmap ( void );
        bool make_peaks ( void );

        Builder ( const Peaks *peaks );
    };

    /* FIXME: Is this ever accessed by multiple threads? */
    static peakbuffer _peakbuf;

    Audio_File *_clip;

    mutable float _fpp;

    nframes_t read_peaks ( nframes_t s, nframes_t npeaks, nframes_t chunksize ) const;
    nframes_t read_source_peaks ( Peak *peaks, nframes_t s, nframes_t npeaks, nframes_t chunksize ) const;
    nframes_t read_source_peaks ( Peak *peaks, nframes_t npeaks, nframes_t chunksize ) const;
    nframes_t read_peakfile_peaks ( Peak *peaks, nframes_t s, nframes_t npeaks, nframes_t chunksize ) const;

    Streamer * volatile _peak_writer; /* exists when streaming peaks to disk */

    /* not permitted */
    Peaks ( const Peaks &rhs );
    const Peaks &operator= ( const Peaks &rhs );

    bool current ( void ) const;

public:

    static bool mipmapped_peakfiles;

    static const int cache_minimum;
    static const int cache_levels;
    static const int cache_step;

    Peaks ( Audio_File *c );
    ~Peaks ( );

    Peak *peakbuf ( void ) const { return Peaks::_peakbuf.buf->data; }
    void clip ( Audio_File *c ) { _clip = c; }

    int fill_buffer ( float fpp, nframes_t s, nframes_t e ) const;

    bool peakfile_ready ( void ) const;

    void read ( int X, float *hi, float *lo ) const;
    bool ready ( nframes_t s, nframes_t npeaks, nframes_t chunksize ) const;

    bool make_peaks ( void ) const;
    bool make_peaks_mipmap ( void ) const;    
    void make_peaks_asynchronously ( void(*callback)(void*), void *userdata ) const;

    void prepare_for_writing ( void );
    void finish_writing ( void );
    void write ( sample_t *buf, nframes_t nframes );

};
