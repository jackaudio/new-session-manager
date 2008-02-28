
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

#include "Track_Point.H"

#define __CLASS__ "Tempo_Point"

class Tempo_Point : public Track_Point
{
    float _tempo;

    void
    _make_label ( void )
        {
            if ( ! _label )
                _label = new char[40];

            snprintf( _label, 40, "%.1f", _tempo );
        }

protected:

    const char *class_name ( void ) { return "Tempo_Point"; }

    char ** get ( void )
        {
            char **sa = (char**)malloc( sizeof( char* ) * 4 );

            int i = 0;

            asprintf( &sa[i++], ":track 0x%X", _track ? _track->id() : 0 );
            asprintf( &sa[i++], ":x %lu", _offset );
            asprintf( &sa[i++], ":tempo %.2f", _tempo );

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
                    _offset = atol( v );
                else
                    if ( ! strcmp( s, ":tempo" ) )
                        _tempo = atof( v );
                    else
                        if ( ! strcmp( s, ":track" ) )
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

            timeline->rulers->redraw();
            timeline->tracks->redraw();


            _make_label();
        }

    Tempo_Point ( )
        {

        }

public:

    /* for loggable */
    static Loggable *
    create ( char **sa )
        {
            Tempo_Point *r = new Tempo_Point;

            r->set( sa );

            r->log_create();

            return (Loggable *)r;
        }


    Tempo_Point ( nframes_t when, float bpm )
        {
            _tempo = bpm;
            _offset = when;

            _make_label();

            log_create();
        }

    ~Tempo_Point ( )
        {
            if ( _label ) delete[] _label;
            log_destroy();
        }

    float tempo ( void ) const { return _tempo; }
    void tempo ( float v ) { _tempo = v; }

    int
    handle ( int m )
        {
            int r = Track_Widget::handle( m );

            if ( m == FL_RELEASE )
            {
                _track->sort();
                timeline->rulers->redraw();
                timeline->tracks->redraw();
            }
            return r;
        }

};
#undef __CLASS__
