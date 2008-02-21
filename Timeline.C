
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


#include "Timeline.H"
#include "Tempo_Track.H"



float
Timeline::beats_per_minute ( nframes_t when ) const
{
    return tempo_track->beats_per_minute( when );
}

void
Timeline::beats_per_minute ( nframes_t when, float bpm )
{
    tempo_track->add( new Tempo_Point( when, bpm ) );
}

/* draw appropriate measure lines inside the given bounding box */
void
Timeline::draw_measure_lines ( int X, int Y, int W, int H, Fl_Color color )
{
    fl_line_style( FL_DASH, 2 );
    fl_color( fl_color_average( FL_BLACK, color, 0.65f ) );

//            int measure = ts_to_x( sample_rate * 60 / beats_per_minute() );

    int measure;

    for ( int x = X; x < X + W; ++x )
    {
        measure = ts_to_x( (double)(sample_rate * 60) / beats_per_minute( x_to_ts( x ) + xoffset ));
        if ( 0 == (ts_to_x( xoffset ) + x) % measure )
            fl_line( x, Y, x, Y + H );

    }
    fl_line_style( FL_SOLID, 0 );
}
