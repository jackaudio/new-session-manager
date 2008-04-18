
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

#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>
#include <FL/Fl.H>
// #include "Region.H"

#include <stdio.h>

#include "Loggable.H"

#include <assert.h>

#include <list>

// using namespace std;

class Region;
class Track_Widget;


#include "types.h"

/* This is the base class for all track types. */

class Track : public Fl_Widget, public Loggable
{

    char *_name;

    static queue <Track_Widget *> _delete_queue;

protected:

    std::list <Track_Widget *> _widgets;
    Track_Widget *event_widget ( void );

    virtual const char *class_name ( void ) { return "Track"; }


    void set ( char ** ) { return; }

    char ** get ( void )
        {
            //          char *r;

            char **sa = (char**)malloc( sizeof( char* ) * 2);
            sa[0] = (char*)malloc( (_widgets.size() * ((sizeof( int ) * 2) + 3)) + 1 );
            sa[1] = NULL;

            sa[0][0] = '\0';

/*             char *s = sa[0]; */

/*             s += sprintf( s, ":items " ); */
/*             for ( list <Track_Widget *>::const_iterator i = _widgets.begin(); i != _widgets.end(); i++ ) */
/*             { */
/*                 s += sprintf( s, "0x%X", ((Loggable*)(*i))->id() ); */

/*                 list <Track_Widget *>::const_iterator e = i; */
/*                 if ( ++e != _widgets.end() ) */
/*                     s += sprintf( s, "," ); */
/*             } */

            return sa;
        }


public:


    Track ( int X, int Y, int W, int H );
    virtual ~Track (  );

    const char * name ( void ) const { return _name; }
    void name ( char *s ) { if ( _name ) free( _name ); _name = s; label( _name ); }

    void sort ( void );

    void remove ( Track_Widget *r );
    void add ( Track_Widget *r );

    void select_range ( int X, int W );

    void remove_selected ( void );

    const std::list <Track_Widget *> widgets ( void ) const { return _widgets; }

    void queue_delete ( Track_Widget *r )
        {
            _delete_queue.push( r );
        }

    Track_Widget * overlaps ( Track_Widget *r );

    virtual Track * clone ( void )
        {
            assert( 0 );
        }

    virtual Track * clone_empty ( void )
        {
            return NULL;
        }

    virtual void snap ( Track_Widget *r );
    virtual int handle ( int m );
    virtual void draw ( void );

    virtual nframes_t process ( nframes_t nframes ) { return 0; }

};
