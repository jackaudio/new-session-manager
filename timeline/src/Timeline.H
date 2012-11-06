
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


/* FIXME: this class needs a lot of cleaning up. Too many public
 * members etc. */

/* #include "Audio_File.H" // just for nframes_t */

#include "types.h"

#include <math.h>
#include <assert.h>
#include <list>

#include "OSC_Thread.H"

class Fl_Scroll;
class Fl_Pack;
class Fl_Scrollbar;
class Fl_Widget;

class Fl_Menu_Button;
class Fl_Menu_;

class Timeline;
extern Timeline *timeline;

struct BBT;
class Tempo_Sequence;
class Time_Sequence;
class Annotation_Sequence;
class Cursor_Sequence;
class Track;
class Sequence;
class Sequence_Widget;
class Cursor_Region;
class Cursor_Point;
class Fl_Panzoomer;
class Fl_Tile;

namespace OSC { class Endpoint; }

#define USE_WIDGET_FOR_TIMELINE


#include <lo/lo.h>

#ifndef USE_SINGLEBUFFERED_TIMELINE
#include <FL/Fl_Overlay_Window.H>
#else
#include <FL/Fl_Single_Window.H>
#endif

struct position_info;

struct Rectangle
{
    int x;
    int y;
    int w;
    int h;

    Rectangle ( ) : x( 0 ), y( 0 ), w( 0 ), h( 0 ) {}
    Rectangle ( int X, int Y, int W, int H ) : x( X ), y( Y ), w( W ), h( H ) {}
};

#include "RWLock.H"

#ifdef USE_WIDGET_FOR_TIMELINE
class Timeline : public Fl_Group, public RWLock
#else
#ifndef USE_SINGLEBUFFERED_TIMELINE
class Timeline : public Fl_Overlay_Window, public RWLock
#else
class Timeline : public Fl_Single_Window, public RWLock
#endif
#endif


