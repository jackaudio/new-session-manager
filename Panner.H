
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

#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>
#include <FL/Fl.H>

#include <math.h>

#include <vector>
using namespace std;

class Panner : public Fl_Widget
{

    struct Point
    {
        /* axes */
        float x, y;

        Point ( ) : x( 0.0f ), y( 0.0f ) { }
        Point ( float X, float Y ) : x( X ), y( Y ) { }


        /* return the distance between the point and that referenced by /p/ */
        float
        distance ( const Point &p )
            {
                return sqrt( pow( x - p.x, 2 ) + pow( y - p.y, 2 ) );
            }
    };


    /* channel configuration */
    int _ins,
        _outs;

    vector <Point> _points;

    static int pw ( void ) { return 6; }
    static int ph ( void ) { return 6; }

    static int _configs[][12];

    void bbox ( int &X, int &Y, int &W, int &H )
        {
            W = w() - Fl::box_dw( box() );
            H = h() - Fl::box_dh( box() );
            X = x() + Fl::box_dx( box() );
            Y = y() + Fl::box_dy( box() );
        }

    Point * event_point ( void );
    Point angle_to_axes ( float a );

public:

    Panner ( int X, int Y, int W, int H, const char *L = 0 ) :
        Fl_Widget( X, Y, W, H, L )
        {
            _ins = _outs = 4;
            _points.push_back( Point( -1, -1 ) );
            _points.push_back( Point( 1, 1 ) );
            _points.push_back( Point( -1, 1 ) );
            _points.push_back( Point( 1, -1 ) );

            _outs = 5;
        }

    virtual ~Panner ( ) { }

    virtual void draw ( void );
    virtual int handle ( int );


};
