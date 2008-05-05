
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

#include "Sequence.H"
#include "Tempo_Point.H"

#include <list>

class Tempo_Sequence : public Sequence
{

protected:

    /* not used */
    void get ( Log_Entry &e ) const { }
    void set ( Log_Entry &e ) { }

public:

    Fl_Cursor cursor ( void ) const { return FL_CURSOR_DEFAULT; }

    Tempo_Sequence ( int X, int Y, int W, int H ) : Sequence ( X, Y, W, H )
        {
//            box( FL_DOWN_BOX );
        }


    /** return a pointer to the closest tempo point *before* /when/ */
    Tempo_Point *
    at ( nframes_t when )
        {
            for ( std::list <Sequence_Widget *>::const_reverse_iterator i = _widgets.rbegin();
                  i != _widgets.rend(); i++ )
                if ( (*i)->offset() < when )
                    return ((Tempo_Point*)(*i));

            return NULL;
        }

    float
    beats_per_minute ( nframes_t when )
        {
//            sort();

            Tempo_Point *p = at( when );

            if ( p )
                return p->tempo();
            else
                return 120.0;
        }

    void
    beats_per_minute ( nframes_t when, float bpm )
        {
            add( new Tempo_Point( when, bpm ) );
            sort();
        }

    int
    handle ( int m )
        {
            int r = Sequence::handle( m );

            if ( r )
                return r;

            switch ( m )
            {
                case FL_PUSH:
                    if ( Fl::event_button1() )
                    {
                        static float t = 120.0f;

                        if ( Tempo_Point::edit( &t ) )
                        {
                            add( new Tempo_Point( timeline->x_to_offset( Fl::event_x() ), t ) );

                            timeline->redraw();
                        }
                        return 0;
                    }
                default:
                    return 0;

            }

        }
};
