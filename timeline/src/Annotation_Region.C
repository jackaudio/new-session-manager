
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

#include <FL/Fl_Tooltip.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>

#include "Annotation_Region.H"



void
Annotation_Region::get ( Log_Entry &e ) const
{
    Sequence_Region::get( e );

    e.add( ":label", _label );
}

void
Annotation_Region::set ( Log_Entry &e )
{
    Sequence_Region::set( e );

    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":label" ) )
            label( v );
    }

//            timeline->redraw();
}

Annotation_Region::Annotation_Region ( Sequence *sequence, nframes_t when, const char *label )
{
    _sequence = NULL;

    _r->start = when;

    /* FIXME: hack */
    _r->length = 400;

    _label = strdup( label );

    sequence->add( this );

    log_create();
}

Annotation_Region::Annotation_Region ( const Annotation_Region &rhs ) : Sequence_Region( rhs )
{
    log_create();
}


Annotation_Region::~Annotation_Region ( )
{
    log_destroy();
}

void
Annotation_Region::draw_box ( void )
{
    Sequence_Region::draw_box();
}

void
Annotation_Region::draw ( void )
{
    draw_label( _label, (Fl_Align)(FL_ALIGN_LEFT | FL_ALIGN_INSIDE | FL_ALIGN_TOP | FL_ALIGN_CLIP ) );
}

#include <FL/Fl_Text_Edit_Window.H>
#include <FL/test_press.H>

int
Annotation_Region::handle ( int m )
{
    Logger _log( this );

    if ( m == FL_PUSH )
    {
        if ( test_press( FL_BUTTON3 ) )
        {
            char *s = fl_text_edit( "Annotation text:", "&Save", label() );

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
