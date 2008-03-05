
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
#include "Audio_File.H"
#include "Timeline.H"
#include <list>
#include <algorithm>
using namespace std;

struct Drag
{
    /* mouse coords at start of drag */
    int x;
    int y;
    int state;

    Drag( int X, int Y ) : x( X ), y( Y ) { state = 0; }
};

/* Base class for virtual widget on a track */
class Track_Widget : public Loggable
{

    static list <Track_Widget *> _selection;                    /* all the widgets making up the selection */
    static Track_Widget * _current;                             /* the widget initiating events that affect the selection */

protected:

    Track *_track;                                              /* track this region belongs to */

    nframes_t _offset;                                          /* where on the timeline */
    nframes_t _start;                                           /* first sample from clip */
    nframes_t _end;                                             /* last sample from clip */

    Fl_Color _color;                                            /* color of waveform */
    Fl_Color _box_color;                                        /* color of background (box) */


    Drag *_drag;

public:

    Track_Widget ( )
        {
            _track = NULL;

            _offset = _start = _end = 0;

            _drag = NULL;
        }

    virtual ~Track_Widget ( )
        {
            redraw();

            _track->remove( this );

            _selection.remove( this );
        }

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

    static Track_Widget *current ( void ) { return _current; }

    void
    offset ( nframes_t where )
        {

            if  ( ! selected() )
            {
                redraw();
                _offset = where;
            }
            else
            {
                long d = where - _offset;

                for ( list <Track_Widget *>::iterator i = _selection.begin(); i != _selection.end(); i++ )
                {
                    (*i)->redraw();

                    if ( d < 0 )
                        (*i)->_offset -= 0 - d;
                    else
                        (*i)->_offset += d;

                }
            }
        }


    int dispatch ( int m );

    Fl_Group * parent ( void ) const { return _track; }

    int scroll_x ( void ) const { return timeline->ts_to_x( timeline->xoffset ); }
    nframes_t scroll_ts ( void ) const { return timeline->xoffset; }

    int y ( void ) const { return _track->y(); }
    int h ( void ) const { return _track->h(); }

    int x ( void ) const { return _offset < timeline->xoffset ? _track->x() - 1 : min( 32767, _track->x() + timeline->ts_to_x( _offset - timeline->xoffset ) ); }
    virtual int w ( void ) const
        {
            int tx = timeline->ts_to_x( _offset );

            int rw;
            if ( tx < scroll_x() )
                rw = abs_w() - (scroll_x() - tx);
            else
                rw = abs_w();

            return min( rw, _track->w() );
        }

    int abs_x ( void ) const { return timeline->ts_to_x( _offset ); }
    virtual int abs_w ( void ) const { return timeline->ts_to_x( _end - _start ); }

    Fl_Color color ( void ) { return _color; }
    Fl_Color box_color ( void ) { return _box_color; }

    Track * track ( void ) const { return _track; }
    void track ( Track *t ) {  _track = t; }

    nframes_t offset ( void ) const { return _offset; }
//    void offset ( nframes_t o ) { _offset = o; }

    void end ( nframes_t v ) { _end = v; }
    nframes_t end ( void ) const { return _end; }
    void start ( nframes_t v ) { _start = v; }
    nframes_t start ( void ) const { return _start; }

    virtual nframes_t length ( void ) const { return _end - _start; }

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
            fl_draw_box( box(), x(), y(), w(), h(), _box_color );
        }

    virtual void
    draw ( int X, int Y, int W, int H )
        {
            draw_box( X, Y, W, H );
        }

    bool
    operator< ( const Track_Widget & rhs )
        {
            return _offset < rhs._offset;
        }

    virtual void draw_label ( const char *label, Fl_Align align );
    virtual int handle ( int m );

    static bool
    sort_func ( Track_Widget *lhs, Track_Widget *rhs )
        {
            return *lhs < *rhs;
        }

};
