
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

/* Simple extension of Fl_Button to support different lables for the
 * two button states. Simpler than using a an Fl_Choice when all you
 * need is two states. Set the label to "foo/bar" for "foo" to be the
 * off label and "bar" the on. Obviously you should make sure that
 * each label will fit. */

#pragma once

#include <FL/Fl_Button.H>

#include <stdlib.h>
#include <stdio.h>

class Fl_Flip_Button : public Fl_Button
{

    char *_off;
    char *_on;

public:

    Fl_Flip_Button ( int X, int Y, int W, int H, const char *L = 0 ) :
        Fl_Button( X, Y, W, H, 0 )
        {
            sscanf( L, "%m[^/]/%ms", &_off, &_on );

            type( FL_TOGGLE_BUTTON );
        }

    virtual ~Fl_Flip_Button ( )
        {
            if ( _off ) free( _off );
            if ( _on )  free( _on );
        }

protected:

    virtual void
    draw ( void )
        {
            if ( value() )
            {
                if ( label() != _on )
                    label( _on );
            }
            else
                if ( label() != _off )
                    label( _off );

            draw_box( box(), x(), y(), w(), h(), value() ? selection_color() : color() );
            draw_label();
        }
};
