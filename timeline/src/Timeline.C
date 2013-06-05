
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

/* This is the Timeline widget, which contains all the tracks and
 * provides cursor overlays, scrolling, zooming, measure lines, tempo
 * map and just about everything else. */

#include <FL/Fl.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Panzoomer.H>
#include <FL/Fl_Tile.H>

#include "Timeline.H"
#include "Tempo_Sequence.H"
#include "Time_Sequence.H"
#include "Cursor_Sequence.H"
#include "Audio_Sequence.H"
#include "Control_Sequence.H"
#include "Sequence.H"
#include "Annotation_Sequence.H"
#include "Track.H"
#include "Transport.H"

#include "FL/menu_popup.H"

#include "const.h"
#include "debug.h"

/* these headers are just for the NSM support */
#include "Project.H"
#include "TLE.H"
/*  */

#include "OSC_Thread.H"
#include "OSC/Endpoint.H"

#include <unistd.h>

#include <nsm.h>
extern nsm_client_t *nsm;

#define BASE Fl_Group
#define BX this->x()
#define BY this->y()



bool Timeline::draw_with_measure_lines = true;
bool Timeline::draw_with_cursor_overlay = true;
Timeline::snap_e Timeline::snap_to = Bars;
bool Timeline::snapping_on_hold = false;
bool Timeline::snap_magnetic = true;
bool Timeline::follow_playhead = true;
bool Timeline::center_playhead = true;

const float UPDATE_FREQ = 1.0f / 18.0f;

extern const char *instance_name;
extern TLE *tle;



/** return the combined height of all visible children of (veritcal)
    pack, /p/. This is necessary because pack sizes are adjusted only
    when the relevant areas are exposes. */
static int
pack_visible_height ( const Fl_Pack *p )
{
    int th = 0;

    const Fl_Widget* const *w = p->array();
    for ( int i = p->children(); i--; ++w )
        if ( (*w)->visible() )
            th += (*w)->h() + p->spacing();

    return th;
}

#define BP fl_begin_polygon()
#define EP fl_end_polygon()
#define vv(x,y) fl_vertex( x, y )
#define BL fl_begin_line()
#define EL fl_end_line()

void
draw_full_arrow_symbol ( Fl_Color color )
{
    /* draw cap */
    fl_color( color );

    BP;
    vv( -1, -1 );
    vv( 0, 1 );
    vv( 1, -1 );
    EP;

    /* draw cap outline */
    fl_color( FL_BLACK );

    BL;
    vv( -1, -1 );
    vv( 0, 1 );
    vv( 1, -1 );
    EL;

}



class Timeline::Timeline_Panzoomer : public Fl_Panzoomer 
{
public:
    Timeline_Panzoomer ( int X,int Y,int W,int H, const char *L=0)
        : Fl_Panzoomer(X,Y,W,H)
        {
        }
protected:
    void
    draw_background ( int X, int Y,int W, int H )
        {
            nframes_t ef = timeline->x_to_ts( _xmax );
            
            double ty = Y;
            
            for ( int i = 0; i < timeline->tracks->children(); i++ )
            {
                Track *t = (Track*)timeline->tracks->child( i );
                
                Sequence *s = t->sequence();
                
                if ( !s )
                    continue;
                
                fl_color( FL_DARK1 );
                
                const double scale = (double)H / ( pack_visible_height( timeline->tracks ) );
                
//        double th =  (double)H / timeline->tracks->children();
                const double th = t->h() * scale;
                
                fl_line( X, ty,
                         X + W, ty );
                
                for ( list <Sequence_Widget *>::const_iterator r = s->_widgets.begin(); 
                      r != s->_widgets.end(); ++r )
                {
                    fl_rectf(
                        X + ( W * ( (double)(*r)->start() / ef ) ),
                        ty,
                        W * ( (double)(*r)->length() / ef ),
                        th,
                        (*r)->actual_box_color());
                }
                
                fl_font( FL_HELVETICA, th );
                fl_color( FL_FOREGROUND_COLOR );
                fl_draw( t->name(), X, ty, W, th, (Fl_Align)(FL_ALIGN_LEFT | FL_ALIGN_INSIDE ));
                
                ty += th;
            }
        }
};





void
Timeline::redraw_overlay ( void )
{
    ((Fl_Overlay_Window*)window())->redraw_overlay();
}

nframes_t 
Timeline::range_start ( void ) const
{
    if ( edit_cursor_track->active_cursor() )
        return edit_cursor_track->active_cursor()->start();
    else
        return 0;
}

nframes_t
Timeline::range_end ( void ) const 
{ 
    if ( edit_cursor_track->active_cursor() )
        return edit_cursor_track->active_cursor()->start() + edit_cursor_track->active_cursor()->length();
    else
        return 0;
}

void
Timeline::range_start ( nframes_t n )
{
    if ( ! edit_cursor_track->active_cursor() )
        new Cursor_Region( 0, 0, "Edit", NULL );

    Logger log( edit_cursor_track->active_cursor() );

    edit_cursor_track->active_cursor()->set_left( n );
}

void
Timeline::range_end ( nframes_t n )
{
    if ( ! edit_cursor_track->active_cursor() )
        new Cursor_Region( 0, 0, "Edit", NULL );

    Logger log( edit_cursor_track->active_cursor() );

    edit_cursor_track->active_cursor()->set_right( n );
}

/** return first frame of playback (might not be 0) */
nframes_t 
Timeline::playback_home ( void ) const
{
    if ( play_cursor_track->active_cursor() )
        return play_cursor_track->active_cursor()->start();
    else
        return 0;
}

/** return last frame of playback */
nframes_t
Timeline::playback_end ( void ) const
{
    if ( play_cursor_track->active_cursor() )
        return play_cursor_track->active_cursor()->start() + play_cursor_track->active_cursor()->length();
    else
        return length();
}

void
Timeline::reset_range ( void )
{
    delete edit_cursor_track->active_cursor();
}

/** callback used by Loggable class to create a snapshot of system
 * state. */
void
Timeline::snapshot ( void )
{
    tempo_track->log_children();
    time_track->log_children();
    edit_cursor_track->log_children();
    punch_cursor_track->log_children();
    play_cursor_track->log_children();

    update_track_order();

    for ( int i = 0; i < tracks->children(); ++i )
    {
        ((Track*)tracks->child( i ))->log_children();
    }
}


/** recalculate the size of horizontal scrolling area and inform scrollbar */
void
Timeline::adjust_panzoomer ( void )
{
    panzoomer->y_value( panzoomer->y_value(), h() - rulers->h() - panzoomer->h(), 0, pack_visible_height( tracks ));
    panzoomer->x_value( ts_to_x( xoffset ),                     /* pos */
                        tracks->w() - Track::width(),           /* window size */
                        0,                                      /* min */
                        max( ts_to_x( length() ), tracks->w() - Track::width() )/* max */
        );
}

