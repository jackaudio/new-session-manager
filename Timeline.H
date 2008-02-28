
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

#include "Scalebar.H"

#include "Audio_File.H" // just for nframes_t
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

    static void draw_clip ( void * v, int X, int Y, int W, int H );

    int _old_xposition;
    int _old_yposition;

    enum snap_flags_e {
        SNAP_TO_REGION,
        SNAP_TO_BAR,
        SNAP_TO_BEAT
    } snap_to;

    Fl_Scroll *scroll;
    Fl_Pack *tracks;
    Fl_Pack *rulers;
    Scalebar *hscroll;
    Fl_Scrollbar *vscroll;

    Tempo_Track *tempo_track;
    Time_Track *time_track;

    float fpp;                                                  /* frames per pixel */
//    nframes_t fpp;

    nframes_t sample_rate;

    nframes_t xoffset;

    nframes_t length;

    int yposition;

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

/* #define FOR_CHILDREN_OF( name, ind )                                    \ */
/*     for ( int i = (name) ->children(); i-- && ( (ind) = (name) ->child( i ) ); ) */

    float beats_per_minute ( nframes_t when ) const;
    int beats_per_bar ( nframes_t when ) const;
    void beats_per_minute ( nframes_t when, float bpm );
    int nearest_line ( int ix );

    void draw_measure_lines ( int X, int Y, int W, int H, Fl_Color color );
    void position ( int X );
    void draw ( void );
    int handle ( int m );

};
