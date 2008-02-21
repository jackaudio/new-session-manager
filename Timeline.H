
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

#include <FL/Fl_Scroll.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Scrollbar.H>

#include "Clip.H"
#include <math.h>

#include <assert.h>

#include <FL/fl_draw.H>

class Timeline;
extern Timeline timeline;

#include "Track.H"
// #include "Tempo_Point.H"
// #include "Region.H"

#include <list>
using std::list;

class timestamped
{
public:

    nframes_t timestamp;

    bool
    operator< ( const timestamped &rhs ) const { return timestamp < rhs.timestamp; }
};

struct Timeline {

    struct tempo_event : timestamped
    {
        float beats_per_minute;
    };

    struct signature_event : timestamped
    {
        int beats_per_bar;
        int beat_type;
    };

    enum snap_flags_e {
        SNAP_TO_REGION,
        SNAP_TO_BAR,
        SNAP_TO_BEAT
    } snap_to;

    list <tempo_event> tempo_points;
    list <signature_event> signature_points;

    Fl_Scroll *scroll;
    Fl_Pack *tracks;
    Fl_Scrollbar *scrollbar;

    Track *tempo_track;

    float fpp;                                                  /* frames per pixel */
//    nframes_t fpp;

    nframes_t sample_rate;

    nframes_t xoffset;

    nframes_t length;

    int _beats_per_bar;
    float _beats_per_minute;

    int
    ts_to_x( nframes_t ts )
        {
//            assert( ts / fpp > 0 );
            return  ts / fpp;
        }

    nframes_t
    x_to_ts ( int x )
        {
            return x * fpp;
        }


    float
    beats_per_minute ( void ) const
        {
            // TODO: this should check a tempo map.
            return _beats_per_minute;
        }


    float
    beats_per_minute ( nframes_t when ) const
        {
            for ( list <tempo_event>::const_reverse_iterator i = tempo_points.rbegin();
                  i != tempo_points.rend(); i++ )
            {
                if ( i->timestamp < when )
                    return i->beats_per_minute;
            }

            return _beats_per_minute;
        }

    void
    beats_per_minute ( nframes_t when, float bpm )
        {
//            tempo_track->add( new Tempo_Point( when, bpm ) );
        }

/*     /\* return the offset of the closest measure line to /t/ *\/ */
/*     nframes_t */
/*     snap ( nframes t ) */
/*         { */





/*         } */

    /* draw appropriate measure lines inside the given bounding box */
    void
    draw_measure_lines ( int X, int Y, int W, int H, Fl_Color color )
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
};
