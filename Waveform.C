
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
#include "Clip.H"

// extern Timeline timeline;
// #include "Timeline.H"

#include <math.h>

extern Fl_Color velocity_colors[];

Waveform::Waveform ( int X, int Y, int W, int H, const char *L ) : Fl_Widget( X, Y, W, H, L )
{
    _scale = 1;
    _clip = NULL;

    _start = _end = 0;
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
Waveform::read_peaks ( int X, float *hi, float *lo )
{
    _clip->peaks()->read( X, hi, lo );
}

void
Waveform::draw ( int X, int Y, int W, int H )
{
    fl_push_clip( X, Y, W, H );

    fl_push_matrix();

    int j;

//    int start = (_start + (X - x())) * 2;

    j = 0;
    for ( int x = X; x < X + W; ++x )
    {

//        read_peaks( x, &hi, &lo );
        Peak p = (*_clip->peaks())[  x ];

        int mid = Y + (H / 2);

        // FIXME: cache this stuff.
//        fl_color( fl_color_average( selection_color(), fl_contrast( fl_darker( FL_BLUE ), selection_color() ),  fabs( hi - lo ) ) );
        fl_color( fl_color_average( FL_RED, selection_color(),  fabs( p.max - p.min ) ) );

        p.max *= _scale;
        p.min *= _scale;

        if ( p.min < -1.0 || p.max > 1.0 )
            fl_color( FL_RED );

        fl_line( x, mid + (H / 2 * p.min), x, mid + (H / 2 * p.max) );

    }

    fl_color( fl_darker( fl_darker( selection_color() ) ) );


    fl_line_style( FL_SOLID, 2 );

    fl_begin_line();

    for ( int x = X; x < X + W; ++x )
    {
        Peak p = (*_clip->peaks())[ x ];

        p.min *= _scale;

        fl_vertex( x, Y + (H / 2) + ((float)H / 2 *  p.min ));
    }

    fl_end_line();

    fl_begin_line();

    for ( int x = X; x < X + W; ++x )
    {
        Peak p = (*_clip->peaks())[ x ];

        p.max *= _scale;

        fl_vertex( x, Y + (H / 2) + ((float)H / 2 * p.max ));
    }

    fl_end_line();

    fl_line_style( FL_SOLID, 0 );

    fl_pop_matrix();

    fl_pop_clip();
}


void
Waveform::normalize ( void )
{
    float mhi, mlo;

    _clip->peaks()->downsample( _start, _end, &mhi, &mlo );

    _scale = 1.0f / (float)mhi;

    if ( _scale * mlo < -1.0 )
        _scale = 1 / fabs( mlo );

    _scale = fabs( _scale );

    redraw();
}