void
Timeline::cb_scroll ( Fl_Widget *w, void *v )
{
    ((Timeline*)v)->cb_scroll( w );
}

void
Timeline::cb_scroll ( Fl_Widget *w )
{
    //adjust_panzoomer();

    if ( panzoomer->zoom_changed() )
    {
        nframes_t under_mouse = x_to_offset( Fl::event_x() );

        _fpp = panzoomer->zoom();

        panzoomer->x_value( ts_to_x( under_mouse ) );

        redraw();
    }

    if ( _old_yposition != panzoomer->y_value() )
    {
        tracks->position( tracks->x(), track_window->y() - (int)panzoomer->y_value() );  
        damage( FL_DAMAGE_SCROLL );
    }

    if ( _old_xposition != x_to_ts( panzoomer->x_value() ))
    {
        damage( FL_DAMAGE_SCROLL );
        xposition( panzoomer->x_value() );
    }
}

void
Timeline::menu_cb ( Fl_Widget *w, void *v )
{
    ((Timeline*)v)->menu_cb( (Fl_Menu_*)w );
}

/** ensure that p1 is less than range_end() */
void
Timeline::fix_range ( void )
{
    if ( range_start() > range_end() )
    {
        nframes_t t = range_end();
        range_end( range_start() );
        range_start( t );
    }
}

/** set the range to /start/ + /length/ */
void
Timeline::range ( nframes_t start, nframes_t length )
{
    range_start( start );
    range_end( start + length );

    redraw();
}

/** create a new take for every armed track */
void
Timeline::add_take_for_armed_tracks ( void )
{
    THREAD_ASSERT( UI );

    wrlock();

    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );
    
        if ( t->armed() && t->sequence()->_widgets.size() )
            t->sequence( new Audio_Sequence( t ) );
    }

    unlock();
}

void
Timeline::menu_cb ( Fl_Menu_ *m )
{
    if ( ! active_r() )
        return;

    const char *picked = m->mvalue()->label();

/*     m->item_pathname( picked, sizeof( picked ) ); */

    DMESSAGE( "%s", picked );

    if ( ! strcmp( picked, "Add audio track" ) )
    {
        /* FIXME: prompt for I/O config? */

        Loggable::block_start();

        /* add audio track */
        char *name = get_unique_track_name( "Audio" );

        Track *t = new Track( name );

        Audio_Sequence *o = new Audio_Sequence( t );

        add_track( t );

        t->sequence( o );

        t->take_focus();

        Loggable::block_end();
    }
    else if ( ! strcmp( picked, "Tempo from edit (beat)" ) )
    {
        if ( range_start() != range_end() )
        {
            fix_range();

            beats_per_minute( range_start(), sample_rate() * 60 / (float)( range_end() - range_start() ) );

            range_end( range_start() );
        }
    }
    else if ( ! strcmp( picked, "Tempo from edit (bar)" ) )
    {
        if ( range_start() != range_end() )
        {
            fix_range();

            position_info pi = solve_tempomap( range_start() );

            beats_per_minute( range_start(), sample_rate() * 60 / (float)( ( range_end() - range_start() ) / pi.beats_per_bar ) );

            range_end( range_start() );
        }
    }
    else if ( ! strcmp( picked, "Playhead to mouse" ) )
    {
        int X = Fl::event_x() - Track::width();

        if ( X > 0 )
        {
            transport->locate( xoffset + x_to_ts( X ) );
        }
    }
    else if ( ! strcmp( picked, "Edit start to mouse" ) )
    {
        int X = Fl::event_x() - Track::width();

        if ( X > 0 )
        {
            range_start( xoffset + x_to_ts( X ) );
        }

	fix_range();

        /* FIXME: only needs to damage the location of the old cursor! */
        redraw();
    }
    else if ( ! strcmp( picked, "Edit end to mouse" ) )
    {
        int X = Fl::event_x() - Track::width();

        if ( X > 0 )
        {
            range_end( xoffset + x_to_ts( X ) );
        }

	fix_range();

        /* FIXME: only needs to damage the location of the old cursor! */
        redraw();
    }
    else if ( ! strcmp( picked, "Playhead left beat" ) )
    {
        nframes_t f = transport->frame;

        if ( prev_line( &f ) )
            transport->locate( f );
    }
    else if ( ! strcmp( picked, "Playhead right beat" ) )
    {
        nframes_t f = transport->frame;

        if ( next_line( &f ) )
            transport->locate( f );
    }
    else if ( ! strcmp( picked, "Playhead left bar" ) )
    {
        nframes_t f = transport->frame;

        if ( prev_line( &f, true ) )
            transport->locate( f );
    }
    else if ( ! strcmp( picked, "Playhead right bar" ) )
    {
        nframes_t f = transport->frame;

        if ( next_line( &f, true ) )
            transport->locate( f );
    }
    else if ( ! strcmp( picked, "Swap edit start and playhead" ) )
    {
        nframes_t t = transport->frame;

        transport->locate( range_start() );

        range_start( t );

        redraw();
    }
    else if ( ! strcmp( picked, "Swap edit end and playhead" ) )
    {
        nframes_t t = transport->frame;

        transport->locate( range_end() );

        range_end( t );

        redraw();
    }
    else if ( ! strcmp( picked, "Edit start to playhead" ) )
    {
        range_start( transport->frame );

        redraw();
    }
    else if ( ! strcmp( picked, "Edit end to playhead" ) )
    {
        range_end( transport->frame );

        redraw();
    }
    else if ( ! strcmp( picked, "Punch from edit" ) )
    {
        if ( range_start() != range_end() )
        {
            Loggable::block_start();

            new Cursor_Region( range_start(), range_end() - range_start(), "Punch", NULL );
            reset_range();

            Loggable::block_end();
        }

        redraw();
    }
    else if ( ! strcmp( picked, "Playback from edit" ) )
    {
        if ( range_start() != range_end() )
        {
            Loggable::block_start();

            if ( play_cursor_track->active_cursor() )
            {
                play_cursor_track->active_cursor()->start( range_start() );
                play_cursor_track->active_cursor()->set_right( range_end() );
            }
            else
            {
                new Cursor_Region( range_start(), range_end() - range_start(), "Playback", NULL );
            }

            reset_range();

            Loggable::block_end();
        }

        redraw();
    }
    else if ( ! strcmp( picked, "Redraw" ) )
    {
        redraw();
    }
    else
        WARNING( "programming error: Unknown menu item" );
}

