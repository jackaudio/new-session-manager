
/*******************************************************************************/
/* Copyright (C) 2007-2008 Jonathan Moore Liles                                */
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

#include "event.H"
#include <stdio.h>
#include <string.h>

namespace MIDI
{
    void
    event::_init ( void )
    {
        _link = _next = _prev = NULL;
        _selected = 0;
    }

    event::event ( void )
    {
        _init();
    }

    event::~event ( void )
    {
        _link = _next = _prev = NULL;
    }

/* copy constructor */
    event::event ( const event &e ) : midievent( e )
    {
        _link = _next = _prev = NULL;
        _selected = e._selected;
    }

    event::event ( const midievent &e ) : midievent( e )
    {
        _init();
    }


    void
    event::link ( event *event )
    {
        if ( event == NULL )
        {
            if ( _link )
            {
                _link->_link = NULL;
                _link = NULL;
            }
            return;
        }

        _link = event;
        _link->_link = this;
    }

    event *
    event::link ( void ) const
    {
        return _link;
    }

    bool
    event::linked ( void ) const
    {
        return _link != NULL;
    }

    void
    event::select ( void )
    {
        _selected = 1;

        if ( _link )
            _link->_selected = 1;
    }

    void
    event::deselect ( void )
    {
        _selected = 0;

        if ( _link )
            _link->_selected = 0;
    }

    bool
    event::selected ( int n ) const
    {
        return _selected == n;
    }

    bool
    event::selected ( void ) const
    {
        return _selected == 1;
    }

/* override this so we can update linked event */
    void
    event::note ( char note )
    {
        midievent::note( note );

        if ( _link )
            _link->midievent::note( note );
    }

/* stupid C++ makes us override the all polymorphic functions... */
    unsigned char
    event::note ( void ) const
    {
        return midievent::note();
    }

    tick_t
    event::note_duration ( void ) const
    {
        return _link ? _link->timestamp() - timestamp() : 0;
    }

    void
    event::note_duration ( tick_t l )
    {
        if ( _link )
            _link->timestamp( timestamp() + l );
    }

    void
    event::get_note_properties ( note_properties *p ) const
    {
        p->start = timestamp();
        p->duration = note_duration();
        p->velocity = note_velocity();
        p->note = note();
        p->selected = selected();
    }

    void
    event::set_note_properties ( const note_properties *p )
    {
        timestamp( p->start );
        note_duration( p->duration );
        note_velocity( p->velocity );
        note( p->note );
        selected( p->selected );
    }
}
