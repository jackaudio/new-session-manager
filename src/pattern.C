
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

#include "pattern.H"
#include "non.H"
#include "common.h"
#include "smf.H"

#include "jack.H"
#include "transport.H"

event_list pattern::_recorded_events;
vector <pattern*> pattern::_patterns;
int pattern::_solo;
int pattern::_pattern_recording;

signal <void> pattern::signal_create_destroy;

pattern::pattern ( void )
{
    viewport.h = 32;
    viewport.w = 32;

    _draw_shape = CIRCLE;
    _channel = _port = 0;

    _ppqn = 4;
    _bpb  = 4;
    _note = 8;
    int _bars = 2;

    // we need to reinitalize this.
    data *d = const_cast< data * >( _rd );

    d->length = x_to_ts( _bpb * _ppqn * _bars );

//    mapping.open( Mapping::INSTRUMENT, "Default" );
    mapping.open( Mapping::SCALE, "Major" );

    _add();

    char *s;
    asprintf( &s, "Pattern %d", number() );

    name( s );
}

void
pattern::_add ( void )
{
    // keep track of all the patterns
    pattern::_patterns.push_back( this );
    _number = patterns();

    signal_create_destroy();
}

pattern::~pattern ( void )
{
    DMESSAGE( "deleting pattern %d", number() );
    signal_create_destroy();
}

/* copy constructor */
pattern::pattern ( const pattern &rhs ) : Grid( rhs )
{
    _note    = rhs._note;
    _port    = rhs._port;
    _channel = rhs._channel;

    mapping = rhs.mapping;

    _add();
}

pattern *
pattern::clone ( void )
{
    return new pattern( *this );
}

/******************/
/* Static methods */
/******************/

int
pattern::solo ( void )
{
    return pattern::_solo;
}

int
pattern::patterns ( void )
{
    return pattern::_patterns.size();
}

// this is the static one
pattern *
pattern::pattern_by_number ( int n )
{
    if ( n <= patterns() && n > 0 )
    {
        return pattern::_patterns[ n - 1 ];
    }
    return NULL;
}

/** delete all patterns in preparation for a reload */
void
pattern::reset ( void )
{
    for ( int n = pattern::patterns(); n-- ; )
    {
        delete pattern::_patterns.back();
        pattern::_patterns.pop_back();
    }
}

void
pattern::record_event ( const midievent *me )
{
    /* set the events aside in a dedicated list--the recording pattern
     * will decide what to do with them the next time around the
     * loop */

    /* FIXME: how does the pattern decide when to loop? It seems
       reasonable that /merge/ and /replace/ modes should be bound to
       the previous pattern length, but what about "NEW" mode? Should it
       just use this entire list as a new pattern (of whatever length)
       when recording is halted? */

    event *e = new event;

    *e = *me;

    pattern::_recorded_events.append( e );

    record_mode_e mode = config.record_mode;

    if ( mode == OVERWRITE || mode == LAYER )
    {
        pattern *p = pattern::recording();

        if ( ! p->_cleared )
        {

            if ( mode == LAYER )
            {
                p->record_stop();

                p = p->clone();

                p->record( 0 );
            }

            p->clear();

            p->_cleared = true;
        }

        mode = MERGE;
    }

    /* let's fill in the pattern 'live' in merge mode. looks a little
       complicated because we have to wait for a note-off before it's
       safe to insert */
    if ( mode == MERGE || mode == NEW )
    {

        pattern *p = pattern::recording();

        p->lock();

        event_list *el = &pattern::_recorded_events;

        if ( e->is_note_off() )
        {
            event *off = e;

            for ( event *on = el->last(); on; on = on->prev() )
            {
                if ( on->is_note_on() &&
                     on->is_same_note( off ) )
                    // &&
                    //   *on < *e )
                {
                    el->unlink( on );
                    el->unlink( off );

                    tick_t duration = off->timestamp() - on->timestamp();

                    /* place within loop */
                    on->timestamp( ( on->timestamp() - p->_start ) % p->_rw->length );

                    on->link( off );
                    on->note_duration( duration );

                    p->_rw->events.mix( on );

                    break;
                }
            }
        }
        else
            if ( ! e->is_note_on() )
            {

                // if ( ! filter )

                e->timestamp( e->timestamp() % p->_rw->length );

                el->unlink( e );
                p->_rw->events.insert( e );
            }

        p->unlock();
    }
}

