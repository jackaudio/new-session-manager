
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

#pragma once

#include <FL/Fl_Valuator.H> // for FL_HORIZONTAL and FL_VERTICAL

class VU_Meter : public Fl_Widget
{

    float _peak;

    float _value;
    float _minimum;
    float _maximum;

    int _type;
    int _divisions;
    float _dim;

    Fl_Color _min_color;
    Fl_Color _max_color;

    int pos ( float v ) { return ( v / _maximum ) * _divisions; }

protected:

    virtual void draw ( void );
    virtual int handle ( int m );

public:

    VU_Meter ( int X, int Y, int W, int H,  const char *L = 0 );

    void maximum( float v ) { _maximum = v; redraw(); }
    float maximum ( void ) const { return _maximum; }

    void minimum ( float v ) { _minimum = v; redraw(); }
    float minimum ( void ) const { return _minimum; }

    void value ( float v ) { if ( pos( v ) != pos( _value ) ) redraw();  _value = v; }
    float value ( void ) const { return _value; }

    void reset ( void ) { _peak = 0.0f; }

    bool divisions ( void ) const { return _divisions; }
    void divisions ( int v ) { _divisions = v; }

    int type ( void ) const { return _type; }
    void type ( int v ) { _type = v; }

    float dim ( void ) const { return _dim; }
    void dim ( float v ) { _dim = v; redraw(); }

    Fl_Color min_color ( void ) const { return _min_color; }
    void min_color ( Fl_Color v ) { _min_color = v; }

    Fl_Color max_color ( void ) const { return _max_color; }
    void max_color ( Fl_Color v ) { _max_color = v; }

};
