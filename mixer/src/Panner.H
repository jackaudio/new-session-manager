
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

        /* distance from center (from 0 to 1) */
        float d;
        /* angle */
        float a;

        Point ( ) : d( 0.0f ), a( 0.0f ) { }
        Point ( float D, float A ) : d( D ), a( A ) { }

        /** translate angle /a/ into x/y coords and place the result in /X/ and /Y/ */
        void
        axes ( float *X, float *Y ) const
            {
                /* rotate */
                float A = ( 270 - a ) * ( M_PI / 180 );

                *X = -d * cosf( A );
                *Y = d * sinf( A );
            }

        float azimuth ( void ) const
            {
                return a;
            }

        float elevation ( void ) const
            {
                return ( 1.0f - d ) * 90.0f;
            }

        /** set point position in X, Y coordinates (0.0 to 1.0) */
        void
        angle ( float X1, float Y1 )
            {

                float X2, Y2;

                Y2 = X2 = 0;

                float t;

                t = atan2( X2 - X1, Y2 - Y1 );

                a = t * (180 / M_PI);

                if ( a < 0 )
                    a = 360 + a;

                a = 360 - a;

                /* standard distance calculation */
                d = sqrt( pow( X2 - X1, 2 ) + pow( Y2 - Y1, 2 ) );

                if ( d > 1.0f )
                    d = 1.0f;
            }

        /** return the distance between the point and that referenced by /p/ */
        float
        distance ( const Point &p )
            {
                /* first, transform point coords */

                float x1, y1, x2, y2;

                axes( &x1, &y1 );
                p.axes( &x2, &y2 );

                /* standard distance calculation */
                return sqrt( pow( x1 - x2, 2 ) + pow( y1 - y2, 2 ) );
            }

    };


    /* channel configuration */
    int _ins,
        _outs;

    bool _bypassed;

    vector <Point> _points;

    static int pw ( void ) { return 12; }
    static int ph ( void ) { return 12; }

    static int _configs[][12];

    void bbox ( int &X, int &Y, int &W, int &H ) const
        {
            W = w() - Fl::box_dw( box() );
            H = h() - Fl::box_dh( box() );
            X = x() + Fl::box_dx( box() );
            Y = y() + Fl::box_dy( box() );
        }

    void point_bbox ( const Point *p, int *X, int *Y, int *W, int *H ) const;

    Point * event_point ( void );
    Point angle_to_axes ( float a );

    enum {
        NONE = -1,
        R    = 90,
        L    = 270,
        C   = 0,
        FL   = 315,
        FR   = 45,
        RL   = 225,
        RR   = 135,
    };


protected:

    virtual void draw ( void );
    virtual int handle ( int );

public:


    Panner ( int X, int Y, int W, int H, const char *L = 0 ) :
        Fl_Widget( X, Y, W, H, L )
        {
            _bypassed = false;

            _ins = 1;

            _outs = 1;

//            _ins = _outs = 4;

//            _points.push_back( Point( 1, FL ) );
            _points.push_back( Point( 1, C ) );

/*             _points.push_back( Point( 1, FR ) ); */
/*             _points.push_back( Point( 1, RL ) ); */
/*             _points.push_back( Point( 1, RR ) ); */


        }

    virtual ~Panner ( ) { }

    Panner::Point point ( int i );

};
