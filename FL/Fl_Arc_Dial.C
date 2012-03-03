
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


#include "Fl_Arc_Dial.H"

#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>

int Fl_Arc_Dial::_default_style = Fl_Arc_Dial::PLASTIC_DIAL;

/** This simple box is suitable for use with knob-type widgets. It
 * comprises a border with shadow, and a cap with glare-lines akin
 * to those seen on burnished aluminum knobs. */
static void
burnished_oval_box ( int x, int y, int w, int h, Fl_Color c )
{
    /* draw background */
    fl_color( fl_darker( c ) );
    fl_pie( x, y, w, h, 0, 360 );
    fl_color( fl_darker( fl_darker( c ) ) );
    fl_pie( x, y, w, h, 180 + 215, 180 + 45 );

    /* shrink */
    x += 4;
    y += 4;
    w -= 7;
    h -= 7;

    /* draw cap */
    fl_color( c );
    fl_pie( x, y, w, h, 0, 360 );

    /* draw glare */

    const int a1 = 10;
    const int a2 = 90;

    fl_color( fl_color_average( FL_WHITE, c, 0.15f ) );
    fl_pie( x, y, w, h, a1, a2 );
    fl_pie( x, y, w, h, 180 + a1, 180 + a2 );
    fl_color( fl_color_average( FL_WHITE, c, 0.25f ) );

    const int d = (a2 - a1) / 2;
    fl_pie( x, y, w, h, a1 + (d / 2), a2 - (d / 2) );
    fl_pie( x, y, w, h, 180 + a1 + (d / 2), 180 + a2 - (d / 2) );
}




void
Fl_Arc_Dial::draw_box ( void )
{
    if ( type() == ARC_DIAL )
        fl_draw_box( FL_ROUNDED_BOX, x(), y(), w(), h(), color() );
}

int
Fl_Arc_Dial::handle ( int m )
{
    /* Fl_Dial and friends should really handle mousewheel, but they don't in FTLK1 */

    switch ( m )
    {
        case FL_MOUSEWHEEL:
        {
            if ( this != Fl::belowmouse() )
                return 0;

            int steps = 16;

            if ( Fl::event_ctrl() )
                steps = 128;

            float step = fabs( maximum() - minimum() ) / (float)steps;

            float d = ((float)Fl::event_dy()) * step;

            double v = value() + d;

            if ( maximum() > minimum() )
            {
                if ( v < minimum() )
                    v = minimum();
                else if ( v > maximum() )
                    v = maximum();
            }
            else
            {
                if ( v > minimum() )
                    v = minimum();
                else if ( v < maximum() )
                    v = maximum();
            }

            value( v );
            do_callback();

            return 1;
        }
    }

    return Fl_Dial::handle( m );
}

void
Fl_Arc_Dial::draw ( void )
{
    int X = x();
    int Y = y();
    int W = w();
    int H = h();

    draw_box();
    draw_label();

    X += Fl::box_dx(box());
    Y += Fl::box_dy(box());
    W -= Fl::box_dw(box());
    H -= Fl::box_dh(box());

    double angle = ( angle2() - angle1() ) * ( value() - minimum()) / ( maximum() - minimum() ) + angle1();


    X += W / 8;
    Y += H / 8;
    W -= W / 4;
    H -= H / 4;

    if ( type() == ARC_DIAL )
    {
        fl_line_style( FL_SOLID, W / 6 );

        /* background arc */
        fl_color( fl_darker( color() ) );
        fl_arc( X, Y, W, H, 270 - angle1(), 270 - angle2() );

        /* foreground arc */
        fl_color( selection_color() );
        fl_arc( X, Y, W, H, 270 - angle1(), 270 - angle  );

        fl_line_style( FL_SOLID, 0 );

        fl_color( fl_contrast( labelcolor(), color() ) );
    }
    else if ( type() == PLASTIC_DIAL || type() == BURNISHED_DIAL )
    {
//    fl_color( fl_color_average( FL_RED, selection_color(), ( value() - minimum() ) / ( maximum() - minimum() ) ) );
        draw_knob();
        
        fl_line_style( FL_SOLID, W / 6 );
        fl_color( fl_contrast( selection_color(), FL_BACKGROUND_COLOR ) );
        
        const int d = 6;
        
        /* account for edge conditions */
        angle = angle < angle1() + d ? angle1() + d : angle;
        angle = angle > angle2() - d ? angle2() - d : angle;
        
        fl_arc( X + 5, Y + 5 , W - 10, H - 10, 270 - (angle - d), 270 - (angle + d) );

        fl_line_style( FL_SOLID, 0 );

    }

    
    
    char s[10];
    
    fl_font( FL_HELVETICA, 8 );
    
    snprintf( s, sizeof( s ), "%.1f", value() );
    if ( type() == ARC_DIAL )
        fl_draw( s, X, Y, W, H, FL_ALIGN_BOTTOM );
    else
        if ( type() == PLASTIC_DIAL )
        {
                fl_draw( s, x(), y(), w(), H, FL_ALIGN_BOTTOM );
        }


}

