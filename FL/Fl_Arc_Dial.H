
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

/* simple stylistic variation on Fl_Dial */

#pragma once

#include <FL/Fl_Dial.H>

class Fl_Arc_Dial : public Fl_Dial
{
    static int _default_style;

    int _scaleticks;

    void draw_knob ( void );
    void draw_scale ( int ox,  int oy,  int side );
    void draw_cursor ( int ox,  int oy,  int sidei );


protected:

    virtual int handle ( int );
    virtual void draw ( void );
    virtual void draw_box ( void );
    
public:

    void scaleticks ( int tck );

    int
    type ( void ) const
        { 
        if ( Fl_Dial::type() == DEFAULT )
            return Fl_Arc_Dial::_default_style;
        else
            return Fl_Dial::type();
        }

    void type ( int n )
        { 
            Fl_Dial::type( n );
        }
    
    static void default_style ( int n ) { Fl_Arc_Dial::_default_style = n; }

    enum 
    {
        DEFAULT,
        BURNISHED_DIAL,
        ARC_DIAL,
        PLASTIC_DIAL
    };


    Fl_Arc_Dial ( int X, int Y, int W, int H, const char *L = 0 ) :
        Fl_Dial( X, Y, W, H, L )
        {
            _scaleticks = 12;
            
            box( FL_NO_BOX );
        }

};