int
Timeline::ntracks ( void ) const
{
    return tracks->children();
}


Timeline::~Timeline ( )
{
    delete osc_thread;
    osc_thread = 0;
}

Timeline::Timeline ( int X, int Y, int W, int H, const char* L ) : BASE( X, Y, W, H, L )
{
    Loggable::snapshot_callback( &Timeline::snapshot, this );

    edit_cursor_track = NULL;
    punch_cursor_track = NULL;
    play_cursor_track = NULL;

    _created_new_takes = 0;
    _punched_in = 0;
    _punch_in_frame = 0;
    _punch_out_frame = 0;
    osc_thread = 0;
    _sample_rate = 44100;

    box( FL_FLAT_BOX );
    xoffset = 0;
    _old_yposition = 0;
    _old_xposition = 0;

//    range_start( range_end( 0 ) );

    menu = new Fl_Menu_Button( 0, 0, 0, 0, "Timeline" );

/*     menu->add( "Add Track", 0, 0, 0  ); */

    int ruler_height = 0;

    menu->add( "Add audio track", 'a', 0, 0 );
    menu->add( "Tempo from edit (beat)", 't', 0, 0 );
    menu->add( "Tempo from edit (bar)", FL_CTRL + 't', 0, 0 );
    menu->add( "Playhead to mouse", 'p', 0, 0 );
    menu->add( "Edit start to mouse", '[', 0, 0 );
    menu->add( "Edit end to mouse", ']', 0, 0 );
    menu->add( "Playhead left beat", FL_SHIFT + FL_Left, 0, 0 );
    menu->add( "Playhead right beat", FL_SHIFT + FL_Right, 0, 0 );
    menu->add( "Playhead left bar", FL_CTRL + FL_SHIFT + FL_Left, 0, 0 );
    menu->add( "Playhead right bar", FL_CTRL + FL_SHIFT + FL_Right, 0, 0 );
    menu->add( "Swap edit start and playhead", FL_CTRL + FL_SHIFT + '[', 0, 0 );
    menu->add( "Swap edit end and playhead", FL_CTRL + FL_SHIFT + ']', 0, 0 );
    menu->add( "Edit start to playhead", FL_CTRL + '[', 0, 0 );
    menu->add( "Edit end to playhead", FL_CTRL + ']', 0, 0 );
    menu->add( "Punch from edit", FL_CTRL + FL_SHIFT + 'p', 0, 0 );
    menu->add( "Playback from edit", FL_CTRL + FL_SHIFT + 'l', 0, 0 );
    menu->add( "Redraw", FL_CTRL + 'l', 0, 0 );

    menu_set_callback( const_cast<Fl_Menu_Item*>(menu->menu()), &Timeline::menu_cb, (void*)this );


    {
        Fl_Pack *o = new Fl_Pack( X, Y, W, 1, "rulers" );
        o->type( Fl_Pack::VERTICAL );

        {
            Tempo_Sequence *o = new Tempo_Sequence( 0, 0, 800, 18 );

            o->color( FL_DARK1 );

            /* o->label( "Tempo" ); */

            tempo_track = o;
        }

        {
            Time_Sequence *o = new Time_Sequence( 0, 24, 800, 18 );

            o->color( FL_DARK2 );

            /* o->name( "Time" ); */

            time_track = o;
        }

        {
            Cursor_Sequence *o = new Cursor_Sequence( 0, 24, 800, 18 );

            o->color( FL_DARK1 );

            o->label( "Edit" );
            o->cursor_color( FL_YELLOW );

            edit_cursor_track = o;
        }

        {
            Cursor_Sequence *o = new Cursor_Sequence( 0, 24, 800, 18 );

            o->color( FL_DARK2 );

            o->label( "Punch" );
            o->cursor_color( FL_RED );

            punch_cursor_track = o;
        }

        {
            Cursor_Sequence *o = new Cursor_Sequence( 0, 24, 800, 18 );

            o->color( FL_DARK1 );

            o->label( "Playback" );
            o->cursor_color( FL_GREEN );

            play_cursor_track = o;
        }

/*         { */
/*             Annotation_Sequence *o = new Annotation_Sequence( 0, 24, 800, 24 ); */

/*             o->color( fl_gray_ramp( 'F' ) ); */

/*             o->label( "Ruler" ); */
/*             o->align( FL_ALIGN_LEFT ); */

/*             ruler_track = o; */
/*         } */

        rulers = o;
        o->end();

        ruler_height = o->child( 0 )->h() * o->children();

        DMESSAGE( "Ruler height: %i", ruler_height );

        o->size( o->w(), ruler_height );
    }

    { 
        Fl_Tile *o = new Fl_Tile( X, rulers->y() + ruler_height, W, H - rulers->h() );
        o->box(FL_FLAT_BOX);
        o->when( FL_WHEN_RELEASE );
        { 
            Fl_Group *o = new Fl_Group( X, rulers->y() + rulers->h(), W, ( H - rulers->h() ) - 50 );
            o->box(FL_FLAT_BOX);
            {
                _fpp = 8;
                Fl_Pack *o = new Fl_Pack( X, rulers->y() + rulers->h(), W, 1 );
                o->type( Fl_Pack::VERTICAL );
                o->spacing( 1 );
                
                tracks = o;
                o->end();
                /* Fl_Group::current()->resizable( o ); */
            }
            
            o->end();
            track_window = o;
        }
        {
            Fl_Panzoomer *o = new Timeline_Panzoomer( X, 
                                                track_window->y() + track_window->h(),
                                                W,
                                                50 );
        
            o->zoom_range( 1, 20 );
            o->zoom( 8 );
        
            o->box( FL_FLAT_BOX );
            o->color( FL_DARK1 );
            o->type( FL_HORIZONTAL );
            o->callback( cb_scroll, this );
        
            panzoomer = o;
        }
        
        Fl_Box *spacebox = new Fl_Box( 0,0,1,1 );

        o->end();
        o->resizable( spacebox );

        spacebox->resize( X, rulers->y() + ( ( H - rulers->h() ) - 50 ),
            W, 125 );

        o->position( panzoomer->x(), panzoomer->y(),
                     panzoomer->x(), track_window->y() + track_window->h() );

        tile = o;
        resizable(o);
    }

    /* rulers go above tracks... */
    add( rulers );

    /* redraw(); */

    end();

    Fl::add_timeout( UPDATE_FREQ, update_cb, this );
}

void
Timeline::beats_per_minute ( nframes_t when, float bpm )
{
    tempo_track->add( new Tempo_Point( when, bpm ) );
}

