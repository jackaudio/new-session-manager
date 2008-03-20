
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

#include "Boxtypes.H"
#include <FL/fl_draw.H>

/** This simple box is suitable for use with knob-type widgets. It
 * comprises a border with shadow, and a cap with glare-lines akin
 * to those seen on burnished aluminum knobs. */
static void
draw_burnished_oval_box ( int x, int y, int w, int h, Fl_Color c )
{
    /* draw background */
    fl_color( fl_darker( c ) );
    fl_pie( x, y, w, h, 0, 360 );
    fl_color( fl_darker( fl_darker( c ) ) );
    fl_pie( x, y, w, h, 180 + 215, 180 + 45 );

    /* shrink */
    x += 4;
    y += 4;
    w -= 7;
    h -= 7;

    /* draw cap */
    fl_color( c );
    fl_pie( x, y, w, h, 0, 360 );

    /* draw glare */

    const int a1 = 10;
    const int a2 = 90;

    fl_color( fl_lighter( c ) );
    fl_pie( x, y, w, h, a1, a2 );
    fl_pie( x, y, w, h, 180 + a1, 180 + a2 );
    fl_color( fl_lighter( fl_lighter( c ) ) );

    const int d = (a2 - a1) / 2;
    fl_pie( x, y, w, h, a1 + (d / 2), a2 - (d / 2) );
    fl_pie( x, y, w, h, 180 + a1 + (d / 2), 180 + a2 - (d / 2) );
}




void
init_boxtypes ( void )
{
    Fl::set_boxtype( FL_BURNISHED_OVAL_BOX, draw_burnished_oval_box, 4, 4, 7, 7 );
}
