
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

#include "Sequence_Region.H"
#include "Track.H"


void
Sequence_Region::draw_box ( void )
{
    fl_draw_box( box(), x(), y(), w(), h(), box_color() );
}

void
Sequence_Region::draw ( void )
{
}

void
Sequence_Region::trim ( enum trim_e t, int X )
{

    X -= _track->x();
    redraw();

    switch ( t )
    {
        case LEFT:
        {
/*             if ( d < 0 ) */
/* //                _track->damage( FL_DAMAGE_EXPOSE, x() + d, y(), 1 - d, h() ); */
/*                 _track->damage( FL_DAMAGE_EXPOSE, x(), y(), w(), h() ); */
/*             else */
/*                 _track->damage( FL_DAMAGE_EXPOSE, x(), y(), d, h() ); */

            int d = X - ( abs_x() - scroll_x() );

            long td = timeline->x_to_ts( d );

            if ( td < 0 && _r->start < 0 - td )
                td = 0 - _r->start;

            if ( _r->start + td >= _r->end )
                td = (_r->end - _r->start) - timeline->x_to_ts( 1 );

            _r->start += td;
            _r->offset += td;
            break;
        }
        case RIGHT:
        {
            int d = (( abs_x() - scroll_x() ) + abs_w() ) - X;

/*             _track->damage( FL_DAMAGE_EXPOSE, x() + w(), y(), d, h() ); */

            long td = timeline->x_to_ts( d );

//            printf( "%li %li\n", td, _r->end - _r->start );

            if ( td >= 0 && _r->end - _r->start < td )
                _r->end = _r->start + timeline->x_to_ts( 1 );
            else
                _r->end -= td;

            break;
        }
        default:
            return;

    }
}



int
Sequence_Region::handle ( int m )
{
    static int ox, oy;
    static enum trim_e trimming;

    static bool copied = false;
    static nframes_t os;


/*     if ( ! active_r() ) */
/*         return 0; */

//    int X = Fl::event_x() - _track->x();
    int X = Fl::event_x();
    int Y = Fl::event_y();

    int ret;

    Logger _log( this );
//log_r->start();

    switch ( m )
    {
        case FL_PUSH:
        {
            /* trimming */
            if ( Fl::event_shift() && ! Fl::event_ctrl() )
            {
                switch ( Fl::event_button() )
                {
                    case 1:
                        trim( trimming = LEFT, X );
                        begin_drag( Drag( x() - X, y() - Y ) );
                        _log.hold();
                        break;
                    case 3:
                        trim( trimming = RIGHT, X );
                        begin_drag( Drag( x() - X, y() - Y ) );
                        _log.hold();
                        break;
                    default:
                        return 0;
                        break;
                }

                fl_cursor( FL_CURSOR_WE );
                return 1;
            }
            else if ( Fl::test_shortcut( FL_BUTTON3 ) )
            {
                if ( Sequence_Widget::current() == this )
                {
                    if ( selected() )
                        deselect();
                    else
                        select();
                }

                redraw();
                return 1;
            }
            else if ( Fl::test_shortcut( FL_CTRL + FL_BUTTON1 ) )
            {
                /* duplication */
                return 1;
            }
            else
                return Sequence_Widget::handle( m );
            break;
        }
        case FL_RELEASE:
        {
            Sequence_Widget::handle( m );

            copied = false;
            if ( trimming != NO )
                trimming = NO;

            return 1;
        }
        case FL_DRAG:
        {
            if ( ! _drag )
            {
                begin_drag( Drag( x() - X, y() - Y, x_to_offset( X ) ) );
                _log.hold();
            }

            /* trimming */
            if ( Fl::event_state() & FL_SHIFT )
                if ( trimming )
                {
                    trim( trimming, X );
                    return 1;
                }
                else
                    return 0;

            /* track jumping */
            if ( ! selected() )
            {
                if ( Y > y() + h() || Y < y() )
                {
                    printf( "wants to jump tracks\n" );

                    Track *t = timeline->track_under( Y );

                    fl_cursor( (Fl_Cursor)1 );

                    if ( t )
                        t->handle( FL_ENTER );

                    return 0;
                }
            }

            ret = Sequence_Widget::handle( m );
            return ret | 1;
        }
        default:
            return Sequence_Widget::handle( m );
            break;
    }
}
