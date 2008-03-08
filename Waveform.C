
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

#include <stdio.h>

#include <FL/Enumerations.H>
#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include "Timeline.H"
#include "Audio_File.H"


#include <math.h>

const bool outline = true;
const bool vary_color = true;

/** draw a portion of /clip/'s waveform. coordinates are the portion to draw  */
void
draw_waveform ( int ox, int X, int Y, int W, int H, Audio_File *_clip, int channel, float fpp, nframes_t _start, nframes_t _end, float _scale, Fl_Color color )
{
    fl_push_clip( X, Y, W, H );

    int j;

//    int start = timeline->ts_to_x( _start );
    int start = timeline->ts_to_x( _start ) + (X - ox);

    const Peaks *pk = _clip->peaks( channel );

    _start = timeline->x_to_ts( start );

    pk->fill_buffer( fpp, _start,  _start + timeline->x_to_ts( W ) );

    const int halfheight = H / 2;
    const int mid = Y + halfheight;

    fl_line_style( FL_SOLID, 1 );

    j = start;
    for ( int x = X; x <= X + W; ++x, ++j )
    {
        Peak p = (*pk)[ j ];

        p.max *= _scale;
        p.min *= _scale;

        const float diff = fabs( p.max - p.min );

        if ( vary_color )
            fl_color( fl_color_average( FL_WHITE, color, min( 1.0f, diff ) ) );
        else
            fl_color( color );

        if ( diff > 1.0f )
            fl_color( FL_RED );

        const int ty = mid + (halfheight * p.min);
        const int by = mid + (halfheight * p.max );
        fl_line( x, ty, x, by );

/*         if ( outline ) */
/*         { */
/*             fl_color( fl_darker( fl_darker( color ) ) ); */
/*             fl_line( x, ty - 2, x, ty ); */
/*             fl_line( x, by + 2, x, by ); */
/*         } */

    }

    fl_line_style( FL_SOLID, 0 );

    if ( outline )
    {

        fl_color( fl_darker( fl_darker( color ) ) );

        fl_line_style( FL_SOLID, 2 );

        fl_begin_line();

        j = start;
        for ( int x = X; x <= X + W; ++x, ++j )
        {
            Peak p = (*pk)[ j ];

            p.min *= _scale;

            fl_vertex( x, Y + (H / 2) + ((float)H / 2 *  p.min ));
        }

        fl_end_line();

        fl_begin_line();

        j = start;
        for ( int x = X; x <= X + W; ++x, ++j )
        {
            Peak p = (*pk)[ j ];

            p.max *= _scale;

            fl_vertex( x, Y + (H / 2) + ((float)H / 2 * p.max ));
        }

        fl_end_line();

        fl_line_style( FL_SOLID, 0 );

    }

    fl_pop_clip();
}
