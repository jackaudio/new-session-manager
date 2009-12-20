
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


#include "Fl_Arc_Dial.H"

#include <Fl/fl_draw.H>
#include <Fl/Fl.H>

int
Fl_Arc_Dial::handle ( int m )
{
    /* Fl_Dial and friends should really handle mousewheel, but they don't in FTLK1 */

    switch ( m )
    {
        case FL_MOUSEWHEEL:
        {
            if ( this != Fl::belowmouse() )
                return 0;

            int d = Fl::event_dy();

            double v = increment( value(), d );

            if ( v < minimum() )
                v = minimum();
            else if ( v > maximum() )
                v = maximum();

            value( v );
            do_callback();

            return 1;
        }
    }

    return Fl_Dial::handle( m );
}


void
Fl_Arc_Dial::draw ( void )
{
    int X = x();
    int Y = y();
    int W = w();
    int H = h();

    draw_box();
    draw_label();

    X += Fl::box_dx(box());
    Y += Fl::box_dy(box());
    W -= Fl::box_dw(box());
    H -= Fl::box_dh(box());

    double angle = ( angle2() - angle1() ) * ( value() - minimum()) / ( maximum() - minimum() ) + angle1();

    fl_line_style( FL_SOLID, W / 6 );

    X += W / 8;
    Y += H / 8;
    W -= W / 4;
    H -= H / 4;

    if ( box() == FL_NO_BOX )
    {
        /* draw backgrond arc */
        fl_color( fl_color_average( FL_BLACK, selection_color(), 0.80f ) );
        fl_arc( X, Y, W, H, 270 - angle1(), 270 - angle2() );
    }

    fl_color( selection_color() );

    if ( type() == FL_FILL_DIAL )
        fl_arc( X, Y, W, H, 270 - angle1(), 270 - angle  );
    else
    {
        const int d = 6;

        /* account for edge conditions */
        angle = angle < angle1() + d ? angle1() + d : angle;
        angle = angle > angle2() - d ? angle2() - d : angle;

        fl_arc( X, Y, W, H, 270 - (angle - d), 270 - (angle + d) );

        fl_line_style( FL_SOLID, 0 );
    }
}
