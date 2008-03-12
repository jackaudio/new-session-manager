
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

/* a Digital Peak Meter, either horizontal or vertical.  Color is a
   gradient from min_color() to max_color(). box() is used to draw the
   individual 'lights'. division() controls how many 'lights' there
   are. value() is volume in dBFS */

#include "DPM.H"

#include <FL/Fl.H>
#include <FL/fl_draw.H>

DPM::DPM ( int X, int Y, int W, int H, const char *L ) :
    Meter( X, Y, W, H, L )
{
    divisions( 32 );
    type( FL_VERTICAL );

    dim( 0.80f );

    min_color( FL_GREEN );
    max_color( FL_RED );

    box( FL_ROUNDED_BOX );
}

/* which marks to draw beside meter */
const int marks [] = { -70, -50, -40, -30, -10, -3, 0, 4 };

void
DPM::draw ( void )
{
//    draw_box( FL_FLAT_BOX, x(), y(), w(), h(), color() );
    int v = pos( value() );
    int pv = pos( peak() );

    int bh = h() / _divisions;
    int bw = w() / _divisions;

    for ( int p = _divisions; p > 0; p-- )
    {
        Fl_Color c = fl_color_average( _max_color, _min_color, (float)p / _divisions );

        if ( p > v && p != pv )
//            c = fl_color_average( color(), c, _dim );
            c = fl_color_average( FL_BLACK, c, _dim );

        if ( ! active_r() )
            c = fl_inactive( c );

        if ( _type == FL_HORIZONTAL )
            draw_box( box(), x() + (p * bw), y(), bw, h(), c );
        else
            draw_box( box(), x(), y() + h() - (p * bh), w(), bh, c );
    }
}
