
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

    j = start;
    for ( int x = X; x <= X + W; ++x, ++j )
    {
        Peak p = (*pk)[ j ];

        int mid = Y + (H / 2);


        p.max *= _scale;
        p.min *= _scale;

        // FIXME: cache this stuff.
        fl_color( fl_color_average( FL_RED, color, fabs( p.max - p.min ) - 1.0 ) );

        if ( p.min < -1.0 || p.max > 1.0 )
            fl_color( FL_RED );

        fl_line( x, mid + (H / 2 * p.min), x, mid + (H / 2 * p.max) );

    }

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

    fl_pop_clip();
}
