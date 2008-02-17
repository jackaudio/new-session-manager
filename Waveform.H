
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

#include "Timeline.H"

#include "Clip.H"

class Waveform : public Fl_Widget
{

protected:

    Clip *_clip;                                                /* clip this waveform represents */

    nframes_t _start;
    nframes_t _end;

    float _scale;                                               /* vertical scaling */
    float _zoom;                                                /* horizontal zoom */

public:

    Waveform ( int X, int Y, int W, int H, const char *L=0 );

    Waveform ( Clip *c ) : Fl_Widget( 0, 0, 500, 100, "" )
        {
            _clip = c;
            _scale = 1;
            label( _clip->name() );

            _end = _clip->length();
        }

    Waveform ( const Waveform & rhs ) : Fl_Widget( rhs.x(), rhs.y(), rhs.w(), rhs.h(), rhs.label() )
        {
            _clip  = rhs._clip;
            _start = rhs._start;
            _end   = rhs._end;

            _scale = rhs._scale;
        }


    void draw ( void );
    void draw ( int X, int Y, int W, int H );

    void start ( nframes_t s ) { _start = s; }
    void end ( nframes_t e ) { _end = e; }
//    void peaks ( float *p ) { _peaks = p; }
    void normalize ( void );

    void read_peaks ( int X, float *hi, float *lo );

};