void
Fl_Arc_Dial::draw_knob ( void )
{
    int ox, oy, ww, hh, side;
    ox = x();
    oy = y();
    ww = w();
    hh = h();
    draw_label();
    fl_clip(ox, oy, ww, hh);
    if (ww > hh)
    {
        side = hh;
        ox = ox + (ww - side) / 2;
    }
    else
    {
        side = ww;
        oy = oy + (hh - side) / 2;
    }
    side = w() > h() ? hh : ww;

    // background
    /* fl_color(FL_BACKGROUND_COLOR); */
    /* fl_rectf(ox, oy, side, side); */
        
    /* scale color */
    fl_color(fl_color_average(color(), FL_BACKGROUND2_COLOR, .6));


    fl_pie(ox + 1, oy + 3, side - 2, side - 12, 0, 360);
    // scale
    draw_scale(ox, oy, side);

    Fl_Color c = active_r() ? fl_color_average(FL_BACKGROUND_COLOR, FL_WHITE, .7) : FL_INACTIVE_COLOR;

    if ( type() == BURNISHED_DIAL )
    {
        burnished_oval_box( ox + 5, oy + 5, side - 12, side - 12, c );
    }
    else
    {
            
        fl_color(FL_BACKGROUND_COLOR);
        fl_pie(ox + 6, oy + 6, side - 12, side - 12, 0, 360);

        // shadow

        fl_color(fl_color_average(FL_BACKGROUND_COLOR, FL_BLACK, .8f));
        fl_pie(ox + 8, oy + 12, side - 16, side - 16, 0, 360);
        fl_color(fl_color_average(FL_BACKGROUND_COLOR, FL_BLACK, .2f));
        fl_pie(ox + 9, oy + 12, side - 18, side - 18, 0, 360);

        // knob edge
        fl_color( c);


        fl_pie(ox + 8, oy + 8, side - 16, side - 16, 0, 360);

        fl_color(fl_color_average(FL_BACKGROUND_COLOR, FL_WHITE, .6));
        fl_pie(ox + 10, oy + 10, side - 20, side - 20, 0, 360);
        
    }
    fl_pop_clip();
}


void
Fl_Arc_Dial::draw_scale ( int ox, int oy, int side )
{
    float x1, y1, x2, y2, rds, cx, cy, ca, sa;
    rds = side / 2;
    cx = ox + side / 2;
    cy = oy + side / 2;
    if (_scaleticks == 0)
        return;
    double a_step = (10.0 * 3.14159 / 6.0) / _scaleticks;
    double a_orig = -(3.14159 / 3.0);
    for (int a = 0; a <= _scaleticks; a++)
    {
        double na = a_orig + a * a_step;
        ca = cos(na);
        sa = sin(na);
        x1 = cx + (rds) * ca;
        y1 = cy - (rds) * sa;
        x2 = cx + (rds - 6) * ca;
        y2 = cy - (rds - 6) * sa;
        fl_color(FL_BACKGROUND_COLOR);
        fl_line(x1, y1, x2, y2);
    }
}

void 
Fl_Arc_Dial::scaleticks ( int tck )
{
    _scaleticks = tck;
    if (_scaleticks < 0)
        _scaleticks = 0;
    if (_scaleticks > 31)
        _scaleticks = 31;
    if (visible())
        damage(FL_DAMAGE_ALL);
}
