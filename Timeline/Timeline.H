
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
class Annotation_Sequence;
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


#include "Engine.H" // for sample_rate()

// class Engine;

#include "RWLock.H"

class Timeline : public Fl_Overlay_Window, public RWLock
{
    static void draw_clip ( void * v, int X, int Y, int W, int H );

    int _old_xposition;
    int _old_yposition;

    Rectangle _selection;

    Fl_Scroll    *scroll;
    Fl_Pack      *tracks;
    Fl_Pack      *rulers;
    Scalebar     *hscroll;
    Fl_Scrollbar *vscroll;


    void adjust_vscroll ( void );
    static void cb_scroll ( Fl_Widget *w, void *v );
    void cb_scroll ( Fl_Widget *w );

    int _fpp;                                                 /* frames per pixel, power of two */
    nframes_t _length;


    nframes_t p1, p2;                                           /* cursors */

    /* not permitted */
    Timeline ( const Timeline &rhs );
    Timeline & operator = ( const Timeline &rhs );

public:

    enum snap_e {
        Bars,
        Beats,
        None
    };

    static bool draw_with_measure_lines;
    static snap_e snap_to;
    static bool snap_magnetic;
    static bool follow_playhead;
    static bool center_playhead;

    Tempo_Sequence *tempo_track;
    Time_Sequence  *time_track;
    Annotation_Sequence *ruler_track;


    nframes_t xoffset;

    int _yposition;

    Timeline ( int X, int Y, int W, int H, const char *L=0 );

    nframes_t fpp ( void ) const { return 1 << _fpp; }
    nframes_t length ( void ) const { return _length; }
    nframes_t sample_rate ( void ) const { return engine->sample_rate(); }
    int ts_to_x( nframes_t ts ) const { return  ts >> _fpp; }
    nframes_t x_to_ts ( int x ) const { return x << _fpp; }
    nframes_t x_to_offset ( int x ) const;

    float beats_per_minute ( nframes_t when ) const;
    int beats_per_bar ( nframes_t when ) const;
    void beats_per_minute ( nframes_t when, float bpm );
    void time ( nframes_t when, int bpb, int beat_type );
    bool nearest_line ( nframes_t when, nframes_t *f ) const;

    typedef void (measure_line_callback)( nframes_t frame, int X, int Y, int H, void *arg );

    void draw_measure_lines ( int X, int Y, int W, int H, Fl_Color color );
    void draw_measure_BBT ( int X, int Y, int W, int H, Fl_Color color );
    void draw_measure ( nframes_t when, int Y, int W, int H, Fl_Color color, measure_line_callback *cb, void *arg, bool BBT ) const;

    void xposition ( int X );
    void yposition ( int Y );
    void draw_cursor ( nframes_t frame, Fl_Color color );
    void draw_playhead ( void );
    void redraw_playhead ( void );
    void resize ( int X, int Y, int W, int H );

    void draw ( void );
    void draw_overlay ( void );
    int handle ( int m );
    static void update_cb ( void *arg );

    void select( const Rectangle &r );
    Track * track_under ( int Y );


    void delete_selected ( void );
    void select_none ( void );

    void add_track ( Track *track );
    void remove_track ( Track *track );

    int  total_input_buffer_percent ( void );
    int  total_output_buffer_percent ( void );

    int total_playback_xruns ( void );
    int total_capture_xruns ( void );

    bool record ( void );
    void stop ( void );

    void zoom ( float secs );
    void zoom_in ( void );
    void zoom_out ( void );
    void zoom_fit ( void );


private:

    friend class Engine; // FIXME: only Engine::process() needs to be friended.x

    Track * track_by_name ( const char *name );
    char * get_unique_track_name ( const char *name );

    /* Engine */
    nframes_t process ( nframes_t nframes );
    void seek ( nframes_t frame );
    int seek_pending ( void );
};
