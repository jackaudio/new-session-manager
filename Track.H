
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

#include <FL/Fl_Group.H>
#include <FL/Fl.H>
#include "Region.H"

#include <stdio.h>

#include <list>
using std::list;

class Track : public Fl_Group
{
    Track *_next;
    Track *_prev;

    list <Region *> _regions;

    char *_name;

public:

    Track ( int X, int Y, int W, int H ) : Fl_Group( X, Y, W, H )
        {
            _next = _prev = NULL;
            _name = NULL;

            box( FL_DOWN_BOX );
            color( fl_darker( FL_GRAY ) );
        }

    Track *next ( void ) { return _next; }
    Track *prev ( void ) { return _prev; }
    void prev ( Track *t ) { _prev = t; }
    void next ( Track *t ) { _next = t; }

    void remove_region ( Region *r )
        {
            _regions.remove( r );
        }

    void add ( Region *r )
        {

            printf( "add" );

            if ( r->track() )
            {
                r->track()->remove_region( r );
                r->track()->redraw();
            }

            _regions.push_back( r );

            r->track( this );

            Fl_Group::add( r );
//            add( r );

            r->position( r->x(), y() );
            r->redraw();
        }

    int handle ( int m )
        {
            switch ( m )
            {
		case FL_DND_DRAG:
		case FL_DND_ENTER:
		case FL_ENTER:
                    return 1;
		case FL_DND_LEAVE:
		case FL_DND_RELEASE:
                    return 1;
                case FL_PASTE:
                {
                    const char *text = Fl::event_text();

                    char *file;

                    if ( ! sscanf( text, "file://%a[^\r\n]\n", &file ) )
                    {
                        printf( "invalid drop \"%s\"\n", text );
                        return 0;
                    }

                    printf( "pasted file \"%s\"\n", file );

                    Clip *c = Clip::from_file( file );

                    if ( ! c )
                    {
                        free( file );
                        return 0;
                    }


                    Region *r = new Region( c );

                    r->offset( timeline.x_to_ts( Fl::event_x() ) );
                    r->position( Fl::event_x(), r->y() );

                    this->add( r );

                    return 1;
                }
                default:
                    return Fl_Group::handle( m );
            }
        }
};
