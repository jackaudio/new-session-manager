
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

/* TODO:

   What if we solve the continuous-modification/sync issue by making a
   copy of the 'pushed' widget, and operating on that instead (hiding
   the original), then when the widget is released, copy its data into
   the original?

 */
#include "Sequence_Widget.H"

list <Sequence_Widget *> Sequence_Widget::_selection;
Sequence_Widget * Sequence_Widget::_current = NULL;
Sequence_Widget * Sequence_Widget::_pushed = NULL;
Sequence_Widget * Sequence_Widget::_belowmouse = NULL;
Fl_Color Sequence_Widget::_selection_color = FL_MAGENTA;

void
Sequence_Widget::draw_label ( const char *label, Fl_Align align, Fl_Color color )
{
    int X, Y;

    X = x();
    Y = y();

    /* FIXME: why do we have to to this here? why doesn't Fl_Lable::draw take care of this stuff? */
    if ( ! (align & FL_ALIGN_INSIDE) )
    {
        if ( align & FL_ALIGN_RIGHT )
        {
            X += abs_w();
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
//    lab.type = FL_SHADOW_LABEL;
    lab.type = FL_NORMAL_LABEL;
    lab.value = label;
    lab.font = FL_HELVETICA;
    lab.size = 14;

    int lw = 0, lh = 0;

    fl_font( lab.font, lab.size );
    fl_measure( lab.value, lw, lh );

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

    /* adjust for scrolling */
    if ( abs_x() < scroll_x() )
        dx = min( 32767, scroll_x() - abs_x() );

//        const Fl_Boxtype b = FL_ROUND_UP_BOX;
    const Fl_Boxtype b = FL_ROUNDED_BOX;
    const int bx = Fl::box_dx( b ) + 1;
    const int bw = Fl::box_dw( b ) + 1;

    if ( align & FL_ALIGN_BOTTOM )
        fl_draw_box( b, X - dx - bx, Y + H - lh, lw + bw, lh, FL_GRAY );
    else if ( align == FL_ALIGN_LEFT )
        fl_draw_box( b, X - dx, Y + ((H >> 1) - (lh >> 1)), lw + bw, lh, FL_GRAY );
    else if ( align & FL_ALIGN_TOP )
        fl_draw_box( b, X - dx - bx + ((W >> 1) - (lw >> 1)), Y + ((H >> 1) - (lh >> 1)), lw + bw, lh, FL_GRAY );

//    lab.draw( X - dx, Y, W, H, align );
    fl_color( color );
    fl_draw( label, ( X - dx ) + bx, Y, W, H, align );

    if ( align & FL_ALIGN_CLIP ) fl_pop_clip();

}

int
Sequence_Widget::dispatch ( int m )
{
    Sequence_Widget::_current = this;

    if ( selected() )
    {
        Loggable::block_start();

        int r = 0;


        for ( list <Sequence_Widget *>::iterator i = _selection.begin(); i != _selection.end(); i++ )
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
Sequence_Widget::handle ( int m )
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
            fl_cursor( sequence()->cursor() );
            return 1;
        case FL_PUSH:
        {
            /* deletion */
            if ( Fl::event_state() & FL_CTRL &&
                 Fl::event_button3() )
            {
                redraw();
                sequence()->queue_delete( this );
                return 1;
            }
            else
                if ( Fl::event_button1() )
                    return 1;

            return 0;
        }
        case FL_RELEASE:
            if ( _drag )
            {
                end_drag();
                _log.release();
            }

            fl_cursor( FL_CURSOR_HAND );

            return 1;
        case FL_DRAG:
        {
            if ( ! _drag )
            {
                begin_drag ( Drag( x() - X, y() - Y, x_to_offset( X ) ) );
                _log.hold();
            }

            fl_cursor( FL_CURSOR_MOVE );

            redraw();

            {
                const nframes_t of = timeline->x_to_offset( X );

                if ( of >= _drag->offset )
                {
                    _r->offset = of - _drag->offset;

                    if ( Sequence_Widget::_current == this )
                        sequence()->snap( this );
                }
                else
                    _r->offset = 0;

            }

            if ( X >= sequence()->x() + sequence()->w() ||
                 X <= sequence()->x() )
            {
                /* this drag needs to scroll */

                nframes_t pos = timeline->xoffset;

                nframes_t d = timeline->x_to_ts( 100 );

                if ( X <= sequence()->x() )
                {

                    if ( pos > d )
                        pos -= d;
                    else
                        pos = 0;
                }
                else
                    pos += d;

                timeline->xposition( timeline->ts_to_x(  pos ) );

                /* FIXME: why isn't this enough? */
//                sequence()->redraw();
                timeline->redraw();
            }

            return 1;
        }
        default:
            return 0;
    }
}
