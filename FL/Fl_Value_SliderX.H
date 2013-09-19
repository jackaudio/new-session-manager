
/*******************************************************************************/
/* Copyright (C) 2009 Jonathan Moore Liles                                     */
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

#ifndef Fl_Value_SliderX_H
#define Fl_Value_SliderX_H

#include "Fl_SliderX.H"
#include <FL/Fl_Input.H>

/**
  The Fl_Value_SliderX widget is a Fl_SliderX widget
  with a box displaying the current value.
  <P ALIGN=CENTER>\image html value_slider.png 
  \image latex  value_slider.png "Fl_Value_SliderX" width=4cm
*/
class FL_EXPORT Fl_Value_SliderX : public Fl_SliderX {
  /* This is the encapsulated Fl_input attribute to which 
  this class delegates the value font, color and shortcut */
  Fl_Input input;

private:
  char soft_;
  static void input_cb(Fl_Widget*,void*);
  virtual void value_damage(); // cause damage() due to value() changing

protected:
    void draw();
public:
    int handle(int);
    Fl_Value_SliderX(int x,int y,int w,int h, const char *l = 0);
    virtual ~Fl_Value_SliderX ( );

/** See void Fl_Value_Input::soft(char s) */
  void soft(char s) {soft_ = s;}
  /**
    If "soft" is turned on, the user is allowed to drag
    the value outside the range. If they drag the value to one of
    the ends, let go, then grab again and continue to drag, they can
    get to any value. The default is true.
  */
  char soft() const {return soft_;}
  /**
   Returns the current shortcut key for the Input.
   \see Fl_Value_Input::shortcut(int) 
  */
  int shortcut() const {return input.shortcut();}
  /** 
   Sets the shortcut key to \p s. Setting this
   overrides the use of '&' in the label().  The value is a bitwise
   OR of a key and a set of shift flags, for example FL_ALT | 'a'
   , FL_ALT | (FL_F + 10), or just 'a'.  A value
   of 0 disables the shortcut.
   
   The key can be any value returned by 
   Fl::event_key(), but will usually be an ASCII letter.  Use
   a lower-case letter unless you require the shift key to be held down.
   
   The shift flags can be any set of values accepted by 
   Fl::event_state().  If the bit is on that shift key must
   be pushed.  Meta, Alt, Ctrl, and Shift must be off if they are not in
   the shift flags (zero for the other bits indicates a "don't care"
   setting).
   */
  void shortcut(int s) {input.shortcut(s);}
  /** Gets the typeface of the text in the value box.  */
  Fl_Font textfont() const {return input.textfont();}
  /** Sets the typeface of the text in the value box.  */
  void textfont(Fl_Font s) {input.textfont(s);}
  /** Gets the size of the text in the value box.  */
  Fl_Fontsize textsize() const {return input.textsize();}
  /** Sets the size of the text in the value box.  */
  void textsize(Fl_Fontsize s) {input.textsize(s);}
  /** Gets the color of the text in the value box.  */
  Fl_Color textcolor() const {return input.textcolor();}
  /** Sets the color of the text in the value box.*/
  void textcolor(Fl_Color n) {input.textcolor(n);}
  /** Gets the color of the text cursor. The text cursor is black by default. */
  Fl_Color cursor_color() const {return input.cursor_color();}
  /** Sets the color of the text cursor. The text cursor is black by default. */
  void cursor_color(Fl_Color n) {input.cursor_color(n);}
};

#endif

