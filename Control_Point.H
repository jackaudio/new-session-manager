
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

class Control_Point : public Track_Point
{
    float _y;

/*     void */
/*     _make_label ( void ) */
/*         { */
/*             if ( ! _label ) */
/*                 _label = new char[40]; */

/*             snprintf( _label, 40, "%.1f", _y ); */
/*         } */

protected:

    const char *class_name ( void ) { return "Control_Point"; }

    char ** get ( void )
        {
            char **sa = (char**)malloc( sizeof( char* ) * 4 );

            int i = 0;

            asprintf( &sa[i++], ":track 0x%X", _track ? _track->id() : 0 );
            asprintf( &sa[i++], ":x %lu", _offset );
            asprintf( &sa[i++], ":y %.2f", _y );

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
                    if ( ! strcmp( s, ":control" ) )
                        _y = atof( v );
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

            timeline->redraw();

            //          _make_label();
        }

    Control_Point ( )
        {

        }

public:

    /* for loggable */
    static Loggable *
    create ( char **sa )
        {
            Control_Point *r = new Control_Point;

            r->set( sa );

            return (Loggable *)r;
        }


    Control_Point ( nframes_t when, float y )
        {
            _y = y;
            _offset = when;

//            _make_label();

            log_create();
        }

    ~Control_Point ( )
        {
//            if ( _label ) delete[] _label;
            log_destroy();
        }

    float control ( void ) const { return _y; }
    void control ( float v ) { _y = v; }

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

    int
    y ( void ) const
        {
            return _track->y() + ((float)h() * _y);
        }

    void
    draw_box ( int X, int Y, int W, int H )
        {
//            Track_Widget::draw_box( x(), y(), w(), h() );
        }

     void
    draw ( int X, int Y, int W, int H )
        {
            fl_color( FL_RED );
            fl_draw_box( FL_UP_BOX, x(), y(), 6, 6, FL_RED );
//            fl_rectf( x(), y() + ry, 6, 6 );
        }
};
