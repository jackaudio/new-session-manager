
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

#include <stdio.h>

#include <FL/Enumerations.H>
#include <FL/Fl.H>

#include "Waveform.H"


#include <math.h>

extern Fl_Color velocity_colors[];


Waveform::Waveform ( int X, int Y, int W, int H, const char *L ) : Fl_Widget( X, Y, W, H, L )
{


}

#if 0
void
Waveform::buble_draw ( void )
{
    int inc = 10;
    for ( tick_t x = 0; x < w() && x < _end; ++x )
    {
        float v1 = _peaks[ _start + (x * inc) ] / (float)127;
        int lh1 = (float)(h() / 2) * fabs( v1 );
        int ly1 = (h() / 2) - (lh1 / 2);


        fl_color( selection_color() );

        fl_color( velocity_colors[ 127 - (int)(127 * fabs( v1 )) ] );

        fl_pie( x, y() + ly1, inc, lh1, 0, 360 );

        fl_color( fl_darker( selection_color() ) );
        fl_arc( x, y() + ly1, inc, lh1, 0, 360 );

    }
}
#endif

int
Waveform::handle ( int m )
{

    if ( m == FL_PUSH )
    {
        switch ( Fl::event_button() )
        {
            case 1:
                _start += 100;
                _end += 100;
                redraw();
                break;
            case 3:
                _start -= 100;
                _end -= 100;
                redraw();
                break;
            case 2:
                selection_color( selection_color() + 10 );
                redraw();
                break;
            default:
                return 0;
        }

        return 1;
    }

    return 0;
}


int measure = 50;

void
Waveform::draw ( void )
{
//    draw_box( FL_PLASTIC_UP_BOX, x(), y(), w(), h(), color() );

/*     fl_color( fl_lighter( color() ) ); */

/*     for ( int nx = x(); nx < x() + w(); ++nx ) */
/*         if ( ! (nx % measure) ) */
/*             fl_line( nx, y(), nx, y() + h() ); */

    int X, Y, W, H;

    fl_clip_box( x(), y(), w(), h(), X, Y, W, H );

    draw( X, y(), W, h() );
}

void
Waveform::draw ( int X, int Y, int W, int H )
{
//    fl_push_clip( X, Y, W, H );

    fl_push_matrix();

    fl_color( selection_color() );

    int j;

    float _scale = 1;

    int start = (_start + (X - x())) * 2;

    j = 0;
    for ( int x = X; x < X + W; ++x )
    {
        float lo = _peaks[ start + j++ ] * _scale;
        float hi = _peaks[ start + j++ ] * _scale;

        int mid = Y + (H / 2);

        fl_line( x, mid + (H * lo), x, mid + (H * hi) );

    }

    fl_color( fl_darker( fl_darker( selection_color() ) ) );

    fl_line_style( FL_SOLID, 2 );

    fl_begin_line();

    j = 0;
    for ( int x = X; x < X + W; ++x )
    {
        float v = _peaks[ start + j ] * _scale;
        j += 2;
        fl_vertex( x, Y + (H / 2) + ((float)H *  v ));
    }

    fl_end_line();

    fl_begin_line();

    j = 1;
    for ( int x = X; x < X + W; ++x )
    {
        float v = _peaks[ start + j ] * _scale;
        j += 2;
        fl_vertex( x, Y + (H / 2) + ((float)H *  v ));
    }

    fl_end_line();

    fl_line_style( FL_SOLID, 0 );

    fl_pop_matrix();

    //  fl_pop_clip();
}

#if 0
void
Waveform::draw ( void )
{
    fl_push_clip( x(), y(), w(), h() );
    fl_push_matrix();

    int inc = 1;
    int j = 0;
    for ( tick_t x = 0; x < w() && x < _end; ++x )
    {
        float lo = _peaks[ _start + j++ ];
        float hi = _peaks[ _start + j++ ];

        int mid = y() + (h() / 2);

        int rx = this->x() + x;

        fl_color( selection_color() );

        fl_line( rx, mid + (h() * lo), rx, mid + (h() * hi) );
    }

    fl_pop_matrix();

}
#endif
