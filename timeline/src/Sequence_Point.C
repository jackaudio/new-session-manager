
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

#include "Sequence_Point.H"

#include <FL/fl_draw.H>



Sequence_Point::Sequence_Point ( const Sequence_Point &rhs ) : Sequence_Widget( rhs )
{
    if ( rhs._label )
        _label = strdup( rhs._label );
    else
        _label = 0;
}

Sequence_Point::Sequence_Point ( )
{
    _label = NULL;

    color( FL_CYAN );
}

Sequence_Point::~Sequence_Point ( )
{
    if ( _label )
        free( _label );
}



void
Sequence_Point::get ( Log_Entry &e ) const
{
    Sequence_Widget::get( e );
}

void
Sequence_Point::set ( Log_Entry &e )
{
    Sequence_Widget::set( e );

    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( ":start", s ) )
        {
            sequence()->sort();
        }

    }
}

static void
draw_marker ( Fl_Color c )
{
    fl_color( c );

    fl_begin_polygon();

#define vv(x,y) fl_vertex( x, y );

    vv( 0.0, 0.0 );
    vv( 0.0, 0.6 );
    vv( 0.5, 1.0 );
    vv( 1.0, 0.6 );
    vv( 1.0, 0.0 );
    vv( 0.0, 0.0 );

    fl_end_polygon();

    fl_color( fl_darker( c ) );

    fl_begin_line();

    vv( 0.0, 0.0 );
    vv( 0.0, 0.6 );
    vv( 0.5, 1.0 );
    vv( 1.0, 0.6 );
    vv( 1.0, 0.0 );
    vv( 0.0, 0.0 );

    fl_end_line();
}

void
Sequence_Point::draw_box ( void )
{
    const int X = x() - (abs_w() >> 1);

    const int Y = y();

    fl_push_matrix();
    fl_translate( X, Y );
    fl_scale( abs_w(), h() );

    Fl_Color c = selected() ? selection_color() : color();

    draw_marker( c );

    fl_pop_matrix();
}

void
Sequence_Point::draw ( void )
{
//            Sequence_Widget::draw();

    draw_label( _label, align() );
}
