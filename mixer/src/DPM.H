
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

#include "Meter.H"

class DPM : public Meter
{
    char peak_string[10];

    int _segments;
    int _pixels_per_segment;
    int _last_drawn_hi_segment;

    float _value;

    int pos ( float v )
        {
            float pv = deflection( v ) * ( _segments - 1 );
            
            return pv;
        }

    static float _dim;
    static Fl_Color _gradient[];
    static Fl_Color _dim_gradient[];

    Fl_Color
    div_color ( int i )
        {
            return _gradient[ i * 127 / _segments ];
        }

    Fl_Color
    dim_div_color ( int i )
        {
            return _dim_gradient[ i * 127 / _segments ];
        }


protected:

    virtual void draw_label ( void );
    virtual void draw ( void );
    virtual void resize ( int, int, int, int );

    void bbox ( int &X, int &Y, int &W, int &H );

public:

    DPM ( int X, int Y, int W, int H,  const char *L = 0 );

    void value ( float v )
        {
            if ( _value != v )
            {
                if ( pos( v ) != pos( _value ) ) 
                    Meter::value( v );
            }

            _value = v;
        }

    float value ( void ) const
        {
            return _value;
        }

    void pixels_per_segment ( int v ) { _pixels_per_segment = v; }

    float dim ( void ) const { return _dim; }
    void dim ( float v ) { _dim = v; if ( visible_r() ) redraw(); }

    static
    void
    blend ( int nbreaks, int* b, Fl_Color *c )
        {
            for ( int i = 0; i < nbreaks - 1; i++ )
            {
                int k = 0;
                for ( int j = b[i]; j <= b[i+1]; j++, k++ )
                    _gradient[ j ] = fl_color_average( c[i+1], c[i], ( k ) / (float)(b[i+1] - b[i] ));
            }

            for ( int i = 0; i < 128; i++ )
                _dim_gradient[ i ] = fl_color_average( FL_BLACK, _gradient[ i ], _dim );
        }
};
