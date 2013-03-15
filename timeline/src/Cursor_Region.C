
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

#include <FL/Fl_Tooltip.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>

#include "Cursor_Region.H"
#include "Cursor_Sequence.H"
#include "Timeline.H"

Fl_Color Cursor_Region::box_color ( void ) const
{
    return ((Cursor_Sequence*)sequence())->cursor_color();
}


void Cursor_Region::box_color ( Fl_Color c )
{
    ((Cursor_Sequence*)sequence())->cursor_color( c );
}



void
Cursor_Region::get ( Log_Entry &e ) const
{
//    Sequence_Region::get( e );
    e.add( ":start", start() );
    e.add( ":length", length() );
    e.add( ":label", label() );
    e.add( ":type", type() );
}

void
Cursor_Region::set ( Log_Entry &e )
{
    Sequence_Region::set( e );

    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":label" ) )
            label( v );
        if ( ! strcmp( s, ":type" ) )
        {
            type( v );
            timeline->add_cursor( this );
        }
    }

//            timeline->redraw();
}

Cursor_Region::Cursor_Region ( nframes_t when, nframes_t length, const char *type, const char *label )
{
    _label = NULL;
    _type = NULL;

    this->label( label );
    this->type( type );

    start( when );
    this->length( length );

    timeline->add_cursor( this );

    log_create();
}

Cursor_Region::Cursor_Region ( const Cursor_Region &rhs ) : Sequence_Region( rhs )
{
    _label = strdup( rhs._label );
    _type = strdup( rhs._type );

    log_create();
}


Cursor_Region::~Cursor_Region ( )
{
//    timeline->cursor_track->remove( this );

    log_destroy();

    label(NULL);
    type(NULL);
}

void
Cursor_Region::draw_box ( void )
{
    Sequence_Region::draw_box();
}

void
Cursor_Region::draw ( void )
{
    draw_label( _label, (Fl_Align)(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_TOP | FL_ALIGN_CLIP ) );
}

#include <FL/Fl_Text_Edit_Window.H>
#include <FL/test_press.H>

int
Cursor_Region::handle ( int m )
{
    Logger _log( this );

    if ( m == FL_PUSH )
    {
        if ( test_press( FL_BUTTON3 ) )
        {
            char *s = fl_text_edit( "Cursor text:", "&Save", label() );

            if ( s )
                label( s );

            free( s );

            return 0;
        }
    }

    int r = Sequence_Region::handle( m );

    if ( m == FL_RELEASE )
    {
        sequence()->sort();
        timeline->redraw();
    }

    return r;
}