{

    class Timeline_Panzoomer;

    static void draw_clip ( void * v, int X, int Y, int W, int H );
 
    nframes_t _old_xposition;
    int _old_yposition;

    Rectangle _selection;
    
    Fl_Group     *track_window;
    Fl_Pack      *tracks;
    Fl_Pack      *rulers;
    Fl_Panzoomer *panzoomer;
    Fl_Tile      *tile;

    void adjust_panzoomer ( void );
    static void cb_scroll ( Fl_Widget *w, void *v );
    void cb_scroll ( Fl_Widget *w );
    static void menu_cb ( Fl_Widget *w, void *v );
    void menu_cb ( Fl_Menu_ *m );
    void fix_range ( void );

    int _fpp;                                                 /* frames per pixel, power of two */

//    nframes_t p1, p2;                                           /* cursors */
    nframes_t _playhead;

    /* not permitted */
    Timeline ( const Timeline &rhs );
    Timeline & operator = ( const Timeline &rhs );

    std::list <const Sequence_Widget*> _tempomap;

    static void handle_peer_scan_complete ( void * v );

    void update_track_order ( void );
    void apply_track_order ( void );
    void insert_track ( Track *track, int n );

public:

    OSC::Endpoint *osc;
    OSC_Thread *osc_thread;

    void process_osc ( void );
#undef Bars
#undef Beats
#undef None

    enum snap_e {
        Bars,
        Beats,
        None
    };

    /* configuration values */
    static bool draw_with_measure_lines;
    static bool draw_with_cursor_overlay;
    static snap_e snap_to;
    static bool snapping_on_hold;
    static bool snap_magnetic;
    static bool follow_playhead;
    static bool center_playhead;
    static bool loop_playback;
    static bool automatically_create_takes;




    Tempo_Sequence *tempo_track;
    Time_Sequence  *time_track;
    Annotation_Sequence *ruler_track;
    Cursor_Sequence *edit_cursor_track;
    Cursor_Sequence *punch_cursor_track;
    Cursor_Sequence *play_cursor_track;

    Fl_Menu_Button *menu;

    int _punched_in;
    nframes_t _punch_out_frame;
    nframes_t _punch_in_frame;
    bool _created_new_takes;

    nframes_t xoffset;

    nframes_t _sample_rate;

    Timeline ( int X, int Y, int W, int H, const char *L=0 );
    
    virtual ~Timeline ( );

    void update_tempomap ( void );

    const char *session_manager_name ( void );

    
    nframes_t fpp ( void ) const { return 1 << _fpp; }
    void range ( nframes_t start, nframes_t length );

    nframes_t range_start ( void ) const;
    nframes_t range_end ( void ) const;
    void range_start ( nframes_t n );
    void range_end ( nframes_t n );
    void reset_range ( void );
    nframes_t playback_home ( void ) const;
    nframes_t playback_end ( void ) const;

//    nframes_t playhead ( void ) const { return transport->frame; }
    nframes_t length ( void ) const;
    void sample_rate ( nframes_t r ) { _sample_rate = r; }
    nframes_t sample_rate ( void ) const { return _sample_rate; }
    int ts_to_x( nframes_t ts ) const { return  ts >> _fpp; }
    nframes_t x_to_ts ( int x ) const { return x << _fpp; }
    nframes_t x_to_offset ( int x ) const;

    float beats_per_minute ( nframes_t when ) const;
    int beats_per_bar ( nframes_t when ) const;
    void beats_per_minute ( nframes_t when, float bpm );
    void time ( nframes_t when, int bpb, int beat_type );
    bool nearest_line ( nframes_t *f, bool snap=true ) const;
    bool next_line ( nframes_t *f, bool bar=false ) const;
    bool prev_line ( nframes_t *f, bool bar=false ) const;

    typedef void (measure_line_callback)( nframes_t frame, const BBT & bbt, void *arg );

    position_info solve_tempomap ( nframes_t when ) const;
    void draw_measure_lines ( int X, int Y, int W, int H );
    void draw_measure_BBT ( int X, int Y, int W, int H );
    position_info render_tempomap ( nframes_t start, nframes_t length, measure_line_callback *cb, void *arg ) const;

    void xposition ( int X );
    void draw_cursor ( nframes_t frame, Fl_Color color, void (*symbol)(Fl_Color) ) const;
    void draw_cursors ( Cursor_Sequence *o ) const;
    void draw_cursors ( void ) const;
    void draw_playhead ( void );
    void redraw_playhead ( void );
    void resize ( int X, int Y, int W, int H );

    void draw ( void );
    void draw_overlay ( void );
    int handle_scroll ( int m );
    int handle ( int m );
    static void update_cb ( void *arg );

    void select( const Rectangle &r );
    Track * track_under ( int Y );

    int nselected ( void ) const;
    void delete_selected ( void );
    void select_none ( void );

    void add_track ( Track *track );
    void remove_track ( Track *track );

    void move_track_up ( Track *track );
    void move_track_down ( Track *track );

    int find_track ( const Track * track ) const;

    int ntracks ( void ) const;

    void zoom ( float secs );
    void zoom_in ( void );
    void zoom_out ( void );
    void zoom_fit ( void );


    /* Engine */
    int  total_input_buffer_percent ( void );
    int  total_output_buffer_percent ( void );

    int total_playback_xruns ( void );
    int total_capture_xruns ( void );
    nframes_t total_output_latency ( void ) const;

    bool record ( void );
    void stop ( void );
    void punch_in ( nframes_t frame );
    void punch_out ( nframes_t frame );

    void wait_for_buffers ( void );
    bool seek_pending ( void );

    bool command_load ( const char *name, const char *display_name );
    bool command_new ( const char *name, const char *display_name );
    bool command_save ( void );
    void command_quit ( void );

    /* OSC */
    
    void connect_osc ( void );

    void discover_peers ( void );

    static void check_osc ( void * v );
    
    int init_osc ( const char *osc_port );
    
    static int osc_reply ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data );
    static int osc_non_hello ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data );
    

    void reply_to_finger ( lo_message msg );

    void add_cursor ( Cursor_Region *o );
    void add_cursor ( Cursor_Point *o );

private:
    
    void add_take_for_armed_tracks();
    void resize_rulers ( void );

    static void snapshot ( void *v ) { ((Timeline*)v)->snapshot(); }
    void snapshot ( void );

    friend class Engine; // FIXME: only Engine::process() needs to be friended.x

    Track * track_by_name ( const char *name );
    char * get_unique_track_name ( const char *name );

    /* Engine */
    void resize_buffers ( nframes_t nframes );
    nframes_t process ( nframes_t nframes );
    void seek ( nframes_t frame );
};
