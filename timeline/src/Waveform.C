
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

/* routines to draw a waveform from peak data */

#include <FL/fl_draw.H>

#include "Waveform.H"

#include <math.h>
#include <algorithm>
using std::min;
using std::max;



bool Waveform::fill = true;
bool Waveform::outline = true;
bool Waveform::vary_color = true;
bool Waveform::logarithmic = true;



/* TODO: split the variations into separate functions. eg, plain,
 * outlined, filled, polygonal, rectified. */

void
Waveform::scale ( Peak *pbuf, int npeaks, float gain )
{
    /* scale it */
    for ( int j = npeaks; j--; )
    {
        pbuf[ j ].min *= gain;
        pbuf[ j ].max *= gain;
    }
}

/** draw a portion of /clip/'s waveform. coordinates are the portion to draw  */
void
Waveform::draw ( int X, int Y, int W, int H,
                 const Peak *pbuf, int peaks, int skip,
                 Fl_Color color )
{
    int j;

    int start = 0;

    const int halfheight = H / 2;
    const int mid = Y + halfheight;

    W = min( peaks, W );

    if ( Waveform::fill )
    {
        j = start;
        for ( int x = X; x < X + W; ++x, j += skip )
        {
            const Peak p = pbuf[ j ];

            const float diff = fabs( p.max - p.min );

            if ( diff > 2.0f )
                fl_color( FL_RED );
            else
                if ( Waveform::vary_color )
                    fl_color( fl_color_average( FL_WHITE, color, diff * 0.5f ) );
                else
                    fl_color( color );

            const int ty = mid - ( halfheight * p.min );
            const int by = mid - ( halfheight * p.max );
            fl_line( x, ty, x, by );
        }
    }

    const int ty = Y + halfheight;

    if ( Waveform::outline )
    {
        fl_color( fl_darker( fl_darker( color ) ) );

        fl_line_style( FL_SOLID | FL_CAP_FLAT, 2 );

        fl_begin_line();

        j = start;

        for ( int x = X; x < X + W; ++x, j += skip )
            fl_vertex( x, ty - ( halfheight * pbuf[ j ].min ) );

        fl_end_line();

        fl_begin_line();

        j = start;

        for ( int x = X; x < X + W; ++x, j += skip )
            fl_vertex( x, ty - ( halfheight * pbuf[ j ].max ) );

        fl_end_line();

        fl_line_style( FL_SOLID, 0 );
    }
}
