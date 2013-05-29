
/*******************************************************************************/
/* Copyright (C) 2013 Jonathan Moore Liles                                     */
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

#include "Fl_DialX.H"
#include <FL/Fl.H>
#include <FL/fl_draw.H>

void
Fl_DialX::draw ( void )
{
    int X,Y,S;

    {
    int ox, oy, ww, hh, side;
    ox = x();
    oy = y();
    ww = w();
    hh = h();
    
    if (ww > hh)
    {
        side = hh;
        ox = ox + (ww - side) / 2;
    }
    else
    {
        side = ww;
        oy = oy + (hh - side) / 2;
    }
    side = w() > h() ? hh : ww;

    X = ox;
    Y = oy;
    S = side;
    }

    draw_box();
    draw_label();

    double angle = ( angle2() - angle1() ) * ( value() - minimum()) / ( maximum() - minimum() ) + angle1();

    fl_draw_box( box(), X, Y, S, S, color() );
    
    /* shrink a bit */
    int OX = x();
    int OY = y();
    X += S / 8;
    Y += S / 8;
    int OS = S;
    S -= S / 4;
    
    fl_line_style( FL_SOLID, S / 12 );
        
    /* background arc */
    fl_color( fl_darker( color() ) );
    fl_arc( X, Y, S, S, 270 - angle1(), 270 - angle2() );
        
    /* foreground arc */
    fl_color( selection_color() );
    fl_arc( X, Y, S, S, 270 - angle1(), 270 - angle  );


    fl_line_style( FL_SOLID, 0 );
 
    if ( active_r() )
    {
        int W = OS;
        int H = OS;

        fl_push_matrix();
        fl_translate(OX+W/2, OY+H/2);
        fl_scale(W, H);
        fl_rotate(310+angle);
        fl_color( fl_color_add_alpha( FL_WHITE, 127 ));
        fl_begin_polygon(); fl_circle(-0.26, 0.26, 0.12); fl_end_polygon();
        fl_color( FL_WHITE );
        fl_begin_polygon(); fl_circle(-0.26, 0.26, 0.06); fl_end_polygon();
            
        fl_pop_matrix();
    }
    
    fl_color( fl_contrast( labelcolor(), color() ) );
 
    if ( Fl::belowmouse() == this )
    {
        char s[10];
    
        fl_font( FL_HELVETICA, 10 );
    
        snprintf( s, sizeof( s ), "%.1f", value() );

        fl_color( FL_FOREGROUND_COLOR );
        fl_draw( s, X, Y, S, S, FL_ALIGN_CENTER );
    }
}
