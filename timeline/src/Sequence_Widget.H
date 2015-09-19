
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

#include "Sequence.H"
#include "Loggable.H"
#include "Timeline.H"
#include <list>
#include <algorithm>
using std::min;
using std::max;

class Sequence_Widget;

struct Drag
{
    /* mouse coords at offset of drag */
    int x;
    int y;
    int state;

//    nframes_t start;
    int64_t offset;

    Drag( int X, int Y, uint64_t offset=0 ) : x( X ), y( Y ), offset( offset ) { state = 0; }
};

/* most common position description. /offset/ is only used by Regions,
   but it's more convenient to have it here  */
struct Range
{
    nframes_t start;                       /* where on the timeline */
    nframes_t offset;                      /* first sample from clip */
    nframes_t length;                      /* total number of samples */

    void
    set_left ( nframes_t f )
        {
            offset += f - start;
            length -= f - start;
            start = f;
        }

    void
    set_right ( nframes_t f )
        {
            length = f - start;
        }

    Range ( ) : start( 0 ), offset( 0 ), length( 0 )
        {
        }
};

/* Used by time/tempo points or any other child of Sequence_Widget
   which must be locked to a point in musical time rather than wallclock
   time. Bar and beat start at 1. */
struct BBT
{
    unsigned short bar;
    unsigned char beat;
    unsigned short tick;

    BBT ( ) : bar( 0 ), beat( 0 ), tick( 0 )
        {
        }
};


/* FIXME: wrong place for this */
struct position_info
{
    nframes_t frame;

    float tempo;
    int beats_per_bar;
    int beat_type;

    BBT bbt;
};

#define SEQUENCE_WIDGET_CLONE_FUNC(class)               \
    virtual Sequence_Widget *clone ( void ) const       \
    {                                                   \
        return new class ( *this );                     \
    }


/* Base class for virtual widget on a track */
class Sequence_Widget : public Loggable
{

    static std::list <Sequence_Widget *> _selection;                    /* all the widgets making up the selection */

    /* FIXME: is this not the same as /pushed/? */
    static Sequence_Widget * _current;                             /* the widget initiating events that affect the selection */

    /* these are actually managed in the Sequence classes */
    static Sequence_Widget * _pushed;                              /* the widget receiving drag events (a copy) */
    static Sequence_Widget * _original;                            /* the original of the /pushed/ widget */
    static Sequence_Widget * _belowmouse;                          /* the widget below the mouse cursor */


    static Fl_Color _selection_color;


protected:
    
    char *_label;

    Sequence *_sequence;                                              /* track this region belongs to */
   
    Range _range;                                      /* always used by RT/osc threads */
    Range _dragging_range;                                          /* used when dragging */

    Range *_r;                                                  /* range for editing / display (points to the same thing as above, except for when dragging etc) */

    Fl_Color _color;                                            /* color of waveform */
    Fl_Color _box_color;                                        /* color of background (box) */

    Drag *_drag;

    virtual void get ( Log_Entry &e ) const;
    virtual void set ( Log_Entry &e );

    Sequence_Widget ( const Sequence_Widget &rhs );
    Sequence_Widget ( );

    const Sequence_Widget &
    operator= ( const Sequence_Widget &rhs );

public:


    virtual ~Sequence_Widget ( );

    virtual Sequence_Widget *clone ( void ) const = 0;

    bool selected ( void ) const;
    static int nselected ( void )
        { return _selection.size(); }
    void select ( void );
    void deselect ( void );
    void remove ( void );

    static void delete_selected ( void );
    static void select_none ( void );

    static Sequence_Widget *current    ( void ) { return Sequence_Widget::_current; }
    static Sequence_Widget *pushed     ( void ) { return Sequence_Widget::_pushed; }
    static Sequence_Widget *belowmouse ( void ) { return Sequence_Widget::_belowmouse; }

    static void pushed     ( Sequence_Widget *w ) { Sequence_Widget::_pushed     = w; }
    static void belowmouse ( Sequence_Widget *w ) { Sequence_Widget::_belowmouse = w; }

    void begin_drag ( const Drag &d );
    void end_drag ( void );

    int dispatch ( int m );

    Fl_Widget * parent ( void ) const { return _sequence; }

    int scroll_x ( void ) const { return timeline->ts_to_x( timeline->xoffset ); }
    nframes_t scroll_ts ( void ) const { return timeline->xoffset; }

    virtual int y ( void ) const { return _sequence->y(); }
    virtual int h ( void ) const { return _sequence->h(); }

    /* used by regions */
    
    int get_x( nframes_t frame ) const
        {
            return  frame < timeline->xoffset ?
                _sequence->drawable_x() - 10 : 
                min( _sequence->drawable_x() + _sequence->drawable_w(),
                     _sequence->drawable_x() + timeline->ts_to_x( frame - timeline->xoffset ) );
        }

