
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

#include <math.h>

void
Fl_SliderX::draw ( int X, int Y, int W, int H)
{
    slider_size( horizontal() ? H / (float)W : W / (float)H );

    int act = active_r();

    if (damage()&FL_DAMAGE_ALL) draw_box();
    
 
    int ww = (horizontal() ? W : H);
    int hh = (horizontal() ? H : W);
    int xx, S;
   
    xx = slider_position( value(), ww );

    S = (horizontal() ? H : W );
 
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
    }
    //draw_bg(X, Y, W, H);

    fl_line_style( FL_SOLID, hh/6 );
  
    Fl_Color c = fl_darker(color());

    if ( !act )
        c = fl_inactive(c);

    fl_color(c);

    if ( horizontal() )
        fl_line ( X + S/2, Y + hh/2, X + W - S/2, Y + hh/2 );
    else
        fl_line ( X + hh/2, Y + S/2, X + hh/2, Y + H - S/2 );

    c = selection_color();

    if ( !act )
        c = fl_inactive(c);

    fl_color( c );

    if ( horizontal() )
        fl_line ( X + S/2, ysl, xsl + S/2, ysl );
    else
        fl_line ( X + S/2, Y + H - S/2, xsl, ysl + (S/2) );
  
    fl_line_style( FL_SOLID, 0 );

    if ( act )
    {
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
    }
  
    draw_label(xsl, ysl, wsl, hsl);

    if (Fl::focus() == this) {
        draw_focus();
    }

    /* draw(x()+Fl::box_dx(box()), */
    /*      y()+Fl::box_dy(box()), */
    /*      w()-Fl::box_dw(box()), */
    /*      h()-Fl::box_dh(box())); */

}

/** return a value between 0.0 and 1.0 which represents the current slider position. */
int
Fl_SliderX::slider_position ( double value, int w )
{
    double A = minimum();
    double B = maximum();
    if (B == A) return 0;
    bool flip = B < A;
    if (flip) {A = B; B = minimum();}
//    if (!horizontal()) flip = !flip;
    // if both are negative, make the range positive:
    if (B <= 0) {flip = !flip; double t = A; A = -B; B = -t; value = -value;}
    double fraction;
    if (!log()) {
	// linear slider
	fraction = (value-A)/(B-A);
    } else if (A > 0) {
	// logatithmic slider
	if (value <= A) fraction = 0;
	else fraction = (::log(value)-::log(A))/(::log(B)-::log(A));
    } else if (A == 0) {
	// squared slider
	if (value <= 0) fraction = 0;
	else fraction = sqrt(value/B);
    } else {
	// squared signed slider
	if (value < 0) fraction = (1-sqrt(value/A))*.5;
	else fraction = (1+sqrt(value/B))*.5;
    }
    if (flip) fraction = 1-fraction;
    
    w -= int(slider_size()*w+.5); if (w <= 0) return 0;
    if (fraction >= 1) return w;
    else if (fraction <= 0) return 0;
    else return int(fraction*w+.5);
}

double
Fl_SliderX::slider_value ( int X, int w )
{
    w -= int(slider_size()*w+.5); if (w <= 0) return minimum();
  double A = minimum();
  double B = maximum();
  bool flip = B < A;
  if (flip) {A = B; B = minimum();}
//  if (!horizontal()) flip = !flip;
  if (flip) X = w-X;
  double fraction = double(X)/w;
  if (fraction <= 0) return A;
  if (fraction >= 1) return B;
  // if both are negative, make the range positive:
  flip = (B <= 0);
  if (flip) {double t = A; A = -B; B = -t; fraction = 1-fraction;}
  double value;
  double derivative;
  if (!log()) {
    // linear slider
    value = fraction*(B-A)+A;
    derivative = (B-A)/w;
  } else if (A > 0) {
    // log slider
    double d = (::log(B)-::log(A));
    value = exp(fraction*d+::log(A));
    derivative = value*d/w;
  } else if (A == 0) {
    // squared slider
    value = fraction*fraction*B;
    derivative = 2*fraction*B/w;
  } else {
    // squared signed slider
    fraction = 2*fraction - 1;
    if (fraction < 0) B = A;
    value = fraction*fraction*B;
    derivative = 4*fraction*B/w;
  }
  // find nicest multiple of 10,5, or 2 of step() that is close to 1 pixel:
  if (step() && derivative > step()) {
    double w = log10(derivative);
    double l = ceil(w);
    int num = 1;
    int i; for (i = 0; i < l; i++) num *= 10;
    int denom = 1;
    for (i = -1; i >= l; i--) denom *= 10;
    if (l-w > 0.69897) denom *= 5;
    else if (l-w > 0.30103) denom *= 2;
    value = floor(value*denom/num+.5)*num/denom;
  }
  if (flip) return -value;
  return value;

}

