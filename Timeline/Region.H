
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
#include "Sequence.H"
#include "Timeline.H"

/* Regions are "virtual" FLTK widgets; this is necessary because the
 * dimensions of real FLTK widgets are limited to 16-bits, which is
 * far too little for our purposes */

#include "Sequence_Widget.H"
#include "Loggable.H"


class Region : public Sequence_Widget
{

public:


    struct Fade
    {
        enum fade_type_e { Linear = 0, Sigmoid, Logarithmic, Parabolic };
        enum fade_dir_e { In, Out };

        fade_type_e type;
        nframes_t length;

        Fade ( )
            {
                type   = Linear;
                length = 0;
            }

        bool
        operator< ( const Fade &rhs ) const
            {
                return length < rhs.length;
            }

        float increment ( void ) const
            {
                return 1.0f / (float)length;
            }

        /** Return gain for frame /index/ of /nframes/ on a gain curve
         * of type /type/.*/
        /* FIXME: calling a function per sample is bad, switching on
         * type mid fade is bad. */
        inline float
        gain ( const float fi ) const
            {
                switch ( type )
                {
                    case Linear:
                        return fi;
                    case Sigmoid:
                        return (1.0f - cos( fi * M_PI )) / 2.0f;
                    case Logarithmic:
                        return pow( 0.1f, (1.0f - fi) * 3.0f );
                    case Parabolic:
                        return 1.0f - (1.0f - fi) * (1.0f - fi);
                    default:
                        return 1.0f;
                }
            }

        void apply ( sample_t *buf, fade_dir_e dir, long start, nframes_t end, nframes_t nframes ) const;
    };

/*     struct Fade_In : public Fade; */
/*     struct Fade_Out : public Fade; */

private:

    Audio_File *_clip;                                          /* clip this region represents */

    float _scale;                                               /* amplitude adjustment */

    Fade _fade_in;
    Fade _fade_out;

    friend class Track;                                  /* for _clip */
protected:

    const char *class_name ( void ) { return "Region"; }

    char ** get ( void )
        {
            //          char *r;
            char **sa = (char**)malloc( sizeof( char* ) * 8 );

            int i = 0;

            asprintf( &sa[ i++ ], ":source \"%s\"", _clip ? _clip->name() : "" );
            asprintf( &sa[ i++ ], ":track 0x%X", _track ? _track->id() : 0 );
            asprintf( &sa[ i++ ], ":x %lu", _r->offset );
            asprintf( &sa[ i++ ], ":l %lu", _r->start );
            asprintf( &sa[ i++ ], ":r %lu", _r->end );
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
                    _r->offset = atol( v );
                else if ( ! strcmp( s, ":l" ) )
                    _r->start = atol( v );
                else if ( ! strcmp( s, ":r" ) )
                    _r->end = atol( v );
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
                    Sequence *t = (Sequence*)Loggable::find( i );

                    assert( t );

                    t->add( this );
                }

                free( s );
            }

            free( sa );

            if ( _track )
                _track->redraw();
        }

public:

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

    bool current ( void ) const { return this == belowmouse(); }

public:


    const char *source_name ( void ) const { return _clip->name(); }

    static Loggable *
    create ( char **sa )
        {
            Region *r = new Region;

            r->set( sa );

            return (Loggable *)r;
        }


    Sequence_Widget *clone ( const Sequence_Widget *r );

    ~Region ( )
        {
            log_destroy();
        }

    Fl_Boxtype box ( void ) const { return Region::_box; }
    Fl_Align align ( void ) const { return  (Fl_Align)(FL_ALIGN_LEFT | FL_ALIGN_BOTTOM /*| FL_ALIGN_CLIP*/ | FL_ALIGN_INSIDE); }

    Region ( const Region & rhs );
    Region ( Audio_File *c );
    Region ( Audio_File *c, Sequence *t, nframes_t o );

    int handle ( int m );
    void draw_fade ( const Fade &fade, Fade::fade_dir_e dir, bool filled, int X, int W );

    void draw_box( void );
    void draw ( void );
    void resize ( void );

    void normalize ( void );

    /* Engine */
    nframes_t read ( sample_t *buf, nframes_t pos, nframes_t nframes, int channel ) const;
    nframes_t write ( sample_t *buf, nframes_t nframes );
    bool finalize ( void );

};
