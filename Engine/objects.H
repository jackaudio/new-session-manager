
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

/* these are all the objects whose state must be recorded, this
   includes tracks, regions, etc. Relevant interfaces must be fully compatible
   with those in the Timeline program. */

#include "Audio_File.H"
#include "Loggable.H"

typedef unsigned long nframes_t;

class Track_Widget
{

protected:

    nframes_t _offset;
    nframes_t _start;
    nframes_t _end;
    bool _selected;

};

/* macros to simplify serialization code */
#define START( n ) char **sa = (char **)malloc( sizeof( char * ) * (1 + (n) ) ); int i = 0
#define PUT( fmt, arg ) asprintf( &sa[ i++ ], (fmt), (arg) )
#define END() sa[ i ] = NULL; return sa;

#define SWITCH do
#define CASE( arg ) if ( ! strcmp( s, ( arg ) ) )
#define GET( arg, act ) CASE( arg ) { act ; break; }


class Track : public Loggable
{



};

class Region : public Loggable, public Track_Widget
{

    Track *_track;
    Audio_File *_clip;                                          /* clip this region represents */
    float _scale;                                               /* amplitude adjustment */

protected:

    const char *class_name ( void ) { return "Region"; }

    char ** get ( void )
        {
            START( 7 );

            PUT( ":source \"%s\"", _clip ? _clip->name() : "" );
            PUT( ":track 0x%X", _track ? _track->id() : 0 );
            PUT( ":x %lu", _offset );
            PUT( ":l %lu", _start );
            PUT( ":r %lu", _end );
            PUT( ":selected %d", _selected );
            PUT( ":gain %f", _scale );

            END();
        }

    void
    set ( char **sa )
        {
            for ( int i = 0; sa[i]; ++i )
            {
                char *s = sa[i];
                char *v = s + strlen( s ) + 1;

                SWITCH
                {
                    GET( ":x", _offset = atol( v ) );
                    GET( ":l" , _start = atol( v ) );
                    GET( ":r", _end = atol( v ) );
                    CASE( ":selected" )
                    {
                        if ( atoi( v ) )
                            select();
                        else
                            deselect();

                        break;
                    }
                    GET( ":gain", _scale = atof( v ) );
                    CASE( ":source" )
                    {
                        if ( ! ( _clip = Audio_File::from_file( v ) ) )
                            printf( "Grave error: could not open source \"%s\"\n", v );
                        break;
                    }
                    CASE( ":track" )
                    {
                        int i;
                        sscanf( v, "%X", &i );
                        Track *t = (Track*)Loggable::find( i );

                        assert( t );

                        t->add( this );
                        break;
                    }
                }


                free( s );
            }

            free( sa );
        }
};


class Time_Point : public Track_Widget
{

protected:

    char ** get ( void )
        {
            START( 4 );

            PUT( ":track 0x%X", _track ? _track->id() : 0 );
            PUT( ":x %lu", _offset );
            PUT( ":beats_per_bar %d", _time.beats_per_bar );
            PUT( ":beat_type %d", _time.beat_type );

            END();
        }

    void
    set ( char **sa )
        {
            for ( int i = 0; sa[i]; ++i )
            {
                char *s = sa[i];
                char *v = s + strlen( s ) + 1;

                SWITCH
                {
                    GET( ":x", _offset = atol( v ) );
                    GET( ":beats_per_bar", _time.beats_per_bar = atoi( v ) );
                    GET( ":beat_type", _time.beat_type = atoi( v ) );
                    CASE( ":track" )
                    {
                        int i;
                        sscanf( v, "%X", &i );
                        Track *t = (Track*)Loggable::find( i );

                        assert( t );

                        t->add( this );
                        break;
                    }

                    free( s );
                }

                free( sa );
            }
        }
};
