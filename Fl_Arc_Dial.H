
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

/* simple stylistic variation on Fl_Dial */

#pragma once

#include <FL/Fl_Dial.H>

class Fl_Arc_Dial : public Fl_Dial
{

public:

    Fl_Arc_Dial ( int X, int Y, int W, int H, const char *L = 0 ) :
        Fl_Dial( X, Y, W, H, L )
        {
            box( FL_OVAL_BOX );
        }

protected:

    virtual void
    draw ( void )
        {
            int X = x();
            int Y = y();
            int W = w();
            int H = h();

            if ( damage() & FL_DAMAGE_ALL )
                draw_box( box(), X, Y, W, H, color() );

            X += Fl::box_dx(box());
            Y += Fl::box_dy(box());
            W -= Fl::box_dw(box());
            H -= Fl::box_dh(box());

            double angle = ( angle2() - angle1() ) * ( value() - minimum()) / ( maximum() - minimum() ) + angle1();

            draw_box();
            draw_label();

            fl_line_style( FL_SOLID, W / 6 );

            X += W / 8;
            Y += H / 8;
            W -= W / 4;
            H -= H / 4;

            fl_color( fl_color_average( FL_BLACK, selection_color(), 0.80f ) );

            fl_arc( X, Y, W, H, 270 - angle1(), 270 - angle2() );

            fl_color( selection_color() );

            if ( type() == FL_FILL_DIAL )
                fl_arc( X, Y, W, H, 270 - angle1(), 270 - angle  );
            else
                fl_arc( X, Y, W, H, 270 - angle - 3, 270 - angle + 3 );

            fl_line_style( FL_SOLID, 0 );
        }
};
