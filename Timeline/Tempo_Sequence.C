
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


void
Tempo_Sequence::draw ( void )
{
    if ( ! fl_not_clipped( x(), y(), w(), h() ) )
        return;

    fl_push_clip( x(), y(), w(), h() );

    /* draw the box with the ends cut off. */
    draw_box( box(),
              x() - Fl::box_dx( box() ) - 1, y(),
              w() + Fl::box_dw( box() ) + 2, h() >> 1, color() );
    draw_box( box(),
              x() - Fl::box_dx( box() ) - 1, y() + (h() >> 1) ,
              w() + Fl::box_dw( box() ) + 2, h() >> 1, color() );

    int X, Y, W, H;

    fl_clip_box( x(), y(), w(), h(), X, Y, W, H );

    if ( Sequence_Widget::pushed() && Sequence_Widget::pushed()->sequence() == this )
    {
        /* make sure the Sequence_Widget::pushed widget is above all others */
        remove( Sequence_Widget::pushed() );
        add( Sequence_Widget::pushed() );
    }

    timeline->draw_measure_lines( X, Y, W, H, color() );

    for ( list <Sequence_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); ++r )
        (*r)->draw_box();

    for ( list <Sequence_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); ++r )
        (*r)->draw();

    fl_pop_clip();
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
                    add( new Tempo_Point( timeline->x_to_offset( Fl::event_x() ), t ) );

                    timeline->redraw();
                }
                return 0;
            }
        default:
            return 0;

    }

}
