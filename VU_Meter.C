
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

/* a VU meter, either horizontal or vertical.  Color is a gradient
   from color() to selection_color(). box() is used to draw the
   individual 'lights'. division() controls how many 'lights' there
   are. */

#include "VU_Meter.H"

#include <FL/Fl.H>
#include <FL/fl_draw.H>

VU_Meter::VU_Meter ( int X, int Y, int W, int H, const char *L ) :
    Fl_Widget( X, Y, W, H, L )
{

    divisions( 32 );
    type( FL_VERTICAL );

    dim( 0.80f );

    color( FL_GREEN );
    selection_color( FL_RED );

    box( FL_ROUNDED_BOX );

    minimum( 0.0f );
    maximum( 1.0f );

    value( 0.4f );
}

void
VU_Meter::draw ( void )
{
    int v;

    v = (value() / maximum()) * _divisions;

    draw_box( FL_FLAT_BOX, x(), y(), w(), h(), FL_BLACK );

    int bh = h() / _divisions;
    int bw = w() / _divisions;

    int b = 0;

    for ( int p = _divisions; p > 0; p-- )
    {
        Fl_Color c = fl_color_average( selection_color(), color(), (float)p / _divisions );

        if ( ! active_r() )
            c = fl_inactive( c );

        if ( p > v )
            c = fl_color_average( FL_BLACK, c, _dim );

        if ( _type == FL_HORIZONTAL )
            draw_box( box(), x() + (p * bw), y(), bw, h(), c );
        else
            draw_box( box(), x(), y() + h() - (p * bh), w(), bh, c );

    }

    // we don't care about the label
    // draw_label (tx, y() + by, tw, h() - bh);
}