void
Timeline::time ( nframes_t when, int bpb, int note_type )
{
    time_track->add( new Time_Point( when, bpb, note_type ) );
}

/************/
/* Snapping */
/************/

struct nearest_line_arg
{
    nframes_t original;
    nframes_t closest;
    bool bar;
};

const int snap_pixel = 10;

static nframes_t
abs_diff ( nframes_t n1, nframes_t n2 )
{
    return n1 > n2 ? n1 - n2 : n2 - n1;
}

static void
nearest_line_snap_cb ( nframes_t frame, const BBT &bbt, void *arg )
{
    nearest_line_arg *n = (nearest_line_arg *)arg;

    if ( n->bar && bbt.beat )
        return;

    if ( Timeline::snap_magnetic &&
         abs_diff( frame, n->original ) > timeline->x_to_ts( snap_pixel ) )
        return;

    if ( abs_diff( frame, n->original ) < abs_diff( n->original, n->closest ) )
        n->closest = frame;
}

static void
nearest_line_cb ( nframes_t frame, const BBT &bbt, void *arg )
{
    nearest_line_arg *n = (nearest_line_arg *)arg;

    if ( n->bar && bbt.beat )
        return;

    if ( abs_diff( frame, n->original ) < abs_diff( n->original, n->closest ) )
        n->closest = frame;
}

static void
prev_next_line_cb ( nframes_t frame, const BBT &bbt, void *arg )
{
    nearest_line_arg *n = (nearest_line_arg *)arg;

    if ( n->bar && bbt.beat )
        return;

    if ( abs_diff( frame, n->original ) < abs_diff( n->original, n->closest ) )
        n->closest = frame;
}

/** Set the value pointed to by /frame/ to the frame number of the of
    the nearest measure line to /when/. Returns true if the new value of
    *frame is valid, false otherwise. */
bool
Timeline::nearest_line ( nframes_t *frame, bool snap ) const
{
    if ( snap && ( snapping_on_hold || None == Timeline::snap_to ) )
        return false;

    nframes_t when = *frame;

    nearest_line_arg n = { when, JACK_MAX_FRAMES, snap && Timeline::Bars == Timeline::snap_to };

    render_tempomap( when > x_to_ts( w() >> 1 ) ? when - x_to_ts( w() >> 1 ) : 0,
                     when + x_to_ts( w() >> 1 ), snap ? nearest_line_snap_cb : nearest_line_cb, &n );

    if ( n.closest == JACK_MAX_FRAMES )
        return false;
    else
    {
        *frame = n.closest;
        return true;
    }
}

/** Set the value pointed to by /frame/ to the frame number of the of
    the nearest measure line to *greater than* /when/. Returns true if
    the new value of *frame is valid, false otherwise. */
bool
Timeline::next_line ( nframes_t *frame, bool bar ) const
{
    const nframes_t when = *frame + 1;

    nearest_line_arg n = { when, JACK_MAX_FRAMES, bar };

    const nframes_t window = sample_rate() * 60;
    nframes_t length = window;

    if ( when > JACK_MAX_FRAMES - length )
        length = JACK_MAX_FRAMES - when;

    render_tempomap( when, length, prev_next_line_cb, &n );

    *frame = n.closest;

    return true;
}

/** Set the value pointed to by /frame/ to the frame number of the of
    the nearest measure line to *less than* /when/. Returns true if
    the new value of *frame is valid, false otherwise. */
bool
Timeline::prev_line ( nframes_t *frame, bool bar ) const
{
    if ( ! *frame )
        return false;

    const nframes_t when = *frame - 1;

    nearest_line_arg n = { when, 0, bar };

    const nframes_t window = sample_rate() * 60;
    nframes_t start = 0;
    nframes_t length = when;

    if ( when > window )
    {
        start = when - window;
        length = window;
    }

    render_tempomap( start, length, prev_next_line_cb, &n );

    *frame = n.closest;

    return true;
}



/** given screen pixel coordinate /x/ return frame offset into
 * timeline, taking into account the current scroll position, widget
 * layout, etc. */
nframes_t
Timeline::x_to_offset ( int x ) const
{
    return x_to_ts( max( 0, x - Track::width() ) ) + xoffset;
}

int
Timeline::offset_to_x ( nframes_t frame ) const
{
    return ts_to_x( frame - xoffset ) + Track::width();
}

/** draws a single measure line */
void
Timeline::draw_measure_cb ( nframes_t frame, const BBT &bbt, void *v )
{
    Timeline *o = (Timeline*)v;

    Fl_Color c = FL_LIGHT3;

    if ( o->panzoomer->zoom() >= 15 )
        return;

    if ( bbt.beat )
    {
        if ( o->panzoomer->zoom() > 12 )
            return;
        else
            c = FL_DARK1;
    }

    fl_color( fl_color_add_alpha( c, 64 ) );
    
    const int x = timeline->ts_to_x( frame - timeline->xoffset ) + Track::width();

    fl_line( x, 0, x, 2000 );
}

/* FIXME: wrong place for this */
const float ticks_per_beat = 1920.0;

/** re-render the unified tempomap based on the current contents of the Time and Tempo sequences */
void
Timeline::update_tempomap ( void )
{
    /* FIXME: we need some type of locking! */
    _tempomap.clear();

    for ( list <Sequence_Widget *>::const_iterator i = time_track->_widgets.begin();
          i != time_track->_widgets.end(); ++i )
        _tempomap.push_back( *i );

    for ( list <Sequence_Widget *>::const_iterator i = tempo_track->_widgets.begin();
          i != tempo_track->_widgets.end(); ++i )
        _tempomap.push_back( *i );

    _tempomap.sort( Sequence_Widget::sort_func );
}

/** return a stucture containing the BBT info which applies at /frame/ */
position_info
Timeline::solve_tempomap ( nframes_t frame ) const
{
    return render_tempomap( frame, 0, 0, 0 );
}

