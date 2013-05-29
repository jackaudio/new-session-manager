
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

#include "Cursor_Sequence.H"
#include "Cursor_Point.H"
#include "Timeline.H"
#include "Track.H"

Cursor_Sequence::Cursor_Sequence( int X, int Y, int W, int H ) : Sequence ( X, Y, W, H )
{
    _cursor_color = FL_CYAN;

    labeltype(FL_NO_LABEL);

    box( FL_FLAT_BOX );

    { Fl_Box *o = new Fl_Box( X, Y, Track::width(), H );
        o->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE );
        o->labelsize( 12 );
        o->labeltype( FL_NORMAL_LABEL );
        Fl_Group::add( o );
    }

    resizable(0);
}



void
Cursor_Sequence::label ( const char *s )
{
    Sequence::label(s);
    child(0)->label(s);
}

const char *
Cursor_Sequence::label ( void ) const
{
    return Sequence::label();
}

void
Cursor_Sequence::handle_widget_change ( nframes_t start, nframes_t length )
{
    sort();
    timeline->redraw_overlay();
    timeline->redraw();
}

Sequence_Widget *
Cursor_Sequence::active_cursor ( void )
{
    if ( _widgets.size() )
        return _widgets.front();
    else
        return 0;
}

int
Cursor_Sequence::handle ( int m )
{
    int r = Sequence::handle( m );

    if ( r )
        return r;

    switch ( m )
    {
        case FL_PUSH:
            /* if ( Fl::event_button1() ) */
            /* { */
            /*     add( new Cursor_Point( timeline->x_to_offset( Fl::event_x() ), "NONE" ) ); */
            /*     timeline->redraw(); */
            /*     return 0; */
            /* } */
            return 0;
        default:
            return 0;

    }
}
