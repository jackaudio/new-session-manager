
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

struct Peak {
    float min;
    float max;

    float normalization_factor ( void ) const;
};

class Audio_File;
class Peak_Writer;

class Peaks
{

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

    static peakbuffer _peakbuf;

    Audio_File *_clip;

    mutable float _fpp;

    int read_peaks ( nframes_t s, nframes_t e, int npeaks, nframes_t chunksize ) const;
    int read_source_peaks ( Peak *peaks, nframes_t s, int npeaks, nframes_t chunksize ) const;
    int read_source_peaks ( Peak *peaks, int npeaks, nframes_t chunksize ) const;
    int read_peakfile_peaks ( Peak *peaks, nframes_t s, int npeaks, nframes_t chunksize ) const;

    Peak_Writer * volatile _peak_writer; /* exists when streaming peaks to disk */

    size_t last_block_pos;                                      /* FIXME: only needed in make_peaks! */

    /* not permitted */
    Peaks ( const Peaks &rhs );
    const Peaks &operator= ( const Peaks &rhs );

    friend class Peak_Builder;

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

    void read ( int X, float *hi, float *lo ) const;
    bool open ( void );

    bool current ( void ) const;
    bool make_peaks ( void ) const;
    bool make_peaks_mipmap ( void ) const;

    void prepare_for_writing ( void );
    void finish_writing ( void );
    void write ( sample_t *buf, nframes_t nframes );
};

#include <stdio.h>

class Peak_Writer
{
    FILE *_fp;
    Peak *_peak;
    int _chunksize;
    int _channels;

    int _index;

    /* not permitted */
    Peak_Writer ( const Peak_Writer &rhs );
    const Peak_Writer &operator= ( const Peak_Writer &rhs );

public:

    Peak_Writer ( const char *filename, nframes_t chunksize, int channels );
    ~Peak_Writer ( );

    void write ( sample_t *buf, nframes_t nframes );

};