    virtual int x ( void ) const
        {
            return get_x( _r->start );
        }

    /* use this as x() when you need to draw lines between widgets. Clamped to -32767-32767 to match max canvas size of FLTK/NTK  */
    int line_x ( void ) const
        {
            return _r->start < timeline->xoffset ? 
                max( -32767, _sequence->drawable_x() - timeline->ts_to_x( timeline->xoffset - _r->start )) :
                min( 32767, _sequence->drawable_x() + timeline->ts_to_x( _r->start - timeline->xoffset  ));
        }

    /* same as above but un-clamped for cairo */
    long curve_x ( void ) const
        {
            return _r->start < timeline->xoffset ? 
                _sequence->drawable_x() - timeline->ts_to_x( timeline->xoffset - _r->start ) :
                _sequence->drawable_x() + timeline->ts_to_x( _r->start - timeline->xoffset  );
        }

    virtual int w ( void ) const
        {
//            int tx = timeline->ts_to_x( _r->start );

            long rw;

            if ( _r->start < timeline->xoffset )
            {
                if ( _r->start + _r->length < timeline->xoffset )
                    rw = 0;
                else
                    rw = timeline->ts_to_x( ( _r->start + _r->length ) - timeline->xoffset ) + 10;
            }
            else
                rw = abs_w();

            return (int)min( rw, (long)(_sequence->drawable_w()) + 20  );
        }

    int abs_x ( void ) const { return timeline->ts_to_x( _r->start ); }
    virtual long abs_w ( void ) const { return timeline->ts_to_x( _r->length ); }

    Fl_Color color ( void ) const { return _color; }
    void color ( Fl_Color v ) { _color = v; }
    virtual Fl_Color box_color ( void ) const { return _box_color; }
    virtual void box_color ( Fl_Color v ) { _box_color = v; }
    virtual Fl_Color selection_color ( void ) const { return _selection_color; }
    virtual void selection_color ( Fl_Color v ) { _selection_color = v; }
    virtual Fl_Color actual_box_color ( void ) const { return box_color(); }

    Sequence * sequence ( void ) const { return _sequence; }
    void sequence ( Sequence *t ) { _sequence = t; }

    nframes_t start ( void ) const { return _r->start; }

/*     void start ( nframes_t o ) { _r->start = o; } */

    void start ( nframes_t where );
    void length ( nframes_t v ) { _r->length = v; }
    virtual nframes_t length ( void ) const { return _r->length; }
    void offset ( nframes_t v ) { _r->offset = v; }
    nframes_t offset ( void ) const { return _r->offset; }

    void set_left ( nframes_t v ) { _r->set_left( v ); }
    void set_right ( nframes_t v ) { _r->set_right( v ); }

    const char *label ( void ) const { return _label; }
    void label ( const char *s )
        {
            if ( _label )
                free( _label );
            
            _label = NULL;
            
            if ( s )
                _label = strdup( s );
            
            redraw();
        }

    /** convert a screen x coord into an start into the region */
    nframes_t x_to_offset ( int X )
        {
            return timeline->x_to_ts( scroll_x() + ( X - _sequence->drawable_x() ) ) - _r->start;
        }

    int active_r ( void ) const { return _sequence->active_r(); }

/** returns true if widget /w/ begins and ends completely within the range of this widget */
    bool contains ( const Sequence_Widget *w ) const
        {
            return w->start() >= start() && w->start() + w->length() <= start() + length();
        }

/** returns true of widget /w/ overlaps this widget in any place */
    bool overlaps ( const Sequence_Widget *w ) const
        {
            return ! ( w->start() > start() + length() || w->start() + w->length() < start() );
        }

    virtual Fl_Boxtype box ( void ) const { return FL_UP_BOX; }
    virtual Fl_Align align ( void ) const { return (Fl_Align)0; }

    virtual void
    redraw ( void )
        {
            if ( ! _sequence )
                return;

            if ( ! (align() & FL_ALIGN_INSIDE) )
            {
                // FIXME: do better..
                _sequence->damage( FL_DAMAGE_USER1 );
            }
            else
                _sequence->damage( FL_DAMAGE_USER1, x(), y(), w(), h() );
        }

    virtual void draw_box ( void );
    virtual void draw ( void );
    virtual void draw_label ( void );
    virtual void draw_label ( const char *label, Fl_Align align, Fl_Color color=(Fl_Color)0, int xo=0, int yo=0 );

    bool
    operator< ( const Sequence_Widget & rhs ) const
        {
            return _r->start < rhs._r->start;
        }

    bool
    operator<=( const Sequence_Widget & rhs ) const
        {
            return _r->start <= rhs._r->start;
        }

    virtual int handle ( int m );

    static bool
    sort_func ( const Sequence_Widget *lhs, const Sequence_Widget *rhs )
        {
            return *lhs < *rhs;
        }

};
