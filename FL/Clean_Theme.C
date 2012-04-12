
/*******************************************************************************/
/* Copyright (C) 2012 Jonathan Moore Liles                                     */
/* Copyright (C) 2001-2005 by Colin Jones                                      */
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

/* Taken from the "Gleam" FLTK scheme, as modified by prodatum */

#include <FL/Fl.H>
#include <FL/fl_draw.H>

#include "FL/Fl_Theme.H"

static void clean_color(Fl_Color c)
{
    c = fl_color_average( FL_GRAY, c, 0.4 );

    if (Fl::draw_box_active())
        fl_color(c);
    else
        fl_color(fl_inactive(c));
}

static void rect(int x, int y, int w, int h, Fl_Color bc)
{
//    clean_color( bc );
    fl_rect( x, y, w, h, bc );
}

static void rectf ( int x, int y,int w, int h, Fl_Color bc )
{
    clean_color(fl_color_average( FL_WHITE, bc, 0.05 ) );
    fl_rectf( x, y, w, h );
}

static void up_frame(int x, int y, int w, int h, Fl_Color c)
{
    rect(x, y, w, h, fl_color_average( FL_WHITE, c, 0.2 ) );
}

static void up_box(int x, int y, int w, int h, Fl_Color c)
{
    rectf(x, y, w, h, c );
    rect(x, y, w, h, fl_color_average( FL_WHITE, c, 0.2 ) );
}

static void down_frame(int x, int y, int w, int h, Fl_Color c)
{
    rect(x, y, w, h, fl_color_average( FL_BLACK, c, 0.2 ) );
}

static void down_box(int x, int y, int w, int h, Fl_Color c)
{
    rectf(x, y, w, h, fl_color_average( FL_WHITE, c, 0.2 ) );
    rect(x, y, w, h, fl_color_average( FL_WHITE, c, 0.4 ) );
}

static void flat_box( int x, int y, int w, int h, Fl_Color c )
{
    rectf( x, y, w, h, c );
}

static void
init_theme ( void )
{
    Fl::set_boxtype(  FL_UP_BOX,         up_box,           1,1,2,2 );
    Fl::set_boxtype(  FL_DOWN_BOX,       down_box,         1,1,2,2 );
    Fl::set_boxtype(  FL_THIN_UP_BOX,         up_box,      1,1,2,2 );
    Fl::set_boxtype(  FL_THIN_DOWN_BOX,       down_box,    1,1,2,2 );
    Fl::set_boxtype(  FL_UP_FRAME,       up_frame,         1,1,2,2 );
    Fl::set_boxtype(  FL_DOWN_FRAME,     down_frame,       1,1,2,2 );
    Fl::set_boxtype(  FL_ROUND_UP_BOX,   up_box,           1,1,2,2 );
    Fl::set_boxtype(  FL_ROUND_DOWN_BOX, down_box,         1,1,2,2 );
    Fl::set_boxtype(  FL_FLAT_BOX,       flat_box,         0, 0, 0, 0 );
}

void
init_clean_theme ( void )
{
    Fl_Theme *t = new Fl_Theme( "Clean", "", "", init_theme );

    Fl_Theme::add( t );
}
