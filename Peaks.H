
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

struct Peak {
    float min;
    float max;
};


class Peaks
{

    struct peaks {
        int chunksize;                                               /* should always be a power of 2 */
        Peak data[];
    };

    peaks *_peaks;

    size_t _len;

public:

    Peaks ( void )
        {
            _peaks = new peaks;

            _peaks->chunksize = 0;
//            _peaks->data = NULL;
            _len = 0;
        }

    void downsample ( int s, int e, float *mhi, float *mlo ) const;
    void read ( int X, float *hi, float *lo ) const;
    bool open ( const char *filename );

    bool make_peaks ( const char *filename, int chunksize );

    Peak & operator[] ( int X ) const;

};
