
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

#include <algorithm>
using namespace std;

/* Base class for virtual widget on a track */
class Track_Widget : public Loggable
{

protected:

    Track *_track;                                              /* track this region belongs to */

    nframes_t _offset;                                          /* where on the timeline */
    nframes_t _start;                                           /* first sample from clip */
    nframes_t _end;                                             /* last sample from clip */

    bool _selected;

    Fl_Color _color;                                            /* color of waveform */
    Fl_Color _box_color;                                        /* color of background (box) */

public:

    Track_Widget ( )
        {
            _track = NULL;

            _offset = _start = _end = 0;

            _selected = false;
        }

    virtual ~Track_Widget ( )
        {
            redraw();
            _track->remove( this );
        }

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
    void offset ( nframes_t o ) { _offset = o; }

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


/*     virtual void */
/*     dump ( void ) */
/*         { */
/*             printf( "Unknown %p %lu %lu %lu\n", this, _offset, _start, _end ); */
/*         } */

    virtual void
    draw_label ( const char *label, Fl_Align align )
        {
            int X, Y;

            X = x();
            Y = y();

            /* FIXME: why do we have to to this here? why doesn't Fl_Lable::draw take care of this stuff? */
            if ( ! (align & FL_ALIGN_INSIDE) )
            {
                if ( align & FL_ALIGN_RIGHT )
                {
                    X += w();
                    align = (Fl_Align)((align & ~FL_ALIGN_RIGHT) | FL_ALIGN_LEFT);
                }
                if ( align & FL_ALIGN_BOTTOM  )
                {
                    Y += h();
                    align = (Fl_Align)((align & ~FL_ALIGN_BOTTOM) | FL_ALIGN_TOP);
                }
            }

/*             fl_font( FL_HELVETICA, 14 ); */
/*             fl_color( FL_BLACK ); */
/*             fl_draw( label, X + 2, Y + 2, w(), h(), align ); */
/*             fl_color( FL_WHITE ); */
/*             fl_draw( label, X, Y, w(), h(), align ); */

            /* for some reasone FL_SHADOW_LABLE doesn't always use
             * black for the shadow color...so we can't rely on it. */

            Fl_Label lab;

            lab.color = FL_WHITE;
            lab.type = FL_SHADOW_LABEL;
            lab.value = label;
            lab.font = FL_HELVETICA;
            lab.size = 14;

            int W = w();
            int H = h();

            if ( align & FL_ALIGN_INSIDE )
            {
                X += Fl::box_dx( box() );
                Y += Fl::box_dy( box() );
                W -= Fl::box_dw( box() );
                H -= Fl::box_dh( box() );
            }

            if ( align & FL_ALIGN_CLIP ) fl_push_clip( X, Y, W, H );

            int dx = 0;

            if ( abs_x() < scroll_x() )
                dx = min( 32767, scroll_x() - abs_x() );

            lab.draw( X - dx, Y, W, H, align );

            if ( align & FL_ALIGN_CLIP ) fl_pop_clip();

        }

    /* base hanlde just does basic dragging */
    virtual int
    handle ( int m )
        {
            static int ox, oy;
            static bool dragging = false;

            int X = Fl::event_x();
            int Y = Fl::event_y();

            Logger _log( this );

            switch ( m )
            {
                case FL_ENTER:
                    fl_cursor( FL_CURSOR_HAND );
                    return 1;
                case FL_LEAVE:
                    fl_cursor( FL_CURSOR_DEFAULT );
                    return 1;
                case FL_PUSH:
                {
                    ox = x() - X;
                    oy = y() - Y;

                    if ( Fl::event_state() & FL_CTRL &&
                         Fl::event_button() == 3 )
                    {
//                        log_destroy();

                        redraw();
                        _track->queue_delete( this );
                        return 0;
                    }

                    return 1;
                }
                case FL_RELEASE:
                    if ( dragging )
                        _log.release();
                    dragging = false;
                    fl_cursor( FL_CURSOR_DEFAULT );
                    return 1;
                case FL_DRAG:
                {
                    if ( ! dragging )
                    {
                        dragging = true;
                        _log.hold();
                    }

                    redraw();

                    if ( timeline->ts_to_x( timeline->xoffset ) + ox + X > _track->x() )
                    {
                        int nx = (ox + X) - _track->x();

                        _offset = timeline->x_to_ts( nx ) + timeline->xoffset;

                        _track->snap( this );
                    }

                    //                  _track->redraw();
                    fl_cursor( FL_CURSOR_MOVE );


                    if ( X >= _track->x() + _track->w() ||
                         X <= _track->x() )
                    {
                        /* this drag needs to scroll */

                        nframes_t pos = timeline->xoffset;

                        nframes_t d = timeline->x_to_ts( 100 );

                        if ( X <= _track->x() )
                        {

                            if ( pos > d )
                                pos -= d;
                            else
                                pos = 0;
                        }
                        else
                            pos += d;

                        timeline->position( timeline->ts_to_x(  pos ) );
                        _track->redraw();
//                        timeline->redraw();
                    }


                    return 1;
                }
                default:
                    return 0;
            }
        }

    bool
    operator< ( const Track_Widget & rhs )
        {
            return _offset < rhs._offset;
        }
};
