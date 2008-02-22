
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

// class Track;

// #include "Waveform.H"

#include "Audio_File.H"
#include "Track.H"
#include "Timeline.H"

#include <algorithm>
using namespace std;
/* Regions are "virtual" FLTK widgets; this is necessary because the
 * dimensions of real FLTK widgets are limited to 16-bits, which is
 * far too little for our purposes */

#include "Track_Widget.H"

class Region : public Track_Widget
{

    Audio_File *_clip;                                                /* clip this region represents */

    float _scale;                                               /* amplitude adjustment */

    static Fl_Boxtype _box;
    static Fl_Color _selection_color;
    static Fl_Color selection_color ( void ) { return _selection_color; }
    static void selection_color ( Fl_Color v ) { _selection_color = v; }

    enum trim_e { NO, LEFT, RIGHT };
    void trim ( enum trim_e t, int X );
    void init ( void );

public:

    Fl_Boxtype box ( void ) const { return Region::_box; }
    Fl_Align align ( void ) const { return  (Fl_Align)(FL_ALIGN_LEFT | FL_ALIGN_BOTTOM /*| FL_ALIGN_CLIP*/ | FL_ALIGN_INSIDE); }

    Region ( const Region & rhs );
    Region ( Audio_File *c );

    int handle ( int m );
    void draw_box( int X, int Y, int W, int H );
    void draw ( int X, int Y, int W, int H );
    void resize ( void );

    void normalize ( void );

    void dump ( void );

};
