
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

/* just like an Fl_Value_Slider, except that it responds to mousewheel events */

#pragma once

#include <FL/Fl_Value_Slider.H>
#include <FL/Fl.H>
#include <math.h>

class Fl_Value_SliderX : public Fl_Value_Slider
{
    static int _default_style;

    int _type;

public:

    enum 
    {
        NICE_SLIDER,
        SIMPLE_SLIDER,
        FILL_SLIDER
    };

    static void default_style ( int n ) { Fl_Value_SliderX::_default_style = n; }

    int type ( void ) const { return _type; }
    void type ( int v ) { _type = v; }
    

    Fl_Value_SliderX ( int X, int Y, int W, int H, const char *L = 0 )
        : Fl_Value_Slider( X, Y, W, H, L )
        {
        }

    virtual ~Fl_Value_SliderX() { }

    virtual void draw ( void );
};
