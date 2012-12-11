
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

#include "Time_Sequence.H"
#include "Track.H"
#include <FL/Fl_Box.H>

Time_Sequence::Time_Sequence ( int X, int Y, int W, int H ) : Sequence ( X, Y, W, H )
{
    box( FL_FLAT_BOX );

    { Fl_Box *o = new Fl_Box( X, Y, Track::width(), H );
        o->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE );
        o->labelsize( 12 );
        o->labeltype( FL_NORMAL_LABEL );
        o->label( "Time" );
        Fl_Group::add( o );
    }

    resizable(0);
}



void
Time_Sequence::handle_widget_change ( nframes_t start, nframes_t length )
{
    Sequence::handle_widget_change( start, length );

    sort();
    timeline->update_tempomap();
    timeline->redraw();
}

int
Time_Sequence::handle ( int m )
{
    int r = Sequence::handle( m );

    if ( r )
        return r;

    switch ( m )
    {
        case FL_PUSH:
            if ( Fl::event_button1() )
            {
                static time_sig t = time_sig( 4, 4 );

                if ( Time_Point::edit( &t ) )
                {
                    add( new Time_Point( timeline->x_to_offset( Fl::event_x() ), t.beats_per_bar, t.beat_type ) );

                    timeline->redraw();
                }
                return 0;
            }
        default:
            return 0;

    }
}
