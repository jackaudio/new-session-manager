
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

#include <stdio.h>

#include "Loggable.H"

#include <assert.h>

#include <list>

// using namespace std;

class Track;
class Sequence_Widget;

#include "types.h"

/* This is the base class for all track types. */

class Sequence : public Fl_Widget, public Loggable
{


    static queue <Sequence_Widget *> _delete_queue;

protected:

    Track *_track;                                              /* track this sequence belongs to */

    char *_name;

    std::list <Sequence_Widget *> _widgets;
    Sequence_Widget *event_widget ( void );

    virtual const char *class_name ( void ) { return "Sequence"; }


    virtual void set ( Log_Entry &e )
        {
            for ( int i = 0; i < e.size(); ++i )
            {
                const char *s, *v;

                e.get( i, &s, &v );

                if ( ! strcmp( ":n", s ) )
                {
                    name( strdup( v ) );
                }
            }
        }

    virtual void get ( Log_Entry &e )
        {
            e.add( ":n", name() );
//            e.add( ":t", _track );

/*             s += sprintf( s, ":items " ); */
/*             for ( list <Sequence_Widget *>::const_iterator i = _widgets.begin(); i != _widgets.end(); i++ ) */
/*             { */
/*                 s += sprintf( s, "0x%X", ((Loggable*)(*i))->id() ); */

/*                 list <Sequence_Widget *>::const_iterator e = i; */
/*                 if ( ++e != _widgets.end() ) */
/*                     s += sprintf( s, "," ); */
/*             } */

        }

public:

    Sequence ( int X, int Y, int W, int H, Track *track=0 );

    virtual ~Sequence (  );

    const char * name ( void ) const { return _name; }
    void name ( const char *s )
        {
            if ( _name ) free( _name );
            _name = strdup( s );
            label( _name );
        }

    void sort ( void );

    Track *track ( void ) const { return _track; }
    void track ( Track *t ) { _track = t; }

    void remove ( Sequence_Widget *r );
    void add ( Sequence_Widget *r );

    void select_range ( int X, int W );

    void remove_selected ( void );

    const std::list <Sequence_Widget *> widgets ( void ) const { return _widgets; }

    void queue_delete ( Sequence_Widget *r )
        {
            _delete_queue.push( r );
        }

    Sequence_Widget * overlaps ( Sequence_Widget *r );

    virtual Sequence * clone ( void )
        {
            assert( 0 );
        }

    virtual Sequence * clone_empty ( void )
        {
            return NULL;
        }

    virtual void snap ( Sequence_Widget *r );
    virtual int handle ( int m );
    virtual void draw ( void );

    virtual nframes_t process ( nframes_t nframes ) { return 0; }

};
