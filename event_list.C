
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

#include "event_list.H"

/* The operations we perform on event lists are clumsy with STL lists
   and iterators so we have a custom doubly-linked list implementation
   here for complete control */

#define RFOR_ALL( it ) for ( event *next, * it = _tail; it && ((next = it ->_prev), true) ; it = next )
#define FOR_ALL( it ) for ( event *next, * it = _head; it && ((next = it ->_next), true) ; it = next )
// #define FOR_ALL( e ) for ( event * e = _head; e; e = e ->_next )
#define FOR_SELECTED( e ) FOR_ALL( e ) if ( e ->selected() )
#define RFOR_SELECTED( e ) RFOR_ALL( e ) if ( e ->selected() )


event_list::event_list ( void )
{
    _head = NULL;
    _tail = NULL;
    _size = 0;
}

event_list::~event_list ( void )
{
    clear();
}

/* copy constructor */
event_list::event_list ( const event_list &el )
{
    _copy( &el );
}

event_list &
event_list::operator= ( const event_list &rhs )
{
    if ( this != &rhs )
    {
        clear();

        _copy( &rhs );
    }

    return *this;
}

event_list &
event_list::operator= ( const list <midievent> &rhs )
{
    clear();

    for ( list <midievent>::const_iterator me = rhs.begin(); me != rhs.end(); me++ )
    {
        event *e = new event( *me );

        _insert( NULL, e );
    }

    relink();

    return *this;
}

/** allow indexing */
event *
event_list::operator[] ( unsigned int index )
{
    unsigned int i = 0;
    for ( event *e = _head; e; (e = e->_next), ++i )
        if ( i == index )
            return e;

    // all else fails.
    return _tail;
}

void
event_list::_copy ( const event_list *el )
{
    if ( ! el->_head )
    {
        _head = _tail = NULL;
        _size = 0;
        return;
    }

    _head = new event( *(el->_head) );
    _head->_prev = NULL;

    event *p = _head;

    for ( event *e = el->_head->_next; e; e = e->_next )
    {
        event *n = new event( *e );

        n->_next = NULL;
        p->_next = n;
        n->_prev = p;

        p = n;
    }

    _tail = p;

    _size = el->_size;

    relink();
}

/** insert event /n/ before event /o/ */
void
event_list::_insert ( event *o, event *n )
{
    ++_size;

    if ( ! o )
    {
        n->_next = NULL;
        n->_prev = _tail;

        if ( _tail )
            _tail->_next = n;

        _tail = n;
        if ( ! _head )
            _head = n;
        return;
    }

    event *t = o->_prev;

    o->_prev = n;
    n->_next = o;
    n->_prev = t;

    if ( ! t )
        _head = n;
    else
        t->_next = n;
}

void
event_list::unlink ( event *e )
{
    if ( e->_next )
        e->_next->_prev = e->_prev;
    else
        _tail = e->_prev;

    if ( e->_prev )
        e->_prev->_next = e->_next;
    else
        _head = e->_next;

    --_size;
}



void
event_list::clear ( void )
{
   for ( event *e = _head; e ; )
    {
        event *n = e->_next;
        delete e;
        e = n;
    }

   _head = NULL;
   _tail = NULL;
   _size = 0;
}

void
event_list::mix ( event *ne )
{
    FOR_ALL( e )
        if ( *e == *ne )
        {
            /* already have an event like this, drop it */

            if ( ne->linked() )
                delete ne->link();

            delete ne;

            return;
        }

    insert( ne );
    if ( ne->linked() )
        insert( ne->link() );

}

/** remove elements from list /el/ to this list */
void
event_list::merge ( event_list *el )
{
    event *n;
    for ( event *e = el->_head; e; e = n )
    {
        n = e->_next;

        el->unlink( e );

        insert( e );
    }
}

/** unlink event e */
void
event_list::remove ( event *e )
{
    unlink( e );
    delete e;
}

/** sorted insert /e/ */
void
event_list::insert ( event *e )
{
    /* find the place to insert */
    RFOR_ALL( i )
        if ( *e >= *i )
        {
            _insert( i->_next, e );
            return;
        }

    _insert( _head, e );
}

/** just append event without sorting */
void
event_list::append ( event *e )
{
    _insert( NULL, e );
}

event *
event_list::first ( void ) const
{
    return _head;
}

event *
event_list::last ( void ) const
{
    return _tail;
}



/*************/
/* Selection */
/*************/

/** select all events from /start/ to /end/ inclusive */
void
event_list::select ( tick_t start, tick_t end )
{
    FOR_ALL( e )
    {
        tick_t ts = e->timestamp();

        /* don't count note offs exactly on start */
        if ( ts == start && e->is_note_off() )
            continue;

        if ( ts >= start && ts < end )
            e->select();
    }
}

/** select ALL events */
void
event_list::select_all ( void )
{
    FOR_ALL( e )
        e->select();
}

void
event_list::select_none ( void )
{
    FOR_ALL( e )
        e->deselect();
}