/* THREAD: UI and RT */
/** draw appropriate measure lines inside the given bounding box */
position_info
Timeline::render_tempomap( nframes_t start, nframes_t length, measure_line_callback * cb, void *arg ) const
{
    const nframes_t end = start + length;

    position_info pos;
    memset( &pos, 0, sizeof( pos ) );

    BBT &bbt = pos.bbt;

    /* default values */
    pos.beat_type = 4;
    pos.beats_per_bar = 4;
    pos.tempo = 120.0;

    const nframes_t samples_per_minute = sample_rate() * 60;

    float bpm = 120.0f;

    time_sig sig;

    sig.beats_per_bar = 4;
    sig.beat_type = 4;

    nframes_t f = 0;
    nframes_t next = 0;

    nframes_t frames_per_beat = samples_per_minute / bpm;

    if ( ! _tempomap.size() )
        return pos;

    for ( list <const Sequence_Widget *>::const_iterator i = _tempomap.begin();
          i != _tempomap.end(); ++i )
    {

        if ( ! strcmp( (*i)->class_name(), "Tempo_Point" ) )
        {
            const Tempo_Point *p = (Tempo_Point*)(*i);

            bpm = p->tempo();
            frames_per_beat = samples_per_minute / bpm;
        }
        else
        {
            const Time_Point *p = (Time_Point*)(*i);

            sig = p->time();

            /* Time point resets beat */
            bbt.beat = 0;
        }

        {
            list <const Sequence_Widget *>::const_iterator n = i;
            ++n;
            if ( n == _tempomap.end() )
                next = end;
            else
//                next = min( (*n)->start(), end );
                /* points may not always be aligned with beat boundaries, so we must align here */
                next = (*n)->start() - ( ( (*n)->start() - (*i)->start() ) % frames_per_beat );
        }

        for ( ; f < next; ++bbt.beat, f += frames_per_beat )
        {

            if ( bbt.beat == sig.beats_per_bar )
            {
                bbt.beat = 0;
                ++bbt.bar;
            }

            if ( f >= start )
            {
                /* in the zone */
                if ( cb )
                    cb( f, bbt, arg );
            }

            /* ugliness to avoid failing out at -1 */
            if ( end >= frames_per_beat )
            {
                if ( f >= end - frames_per_beat )
                    goto done;
            }
            else if ( f + frames_per_beat >= end )
                goto done;
        }
    }

done:

    pos.frame = f;
    pos.tempo = bpm;
    pos.beats_per_bar = sig.beats_per_bar;
    pos.beat_type = sig.beat_type;

    assert( f <= end );

    assert( end - f <= frames_per_beat );

    /* FIXME: this this right? */

    const double frames_per_tick = frames_per_beat / ticks_per_beat;
    bbt.tick = ( end - f ) / frames_per_tick;

    return pos;
}

/** maybe draw appropriate measure lines in rectangle defined by X, Y, W, and H, using color /color/ as a base */
void
Timeline::draw_measure_lines ( int X, int Y, int W, int H )
{
    if ( ! draw_with_measure_lines )
        return;

    fl_line_style( FL_SOLID, 0 );

    const nframes_t start = x_to_offset( X );
    const nframes_t length = x_to_ts( W );

    fl_push_clip( X, Y, W, H );

    render_tempomap( start, length, draw_measure_cb, this );

    fl_pop_clip();
}

void
Timeline::draw_clip_rulers ( void * v, int X, int Y, int W, int H )
{
    Timeline *tl = (Timeline *)v;

    fl_push_clip( X, Y, W, H );

    tl->draw_box();

    tl->draw_child( *tl->rulers );

    fl_pop_clip();
}

void
Timeline::draw_clip_tracks ( void * v, int X, int Y, int W, int H )
{
    Timeline *tl = (Timeline *)v;

    fl_push_clip( X, Y, W, H );

    tl->draw_box();

    fl_damage_t pzd = tl->panzoomer->damage();

    tl->draw_child( *tl->tile );

    tl->panzoomer->damage( pzd );

    fl_pop_clip();
}

/** handle resize event */
void
Timeline::resize ( int X, int Y, int W, int H )
{
    BASE::resize( X, Y, W, H );

    tile->resizable()->resize( X,
                               tile->y() + tile->h() - 150,
                               W, 125 );
    
    /* why is THIS necessary? */
    panzoomer->resize( X,
                       tile->y() + tile->h() - 50,
                       W,
                       50 );

    track_window->resize( X,
                          tile->y(),
                          W,
                          tile->h() - 50);

    /* /\* rulers->resize( X, *\/ */
    /* /\*                 rulers->y(), *\/ */
    /* /\*                 W, *\/ */
    /* /\*                 rulers->h() ); *\/ */

    /* tile->redraw(); */
}


void
Timeline::add_cursor ( Cursor_Region *o )
{    
    if ( !strcmp( o->type(), "Edit" ) )
    {
        DMESSAGE( "Adding cursor to edit track" );
        edit_cursor_track->add( o );
    }
    else if ( !strcmp( o->type(), "Punch" ) )
    {
        DMESSAGE( "Adding cursor to punch track" );
        punch_cursor_track->add( o );       
    }
    else if ( !strcmp( o->type(), "Playback" ) )
    {
        DMESSAGE( "Adding cursor to punch track" );
        play_cursor_track->add( o );       
    }

}

void
Timeline::add_cursor ( Cursor_Point *o )
{
    if ( !strcmp( o->type(), "Edit" ) )
        edit_cursor_track->add( o );
    else if ( !strcmp( o->type(), "Punch" ) )
        punch_cursor_track->add( o );
}

void
Timeline::draw_cursors ( Cursor_Sequence *o ) const
{
    fl_push_clip( tracks->x() + Track::width(), rulers->y() + rulers->h(), tracks->w() - Track::width(), h() - rulers->h() - panzoomer->h() );

    if ( o && o->_widgets.size() > 0 )
    {
        for ( std::list<Sequence_Widget*>::const_iterator i = o->_widgets.begin();
              i != o->_widgets.end();
              i++ )
        {
            if ( Timeline::draw_with_cursor_overlay )
            {
                fl_color( fl_color_add_alpha( (*i)->box_color(), 50 ) );
                
                fl_rectf( (*i)->line_x(), tracks->y(), (*i)->abs_w(), tracks->h() );
            }
            else
            {
                fl_color( fl_color_add_alpha( (*i)->box_color(), 127  ));
                
                fl_line( (*i)->line_x(), tracks->y(), (*i)->line_x(), tracks->y() + tracks->h() );
                
                fl_line( (*i)->line_x() + (*i)->abs_w(), tracks->y(), (*i)->line_x() + (*i)->abs_w(), tracks->y() + tracks->h() );
            }
        }
    }

    fl_pop_clip();
}

/** draw ancillary cursors (not necessarily in the overlay plane) */
void
Timeline::draw_cursors ( void ) const
{
    draw_cursors( edit_cursor_track );

    if ( transport->punch_enabled() )
        draw_cursors( punch_cursor_track );
}
 

