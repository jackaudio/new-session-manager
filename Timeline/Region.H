
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

#include "Audio_File.H"
#include "Track.H"
#include "Timeline.H"

/* Regions are "virtual" FLTK widgets; this is necessary because the
 * dimensions of real FLTK widgets are limited to 16-bits, which is
 * far too little for our purposes */

#include "Track_Widget.H"
#include "Loggable.H"

class Region;

/* Base for engine. Just to maintain state. Must be free of FLTK
 * stuff */
class Region_Base : public Track_Widget
{

private:

    Audio_File *_clip;                                          /* clip this region represents */

    float _scale;                                               /* amplitude adjustment */

protected:

    const char *class_name ( void ) { return "Region"; }

    char ** get ( void )
        {
            //          char *r;
            char **sa = (char**)malloc( sizeof( char* ) * 8 );

            int i = 0;

            asprintf( &sa[ i++ ], ":source \"%s\"", _clip ? _clip->name() : "" );
            asprintf( &sa[ i++ ], ":track 0x%X", _track ? _track->id() : 0 );
            asprintf( &sa[ i++ ], ":x %lu", _offset );
            asprintf( &sa[ i++ ], ":l %lu", _start );
            asprintf( &sa[ i++ ], ":r %lu", _end );
            asprintf( &sa[ i++ ], ":selected %d", selected() );
            asprintf( &sa[ i++ ], ":gain %f", _scale );

            sa[ i ] = NULL;

            return sa;
        }

    void
    set ( char **sa )
        {
            for ( int i = 0; sa[i]; ++i )
            {
                char *s = sa[i];

                strtok( s, " " );

                char *v = s + strlen( s ) + 1;

                if  ( *v == '"' )
                {
                    v++;
                    v[ strlen( v ) - 2 ] = '\0';
                }

                if ( ! strcmp( s, ":x" ) )
                    _offset = atol( v );
                else if ( ! strcmp( s, ":l" ) )
                    _start = atol( v );
                else if ( ! strcmp( s, ":r" ) )
                    _end = atol( v );
                else if ( ! strcmp( s, ":selected" ) )
                {
                    if ( atoi( v ) )
                        select();
                    else
                        deselect();
                }
                else if ( ! strcmp( s, ":gain" ) )
                    _scale = atof( v );
                else if ( ! strcmp( s, ":source" ) )
                {
                    if ( ! ( _clip = Audio_File::from_file( v ) ) )
                    {
                        printf( "Grave error: could not open source \"%s\"\n", v );
                    }
                }
                else if ( ! strcmp( s, ":track" ) )
                {
                    int i;
                    sscanf( v, "%X", &i );
                    Track *t = (Track*)Loggable::find( i );

                    assert( t );

                    t->add( this );
                }

                free( s );
            }

            free( sa );

#ifndef ENGINE
            if ( _track )
                _track->redraw();
#endif
        }

public:

    Region_Base ( )
        {
            _start = _offset = _end = 0;
            _clip = NULL;
            _scale = 1.0f;
        }

#ifdef ENGINE
    /* for loggable */
    static Loggable *
    create ( char **sa )
        {
            Region_Base *r = new Region_Base;

            r->set( sa );

            return (Loggable *)r;
        }
#else
    friend class Region;
#endif

};

#ifndef ENGINE
class Region : public Region_Base
{

    static Fl_Boxtype _box;
    static Fl_Color _selection_color;
    static Fl_Color selection_color ( void ) { return _selection_color; }
    static void selection_color ( Fl_Color v ) { _selection_color = v; }

    enum trim_e { NO, LEFT, RIGHT };
    void trim ( enum trim_e t, int X );
    void init ( void );

    Region ( )
        {
            init();
        }

    bool current ( void ) const { return this == Track::belowmouse(); }

public:

    static Loggable *
    create ( char **sa )
        {
            Region *r = new Region;

            r->set( sa );

            return (Loggable *)r;
        }

    ~Region ( )
        {
            log_destroy();
        }

    Fl_Boxtype box ( void ) const { return Region::_box; }
    Fl_Align align ( void ) const { return  (Fl_Align)(FL_ALIGN_LEFT | FL_ALIGN_BOTTOM /*| FL_ALIGN_CLIP*/ | FL_ALIGN_INSIDE); }

    Region ( const Region & rhs );
    Region ( Audio_File *c );
    Region ( Audio_File *c, Track *t, nframes_t o );

    int handle ( int m );
    void draw_box( int X, int Y, int W, int H );
    void draw ( int X, int Y, int W, int H );
    void resize ( void );

    void normalize ( void );

    void dump ( void );

};
#endif
