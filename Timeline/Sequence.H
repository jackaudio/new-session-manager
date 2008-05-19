
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

class Track;
class Sequence_Widget;

#include "types.h"

/* This is the base class for all track types. */

class Sequence : public Fl_Widget, public Loggable
{

    /* not permitted  */
    Sequence ( const Sequence &rhs );
    Sequence & operator= ( const Sequence &rhs );

    static queue <Sequence_Widget *> _delete_queue;

    void init ( void );

protected:

    Track *_track;                                              /* track this sequence belongs to */

    char *_name;

    friend class Timeline; // for draw_measure
    std::list <Sequence_Widget *> _widgets;
    Sequence_Widget *event_widget ( void );

public:

    /* welcome to C++ */
    LOG_NAME_FUNC( Sequence );

    Sequence ( Track *track=0 );

    Sequence ( int X, int Y, int W, int H );

    virtual ~Sequence (  );

    /* override this to provide cursor  */
    virtual Fl_Cursor cursor ( void ) const = 0;

    nframes_t x_to_offset ( int X );

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

    virtual nframes_t process ( nframes_t ) { return 0; }

};
