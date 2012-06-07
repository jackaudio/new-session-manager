
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

#include <FL/fl_draw.H>

#include "Control_Point.H"



Control_Point::Control_Point ( Sequence *t, nframes_t when, float y )
{
    _sequence = t;
    _y = y;
    _r->start = when;
    _box_color = FL_WHITE;

    log_create();
}

Control_Point::Control_Point ( const Control_Point &rhs ) : Sequence_Point( rhs )
{
    _y = rhs._y;

    log_create();
}

void
Control_Point::get ( Log_Entry &e ) const
{
    Sequence_Point::get( e );

    e.add( ":y", _y );
}

void
Control_Point::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":y" ) )
            _y = atof( v );

        redraw();

        //          _make_label();
    }

    Sequence_Point::set( e );
}

void
Control_Point::draw_box ( void )
{
    if ( selected() )
    {
        fl_color( selection_color() );
        fl_pie( x(), y(), w(), h(), 0, 360 );
    }

    fl_color( box_color() );

    fl_arc( x(), y(), w(), h(), 0, 360 );

    if ( this == Sequence_Widget::belowmouse() ||
         this == Sequence_Widget::pushed() )
    {
        char val[10];
        snprintf( val, sizeof( val ), "%+.2f", 1.0 - _y * 2 );

        Fl_Align a = 0;

        if ( x() < _sequence->x() + ( _sequence->w() / 2 ) )
            a |= FL_ALIGN_RIGHT;
        else
            a |= FL_ALIGN_LEFT;

        if ( y() < _sequence->y() + ( _sequence->h() / 2 ) )
            a |= FL_ALIGN_BOTTOM;
        else
            a |= FL_ALIGN_TOP;
            
        draw_label( val, a, FL_FOREGROUND_COLOR );
    }
}


int
Control_Point::handle ( int m )
{
    int r = Sequence_Widget::handle( m );

    switch ( m )
    {

        case FL_RELEASE:
            sequence()->sort();
            redraw();
            break;
        case FL_ENTER:
        case FL_LEAVE:
            redraw();
            break;
        case FL_DRAG:
        {
            sequence()->sort();

            if ( nselected() > 1 )
                // only allow horizontal movement when part of a selection...
                break;

            int Y = Fl::event_y() - parent()->y();

            if ( Y >= 0 && Y < parent()->h() )
            {
                _y = (float)Y / parent()->h();
                redraw();
            }

            break;
        }
    }

    return r;
}
