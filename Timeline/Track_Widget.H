
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

#include "Track.H"
#include "Loggable.H"
#include "Timeline.H"
#include <list>
#include <algorithm>
using namespace std;

class Track_Widget;

struct Drag
{
    /* mouse coords at start of drag */
    int x;
    int y;
    int state;

    Track_Widget *original;

    Drag( int X, int Y ) : x( X ), y( Y ) { state = 0; }
};

struct Range
{
    nframes_t offset;                      /* where on the timeline */
    nframes_t start;                      /* first sample from clip */
    nframes_t end;                         /* last sample from clip */
};

/* Base class for virtual widget on a track */
class Track_Widget : public Loggable
{

    static list <Track_Widget *> _selection;                    /* all the widgets making up the selection */

    /* FIXME: is this not the same as /pushed/? */
    static Track_Widget * _current;                             /* the widget initiating events that affect the selection */

    /* these are actually managed in the Track classes */
    static Track_Widget * _pushed;                              /* the widget receiving drag events (a copy) */
    static Track_Widget * _original;                            /* the original of the /pushed/ widget */
    static Track_Widget * _belowmouse;                          /* the widget below the mouse cursor */

    /* can't have this */
    Track_Widget ( const Track_Widget &rhs );

protected:

    Track *_track;                                              /* track this region belongs to */

    Range _range;                                               /* range for playback */
    Range *_r;                                                  /* range for editing / display (points to the same thing as above, except for when dragging etc) */

    Fl_Color _color;                                            /* color of waveform */
    Fl_Color _box_color;                                        /* color of background (box) */

    bool _shown;

    Drag *_drag;

public:

    Track_Widget ( )
        {
            _track = NULL;

            _r = &_range;

            _r->offset = _r->start = _r->end = 0;

            _shown = true;

            _drag = NULL;
        }

    virtual ~Track_Widget ( )
        {
            redraw();

            _track->remove( this );

            _selection.remove( this );
        }

    const Track_Widget &
    operator= ( const Track_Widget &rhs )
        {
            if ( this == &rhs )
                return *this;

            _r         = &_range;
            _range     = rhs._range;
            _track     = rhs._track;
            _box_color = rhs._box_color;
            _color     = rhs._color;

            return *this;
        }


/*     Track_Widget ( const Track_Widget &rhs ) */
/*         { */
/*             *this = rhs; */
/*         } */

    virtual Track_Widget *clone ( const Track_Widget *r ) = 0;

    bool selected ( void )
        {
            return ::find( _selection.begin(), _selection.end(), this ) != _selection.end();
        }

    void select ( void )
        {
            if ( selected() )
                return;

            _selection.push_back( this );
            _selection.sort( sort_func );

            redraw();
        }

    void deselect ( void )
        {
            _selection.remove( this );
            redraw();
        }


    static void
    delete_selected ( void )
        {
            while ( _selection.size() )
                delete _selection.front();
        }

    static Track_Widget *current    ( void ) { return Track_Widget::_current; }

    static Track_Widget *pushed     ( void ) { return Track_Widget::_pushed; }
    static Track_Widget *belowmouse ( void ) { return Track_Widget::_belowmouse; }

    static void pushed     ( Track_Widget *w ) { Track_Widget::_pushed     = w; }
    static void belowmouse ( Track_Widget *w ) { Track_Widget::_belowmouse = w; }

//    static void pushed ( Track_Widget *w ) { Track_Widget::_pushed = w; }

    bool shown ( void ) const { return _shown; }
    void show ( void ) { _shown = true; }
    void hide ( void ) { _shown = false; }

    void begin_drag ( const Drag &d )
        {
            _drag = new Drag( d );
            _r = new Range( _range );
        }

    void end_drag ( void )
        {
            _range = *_r;
            delete _r;
            _r = &_range;

            delete _drag;
            _drag = NULL;
        }

