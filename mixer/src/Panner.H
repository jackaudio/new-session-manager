
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

#include <FL/Fl_Group.H>
#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/Fl_Choice.H>
#include <math.h>

#include <vector>
using namespace std;

class Panner : public Fl_Group
{
    Fl_Choice *_range_choice;
    Fl_Choice *_projection_choice;

    void draw_grid( int,int,int,int);
    void draw_the_box( int, int, int, int );

public:

    struct Point
    {
        float x,y,z;

        const char *label;
        void *userdata;
        Fl_Color color;
        bool visible;
        bool radius_enabled;

        Point ( ) {
            x = 1;
            y = 0;
            z = 0;
            label = 0;
            visible = 1;
            color = FL_WHITE;
            radius_enabled = false;
        }

        Point ( float D, float A )
            {
                radius( D );
                azimuth( A );
                label = 0;
                visible = 1;
                color = FL_WHITE;
                radius_enabled = false;
            }

        static inline void spherical_to_cartesian (float a, float e, float &x, float &y, float &z )
            {
                a *= M_PI / 180.0f;
                e *= M_PI / 180.0f;
                
                z = sinf(e);
                const float ce = cosf(e);
                x = ce * cosf(-a);
                y = ce * sinf(-a);
            }
        
        float azimuth ( void ) const {
            return -atan2f( y,x ) * ( 180 / M_PI );
        }
        float elevation ( void ) const { 
            return atan2f(z,sqrtf(powf(x,2)+powf(y,2)) ) * ( 180 / M_PI );
        }
        float radius ( void ) const {
            if ( ! radius_enabled )
                return 1.0f;
            else
                return sqrtf(powf(x,2)+powf(y,2)+powf(z,2));
        }
        
        void azimuth (  float v )
            {
                float r = radius();

                spherical_to_cartesian( v, elevation(), x,y,z );
                x *= r;
                y *= r;
                z *= r;
            }
        
         void elevation ( float v )
            {
                float r = radius();

                spherical_to_cartesian( azimuth(), v, x,y,z );
                x *= r;
                y *= r;
                z *= r;
            }
        

         void radius ( float v )
            {
                if (! radius_enabled )
                    return; 

                float r = v;

                spherical_to_cartesian( azimuth(), elevation(), x,y,z );

                x *= r;
                y *= r;
                z *= r;
            }
    };

private:
    
    float _range;

    
    vector <Point> _points;

    static int _configs[][12];

    void bbox ( int &X, int &Y, int &W, int &H ) const
        {
            W = w() - Fl::box_dw( box() );
            H = h() - Fl::box_dh( box() );
            X = x() + Fl::box_dx( box() );
            Y = y() + Fl::box_dy( box() );

            int S = W > H ? H : W;

            if ( W > H )
                X += ( (W/2) - (S/2) );
            else if ( H > W )
                Y += ( (H/2) - (S/2) );
          
            W = H = S;
        }

    void point_bbox ( const Point *p, int *X, int *Y, int *W, int *H ) const;

    Point * event_point ( void );
    Point angle_to_axes ( float a );

    static Point * drag;

    void set_polar ( Point *p, float x, float y );
    void set_ortho ( Point *p, float x, float y );
    void set_polar_radius ( Point *p, float x, float y );
    void project_polar ( const Point *p, float *X, float *Y, float *S ) const;
    void project_ortho ( const Point *p, float *X, float *Y, float *S ) const;

    int _projection;
    
protected:

    virtual void draw ( void );
    virtual int handle ( int );

public:

    enum { POLAR, ORTHO };

    float projection ( void ) const { return _projection_choice->value(); }
    void projection ( int v ) { _projection_choice->value(v); }

    Panner ( int X, int Y, int W, int H, const char *L = 0 );

    float range ( void ) const { return *((int*)_range_choice->mvalue()->user_data()); }
    /* void range ( float v ) { _range = v; } */

    void clear_points ( void ) { _points.clear(); }

    void add_point( Panner::Point &p )
        {
            _points.push_back( p );
        }

    virtual ~Panner ( ) { }

    Panner::Point *point ( int i );

    int points ( void ) const { return _points.size(); }

    Panner::Point *pushed ( void ) { return drag; }

};
