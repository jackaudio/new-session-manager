
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
};

class Audio_File;

class Peaks
{

    struct peakdata {

        int chunksize;             /* should always be a power of 2 */
        Peak data[];

    };

    struct peakbuffer {

        size_t size;                       /* total allocation size */
        size_t len;                             /* number of peaks  */
        nframes_t offset;                        /* starting sample */

        peakdata *buf;

        peakbuffer ( )
            {
                size = len = 0;
            }
    };

    static peakbuffer _peakbuf;

    Audio_File *_clip;

    size_t _len;

    mutable float _fpp;

    int read_peaks ( nframes_t s, nframes_t e, int npeaks, int chunksize ) const;
    int read_source_peaks ( Peak *peaks, nframes_t s, int npeaks, int chunksize ) const;
    int read_source_peaks ( Peak *peaks, int npeaks, int chunksize ) const;
    int read_peakfile_peaks ( Peak *peaks, nframes_t s, int npeaks, int chunksize ) const;

    const char *peakname ( const char *filename ) const;


    // Peaks ( );

public:

    Peaks ( )
        {
            _len = 0;
            _clip = NULL;
        }

    Peaks ( Audio_File *c )
        {
            _len = 0;
            _clip = c;
        }

    Peak *peakbuf ( void ) const { return Peaks::_peakbuf.buf->data; }
    void clip ( Audio_File *c ) { _clip = c; }

    int fill_buffer ( float fpp, nframes_t s, nframes_t e ) const;

    void downsample ( Peak *peaks, int s, int e, float *mhi, float *mlo ) const;
    void read ( int X, float *hi, float *lo ) const;
    bool open ( void );
    float normalization_factor( float fpp, nframes_t start, nframes_t end ) const;

    bool current ( void ) const;
    bool make_peaks ( int chunksize );

    Peak & peak ( nframes_t start, nframes_t end ) const;

    Peak & operator[] ( int X ) const;

};