void
Timeline::draw ( void )
{
    /* Any code that might affect the structures used for drawing from
     * another thread must use Fl::lock()/unlock()! */ 
    THREAD_ASSERT( UI );

    int X, Y, W, H;

    int bdx = 0;
    int bdw = 0;

    X = tracks->x() + bdx + 1;
    Y = tracks->y();
    W = tracks->w() - bdw - 1;
    H = tracks->h();

    adjust_panzoomer();

    int dx = ts_to_x( _old_xposition ) - ts_to_x( xoffset );
    int dy = _old_yposition - (int)panzoomer->y_value();

    int c = damage();

//    c = FL_DAMAGE_ALL;

    if ( c & FL_DAMAGE_SCROLL )
    {
        {
         /*         draw_child( *rulers ); */

            Y = track_window->y();
            H = track_window->h();
            
            
            if ( dx )
                /* when scrolling horizontally, scroll rulers */
                fl_scroll( rulers->x() + Track::width(), 
                           rulers->y(),
                           rulers->w(),
                           rulers->h(), dx, 0, draw_clip_rulers, this );
 
            if ( dy )
                /* when scrolling vertically, also scroll track headers */
                fl_scroll(
                    X,
                    Y, 
                    Track::width(), 
                    H, 0, dy, draw_clip_tracks, this );
           
            /* scroll sequences */
            fl_scroll( X + Track::width(),
                       Y,
                       W - Track::width(),
                       H, dx, dy, draw_clip_tracks, this );
        }
    }

//    panzoomer->redraw();
    
    if ( c & FL_DAMAGE_ALL )
    {
        DMESSAGE( "complete redraw" );

        draw_box( box(), BX, BY, w(), h(), color() );

        draw_child( *rulers );

        fl_push_clip( tile->x(),
                      tile->y(),
                      tile->w(),
                      tile->h() );

        draw_child(*tile);
        
        fl_pop_clip();
        
        redraw_overlay();
        
        goto done;
    }

    if ( c & FL_DAMAGE_CHILD )
    {
        update_child(*rulers);

        fl_push_clip( tile->x(),
                      tile->y(),
                      tile->w(),
                      tile->h() );

        /* redraw the panzoomer preview whenever tracks change */
        /* if ( tracks->damage() ) */
        /*     panzoomer->redraw(); */

        update_child(*tile);
        
        fl_pop_clip();
        
        redraw_overlay();
    }

done:

    /* panzoomer->redraw(); */
//    update_child( *panzoomer );

    _old_xposition = xoffset;
    _old_yposition = panzoomer->y_value();
}

void
Timeline::damage_sequence ( void )
{
    panzoomer->redraw();
}

/** draw a single cursor line at /frame/ with color /color/ using symbol routine /symbol/ for the cap */
void
Timeline::draw_cursor ( nframes_t frame, Fl_Color color, void (*symbol)(Fl_Color) ) const
{
//    int x = ( ts_to_x( frame ) - ts_to_x( xoffset ) ) + tracks->x() + Track::width();

    if ( frame < xoffset )
        return;

    const int x = ts_to_x( frame - xoffset ) + tracks->x() + Track::width();

    if ( x > tracks->x() + tracks->w() )
        return;

    const int y = rulers->y() + rulers->h();
    const int h = this->h() - rulers->h()  - panzoomer->h();

    fl_push_clip( tracks->x() + Track::width(), y, tracks->w(), h );

    fl_line_style( FL_SOLID, 0 );
    fl_color( color );
    fl_line( x, y, x, y + h );

    fl_push_matrix();
   
    fl_translate( x, y );
    fl_scale( 8, 4 );
   
    symbol( color );
   
    fl_pop_matrix();
   
    fl_pop_clip();
}

void
Timeline::draw_playhead ( void )
{
    draw_cursor( transport->frame, FL_RED, draw_full_arrow_symbol );
//    draw_cursor( length(), FL_BLACK, draw_full_arrow_symbol );
}

void
Timeline::redraw_playhead ( void )
{
//    static nframes_t last_playhead = -1;
    static int last_playhead_x = -1;

    /* FIXME: kind of a hackish way to invoke punch / looping stuff from the UI thread... */

    if ( transport->rolling &&
         transport->rec_enabled() &&
         transport->punch_enabled() )
    {
        if ( _punched_in &&
             transport->frame > _punch_in_frame && 
             transport->frame > _punch_out_frame )
        {
            punch_out( _punch_out_frame );
        }
        else if ( ! _punched_in )
        {
            /* we've passed one or more punch regions... punch in for the next, if available. */
            const Sequence_Widget *w = punch_cursor_track->next( transport->frame );
            
            DMESSAGE( "Delayed punch in" );
            if ( w && 
                 w->start() > transport->frame )
            {
                _punch_in_frame = w->start();
                _punch_out_frame = w->start() + w->length();

                punch_in( w->start() );
            }
        }            
    }
    

    if ( transport->rolling )
    {
        if ( play_cursor_track->active_cursor() )
        {
            if ( transport->loop_enabled() )
            {
                if ( transport->frame > playback_end() )
                {
                    if ( ! seek_pending() )
                    {
                        if ( transport->recording )
                        {
                            stop();
                            transport->locate( playback_home() );
                            record();
                        }
                        else
                        {
                            transport->locate( playback_home() );
                        }
                    }
                }
            }
            else
                if ( transport->frame > playback_end() )
                    transport->stop();
        }
    }

    int playhead_x = ts_to_x( transport->frame );

    if ( last_playhead_x != playhead_x )
    {
        redraw_overlay();
//        last_playhead = transport->frame;
        last_playhead_x = playhead_x;

        if ( follow_playhead )
        {
            if ( center_playhead && active() )
                xposition( max( 0, playhead_x - ( ( tracks->w() - Track::width() ) >> 1 ) ) );
            else if ( playhead_x > ts_to_x( xoffset ) + ( tracks->w() - Track::width() ) )
                xposition( playhead_x );
            adjust_panzoomer();
        }
    }
}

/** called so many times a second to redraw the playhead etc. */
void
Timeline::update_cb ( void *arg )
{
    Fl::repeat_timeout( UPDATE_FREQ, update_cb, arg );

    Timeline *tl = (Timeline *)arg;

    tl->redraw_playhead();
}

/** draw cursors in overlay plane */
void
Timeline::draw_overlay ( void )
{
    draw_playhead();

    draw_cursors();

    if ( ! ( _selection.w && _selection.h ) )
        return;

    fl_push_clip( tracks->x() + Track::width(), rulers->y() + rulers->h(),  tracks->w() - Track::width(), h() - rulers->h() - panzoomer->h() );

    const Rectangle &r = _selection;

    fl_color( FL_MAGENTA );
    fl_line_style( FL_SOLID, 0 );
    fl_rect( r.x, r.y, r.w, r.h );

    fl_pop_clip();
}

