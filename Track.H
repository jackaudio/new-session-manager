
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
// #include "Region.H"

#include <stdio.h>

#include "Loggable.H"

#include <assert.h>

#include <list>
using std::list;

class Region;
class Track_Widget;

class Track : public Fl_Group, public Loggable
{
    Track *_next;
    Track *_prev;
    char *_name;

    static Track_Widget * _queued_widget;

protected:

    list <Track_Widget *> _widgets;
    Track_Widget *event_widget ( void );

    const char *class_name ( void ) { return "Track"; }


    void set ( char ** ) { return; }

    char ** get ( void )
        {
            //          char *r;

            char **sa = (char**)malloc( sizeof( char* ) * 2);
            sa[0] = (char*)malloc( (_widgets.size() * ((sizeof( int ) * 2) + 3)) + 1 );
            sa[1] = NULL;

            char *s = sa[0];

            s += sprintf( s, ":items " );
            for ( list <Track_Widget *>::const_iterator i = _widgets.begin(); i != _widgets.end(); i++ )
            {
                s += sprintf( s, "0x%X", ((Loggable*)(*i))->id() );

                list <Track_Widget *>::const_iterator e = i;
                if ( ++e != _widgets.end() )
                    s += sprintf( s, "," );
            }

            return sa;
        }


public:


    Track ( int X, int Y, int W, int H ) : Fl_Group( X, Y, W, H )
        {
            _next = _prev = NULL;
            _name = NULL;

            box( FL_DOWN_BOX );
            color( fl_darker( FL_GRAY ) );

            end();

            log_create();
        }

    virtual ~Track (  )
        {
            /* FIXME: what to do with regions? */
            parent()->redraw();
            parent()->remove( this );
            log_destroy();
        }

    Track *next ( void ) const { return _next; }
    Track *prev ( void ) const { return _prev; }
    void prev ( Track *t ) { _prev = t; }
    void next ( Track *t ) { _next = t; }

    void sort ( void );

    void draw ( void );
    void remove ( Track_Widget *r );
    void add ( Track_Widget *r );

    void remove_selected ( void );

    const list <Track_Widget *> widgets ( void ) const { return _widgets; }

    void queue_delete ( Track_Widget *r )
        {
            assert( ! _queued_widget );

            _queued_widget = r;
        }

    virtual void snap ( Track_Widget *r );
    virtual int handle ( int m );

};
