
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


#include "Fl_Value_SliderX.H"

int Fl_Value_SliderX::_default_style = NICE_SLIDER;

void
Fl_Value_SliderX::draw ( void )
{
    switch ( _default_style )
    {
        case NICE_SLIDER:
        {
            if ( FL_HORIZONTAL == _type )
                Fl_Value_Slider::type( FL_HOR_NICE_SLIDER );
            else
                Fl_Value_Slider::type( FL_VERT_NICE_SLIDER );
            break;
        }
        case FILL_SLIDER:
        {
            if ( FL_HORIZONTAL == _type )
                Fl_Value_Slider::type( FL_HOR_FILL_SLIDER );
            else
                Fl_Value_Slider::type( FL_VERT_FILL_SLIDER );
            break;
        }
        case SIMPLE_SLIDER:
        {
            if ( FL_HORIZONTAL == _type )
                Fl_Value_Slider::type( FL_HOR_SLIDER );
            else
                Fl_Value_Slider::type( FL_VERT_SLIDER );
            break;
        }
    }

    Fl_Value_Slider::draw();
}