/** select sequence widgets within rectangle /r/ */
void
Timeline::select ( const Rectangle &r )
{
    const int Y = r.y;

    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( ! ( t->y() > Y + r.h || t->y() + t->h() < Y ) )
            t->select( r.x, r.y, r.w, r.h, true, true );
    }
}

/** delete all selected sequence widgets */
void
Timeline::delete_selected ( void )
{
    Sequence_Widget::delete_selected();
}

/** clear the selection of seqeunce widgets */
void
Timeline::select_none ( void )
{
    Sequence_Widget::select_none();
}

int
Timeline::nselected ( void ) const
{
    return Sequence_Widget::nselected();
}

/** An unfortunate necessity for implementing our own DND aside from
 * the (bogus) native FLTK system */
Track *
Timeline::track_under ( int Y )
{
    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( ! ( t->y() > Y  || t->y() + t->h() < Y ) )
            return t;
    }

    return NULL;
}

#include "FL/event_name.H"
#include "FL/test_press.H"

/** a bit of a hack to keep FLTK's focus navigation stuff from
 * stealing the arrow keys from us */
int
Timeline::handle_scroll ( int m )
{
    if ( m == FL_KEYBOARD &&
         Fl::event_key() != FL_Home &&
         Fl::event_key() != FL_End )
        return menu->test_shortcut() || panzoomer->handle( m );
    else
        return 0;
}

Track *
Timeline::event_inside ( void )
{
    for ( int i = tracks->children(); i--; )
        if ( Fl::event_inside( tracks->child(i) ) )
            return (Track*)tracks->child(i);
    
    return NULL;
}

int
Timeline::handle ( int m )
{
    static Drag *drag = NULL;
    static bool range = false;

/*     if ( m != FL_NO_EVENT ) */
/*         DMESSAGE( "%s", event_name( m ) ); */

/*     int r = BASE::handle( m ); */

    switch ( m )
    {
        case FL_ENTER:
            return 1;
        case FL_LEAVE:
            return 1;
        case FL_KEYDOWN:
            if ( Fl::event_state() & ( FL_ALT | FL_CTRL | FL_SHIFT ) )
                /* we don't want any keys with modifiers... */
                return 0;

            if ( Fl::event_key() == 'r' )
            {
                range = true;
                return 1;
            }
            else if ( Fl::event_key() == 's' )
            {
                snapping_on_hold = true;
                return 1;
            }
            return 0;
        case FL_KEYUP:
            if ( Fl::event_state() & ( FL_ALT | FL_CTRL | FL_SHIFT ) )
                /* we don't want any keys with modifiers... */
                return 0;

            if ( Fl::event_key() == 'r' )
            {
                range = false;
                return 1;
            }
            else if ( Fl::event_key() == 's' )
            {
                snapping_on_hold = false;
                return 1;
            }
            return 0;
//        case FL_KEYBOARD:
        case FL_SHORTCUT:
        {
            if ( Fl::event_state() & ( FL_ALT | FL_CTRL | FL_SHIFT ) )
                /* we don't want any keys with modifiers... */
                return 0;

            switch ( Fl::event_key() )
            {
                case FL_Delete:
                case FL_Home:
                case FL_End:
                    /* keep scrollbar from eating these. */
                    return 0;
                default:
                    return BASE::handle( m );
            }

            return 0;
        }
        default:
        {
            int r = BASE::handle( m );

            if ( m != FL_RELEASE && r )
                return r;

            const int X = Fl::event_x();
            const int Y = Fl::event_y();

            switch ( m )
            {
                case FL_PUSH:
                {
                    if (
                        Fl::event_x() >= Track::width() &&
                        ( test_press( FL_BUTTON1 ) || test_press( FL_BUTTON1 + FL_CTRL ) ))
                    {
                        assert( ! drag );
                        
                        drag = new Drag( X, Y );
                        _selection.x = X;
                        _selection.y = Y;
                        
                        if ( ! Fl::event_ctrl() )
                            select_none();
                        
                        return 1;
                    }
                    else if ( test_press( FL_BUTTON3 ) )
                    {
                        menu_popup( menu );
                        
                        return 1;
                    }
                }
                
                return 0;
            
            case FL_DRAG:
            {
                int ox = X - drag->x;
                int oy = Y - drag->y;

                if ( ox < 0 )
                    _selection.x = X;
                if ( oy < 0 )
                    _selection.y = Y;

                _selection.w = abs( ox );
                _selection.h = abs( oy );

                if ( range )
                {
                    range_start( x_to_offset( _selection.x ) );
                    range_end( x_to_offset( _selection.x + _selection.w ) );
                    redraw();
                }

                redraw_overlay();
                return 1;

                break;
            }
            case FL_RELEASE:
            {
                delete drag;
                drag = NULL;

                if ( range )
                {
                    range_start( x_to_offset( _selection.x ) );
                    range_end( x_to_offset( _selection.x + _selection.w ) );
                    redraw();
                }
                else
                    select( _selection );

                _selection.x = _selection.y =_selection.w = _selection.h = 0;

                redraw_overlay();
                return 1;
            }
            default:
                return 0;
                break;
        }

        return 0;
    }
}
}

/** retrun a pointer to the track named /name/, or NULL if no track is named /name/ */
Track *
Timeline::track_by_name ( const char *name )
{
    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( name && t->name() && ! strcmp( name, t->name() ) )
            return t;
    }

    return NULL;
}

/** return a malloc'd string representing a unique name for a new track */
char *
Timeline::get_unique_track_name ( const char *name )
{
    char pat[256];

    strcpy( pat, name );

    for ( int i = 1; track_by_name( pat ); ++i )
        snprintf( pat, sizeof( pat ), "%s.%d", name, i );

    return strdup( pat );
}

/**********/
/* Public */
/**********/

/** return the current length of the timeline, which is arrived at by
 * calculating the end frame of the rightmost audio region on an
 * active audio sequence. Control_Points, etc. do not factor into this
 * calcaulation. */
nframes_t
Timeline::length ( void ) const
{
    nframes_t l = 0;

    for ( int i = tracks->children(); i--; )
    {
        Sequence *t = ((Track*)tracks->child( i ))->sequence();
        
        if ( t )
            l = max( l, ((Track*)tracks->child( i ))->sequence()->length() );
    }

    return l;
}

/** set horizontal scroll postion to absolute pixel coordinate /X/ */
void
Timeline::xposition ( int X )
{
    xoffset = x_to_ts( X );

    int dx = ts_to_x( _old_xposition ) - ts_to_x( xoffset );

    if ( dx )
        damage( FL_DAMAGE_SCROLL );
}

/** zoom in by one zoom step */
void
Timeline::zoom_in ( void )
{
    panzoomer->zoom_in();
}

