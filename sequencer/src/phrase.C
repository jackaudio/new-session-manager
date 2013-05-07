
/*******************************************************************************/
/* Copyright (C) 2007,2008 Jonathan Moore Liles                                */
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

#include "phrase.H"
#include "pattern.H"
#include "smf.H"
#include "common.h"
#include <math.h>

using namespace MIDI;

vector <phrase*> phrase::_phrases;
signal <void> phrase::signal_create_destroy;

phrase::phrase ( void )
{
    viewport.h = 32;
    viewport.w = 32;

    _add();

    char *s;
    asprintf( &s, "Phrase %d", number() );

    name( s );

}


phrase::~phrase ( void )
{
    DMESSAGE( "deleting phrase %d", number() );
    signal_create_destroy();
}

void
phrase::_add ( void )
{
   // keep track of all the phrases
    phrase::_phrases.push_back( this );
    _number = phrases();

    signal_create_destroy();
}

/* copy constructor */
phrase::phrase ( const phrase &rhs ) : Grid( rhs )
{
    _add();
}

phrase *
phrase::clone ( void )
{
    return new phrase( *this );
}

/******************/
/* Static methods */
/******************/

int
phrase::phrases ( void )
{
    return phrase::_phrases.size();
}

phrase *
phrase::phrase_by_number ( int n )
{
    if ( n <= phrases() && n > 0 )
    {
        return phrase::_phrases[ n - 1 ];
    }
    return NULL;
}

void
phrase::reset ( void )
{
    for ( int n = phrase::phrases(); n-- ; )
    {
        delete phrase::_phrases.back();
        phrase::_phrases.pop_back();
    }
}




/*******************/
/* Virtual Methods */
/*******************/

phrase *
phrase::create ( void )
{
    if ( phrase::phrases() < 128 )
    {
        return new phrase;
    }
    else
        return NULL;
}

phrase *
phrase::by_number ( int n ) const
{
    return phrase::phrase_by_number( n );
}

void
phrase::put ( int x, int y, tick_t l, int velocity )
{
    // FIXME: use translation here.
    pattern *p = pattern::pattern_by_number( y + 1 );

    if ( ! p )
        return;

    l = p->length();

    Grid::put( x, y, l );
}

const char *
phrase::row_name ( int r ) const
{
    pattern *p = pattern::pattern_by_number( r + 1 );

    return p ? p->name() : NULL;
}

void
phrase::draw_row_names ( Canvas *c ) const
{
    for ( int y = viewport.h; y--; )
    {
        pattern *p = pattern::pattern_by_number( y + 1 );

        if ( p && p->name() )
            c->draw_row_name( y, p->name(), 0 );
    }
}


void
phrase::trigger ( tick_t start, tick_t end )
{
    _start = start;
    _end = end;
}

// FIXME: so much of this is copied from pattern.C, there has
// to be a way to share more of this code.
void
phrase::play ( tick_t start, tick_t end )
{
    /* get our own copy of this pointer so UI thread can change it. */
    const data *d = const_cast< const data * >(_rd);

    if ( start > _end )
    {
        _playing = false;
        return;
    }
    
    if ( start < _start )
        start = _start;

    if ( end > _end )
        end = _end;

    _playing = true;

    // where we are in the absolute time
    tick_t tick = start - _start;
    int num_played = tick / d->length;
    tick_t offset = _start + (d->length * num_played);

    _index = fmod( tick, d->length );

    if ( _index < end - start )
        DMESSAGE( "Triggered phrase %d at tick %lu (ls: %lu, le: %lu, o: %lu)", number(), start, _start, _end, offset  );

try_again:

    // pattern is empty
    if ( d->events.empty() )
        goto done;

    for ( const event *e = d->events.first(); e; e = e->next() )
    {
        //    MESSAGE( "s[%ld] -> t[%ld] : %ld, len %ld", start, end, e->timestamp(), _length ); // (*e).print();

        const tick_t ts = e->timestamp() + offset;

        if ( ts >= end )
            goto done;

        if ( e->is_note_on() )
        {
            const tick_t tse = offset + e->link()->timestamp();

            if ( tse > start )
            {
                pattern *p = pattern::pattern_by_number( 1 + note_to_y( e->note() ) );
            
                p->trigger( ts, tse );
                
                p->play( start, end );
            }
        }
    }

    // ran out of events, but there's still some loop left to play.
    offset += d->length;
    goto try_again;

    MESSAGE( "out of events, resetting to satisfy loop" );

done: ;
}


void
phrase::load ( smf *f )
{
    lock();

    f->read_phrase_info( this );

    tick_t len;

    list <midievent> *me = f->read_track_events( &len );

    _rw->events = *me;
    delete me;

    if ( len )
        _rw->length = len;

    unlock();
}

void
phrase::dump ( smf *f )
{
    f->open_track( _name, -1 );

    f->write_phrase_info( this );

    f->cue( true );

    Grid::dump( f, 0 );

    f->close_track( length() );
}