pattern *
pattern::recording ( void )
{
    return pattern::pattern_by_number( pattern::_pattern_recording );
}



/*******************/
/* Virtual Methods */
/*******************/

/* allows us to create a new pattern/phrase from a base class pointer  */
pattern *
pattern::create ( void )
{
    return new pattern;
}

pattern *
pattern::by_number ( int n ) const
{
    return pattern::pattern_by_number( n );
}

void
pattern::put ( int x, int y, tick_t l )
{
    l = l ? l : PPQN * 4 / _note;

    Grid::put( x, y, l );

    if ( ! transport.rolling )
    {
        /* echo note */
        midievent e;

        e.status( event::NOTE_ON );
        e.channel( _channel );
        e.timestamp( l );
        e.note( y_to_note( y ) );
        e.note_velocity( 64 );

        midi_output_immediate_event ( _port, &e );
    }
}

const char *
pattern::row_name ( int r ) const
{
    return mapping.note_name( y_to_note( r ) );
}

void
pattern::draw_row_names ( Canvas *c ) const
{
    for ( int y = 128; y-- ; )
        c->draw_row_name( y, mapping.note_name( y_to_note( y ) ), mapping.velocity( y_to_note( y ) ) );
}

void
pattern::trigger ( tick_t start, tick_t end )
{
    ASSERT( start <= end, "programming error: invalid loop trigger! (%lu-%lu)", start, end );

    _start = start;
    _end   = end;
    _index = 0;
}

/* trigger forever */
void
pattern::trigger ( void )
{
    trigger( transport.frame / transport.frames_per_tick, -1 );
}

void
pattern::stop ( void ) const
{
    _playing = false;

    _start = 0;
    _end   = 0;
    _index = 0;
}


void
pattern::mode ( int n )
{
    if ( n == SOLO )
    {
        if ( pattern::_solo )
            ((Grid*)pattern::pattern_by_number( pattern::_solo ))->mode( PLAY );
        pattern::_solo = _number;
        Grid::mode( SOLO );
    }
    else
    {
        if ( pattern::_solo == _number )
            pattern::_solo = 0;

        Grid::mode( n );
    }
}

int
pattern::mode ( void ) const
{
    if ( pattern::_solo )
    {
        if ( pattern::_solo == _number )
            return SOLO;
        else
            return MUTE;
    }
    else
        return Grid::mode();
}

/* WARNING: runs in the RT thread! */
// output notes from /start/ to /end/ (absolute)
void
pattern::play ( tick_t start, tick_t end ) const
{
    /* get our own copy of this pointer so UI thread can change it. */
    const data *d = const_cast< const data * >(_rd);

    if ( start > _end )
    {
        stop();
        WARNING( "attempt to play a loop (pattern %d) that has ended (%lu, %lu)", number(), start, _end );
        return;
    }
    else
        if ( end < _start )
            // not ready yet
            return;

    if ( start < _start )
        start = _start;

    if ( end > _end )
        end = _end;

    // where we are in the absolute time
    tick_t tick = start - _start;
    int num_played = tick / d->length;
    tick_t offset = _start + (d->length * num_played);

    const event *e;

    _index = tick % d->length;

    if ( _index < end - start )
    {
        DMESSAGE( "%s pattern %d at tick %lu (ls: %lu, le: %lu, o: %lu)", _playing ? "Looped" : "Triggered", number(), start, _start, _end, offset  );

        _cleared = false;
    }

    _playing = true;

    if ( mode() == MUTE )
        goto done;

try_again:

    // pattern is empty
    if ( d->events.empty() )
        goto done;

    for ( e = d->events.first(); e; e = e->next() )
    {
        //    MESSAGE( "s[%ld] -> t[%ld] : %ld, len %ld", start, end, e->timestamp(), _length ); // (*e).print();

        tick_t ts = e->timestamp() + offset;

        if ( ts >= end )
            goto done;

        if ( ts >= start )
        {
            midievent me = *e;


            //           MESSAGE( "timestamp %d, tick %d, ts - start == %lu", e->timestamp(), start,
            //                    e->timestamp() - start);

            /* set the channel */
            me.channel( _channel );

            /* set the in-cycle timestamp */
            me.timestamp ( ts - start );

            if ( me.is_note_on() )
            {
                if ( mapping.translate( &me ) )
                    midi_output_event( _port, &me, 1 + e->note_duration() );
            }
            else
                if ( me.is_note_off() )
                {
                    if ( mapping.translate( &me ) )
                        midi_output_event( _port, &me, 0 );
                }
                else
                    /* any other event type */
                    midi_output_event( _port, &me );
        }
    }

    // ran out of events, but there's still some loop left to play.
    offset += d->length;
    goto try_again;

    DMESSAGE( "out of events, resetting to satisfy loop" );

done:

    if ( _end == end )
    {
        /* we're done playing this trigger */
        DMESSAGE( "Pattern %d ended at tick %lu (ls: %lu, le: %lu, o: %lu)", number(), end, _start, _end, offset  );

        stop();
    }

}

