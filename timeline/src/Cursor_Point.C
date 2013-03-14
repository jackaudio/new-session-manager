
/*******************************************************************************/
/* Copyright (C) 2012 Jonathan Moore Liles                                     */
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


#include "Cursor_Point.H"
#include "Cursor_Sequence.H"
#include "Timeline.H" // for timeline->time_track

Cursor_Point::Cursor_Point ( )
{
//    timeline->->add( this );
    _label = NULL;
    _type = NULL;
}

Cursor_Point::Cursor_Point ( nframes_t when, const char *type, const char *label ) 
{
//    _make_label();

    _label = NULL;
    _type = NULL;

    this->label( label );
    this->type( type );

    timeline->add_cursor( this );

    start( when );

    log_create();
}

Cursor_Point::Cursor_Point ( const Cursor_Point &rhs ) : Sequence_Point( rhs )
{
    label( rhs.label() );
    type( rhs.type() );

    log_create();
}

Cursor_Point::~Cursor_Point ( )
{
//    sequence()->remove( this );

    log_destroy();

    label(NULL);
    type(NULL);
}



void
Cursor_Point::get ( Log_Entry &e ) const
{
//    Sequence_Point::get( e );

    e.add( ":start", start() );
    e.add( ":label", label() );
    e.add( ":type", type() );
}

void
Cursor_Point::set ( Log_Entry &e )
{

    Sequence_Point::set( e );

    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":label" ) )
            label( v );
        else if ( ! strcmp( s, ":type" ))
        {
            type( v );

            timeline->add_cursor( this );
        }

/*         /\* FIXME: we need to add this to the time track on creation!!! *\/ */
/*         timeline->time_track->add( this ); */

    }

    sequence()->handle_widget_change( start(), length() );

//    _make_label();
}




int
Cursor_Point::handle ( int m )
{
    Logger log( this );

    /* if ( m == FL_PUSH && Fl::event_button3() && ! ( Fl::event_state() & ( FL_ALT | FL_CTRL | FL_SHIFT ) ) ) */
    /* { */

    /*     time_sig t = _time; */

    /*     edit( &t ); */

    /*     time( t.beats_per_bar, t.beat_type ); */

    /*     return 0; */

    /* } */

    return Sequence_Point::handle( m );
}