/** remove all selected events */
void
event_list::remove_selected ( void )
{
    FOR_SELECTED( e )
    {
        remove( e );
    }
}

/** transpose selected notes (ignoring other event types) by /n/ tones
 * (may span octaves) */
void
event_list::transpose_selected ( int n )
{
    FOR_SELECTED( e )
    {
        if ( e->is_note_on() )
            e->note( e->note() + n );
    }

}

/** change all notes of value /from/ to /to/ */
void
event_list::rewrite_selected ( int from, int to )
{
    FOR_SELECTED( e )
    {
        if ( e->is_note_on() && e->note() == from )
            e->note( to );
    }
}


/** get timestamp of earliest selected event */
tick_t
event_list::selection_min ( void )
{
    FOR_SELECTED( e )
        return e->timestamp();

    return 0;
}

tick_t
event_list::selection_max ( void )
{
    RFOR_SELECTED( e )
        return e->timestamp();

    return 0;
}

/** move selected events by offset /o/ */
void
event_list::move_selected ( long o )
{
    if ( o < 0 )
        if ( selection_min() < (tick_t)( 0 - o ) )
                return;

    if ( o < 0 )
    {
        FOR_SELECTED( e )
            move( e, o );
    }
    else
    {
        RFOR_SELECTED( e )
            move( e, o );
    }
}

void
event_list::push_selection ( void )
{
    FOR_ALL( e )
        if ( e->_selected )
            ++e->_selected;
}

void
event_list::pop_selection ( void )
{
    FOR_ALL( e )
        if ( e->_selected )
            --e->_selected;
}



/** verify that all note ons are linked to note offs */
bool
event_list::verify ( void ) const
{
    FOR_ALL( e )
        if ( e->is_note_on() && ! e->linked() )
            return false;

    return true;
}

/** link /e/ (a note on) with the next corresponding note off */
void
event_list::link ( event *on )
{
    if ( ! on->is_note_on() )
        return;

    for ( event *off = on->_next; off; off = off->_next )
    {
        if ( off->linked() )
            continue;

        if ( off->is_note_off() &&
             off->channel() == on->channel() &&
             off->note() == on->note() )
        {
            on->link( off );
            return;
        }
    }

    WARNING( "no corresponding note_off found for note on, repairing" );

    event *off = new event( *on );

    off->opcode( event::NOTE_OFF );

    on->link( off );

    insert( off );
}

/** insert /l/ ticks of time at /start/ */
void
event_list::insert_time ( tick_t start, tick_t l )
{
    FOR_ALL( e )
    {
        tick_t ts = e->timestamp();

        if ( e->is_note_off() )
            continue;

        if ( ts >= start )
            if ( e->is_note_on() )
            {
                /* only notes ENTIRELY WITHIN the range will be moved */
                e->timestamp( ts + l );
                e->link()->timestamp( e->link()->timestamp() + l );
            }
            else
                e->timestamp( e->timestamp() + l );
    }
}

/** delete events in range and close the gap */
void
event_list::delete_time ( tick_t start, tick_t end )
{
    tick_t l = end - start;

    push_selection();

    select( start, end );

    remove_selected();

    pop_selection();

    /* cut out the slack */
    FOR_ALL( e )
    {
        tick_t ts = e->timestamp();

        if ( ts >= end )
            e->timestamp( ts - l );
    }
}

/** link all note ons to subsequent note offs */
void
event_list::relink ( void )
{
    /* clear links */
    FOR_ALL( e )
        e->link( NULL );

    /* link */
    FOR_ALL( on )
        link( on );

    if ( ! verify() )
        ASSERTION( "event list failed verification" );
}

/** resort event /e/ */
void
event_list::sort ( event *e )
{
    unlink( e );

    insert( e );
}

/** resort entire list */
void
event_list::sort ( void )
{
    event_list *temp = new event_list( *this );

    _head = temp->_head;
    _tail = temp->_tail;

    temp->_head = NULL;

    delete temp;
}

/** move event /e/ by /o/ ticks */
void
event_list::move ( event *e, long o )
{
    e->timestamp( e->timestamp() + o );

    sort( e );
}

bool
event_list::empty ( void ) const
{
    return _head == NULL;
}

size_t
event_list::size ( void ) const
{
    return _size;
}

void
event_list::_hi_lo ( bool sel, int *hi, int *lo ) const
{
    *hi = 0;
    *lo = 127;

    FOR_ALL( e )
    {
        if ( sel && ! e->selected() )
            continue;

        if ( ! e->is_note_on() )
            continue;

        int n = e->note();

        if ( n > *hi )
            *hi = n;

        if ( n < *lo )
            *lo = n;
    }
}

/** set /hi/ and /lo/ to the lowest and highest pitched note events in
 * this list, respectively */
void
event_list::hi_lo_note ( int *hi, int *lo ) const
{
    _hi_lo( false, hi, lo );
}

void
event_list::selected_hi_lo_note ( int *hi, int *lo ) const
{
    _hi_lo( true, hi, lo );
}
