
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

static void gleam_color(Fl_Color c)
{
	if (Fl::draw_box_active())
		fl_color(c);
	else
		fl_color(fl_inactive(c));
}
static void frame_rect(int x, int y, int w, int h, Fl_Color bc)
{

	// Draw the outline around the perimeter of the box
	fl_color(fl_color_average(FL_BLACK, FL_BACKGROUND_COLOR, .1));
	fl_line(x, y, x + w, y);
	fl_line(x + w, y, x + w, y + h);
	fl_line(x + w, y + h, x, y + h);
	fl_line(x, y + h, x, y);

}

static void shade_rect_up(int x, int y, int w, int h, Fl_Color bc)
{
	// Draws the shiny
	float third = (float) h / 3;
	gleam_color(bc);
	fl_rectf(x, y, w, third + 1);

	//gleam_color(fl_color_average(bc, FL_WHITE, .90f));
	//fl_rectf(x, y, w, half + 1);

	float step_size = 0.10 / ((float) h - third);
	int j = 0;
	//step_size = (.1 / (float) half);
	//printf("1 / %i = %f \n", half, (1.0/half));

	/**
	 * This loop generates the nice gradient at the bottom of the
	 * widget
	 **/
	for (float k = 1; k >= .90; k -= step_size)
	{
		j++;
		gleam_color(fl_color_average(bc, FL_WHITE, k));
		fl_line(x, y + j + third - 1, x + w - 1, y + j + third - 1);
	}

}

static void frame_rect_up(int x, int y, int w, int h, Fl_Color bc)
{

	// Draw the outline around the perimeter of the box
	gleam_color(bc);
	fl_line(x, y, x + w, y); //Go across.
	fl_line(x, y + (h / 2), x, y + 1); //Go to top
	fl_line(x + w, y + (h / 2), x + w, y + 1); //Go to top

	gleam_color(fl_darker(bc));
	fl_line(x, y + h, x + w, y + h); //Go across again!
	fl_line(x, y + (h / 2), x, y + h - 1); //Go to top
	fl_line(x + w, y + (h / 2), x + w, y + h - 1); //Go to top

}

static void frame_rect_down(int x, int y, int w, int h, Fl_Color bc)
{

	// Draw the outline around the perimeter of the box
	gleam_color(fl_darker(bc));
	fl_line(x, y, x + w, y); //Go across.
	fl_line(x, y + (h / 2), x, y + 1); //Go to top
	fl_line(x + w, y + (h / 2), x + w, y + 1); //Go to top

	//gleam_color(bc);
	fl_line(x, y + h, x + w, y + h); //Go across again!
	fl_line(x, y + (h / 2), x, y + h - 1); //Go to top
	fl_line(x + w, y + (h / 2), x + w, y + h - 1); //Go to top

}

static void shade_rect_down(int x, int y, int w, int h, Fl_Color bc)
{

	gleam_color(bc);
	Fl_Color color = fl_color();
	fl_rectf(x, y, w, h);
	gleam_color(fl_color_average(bc, fl_darker(color), 0.65));
	fl_line(x, y + 1, x + w, y + 1);
	fl_line(x, y + 1, x, y + h - 2);
	gleam_color(fl_color_average(bc, fl_darker(color), 0.85));
	fl_line(x + 1, y + 2, x + w, y + 2);
	fl_line(x + 1, y + 2, x + 1, y + h - 2);

}

static void up_frame(int x, int y, int w, int h, Fl_Color c)
{
	frame_rect_up(x, y, w - 1, h - 1, fl_darker(c));
}

static void up_box(int x, int y, int w, int h, Fl_Color c)
{
	shade_rect_up(x + 1, y, w - 2, h - 1, c);
	frame_rect_up(x, y, w - 1, h - 1, fl_darker(c));
	//draw the inner rect.
	frame_rect(x + 1, y + 1, w - 3, h - 3, fl_color_average(c, FL_WHITE, .25f));

}

static void down_frame(int x, int y, int w, int h, Fl_Color c)
{
	frame_rect_down(x, y, w - 1, h - 1, fl_darker(c));
}

static void down_box(int x, int y, int w, int h, Fl_Color c)
{
	shade_rect_down(x + 1, y, w - 2, h, c);
	down_frame(x, y, w, h, fl_darker(c));
	//draw the inner rect.
	//frame_rect(x + 1, y + 1, w - 3, h - 3, fl_color_average(c, FL_BLACK, .65));
}

static void
init_theme ( void )
{
    /* replace the gtk+ boxes... (is there a better way?) */
    Fl::set_boxtype(  FL_UP_BOX,         up_box,           2,2,4,4 );
    Fl::set_boxtype(  FL_DOWN_BOX,       down_box,         2,2,3,3 );
    Fl::set_boxtype(  FL_THIN_UP_BOX,         up_box,           2,2,3,3 );
    Fl::set_boxtype(  FL_THIN_DOWN_BOX,       down_box,         2,2,3,3 );
    Fl::set_boxtype(  FL_UP_FRAME,       up_frame,         2,2,3,3 );
    Fl::set_boxtype(  FL_DOWN_FRAME,     down_frame,       2,2,3,3 );
    Fl::set_boxtype(  FL_ROUND_UP_BOX,   up_box,           2,2,3,3 );
    Fl::set_boxtype(  FL_ROUND_DOWN_BOX, down_box,         2,2,3,3 );
}

void
init_gleam_theme ( void )
{
    Fl_Theme *t = new Fl_Theme( "Gleam", "", "", init_theme );

    Fl_Theme::add( t );
}
