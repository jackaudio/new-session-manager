
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
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

    int start = X * timeline.fpp;
    int end = (X + 1) * timeline.fpp;

    downsample( start, end, &p.max, &p.min );

    return p;
}


bool
Peaks::open ( const char *filename )
{
    char file[512];

    snprintf( file, 512, "%s.peak", filename );

    int fd;
    if ( ( fd = ::open( file, O_RDONLY ) ) < 0 )
    {
        /* generate peaks here */
    }

    {
        struct stat st;
        fstat( fd, &st );
        _len = st.st_size;
    }

    _peaks = (peaks*)mmap( NULL, _len, PROT_READ, MAP_SHARED, fd, 0 );

    if ( _peaks == MAP_FAILED )
        printf( "failed to create mapping! " );

    _len = (_len - sizeof( int )) / sizeof( Peak );
}
