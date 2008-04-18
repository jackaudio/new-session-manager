
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

#include "Loggable.H"
#include "Track_Point.H"

class Ruler_Point : public Track_Point
{

public:

    const char *name ( void ) const { return _label; }
    void name ( const char *s )
        {
            if ( _label )
                free( _label );
            _label = strdup( s );
        }

protected:

    const char *class_name ( void ) { return "Ruler_Point"; }

    char ** get ( void )
        {
            char **sa = (char**)malloc( sizeof( char* ) * 4 );

            int i = 0;

            asprintf( &sa[i++], ":track 0x%X", _track ? _track->id() : 0 );
            asprintf( &sa[i++], ":x %lu", _r->offset );
            asprintf( &sa[i++], ":label \"%s\"", _label );

            sa[i] = NULL;

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

                if ( ! strcmp( s, ":x" ) )
                    _r->offset = atol( v );
                else if ( ! strcmp( s, ":label" ) )
                    name( v );
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

            timeline->redraw();
        }

    Ruler_Point ( )
        {

        }

public:

    /* for loggable */
    static Loggable *
    create ( char **sa )
        {
            Ruler_Point *r = new Ruler_Point;

            r->set( sa );

            return (Loggable *)r;
        }


    Ruler_Point ( nframes_t when, const char *name )
        {
            _r->offset = when;

            _label = strdup( name );

            log_create();
        }

    Ruler_Point ( const Ruler_Point &rhs )
        {
            _r->offset = rhs._r->offset;
            _label = strdup( rhs._label );
        }

    Track_Widget *clone ( const Track_Widget *r )
        {
            return new Ruler_Point( *(Ruler_Point*)r );
        }

    ~Ruler_Point ( )
        {
            if ( _label ) free( _label );
            log_destroy();
        }


    int
    handle ( int m )
        {
            int r = Track_Widget::handle( m );

            if ( m == FL_RELEASE )
            {
                _track->sort();
                timeline->redraw();
            }

            return r;
        }

};
