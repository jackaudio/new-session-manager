
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

#include "Tempo_Sequence.H"
#include "Track.H"

Tempo_Sequence::Tempo_Sequence ( int X, int Y, int W, int H ) : Sequence ( X, Y, W, H )
{
    box( FL_FLAT_BOX );
    
    { Fl_Box *o = new Fl_Box( X, Y, Track::width(), H );
        o->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE );
        o->labelsize( 12 );
        o->labeltype( FL_NORMAL_LABEL );
        o->label( "Tempo" );
        Fl_Group::add( o );
    }

    resizable(0);
}

void
Tempo_Sequence::handle_widget_change ( nframes_t start, nframes_t length )
{
    Sequence::handle_widget_change( start, length );

    sort();
    timeline->update_tempomap();
    timeline->redraw();
}

int
Tempo_Sequence::handle ( int m )
{
    int r = Sequence::handle( m );

    if ( r )
        return r;

    switch ( m )
    {
        case FL_PUSH:
            if ( Fl::event_button1() )
            {
                static float t = 120.0f;

                if ( Tempo_Point::edit( &t ) )
                {
                    timeline->sequence_lock.wrlock();
                    
                    new Tempo_Point( timeline->x_to_offset( Fl::event_x() ), t );

                    timeline->sequence_lock.unlock();

                    timeline->redraw();
                }
                return 0;
            }
        default:
            return 0;

    }
}