    void
    offset ( nframes_t where )
        {

            if  ( ! selected() )
            {
                redraw();
                _r->offset = where;
            }
            else
            {
                long d = where - _r->offset;

                for ( list <Track_Widget *>::iterator i = _selection.begin(); i != _selection.end(); i++ )
                {
                    (*i)->redraw();

                    if ( d < 0 )
                        (*i)->_r->offset -= 0 - d;
                    else
                        (*i)->_r->offset += d;

                }
            }
        }


    int dispatch ( int m );

    Fl_Widget * parent ( void ) const { return _track; }

    int scroll_x ( void ) const { return timeline->ts_to_x( timeline->xoffset ); }
    nframes_t scroll_ts ( void ) const { return timeline->xoffset; }

    virtual int y ( void ) const { return _track->y(); }
    virtual int h ( void ) const { return _track->h(); }

    /* used by regions */
    virtual int x ( void ) const { return _r->offset < timeline->xoffset ? _track->x() - 1 : min( 32767, _track->x() + timeline->ts_to_x( _r->offset - timeline->xoffset ) ); }

    /* use this as x() when you need to draw lines between widgets */
    int line_x ( void ) const
        {
            return _r->offset < timeline->xoffset ? max( -32768, _track->x() -  timeline->ts_to_x( timeline->xoffset - _r->offset )) : min( 32767, _track->x() + timeline->ts_to_x( _r->offset - timeline->xoffset ) );
        }

    virtual int w ( void ) const
        {
            int tx = timeline->ts_to_x( _r->offset );

            int rw;
            if ( tx < scroll_x() )
                rw = abs_w() - (scroll_x() - tx);
            else
                rw = abs_w();

            return min( rw, _track->w() );
        }

    int abs_x ( void ) const { return timeline->ts_to_x( _r->offset ); }
    virtual int abs_w ( void ) const { return timeline->ts_to_x( _r->end - _r->start ); }

    Fl_Color color ( void ) { return _color; }
    void color ( Fl_Color v ) { _color = v; }
    Fl_Color box_color ( void ) { return _box_color; }

    Track * track ( void ) const { return _track; }
    void track ( Track *t ) {  _track = t; }

    nframes_t offset ( void ) const { return _r->offset; }
//    void offset ( nframes_t o ) { _r->offset = o; }

    void end ( nframes_t v ) { _r->end = v; }
    nframes_t end ( void ) const { return _r->end; }
    void start ( nframes_t v ) { _r->start = v; }
    nframes_t start ( void ) const { return _r->start; }

    virtual nframes_t length ( void ) const { return _r->end - _r->start; }

    virtual Fl_Boxtype box ( void ) const { return FL_UP_BOX; }
    virtual Fl_Align align ( void ) const { return (Fl_Align)0; }

    virtual void
    redraw ( void )
        {
            if ( ! (align() & FL_ALIGN_INSIDE) )
            {
                // FIXME: to better..
                _track->redraw();
            }
            else
                _track->damage( FL_DAMAGE_EXPOSE, x(), y(), w(), h() );
        }

    /* just draw a simple box */
    virtual void
    draw_box ( int X, int Y, int W, int H )
        {
            if ( x() > X + W || x() + w() < X )
                return;

            fl_draw_box( box(), x(), y(), w(), h(), _box_color );
        }

    virtual void
    draw ( int X, int Y, int W, int H )
        {
            if ( x() > X + W || x() + w() < X )
                return;

            draw_box( X, Y, W, H );
        }

    bool
    operator< ( const Track_Widget & rhs )
        {
            return _r->offset < rhs._r->offset;
        }

    bool
    operator<=( const Track_Widget & rhs )
        {
            return _r->offset <= rhs._r->offset;
        }

    virtual void draw_label ( const char *label, Fl_Align align, Fl_Color color=(Fl_Color)0 );
    virtual int handle ( int m );

    static bool
    sort_func ( Track_Widget *lhs, Track_Widget *rhs )
        {
            return *lhs < *rhs;
        }

};
