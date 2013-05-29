
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

#include "Fl_SliderX.H"
#include <FL/Fl.H>
#include <FL/fl_draw.H>

void
Fl_SliderX::draw ( int X, int Y, int W, int H)
{
    if (damage()&FL_DAMAGE_ALL) draw_box();
    
    double val;

  if (minimum() == maximum())
    val = 0.5;
  else {
    val = (value()-minimum())/(maximum()-minimum());
    if (val > 1.0) val = 1.0;
    else if (val < 0.0) val = 0.0;
  }

  int ww = (horizontal() ? W : H);
  int hh = (horizontal() ? H : W);
  int xx, S;
   
  /* S = int(val*ww+.5); */
  /* if (minimum()>maximum()) {S = ww-S; xx = ww-S;} */
  /* else xx = 0; */
 
  {
      //S = //int(ww+.5);
      S = hh;
      int T = (horizontal() ? H : W)/2+1;
//      if (type()==FL_VERT_NICE_SLIDER || type()==FL_HOR_NICE_SLIDER) T += 4;
      if (S < T) S = T;
      xx = int(val*(ww-S)+.5);
  }

  int xsl, ysl, wsl, hsl;
  if (horizontal()) {
    xsl = X+xx;
    wsl = S;
    ysl = Y + hh/2;
    hsl = hh/4;
  } else {
    ysl = Y+xx;
    hsl = S;
    xsl = X + hh/2;
    wsl = hh/4;
  }

  {
      fl_push_clip(X, Y, W, H);
      draw_box();
      fl_pop_clip();
      
      Fl_Color black = active_r() ? FL_BLACK : FL_INACTIVE_COLOR;
  }
  //draw_bg(X, Y, W, H);

  fl_line_style( FL_SOLID, hh/6 );
  
  fl_color( fl_darker(color()) );

  if ( horizontal() )
      fl_line ( X + S/2, Y + hh/2, X + W - S/2, Y + hh/2 );
  else
      fl_line ( X + hh/2, Y + S/2, X + hh/2, Y + H - S/2 );

  fl_color( selection_color() );

  if ( horizontal() )
      fl_line ( X + S/2, ysl, xsl + S/2, ysl );
  else
      fl_line ( X + S/2, Y + H - S/2, xsl, ysl + (S/2) );
  
  fl_line_style( FL_SOLID, 0 );

  fl_push_matrix();
  if ( horizontal() )
      fl_translate( xsl + (hh/2), ysl);
  else
      fl_translate( xsl, ysl + (hh/2) );

  fl_color( fl_color_add_alpha( FL_WHITE, 127 ));
  fl_begin_polygon(); fl_circle(0.0,0.0, hh/3); fl_end_polygon();
  fl_color( FL_WHITE );
  fl_begin_polygon(); fl_circle(0.0,0.0, hh/6); fl_end_polygon();
  
  fl_pop_matrix();
  
  draw_label(xsl, ysl, wsl, hsl);

  if (Fl::focus() == this) {
      draw_focus();
  }

    /* draw(x()+Fl::box_dx(box()), */
    /*      y()+Fl::box_dy(box()), */
    /*      w()-Fl::box_dw(box()), */
    /*      h()-Fl::box_dh(box())); */

}
