
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

#include "Track_Widget.H"

list <Track_Widget *> Track_Widget::_selection;
Track_Widget * Track_Widget::_current;

void
Track_Widget::draw_label ( const char *label, Fl_Align align, Fl_Color color )
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

    Fl_Label lab;

    lab.color = color;
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

int
Track_Widget::dispatch ( int m )
{
    Track_Widget::_current = this;

    if ( selected() )
    {
        Loggable::block_start();

        int r = 0;


        for ( list <Track_Widget *>::iterator i = _selection.begin(); i != _selection.end(); i++ )
            if ( *i != this )
                r |= (*i)->handle( m );

        r |= handle( m );

        Loggable::block_end();

        return r;
    }
    else
        return handle( m );
}

/* base hanlde just does basic dragging */
int
Track_Widget::handle ( int m )
{
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
            if ( Fl::event_state() & FL_CTRL &&
                 Fl::event_button3() )
            {
                redraw();
                _track->queue_delete( this );
                return 0;
            }
            else
                if ( Fl::event_button1() )
                    return 1;

            return 0;
        }
        case FL_RELEASE:
            if ( _drag )
            {
                _log.release();
                delete _drag;
                _drag = NULL;
            }

            fl_cursor( FL_CURSOR_HAND );
            return 1;
        case FL_DRAG:
        {
            if ( ! _drag )
            {
                _drag = new Drag( x() - X, y() - Y );

                _log.hold();
            }

            const int ox = _drag->x;
// _current->_drag->x;

/*             const int ox = _drag->x; */
/*             const int oy = _drag->y; */

            redraw();

            if ( timeline->ts_to_x( timeline->xoffset ) + ox + X > _track->x() )
            {
                int nx = (ox + X) - _track->x();

                // _offset = timeline->x_to_ts( nx ) + timeline->xoffset;
                offset( timeline->x_to_ts( nx ) + timeline->xoffset );

                if ( Track_Widget::_current == this )
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
