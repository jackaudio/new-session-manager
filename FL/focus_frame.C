
/*******************************************************************************/
/* Copyright (C) 2012 Jonathan Moore Liles                                     */
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

#include <FL/Fl.H>
#include <FL/fl_draw.H>

void
draw_focus_frame ( int x, int y, int w, int h, Fl_Color c )
{
    fl_push_clip( x, y, w, h );

//      fl_color( fl_color_average( FL_GRAY, c, 0.50 ) );
    
    /* fl_color( fl_color_add_alpha( c, 100 ) ); */

    /* fl_line_style( FL_DASH, 2 ); */

    /* fl_rect( x, y, w, h ); */

    fl_line_style( FL_SOLID, 3 );

    fl_color( c );

    int l = 15;

    fl_line( x, y, x + l, y );
    fl_line( x, y + l, x, y );

    fl_line( x + w - 1, y, x + w - l - 1, y );
    fl_line( x + w - 1, y, x + w - 1, y + l - 1 );

    fl_line( x, y + h - 1, x, y + h - l - 1);
    fl_line( x, y + h - 1, x + l, y + h - 1 );

    fl_line( x + w - 1, y + h - 1, x + w - 1, y + h - l - 1 );
    fl_line( x + w - 1, y + h - 1, x + w - l, y + h - 1 );

    fl_line_style( FL_SOLID, 0 );

    fl_pop_clip();
}

void
draw_selection_frame ( int x, int y, int w, int h, Fl_Color c )
{
    fl_push_clip( x, y, w, h );

    fl_color( fl_color_average( FL_GRAY, c, 0.50 ) );
    
///    fl_color( fl_color_add_alpha( c, 100 ) );

    fl_line_style( FL_DASH, 2 );

    fl_rect( x, y, w, h );

    fl_line_style( FL_SOLID, 3 );

    fl_color( c );

    int l = 15;

    fl_line( x, y, x + l, y );
    fl_line( x, y + l, x, y );

    fl_line( x + w - 1, y, x + w - l - 1, y );
    fl_line( x + w - 1, y, x + w - 1, y + l - 1 );

    fl_line( x, y + h - 1, x, y + h - l - 1);
    fl_line( x, y + h - 1, x + l, y + h - 1 );

    fl_line( x + w - 1, y + h - 1, x + w - 1, y + h - l - 1 );
    fl_line( x + w - 1, y + h - 1, x + w - l, y + h - 1 );

    fl_line_style( FL_SOLID, 0 );

    fl_pop_clip();
}

bool
focused_r ( Fl_Widget *w )
{
    for ( Fl_Widget *p = Fl::focus(); p; p = p->parent() )
        if ( p == w )
            return true;
    
    return false;
}