/** zoom out by one zoom step */
void
Timeline::zoom_out ( void )
{
    panzoomer->zoom_out();
}

/** zoom the display to show /secs/ seconds per screen */
void
Timeline::zoom ( float secs )
{
    const int sw = tracks->w() - Track::width();

    int fpp = (int)((secs * sample_rate()) / sw);

    int p = 0;
    while ( 1 << p < fpp ) p++;

    panzoomer->zoom( p );

    redraw();
}

/** fit the zoom to the current length of the timeline (subject to nearest power of two) */
void
Timeline::zoom_fit ( void )
{
    xposition( 0 );

    if ( length() )
        zoom( length() / (float)sample_rate() );
    else
        zoom( 60 );
}

/** add /track/ to the timeline */
void
Timeline::add_track ( Track *track )
{
    DMESSAGE( "added new track to the timeline" );

    tracks->add( track );

    /* FIXME: why is this necessary? doesn't the above add do DAMAGE_CHILD? */
    redraw();

}

void
Timeline::insert_track ( Track *track, Track *before )
{
    tracks->insert( *track, before );

    tracks->redraw();
}

void
Timeline::insert_track ( Track *track, int n )
{
    if ( n > tracks->children() || n < 0 )
        return;

    tracks->insert( *track, n );

    tracks->redraw();

    /* FIXME: why is this necessary? doesn't the above add do DAMAGE_CHILD? */
//    redraw();    
}

static 
bool
compare_tracks ( Track *a, Track *b )
{
    return *a < *b;
}

void
Timeline::apply_track_order ( void )
{
    /* wrlock(); */

    std::list<Track*> tl;
    
    for ( int i = 0; i < tracks->children(); i++ )
        tl.push_back( (Track*)tracks->child( i ) );

    tl.sort(compare_tracks);

    Fl_Widget **a = const_cast<Fl_Widget**>(tracks->array());
    
    int j = 0;
    for ( std::list<Track*>::const_iterator i = tl.begin();
          i != tl.end();
          i++, j++ )
        a[j] = *i;

    update_track_order();

    /* unlock(); */
}

void
Timeline::update_track_order ( void )
{
    for ( int i = 0; i < tracks->children(); i++ )
        ((Track*)tracks->child( i ))->row( i );
}

int
Timeline::find_track ( const Track *track ) const
{
    return tracks->find( *track );
}


/** remove /track/ from the timeline */
void
Timeline::remove_track ( Track *track )
{
    DMESSAGE( "removed track from the timeline" );

    /* FIXME: what to do about track contents? */
    tracks->remove( track );

    /* FIXME: why is this necessary? doesn't the above add do DAMAGE_CHILD? */
    redraw();
}

/************/
/* Commands */
/************/

void
Timeline::command_move_track_up ( Track *track )
{
    wrlock();
    insert_track( track, find_track( track ) - 1 );
    unlock();
}

void
Timeline::command_move_track_down ( Track *track )
{
    wrlock();
    insert_track( track, find_track( track ) + 2 );
    unlock();
}

void
Timeline::command_remove_track ( Track *track )
{
    wrlock();
    remove_track(track);
    unlock();
}

void
Timeline::command_quit ( void )
{
    timeline->wrlock();

    Project::close();

    timeline->unlock();
  
    command_save();
  
    while ( Fl::first_window() ) Fl::first_window()->hide();
}

void
Timeline::command_undo ( void )
{
    wrlock();
    Project::undo();
    unlock();
}

bool
Timeline::command_load ( const char *name, const char *display_name )
{
    if ( ! name )
        return false;
  
    wrlock();
    int r = Project::open( name );
    unlock();

    if ( r < 0 )
    {
  	const char *s = Project::errstr( r );
  	
  	fl_alert( "Could not open project \"%s\":\n\n\t%s", name, s );

        return false;
    }

    Project::set_name ( display_name ? display_name : name );

    apply_track_order();
  
    return true;
}

bool
Timeline::command_save ( )
{
    tle->save_options();

    return true;
}

bool
Timeline::command_new ( const char *name, const char *display_name )
{
    bool b = Project::create( name, NULL );

    Project::set_name ( display_name );

    /* FIXME: there's other stuff that needs to be done here! */
    /* tle->update_menu(); */

    /* tle->main_window->redraw(); */
    
    return b;
}

const char *
Timeline::session_manager_name ( void )
{
    return nsm_get_session_manager_name( nsm );
}


/*******/
/* OSC */
/*******/

int
Timeline::init_osc ( const char *osc_port )
{
    osc = new OSC::Endpoint();
    
    if ( int r = osc->init( LO_UDP, osc_port ) )
        return r;
    
    osc->owner = this;
    
    printf( "OSC=%s\n", osc->url() );
    
    osc->add_method( "/non/hello", "ssss", &Timeline::osc_non_hello, osc, "" );
    
//    osc->start();
    
    if ( ! osc_thread )
    {
        osc_thread = new OSC_Thread();
        
        osc_thread->start();
    }

    return 0;
}

int
Timeline::osc_non_hello ( const char *, const char *, lo_arg **, int , lo_message msg, void * )
{
    THREAD_ASSERT( OSC );

    timeline->handle_hello(msg);
    return 0;
}

void
Timeline::handle_hello ( lo_message msg )
{
    int argc = lo_message_get_argc( msg );
    lo_arg **argv = lo_message_get_argv( msg );

    if ( argc >= 4 )
    {
        const char *url = &argv[0]->s;
        const char *name = &argv[1]->s;
        const char *version = &argv[2]->s;
        const char *id = &argv[3]->s;

        MESSAGE( "Got hello from NON peer %s (%s) @ %s with ID \"%s\"", name, version, url, id );
        
        osc->handle_hello( &argv[3]->s, &argv[0]->s );
    }
}

void
Timeline::say_hello ( void )
{
    if ( nsm_is_active( nsm ) )
    {
        lo_message m = lo_message_new();
        
        lo_message_add( m, "sssss",
                        "/non/hello",
                        osc->url(),
                        APP_NAME,
                        VERSION,
                        instance_name );
        
        nsm_send_broadcast( nsm, m );
    }
}

void
Timeline::connect_osc ( void )
{
    /* reconnect OSC signals */
    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );
        
        t->connect_osc();
    }
}
void
Timeline::update_osc_connection_state ( void )
{
    /* reconnect OSC signals */
    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );
        
        t->update_osc_connection_state();
    }
}

/* runs in the OSC thread... */
void
Timeline::process_osc ( void )
{
    THREAD_ASSERT( OSC );

    /* rdlock(); */

    /* reconnect OSC signals */
    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );
        
        t->process_osc();
    }

    /* unlock(); */
}

