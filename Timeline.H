
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
#include <FL/Fl_Widget.H>

#include "Clip.H"
#include <math.h>

#include <assert.h>

#include <FL/fl_draw.H>

class Timeline;
extern Timeline *timeline;

#include "Track.H"
// #include "Tempo_Track.H"

class Tempo_Track;
class Time_Track;

// #include "Tempo_Point.H"
// #include "Region.H"

#include <list>
using std::list;

struct Timeline : public Fl_Group
{

    int _old_position;

    enum snap_flags_e {
        SNAP_TO_REGION,
        SNAP_TO_BAR,
        SNAP_TO_BEAT
    } snap_to;

    Fl_Scroll *scroll;
    Fl_Pack *tracks;
    Fl_Pack *rulers;
    Fl_Scrollbar *scrollbar;

    Tempo_Track *tempo_track;
    Time_Track *time_track;

    float fpp;                                                  /* frames per pixel */
//    nframes_t fpp;

    nframes_t sample_rate;

    nframes_t xoffset;

    nframes_t length;

    int _beats_per_bar;
    float _beats_per_minute;

    Timeline ( int X, int Y, int W, int H, const char *L=0 );

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

    float beats_per_minute ( nframes_t when ) const;
    void beats_per_minute ( nframes_t when, float bpm );
    void draw_measure_lines ( int X, int Y, int W, int H, Fl_Color color );

    /** set scroll position */
    void
    position ( int X )
        {
            _old_position = xoffset;

            xoffset = x_to_ts( X );

            damage( FL_DAMAGE_SCROLL );

/*             rulers->damage( FL_DAMAGE_SCROLL ); */
/*             tracks->damage( FL_DAMAGE_SCROLL ); */

        }


#define FOR_CHILDREN_OF( name, ind ) \
    for ( int i = (name) ->children(); i-- && ( (ind) = (name) ->child( i ) ); )

    static void
    draw_clip ( void * v, int X, int Y, int W, int H )
        {
            Timeline *tl = (Timeline *)v;

            fl_push_clip( X, Y, W, H );

            fl_color( rand() );
            fl_rectf( X, Y, X + W, Y + H );

//            tl->tracks->redraw();

            tl->draw_child( *tl->rulers );
            tl->draw_child( *tl->tracks );


//            tl->tracks->draw();
//            tl->rulers->draw();

/*             FOR_CHILDREN_OF( tracks, o ) */
/*             { */
/*                 tracks->draw_child( *o ); */
/*                 tracks->draw_outside_label( *o ); */
/*             } */

/*             FOR_CHILDREN_OF( rulers, o ) */
/*             { */
/*                 tracks->draw_child( o ); */
/*                 tracks->draw_outside_label( o ); */
/*             } */

            fl_pop_clip();
        }


    void
    draw ( void )
        {
            int X, Y, W, H;

            X = tracks->x() + Fl::box_dx( tracks->child( 0 )->box() ) + 1;
            Y = tracks->y();
            W = tracks->w() - Fl::box_dw( tracks->child( 0 )->box() ) - 2;
            H = tracks->h();


/*             fl_color( FL_RED ); */
/*             fl_rect( X, Y, X + W, Y + H ); */

            if ( damage() & FL_DAMAGE_ALL )
            {
                draw_clip( this, X, tracks->y(), W, tracks->h() );
                draw_clip( this, X, rulers->y(), W, rulers->h() );
                return;
            }

            if ( damage() & FL_DAMAGE_SCROLL )
            {
                printf( "doing scroll\n" );
                int dx = ts_to_x( _old_position ) - ts_to_x( xoffset );

                fl_scroll( X, tracks->y(), W, tracks->h(), dx, 0, draw_clip, this );
                fl_scroll( X, rulers->y(), W, rulers->h(), dx, 0, draw_clip, this );

                _old_position = 0;
            }

            if ( damage() & FL_DAMAGE_CHILD )
            {
                Fl_Widget *o;
                FOR_CHILDREN_OF( this, o )
                    this->update_child( *o );

/*                 FOR_CHILDREN_OF( tracks, o ) */
/*                     tracks->update_child( o ); */

/*                 FOR_CHILDREN_OF( rulers, o ) */
/*                     rulers->update_child( o ); */

//                fl_pop_clip()
            }
        }

};