/* Import /track/ of /f/ as new pattern */
pattern *
pattern::import ( smf *f, int track )
{
    if ( ! f->seek_track( track ) )
        return NULL;

    pattern *p = new pattern;

    p->lock();

    p->load( f );

    /* file could have any notes in it... Use Chromatic scale to
       ensure all are visible  */
    p->mapping.open( Mapping::SCALE, "Chromatic" );

    p->unlock();

    p->fit();

    return p;
}

/** fill pattern from current track of /f/ */
void
pattern::load ( smf *f )
{
    lock();

    f->read_pattern_info( this );

    tick_t len;

    list <midievent> *e = f->read_track_events( &len );

    /* set channel to channel of first event... */
    if ( e->size() )
        _channel = e->front().channel();

    /* copy events into pattern */
    _rw->events = *e;
    delete e;

    if ( len )
        _rw->length = len;

    unlock();

//    print();
}

/** save (export) pattern to file /name/  */
void
pattern::save ( const char *name ) const
{
    smf f;

    /* open for writing */
    f.open( name, smf::WRITE );

    /* writing SMF 0 track */
    f.write_header( 0 );

    f.open_track( _name, _number );

    Grid::dump( &f, _channel );

    f.close_track( length() );
}

/** dump pattern as a track in an already open MIDI file */
void
pattern::dump ( smf *f ) const
{
    f->open_track( _name, _number );

    f->write_pattern_info( this );

    Grid::dump( f, _channel  );

    f->close_track( length() );
}


void
pattern::randomize_row ( int y, int feel, float probability )
{
    lock();

    int l = PPQN * 4 / _note;

    int bx = ts_to_x( _rw->length - l );

    float *p = (float *)alloca( feel * sizeof( float ) );

    float prob = probability;
    for ( int i = 0; i < feel; i++ )
    {
        p[i] = prob;
        // reduce probability as we move away from center
        prob *= 0.5;
    }

    for ( int x = 0; x < bx; x++ )
    {
        float r = ((float)rand()) / RAND_MAX;

        if ( p[ x % feel ] + r >= 1 )
            put( x, y, l );
    }

    unlock();
}

/*************/
/* Recording */
/*************/

void
pattern::record ( int mode )
{
    _recording = true;
    pattern::_pattern_recording = _number;
}

void
pattern::record_stop ( void )
{
    if ( ! _recording )
        return;

    _recording = false;

    if ( config.record_mode == NEW )
        trim();

    pattern::_recorded_events.clear();
}



/*******************************/
/* Pattern specific accessors. */
/*******************************/


int
pattern::port ( void ) const
{
    return _port;
}

void
pattern::port ( int p )
{
    _port = p;
}

int
pattern::channel ( void ) const
{
    return _channel;
}

void
pattern::channel ( int c )
{
    _channel = c;
}

int
pattern::note ( void ) const
{
    return _note;
}

void
pattern::note ( int n )
{
    _note = n;
}


int
pattern::ppqn ( void ) const
{
    return _ppqn;
}

void
pattern::ppqn ( int n )
{
    _ppqn = n;
}

int
pattern::key ( void ) const
{
    return mapping.key();
}

void
pattern::key ( int k )
{
    mapping.key( k );
}
