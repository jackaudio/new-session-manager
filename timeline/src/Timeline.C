
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

#include "Timeline.H"
#include "Tempo_Sequence.H"
#include "Time_Sequence.H"
#include "Audio_Sequence.H"
#include "Control_Sequence.H"
#include "Scalebar.H"
#include "Sequence.H"
#include "Annotation_Sequence.H"
#include "Track.H"
#include "Transport.H"

#include "Engine/Engine.H" // for lock()

#include "FL/menu_popup.H"

#include "const.h"
#include "debug.h"

/* these headers are just for the NSM support */
#include "Project.H"
#include "TLE.H"
/*  */

#include "OSC_Thread.H"
#include "OSC/Endpoint.H"

#include <nsm.h>
extern nsm_client_t *nsm;

#ifdef USE_WIDGET_FOR_TIMELINE
#define BASE Fl_Group
#define redraw_overlay() ((Fl_Overlay_Window*)window())->redraw_overlay()
#define BX this->x()
#define BY this->y()
#else
#ifdef USE_SINGLEBUFFERED_TIMELINE
#warning Using singlebuffered timeline window. This may cause flicker and makes the cursors invisible.
#define BASE Fl_Single_Window
#define redraw_overlay()
#else
#define BASE Fl_Overlay_Window
#endif
#define BX 0
#define BY 0
#endif



bool Timeline::draw_with_measure_lines = true;
Timeline::snap_e Timeline::snap_to = Bars;
bool Timeline::snapping_on_hold = false;
bool Timeline::snap_magnetic = true;
bool Timeline::follow_playhead = true;
bool Timeline::center_playhead = true;

const float UPDATE_FREQ = 1.0 / 18.0f;

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



/** callback used by Loggable class to create a snapshot of system
 * state. */
void
Timeline::snapshot ( void )
{
    tempo_track->log_children();
    time_track->log_children();

    for ( int i = 0; i < tracks->children(); ++i )
    {
        ((Track*)tracks->child( i ))->log_children();
    }
}

/** recalculate the size of vertical scrolling area and inform scrollbar */
void
Timeline::adjust_vscroll ( void )
{
    vscroll->value( _yposition, h() - rulers->h() - hscroll->h(), 0, pack_visible_height( tracks ) );
}

/** recalculate the size of horizontal scrolling area and inform scrollbar */
void
Timeline::adjust_hscroll ( void )
{
    hscroll->value( ts_to_x( xoffset ), tracks->w() - Track::width(), 0, ts_to_x( length() ) );
}

void
Timeline::cb_scroll ( Fl_Widget *w, void *v )
{
    ((Timeline*)v)->cb_scroll( w );
}

void
Timeline::cb_scroll ( Fl_Widget *w )
{
    if ( w == vscroll )
    {
        tracks->position( tracks->x(), (rulers->y() + rulers->h()) - vscroll->value() );

        yposition( vscroll->value() );

        adjust_vscroll();
    }
    else
    {
        if ( hscroll->zoom_changed() )
        {

            nframes_t under_mouse = x_to_offset( Fl::event_x() );

            _fpp = hscroll->zoom();

            const int tw = tracks->w() - Track::width();
//            hscroll->value( ts_to_x( xoffset ), tw, 0, ts_to_x( length() ) );

            hscroll->value( max( 0, ts_to_x( under_mouse ) - ( Fl::event_x() - tracks->x() - Track::width() ) ),
                            tw, 0, ts_to_x( length() ) );

            redraw();
        }

        xposition( hscroll->value() );
    }
}

void
Timeline::menu_cb ( Fl_Widget *w, void *v )
{
    ((Timeline*)v)->menu_cb( (Fl_Menu_*)w );
}

/** ensure that p1 is less than p2 */
void
Timeline::fix_range ( void )
{
    if ( p1 > p2 )
    {
        nframes_t t = p2;
        p2 = p1;
        p1 = t;
    }
}

/** set the range to /start/ + /length/ */
void
Timeline::range ( nframes_t start, nframes_t length )
{
    p1 = start;
    p2 = start + length;

    redraw();
}

