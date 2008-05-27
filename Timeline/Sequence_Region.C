
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
Sequence_Region::get ( Log_Entry &e ) const
{
    e.add( ":color",  _box_color );
    e.add( ":length", _r->length );

    Sequence_Widget::get( e );
}


void
Sequence_Region::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":color" ) )
            _box_color = (Fl_Color)atoll( v );
        else if ( ! strcmp( s, ":length" ) )
            _r->length = atoll( v );

    }

    Sequence_Widget::set( e );
}


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

    X -= sequence()->x();
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

            if ( td < 0 && _r->offset < (nframes_t)( 0 - td ) )
                td = 0 - _r->offset;

            if ( td > 0 && (nframes_t)td >= _r->length )
                td = _r->length - timeline->x_to_ts( 1 );

//                td = _r->length - timeline->x_to_ts( 1 );

            _r->trim_left( 0 - td );

            nframes_t f;
            /* snap to beat/bar lines */
            if ( timeline->nearest_line( _r->start, &f ) )
                _r->set_left( f );

            break;
        }
        case RIGHT:
        {
            int d = (( abs_x() - scroll_x() ) + abs_w() ) - X;

/*             _track->damage( FL_DAMAGE_EXPOSE, x() + w(), y(), d, h() ); */

            long td = timeline->x_to_ts( d );

//            printf( "%li %li\n", td, _r->length - _r->offset );

            if ( td >= 0 && _r->length < (nframes_t)td )
                td = _r->length - timeline->x_to_ts( 1 );

            _r->trim_right( 0 - td );

            nframes_t f;
            /* snap to beat/bar lines */
            if ( timeline->nearest_line( _r->start + _r->length, &f ) )
                _r->set_right( f );

            break;
        }
        default:
            return;

    }
}

#include "FL/test_press.H"

int
Sequence_Region::handle ( int m )
{
    static enum trim_e trimming;

    static bool copied = false;

    int X = Fl::event_x();
    int Y = Fl::event_y();

    Logger _log( this );

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
            else if ( test_press( FL_BUTTON2 ) )
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
            else if ( test_press( FL_CTRL + FL_BUTTON1 ) )
            {
                /* duplication */
                fl_cursor( FL_CURSOR_MOVE );
                return 1;
            }
            else
                return Sequence_Widget::handle( m );
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
            if ( Fl::event_shift() )
            {
                if ( trimming )
                {
                    trim( trimming, X );
                    return 1;
                }
                else
                    return 0;
            }

            return Sequence_Widget::handle( m );
        }
        default:
            return Sequence_Widget::handle( m );
            break;
    }

    return 0;

}
