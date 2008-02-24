
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

#include "Loggable.H"

/* got I hate C++ */
#define __CLASS__ "Region"

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

protected:

    const char *class_name ( void ) { return "Region"; }

    char ** log_dump ( void )
        {
            //          char *r;
            char **sa = (char**)malloc( sizeof( char* ) * 6 );

            sa[5] = NULL;

            asprintf( &sa[0], ":x %lu", _offset );
            asprintf( &sa[1], ":l %lu", _start );
            asprintf( &sa[2], ":r %lu", _end );
            asprintf( &sa[3], ":selected %d", _selected );
            asprintf( &sa[4], ":gain %f", _scale );

//            asprintf( &r, ":x %lu\n:l %lu\n:r %lu\n:selected %d\n:gain %f", _offset, _start, _end, _selected, _scale );

            return sa;
        }

    void
    set ( char **sa )
        {
            for ( int i = 0; sa[i]; ++i )
            {
                char *s = sa[i];

                strtok( s, " " );

                char *v = s + strlen( s );

                if ( ! strcmp( s, ":x" ) )
                    _offset = atol( v );
                else
                    if ( ! strcmp( s, ":l" ) )
                        _start = atol( v );
                    else
                        if ( ! strcmp( s, ":r" ) )
                            _end = atol( v );
                        else
                            if ( ! strcmp( s, ":selected" ) )
                                _selected = atoi( v );
                            else
                                if ( ! strcmp( s, ":gain" ) )
                                    _scale = atof( v );

                free( s );
            }

            free( sa );
        }

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

#undef __CLASS__