void
Timeline::menu_cb ( Fl_Menu_ *m )
{
    if ( ! active_r() )
        return;

    const char *picked = m->mvalue()->label();

/*     m->item_pathname( picked, sizeof( picked ) ); */

    DMESSAGE( "%s", picked );

    if ( ! strcmp( picked, "Add Audio Track" ) )
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
    else if ( ! strcmp( picked, "Tempo from range (beat)" ) )
    {
        if ( p1 != p2 )
        {
            fix_range();

            beats_per_minute( p1, sample_rate() * 60 / (float)( p2 - p1 ) );

            p2 = p1;
        }
    }
    else if ( ! strcmp( picked, "Tempo from range (bar)" ) )
    {
        if ( p1 != p2 )
        {
            fix_range();

            position_info pi = solve_tempomap( p1 );

            beats_per_minute( p1, sample_rate() * 60 / (float)( ( p2 - p1 ) / pi.beats_per_bar ) );

            p2 = p1;
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
    else if ( ! strcmp( picked, "P1 to mouse" ) )
    {
        int X = Fl::event_x() - Track::width();

        if ( X > 0 )
        {
            p1 = xoffset + x_to_ts( X );
        }

	fix_range();

        /* FIXME: only needs to damage the location of the old cursor! */
        redraw();
    }
    else if ( ! strcmp( picked, "P2 to mouse" ) )
    {
        int X = Fl::event_x() - Track::width();

        if ( X > 0 )
        {
            p2 = xoffset + x_to_ts( X );
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
    else if ( ! strcmp( picked, "Swap P1 and playhead" ) )
    {
        nframes_t t = transport->frame;

        transport->locate( p1 );

        p1 = t;

        redraw();
    }
    else if ( ! strcmp( picked, "Swap P2 and playhead" ) )
    {
        nframes_t t = transport->frame;

        transport->locate( p2 );

        p2 = t;

        redraw();
    }
    else if ( ! strcmp( picked, "P1 to playhead" ) )
    {
        p1 = transport->frame;

        redraw();
    }
    else if ( ! strcmp( picked, "P2 to playhead" ) )
    {
        p2 = transport->frame;

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

    osc_thread = 0;
    _sample_rate = 44100;

    box( FL_FLAT_BOX );
    xoffset = 0;
    _yposition = 0;
    _old_yposition = 0;
    _old_xposition = 0;

#ifndef USE_WIDGET_FOR_TIMELINE
    X = Y = 0;
#endif

    p1 = p2 = 0;

    menu = new Fl_Menu_Button( 0, 0, 0, 0, "Timeline" );

/*     menu->add( "Add Track", 0, 0, 0  ); */

    menu->add( "Add Audio Track", 'a', 0, 0 );
    menu->add( "Tempo from range (beat)", 't', 0, 0 );
    menu->add( "Tempo from range (bar)", FL_CTRL + 't', 0, 0 );
    menu->add( "Playhead to mouse", 'p', 0, 0 );
    menu->add( "P1 to mouse", '[', 0, 0 );
    menu->add( "P2 to mouse", ']', 0, 0 );
    menu->add( "Playhead left beat", FL_SHIFT + FL_Left, 0, 0 );
    menu->add( "Playhead right beat", FL_SHIFT + FL_Right, 0, 0 );
    menu->add( "Playhead left bar", FL_CTRL + FL_SHIFT + FL_Left, 0, 0 );
    menu->add( "Playhead right bar", FL_CTRL + FL_SHIFT + FL_Right, 0, 0 );
    menu->add( "Swap P1 and playhead", FL_CTRL + FL_SHIFT + '[', 0, 0 );
    menu->add( "Swap P2 and playhead", FL_CTRL + FL_SHIFT + ']', 0, 0 );
    menu->add( "P1 to playhead", FL_CTRL + '[', 0, 0 );
    menu->add( "P2 to playhead", FL_CTRL + ']', 0, 0 );
    menu->add( "Redraw", FL_CTRL + 'l', 0, 0 );

    menu_set_callback( const_cast<Fl_Menu_Item*>(menu->menu()), &Timeline::menu_cb, (void*)this );

    {
        Scalebar *o = new Scalebar( X, Y + H - 18, W - 18, 18 );

        o->range( 0, 48000 * 300 );
//        o->zoom_range( 1, 16384 );
//        o->zoom_range( 1, 65536 << 4 );
        o->zoom_range( 1, 20 );

        o->zoom( 8 );

        o->type( FL_HORIZONTAL );
        o->callback( cb_scroll, this );

        hscroll = o;
    }

    {
        Fl_Scrollbar *o = new Fl_Scrollbar( X + W - 18, Y, 18, H - 18 );

        o->type( FL_VERTICAL );
        o->callback( cb_scroll, this );
        vscroll = o;
    }

    {
        Fl_Pack *o = new Fl_Pack( X + Track::width(), Y, (W - Track::width()) - vscroll->w(), H - hscroll->h(), "rulers" );
        o->type( Fl_Pack::VERTICAL );

        {
            Tempo_Sequence *o = new Tempo_Sequence( 0, 0, 800, 24 );

            o->color( fl_gray_ramp( 18 ) );

            o->label( "Tempo" );
            o->align( FL_ALIGN_LEFT );

            tempo_track = o;
        }

        {
            Time_Sequence *o = new Time_Sequence( 0, 24, 800, 24 );

            o->color( fl_gray_ramp( 16 ) );

            o->label( "Time" );
            o->align( FL_ALIGN_LEFT );

            time_track = o;
        }


/*         { */
/*             Annotation_Sequence *o = new Annotation_Sequence( 0, 24, 800, 24 ); */

/*             o->color( fl_gray_ramp( 'F' ) ); */

/*             o->label( "Ruler" ); */
/*             o->align( FL_ALIGN_LEFT ); */

/*             ruler_track = o; */
/*         } */

        o->size( o->w(), o->child( 0 )->h() * o->children() );
        rulers = o;
        o->end();
    }


    {
//        sample_rate() = engine->sample_rate();
        _fpp = 8;
//        length() = sample_rate() * 60 * 2;

        /* FIXME: hack */
//        length() = x_to_ts( W );

        {
            Fl_Pack *o = new Fl_Pack( X, rulers->y() + rulers->h(), W - vscroll->w(), 1 );
            o->type( Fl_Pack::VERTICAL );
            o->spacing( 1 );

            tracks = o;
            o->end();
            resizable( o );
        }
    }

    /* rulers go above tracks... */
    add( rulers );

    /* make sure scrollbars are on top */
    add( vscroll );
    add( hscroll );

    vscroll->range( 0, tracks->h() );

    redraw();

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

    nearest_line_arg n = { when, -1, snap && Timeline::Bars == Timeline::snap_to };

    render_tempomap( when > x_to_ts( w() >> 1 ) ? when - x_to_ts( w() >> 1 ) : 0,
                     when + x_to_ts( w() >> 1 ), snap ? nearest_line_snap_cb : nearest_line_cb, &n );

    if ( n.closest == (nframes_t)-1 )
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
    nframes_t when = *frame + 1;

    nearest_line_arg n = { when, -1, bar };

    render_tempomap( when, x_to_ts( w() ), prev_next_line_cb, &n );

    if ( n.closest == (nframes_t)-1 )
        return false;
    else
    {
        *frame = n.closest;
        return true;
    }
}

/** Set the value pointed to by /frame/ to the frame number of the of
    the nearest measure line to *less than* /when/. Returns true if
    the new value of *frame is valid, false otherwise. */
bool
Timeline::prev_line ( nframes_t *frame, bool bar ) const
{
    nframes_t when = *frame - 1;

    nearest_line_arg n = { when, -1, bar };

    render_tempomap( xoffset, when - xoffset, prev_next_line_cb, &n );

    if ( n.closest == (nframes_t)-1 )
        return false;
    else
    {
        *frame = n.closest;
        return true;
    }
}



/** given screen pixel coordinate /x/ return frame offset into
 * timeline, taking into account the current scroll position, widget
 * layout, etc. */
nframes_t
Timeline::x_to_offset ( int x ) const
{
    return x_to_ts( max( 0, x - Track::width() ) ) + xoffset;
}

/** draws a single measure line */
static void
draw_measure_cb ( nframes_t frame, const BBT &bbt, void * )
{
    Fl_Color c = FL_LIGHT3;

    if ( bbt.beat )
        c = FL_DARK1;

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

    render_tempomap( start, length, draw_measure_cb, NULL );

    fl_pop_clip();
}

void
Timeline::draw_clip ( void * v, int X, int Y, int W, int H )
{
    Timeline *tl = (Timeline *)v;

    fl_push_clip( X, Y, W, H );

/*     fl_color( rand() ); */
/*     fl_rectf( X, Y, X + W, Y + H ); */

    tl->draw_box();

    tl->draw_child( *tl->rulers );

    fl_push_clip( tl->tracks->x(), tl->rulers->y() + tl->rulers->h(), tl->tracks->w(), tl->h() - tl->rulers->h() - tl->hscroll->h() );
    tl->draw_child( *tl->tracks );

    fl_pop_clip();

    fl_pop_clip();
}

/** handle resize event */
void
Timeline::resize ( int X, int Y, int W, int H )
{
    BASE::resize( X, Y, W, H );

    /* why is this necessary? */
    rulers->resize( BX + Track::width(), BY, W - Track::width() - vscroll->w(), rulers->h() );

    /* why is THIS necessary? */
    hscroll->resize( BX, BY + H - 18, hscroll->w(), 18 );
    vscroll->size( vscroll->w(), H - 18 );

    tracks->resize( BX, BY + rulers->h(), W - vscroll->w(), H - vscroll->h() );
}

/** draw ancillary cursors (not necessarily in the overlay plane) */
void
Timeline::draw_cursors ( void ) const
{
    if ( p1 != p2 )
    {
        draw_cursor( p1, FL_BLUE, draw_full_arrow_symbol );
        draw_cursor( p2, FL_GREEN, draw_full_arrow_symbol );
    }
}

void
Timeline::draw ( void )
{
    int X, Y, W, H;

    int bdx = 0;
    int bdw = 0;

    X = tracks->x() + bdx + 1;
    Y = tracks->y();
    W = tracks->w() - bdw - 1;
    H = tracks->h();

    adjust_vscroll();

#ifndef USE_UNOPTIMIZED_DRAWING
    if ( ( damage() & FL_DAMAGE_ALL ) )
#else
    #warning Optimized drawing of timeline disabled. This will waste your CPU.
#endif
    {
        DMESSAGE( "complete redraw" );

        draw_box( box(), BX, BY, w(), h(), color() );

        fl_push_clip( BX, rulers->y(), w(), rulers->h() );
        draw_child( *rulers );
        fl_pop_clip();

        fl_push_clip( tracks->x(), rulers->y() + rulers->h(), tracks->w(), hscroll->y() - (rulers->y() + rulers->h()) );
        draw_child( *tracks );

        fl_pop_clip();

        draw_child( *hscroll );
        draw_child( *vscroll );

        draw_cursors();

        redraw_overlay();

        goto done;
    }

    if ( damage() & FL_DAMAGE_SCROLL )
    {
        int dx = ts_to_x( _old_xposition ) - ts_to_x( xoffset );
        int dy = _old_yposition - _yposition;

/*         draw_child( *rulers ); */

        if ( ! dy )
            fl_scroll( rulers->x(), rulers->y(), rulers->w(), rulers->h(), dx, 0, draw_clip, this );

        Y = rulers->y() + rulers->h();
        H = h() - rulers->h() - hscroll->h();

        if ( dy == 0 )
            fl_scroll( X + Track::width(), Y, W - Track::width(), H, dx, dy, draw_clip, this );
        else
            fl_scroll( X, Y, W, H, dx, dy, draw_clip, this );
    }

    if ( damage() & FL_DAMAGE_CHILD )
    {
        fl_push_clip( rulers->x(), rulers->y(), rulers->w(), rulers->h() );
        update_child( *rulers );
        fl_pop_clip();

        if ( ! ( damage() & FL_DAMAGE_SCROLL ) )
        {
            fl_push_clip( tracks->x(), rulers->y() + rulers->h(), tracks->w(), h() - rulers->h() - hscroll->h() );
            update_child( *tracks );
            fl_pop_clip();
        }

        update_child( *hscroll );
        update_child( *vscroll );

        draw_cursors();
    }

done:

    _old_xposition = xoffset;
    _old_yposition = _yposition;


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
    const int h = this->h() - rulers->h()  - hscroll->h();

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
    static nframes_t last_playhead = -1;
    static int last_playhead_x = -1;


    /* FIXME: kind of a hackish way to invoke punch stop from the UI thread... */

    if ( transport->rolling &&
         transport->rec_enabled() &&
         transport->punch_enabled() &&
         transport->frame > range_end() )
        transport->stop();

    int playhead_x = ts_to_x( transport->frame );

    if ( last_playhead_x != playhead_x )
    {
        redraw_overlay();
        last_playhead = transport->frame;
        last_playhead_x = playhead_x;

        if ( follow_playhead )
        {
            if ( center_playhead && active() )
                xposition( max( 0, playhead_x - ( ( tracks->w() - Track::width() ) >> 1 ) ) );
            else if ( playhead_x > ts_to_x( xoffset ) + ( tracks->w() - Track::width() ) )
                xposition( playhead_x );
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
    fl_push_no_clip();

    draw_playhead();

    if ( ! ( _selection.w && _selection.h ) )
    {
        fl_pop_clip();
        return;
    }

    fl_push_clip( tracks->x() + Track::width(), rulers->y() + rulers->h(),  tracks->w() - Track::width(), h() - rulers->h() - hscroll->h() );

    const Rectangle &r = _selection;

    fl_color( FL_MAGENTA );
    fl_line_style( FL_SOLID, 0 );
    fl_rect( r.x, r.y, r.w, r.h );

    fl_pop_clip();

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
        return menu->test_shortcut() || hscroll->handle( m ) || vscroll->handle( m );
    else
        return 0;
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
                    if ( test_press( FL_BUTTON1 ) || test_press( FL_BUTTON1 + FL_CTRL ) )
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

                    return 0;
                }
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
                        p1 = x_to_offset( _selection.x );
                        p2 = x_to_offset( _selection.x + _selection.w );
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
                        p1 = x_to_offset( _selection.x );
                        p2 = x_to_offset( _selection.x + _selection.w );
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

        if ( ! strcmp( name, t->name() ) )
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
        l = max( l, ((Track*)tracks->child( i ))->sequence()->length() );

//    adjust_hscroll();

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

/** set vertical scroll position to absolute pixel coordinate /Y/ */
void
Timeline::yposition ( int Y )
{
    _yposition = Y;

    int dy = _old_yposition - _yposition;

    if ( dy )
        damage( FL_DAMAGE_SCROLL );
}

/** zoom in by one zoom step */
void
Timeline::zoom_in ( void )
{
    hscroll->zoom_in();
}

/** zoom out by one zoom step */
void
Timeline::zoom_out ( void )
{
    hscroll->zoom_out();
}

/** zoom the display to show /secs/ seconds per screen */
void
Timeline::zoom ( float secs )
{
    const int sw = tracks->w() - Track::width();

    int fpp = (int)((secs * sample_rate()) / sw);

    int p = 0;
    while ( 1 << p < fpp ) p++;

    hscroll->zoom( p );

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

    wrlock();

    engine->lock();

    tracks->add( track );

//    update_track_order();

    engine->unlock();

    unlock();

    /* FIXME: why is this necessary? doesn't the above add do DAMAGE_CHILD? */
    redraw();

}

void
Timeline::insert_track ( Track *track, int n )
{
    if ( n > tracks->children() || n < 0 )
        return;

    wrlock();

    engine->lock();

    tracks->insert( *track, n );

    update_track_order();

    tracks->redraw();

    engine->unlock();

    unlock();

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
    wrlock();

    engine->lock();

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

    engine->unlock();
    
    unlock();
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

void
Timeline::move_track_up ( Track *track )
{
    insert_track( track, find_track( track ) - 1 );
}

void
Timeline::move_track_down ( Track *track )
{
    insert_track( track, find_track( track ) + 2 );
}

/** remove /track/ from the timeline */
void
Timeline::remove_track ( Track *track )
{
    DMESSAGE( "removed track from the timeline" );

    wrlock();

    engine->lock();

    /* FIXME: what to do about track contents? */
    tracks->remove( track );

    update_track_order();

    engine->unlock();

    unlock();


    /* FIXME: why is this necessary? doesn't the above add do DAMAGE_CHILD? */
    redraw();
}

/************/
/* Commands */
/************/

void
Timeline::command_quit ( )
{
  Project::close();
  
  command_save();
  
  while ( Fl::first_window() ) Fl::first_window()->hide();
}

bool
Timeline::command_load ( const char *name, const char *display_name )
{
  if ( ! name )
      return false;
  
  int r = Project::open( name );
  
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

const double OSC_INTERVAL = 0.2f;

void
Timeline::check_osc ( void * v )
{
    ((Timeline*)v)->osc->check();
    Fl::repeat_timeout( OSC_INTERVAL, &Timeline::check_osc, v );
}

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
    
    /* poll so we can keep OSC handlers running in the GUI thread and avoid extra sync */
    Fl::add_timeout( OSC_INTERVAL, &Timeline::check_osc, this );

    osc->peer_scan_complete_callback( &Timeline::handle_peer_scan_complete, this );

    if ( ! osc_thread )
    {
        osc_thread = new OSC_Thread();
        
        osc_thread->start();
    }

    return 0;
}

int
Timeline::osc_non_hello ( const char *path, const char *, lo_arg **argv, int argc, lo_message, void * )
{
    OSC_DMSG();

    if ( argc >= 4 )
    {
        const char *url = &argv[0]->s;
        const char *name = &argv[1]->s;
        const char *version = &argv[2]->s;
        const char *id = &argv[3]->s;

        MESSAGE( "Discovered NON peer %s (%s) @ %s with ID \"%s\"", name, version, url, id );
        MESSAGE( "Registering Signals" );

        timeline->osc->hello( url );
        
        return 0;
    }
    
    return -1;
}

void
Timeline::reply_to_finger ( lo_message msg )
{
    int argc = lo_message_get_argc( msg );
    lo_arg **argv = lo_message_get_argv( msg );

    if ( argc < 1 )
        return;

    lo_address reply = lo_address_new_from_url( &argv[0]->s );
    
    osc->send( reply,
              "/non/hello",
              osc->url(),
              APP_NAME,
              VERSION,
              instance_name );

    lo_address_free( reply );
}

void 
Timeline::handle_peer_scan_complete ( void *o )
{
    ((Timeline*)o)->connect_osc();
}

void
Timeline::connect_osc ( void )
{
    /* try to (re)connect OSC signals */
    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );
        
        for ( int j = t->control->children(); j--; )
        {
            Control_Sequence *c = (Control_Sequence*)t->control->child( j );
            c->connect_osc();
        }
    }
}

void
Timeline::discover_peers ( void )
{
    if ( nsm_is_active( nsm ) )
    {
        lo_message m = lo_message_new();
        
        lo_message_add_string( m, "/non/finger" );
        lo_message_add_string( m, osc->url() );

        nsm_send_broadcast( nsm, m );
        
        lo_message_free( m );
    }
}


/* runs in the OSC thread... */
void
Timeline::process_osc ( void )
{
    THREAD_ASSERT( OSC );

    rdlock();

    /* reconnect OSC signals */
    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );
        
        if ( t->control )
        {
            for ( int j = t->control->children(); j--; )
            {
                Control_Sequence *c = (Control_Sequence*)t->control->child( j );
                c->process_osc();
            }
        }
    }
    
    unlock();
}

