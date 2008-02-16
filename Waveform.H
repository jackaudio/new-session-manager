
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

typedef unsigned long tick_t;

class Waveform : public Fl_Widget
{

protected:

    float *_peaks;

    tick_t _start;
    tick_t _end;

    float _scale;                                               /* vertical scaling */
    float _zoom;                                                /* horizontal zoom */

public:

    Waveform ( int X, int Y, int W, int H, const char *L=0 );

    Waveform ( const Waveform & rhs ) : Fl_Widget( rhs.x(), rhs.y(), rhs.w(), rhs.h(), rhs.label() )
        {
//            Fl_Widget::resize( rhs.x(), rhs.y(), rhs.w(), rhs.h() );

            _peaks = rhs._peaks;
            _start = rhs._start;
            _end   = rhs._end;
        }

    int handle ( int m );
    void bubble_draw ( void );
    void draw ( void );
    void draw ( int X, int Y, int W, int H );

    void start ( tick_t s ) { _start = s; }
    void end ( tick_t e ) { _end = e; }
    void peaks ( float *p ) { _peaks = p; }

};