int Fl_SliderX::handle(int event, int X, int Y, int W, int H) {
  // Fl_Widget_Tracker wp(this);
  switch (event) {
  case FL_PUSH: {
    Fl_Widget_Tracker wp(this);
    if (!Fl::event_inside(X, Y, W, H)) return 0;
    handle_push();
    if (wp.deleted()) return 1; }
    // fall through ...
  case FL_DRAG: {

      static int offcenter;

      int ww = (horizontal() ? W : H);

      if ( event == FL_PUSH )
      {
	  int x = slider_position( value(), ww );

	  offcenter = (horizontal() ? (Fl::event_x()-X) - x : (Fl::event_y()-Y) - x );
      }

      try_again:

      int mx = (horizontal() ? Fl::event_x()-X : Fl::event_y()-Y) - offcenter;
      double v = slider_value( mx, ww );

      if (event == FL_PUSH ) // && v == value()) {
      {
          int os = int(slider_size()*ww+0.5)/2;
          if ( abs( offcenter ) > os )
          {
              offcenter = os;
              event = FL_DRAG;
              goto try_again;
          }
      }

      handle_drag(clamp(v));
    } return 1;
  case FL_RELEASE:
    handle_release();
    return 1;
  case FL_KEYBOARD:
    { Fl_Widget_Tracker wp(this);
      switch (Fl::event_key()) {
	case FL_Up:
	  if (horizontal()) return 0;
	  handle_push();
	  if (wp.deleted()) return 1;
	  handle_drag(clamp(increment(value(),-1)));
	  if (wp.deleted()) return 1;
	  handle_release();
	  return 1;
	case FL_Down:
	  if (horizontal()) return 0;
	  handle_push();
	  if (wp.deleted()) return 1;
	  handle_drag(clamp(increment(value(),1)));
	  if (wp.deleted()) return 1;
	  handle_release();
	  return 1;
	case FL_Left:
	  if (!horizontal()) return 0;
	  handle_push();
	  if (wp.deleted()) return 1;
	  handle_drag(clamp(increment(value(),-1)));
	  if (wp.deleted()) return 1;
	  handle_release();
	  return 1;
	case FL_Right:
	  if (!horizontal()) return 0;
	  handle_push();
	  if (wp.deleted()) return 1;
	  handle_drag(clamp(increment(value(),1)));
	  if (wp.deleted()) return 1;
	  handle_release();
	  return 1;
	default:
	  return 0;
      }
    }
    // break not required because of switch...
  case FL_FOCUS :
  case FL_UNFOCUS :
    if (Fl::visible_focus()) {
      redraw();
      return 1;
    } else return 0;
  case FL_ENTER :
  case FL_LEAVE :
    return 1;
  case FL_MOUSEWHEEL :
  {
      if ( this != Fl::belowmouse() )
          return 0;
      if (Fl::e_dy==0)
          return 0;
      
      const int steps = Fl::event_ctrl() ? 128 : 16;
      
      const float step = fabs( maximum() - minimum() ) / (float)steps;

      int dy = Fl::e_dy;

      /* slider is in 'upside down' configuration, invert meaning of mousewheel */
      if ( minimum() > maximum() )
          dy = 0 - dy;

      handle_drag(clamp(value() + step * dy));
      return 1;
  }
  default:
    return 0;
  }
}

int Fl_SliderX::handle(int event) {
  if (event == FL_PUSH && Fl::visible_focus()) {
    Fl::focus(this);
    redraw();
  }

  return handle(event,
		x()+Fl::box_dx(box()),
		y()+Fl::box_dy(box()),
		w()-Fl::box_dw(box()),
		h()-Fl::box_dh(box()));
}
