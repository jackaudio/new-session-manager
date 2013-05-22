
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

#include "event.H"
#include <list>

namespace MIDI {
    using std::list;

    class midievent;

    class event_list {

        event * _head;
        event * _tail;

        size_t _size;

        void _insert ( event *o, event *n );
        void _copy ( const event_list *el );
        void _hi_lo ( bool sel, int *hi, int *lo ) const;

    public:

        event_list ( void );
        ~event_list ( void );
        event_list ( const event_list &el );

        void clear ( void );
        void merge ( event_list *el );
        void unlink ( event *e );
        void remove ( event *e );
        void insert ( event *e );
        event * first ( void ) const;
        event * last ( void ) const;
        void select ( tick_t start, tick_t end );
        void select ( tick_t start, tick_t end, int hi, int lo );

        void select_all ( void );
        void select_none ( void );
        void invert_selection ( void );
        void copy_selected ( event_list *el ) const;
        void paste ( tick_t offset, const event_list *el );

        void remove_selected ( void );
        void transpose_selected ( int n );
        tick_t selection_min ( void ) const;
        tick_t selection_max ( void ) const;
        void move_selected ( tick_t tick );
        void nudge_selected ( long o );
        void push_selection ( void );
        void pop_selection ( void );
        void selected_velocity ( int v );
        bool verify ( void ) const;
        void link ( event *on );
        void insert_time ( tick_t start, tick_t l );
        void delete_time ( tick_t start, tick_t end );
        void relink ( void );
        void sort ( event *e );
        void sort ( void );
        void move ( event *e, long o );
        bool empty ( void ) const;
        size_t size ( void ) const;
        void append ( event *e );
        void mix ( event *ne );
        void hi_lo_note ( int *hi, int *lo ) const;
        void rewrite_selected ( int from, int to );
        void selected_hi_lo_note ( int *hi, int *lo ) const;

        event_list & operator= ( const event_list &rhs );
        event_list & operator= ( const list <midievent> &rhs );
        event *operator[] ( unsigned int index );

        //    friend class event;
    };
}
