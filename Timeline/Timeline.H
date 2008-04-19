
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
#include <FL/fl_draw.H>

#include "Scalebar.H"

/* FIXME: this class needs a lot of cleaning up. Too many public
 * members etc. */

/* #include "Audio_File.H" // just for nframes_t */

#include "types.h"

#include <math.h>
#include <assert.h>

class Timeline;
extern Timeline *timeline;

#include "Sequence.H"

class Tempo_Sequence;
class Time_Sequence;
class Ruler_Sequence;
class Track;

// disables double-buffering to make unnecessary redrawing more apparent
// #define DEBUG_TIMELINE_DRAWING

#ifndef DEBUG_TIMELINE_DRAWING

#include <Fl/Fl_Overlay_Window.H>

#else

#include <FL/Fl_Single_Window.H>
#define Fl_Overlay_Window Fl_Single_Window
#define redraw_overlay()
#endif



struct Rectangle
{
    int x;
    int y;
    int w;
    int h;

    Rectangle ( ) : x( 0 ), y( 0 ), w( 0 ), h( 0 ) {}
    Rectangle ( int X, int Y, int W, int H ) : x( X ), y( Y ), w( W ), h( H ) {}
};


class Engine;

#include "RWLock.H"

class Timeline : public Fl_Overlay_Window, public RWLock
{
    static void draw_clip ( void * v, int X, int Y, int W, int H );

    int _old_xposition;
    int _old_yposition;

    Rectangle _selection;

    bool _enable_measure_lines;

    enum snap_flags_e {
        SNAP_TO_REGION,
        SNAP_TO_BAR,
        SNAP_TO_BEAT
    } snap_to;

    Fl_Scroll    *scroll;
    Fl_Pack      *tracks;
    Fl_Pack      *rulers;
    Scalebar     *hscroll;
    Fl_Scrollbar *vscroll;


    static void cb_scroll ( Fl_Widget *w, void *v );
    void cb_scroll ( Fl_Widget *w );

    float _fpp;                                                  /* frames per pixel */
    nframes_t _sample_rate;
    nframes_t _length;


public:

    Tempo_Sequence *tempo_track;
    Time_Sequence  *time_track;
    Ruler_Sequence *ruler_track;


    nframes_t xoffset;

    int _yposition;

    Timeline ( int X, int Y, int W, int H, const char *L=0 );

    float fpp ( void ) const { return _fpp; }
    nframes_t length ( void ) const { return _length; }
    nframes_t sample_rate ( void ) const { return _sample_rate; }
    int ts_to_x( nframes_t ts ) const { return  ts / _fpp; }
    nframes_t x_to_ts ( int x ) const { return x * _fpp; }

    float beats_per_minute ( nframes_t when ) const;
    int beats_per_bar ( nframes_t when ) const;
    void beats_per_minute ( nframes_t when, float bpm );
    int nearest_line ( int ix );

    void draw_measure_lines ( int X, int Y, int W, int H, Fl_Color color );
    void draw_measure_BBT ( int X, int Y, int W, int H, Fl_Color color );
    void draw_measure ( int X, int Y, int W, int H, Fl_Color color, bool BBT );

    void xposition ( int X );
    void yposition ( int Y );
    void draw_playhead ( void );
    void redraw_playhead ( void );
    void draw ( void );
    void draw_overlay ( void );
    int handle ( int m );
    static void update_cb ( void *arg );

    void select( const Rectangle &r );

    void add_track ( Track *track );
    void remove_track ( Track *track );

private:

    friend class Engine; // FIXME: only Engine::process() needs to be friended.x

    bool track_name_exists ( const char *name );
    char * get_unique_track_name ( const char *name );

    /* Engine */
    nframes_t process ( nframes_t nframes );
    void seek ( nframes_t frame );
    int seek_pending ( void );
};
