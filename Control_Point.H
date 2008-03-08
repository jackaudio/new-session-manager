
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


    Control_Point ( Track *t, nframes_t when, float y )
        {
            _track = t;
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

            switch ( m )
            {

                case FL_RELEASE:
                    _track->sort();
                    redraw();
                    break;
                case FL_DRAG:
                {
                    int Y = Fl::event_y() - parent()->y();

                    if ( Y >= 0 && Y < parent()->h() )
                    {
                        _y = (float)Y / parent()->h();
                        redraw();
                    }
                    break;
                }
            }

            return r;
        }

    int y ( void ) const { return parent()->y() + ((float)parent()->h() * _y); }
    int w ( void ) const { return 6; }
    int h ( void ) const { return 6; }

/*     void */
/*     draw_box ( int X, int Y, int W, int H ) */
/*         { */
/*         } */

     void
    draw ( int X, int Y, int W, int H )
        {

//            fl_draw_box( box(), x(), y(), w(), h(), box_color() );
            return;
        }

};
