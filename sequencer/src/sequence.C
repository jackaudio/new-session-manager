
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

#include "sequence.H"
#include "phrase.H"
#include "pattern.H"
#include "smf.H"

#include "non.H"

#include <errno.h>

/* #include <string> */

/* using std::string; */

sequence::sequence ( void )
{
    _rd = new data;
    _name = _notes = NULL;

    _index = 0;
    _playing = 0;
}


void
sequence::lock ( void )
{
    // create a copy of the lock-free data.
    _rw = new data;

    data *d = const_cast< data *> (_rd);

    _rw->phrases = d->phrases;
    _rw->num     = d->num;
}

void
sequence::unlock ( void )
{
    _history.push_back( const_cast<data *>( _rd ) );

    if ( _history.size() > MAX_UNDO + 1 )
    {
        data *d = _history.front();

        if ( d == _rw || d == _rd )
            ASSERTION( "something bad has happend." );

        delete d;

        _history.pop_front();
    }

    // swap the copy back in (atomically).
    _rd = _rw;

    _rw = NULL;

    song.set_dirty();
}

void
sequence::insert ( unsigned int n, int pn )
{
    lock();

/*     if ( n > _rw->phrases.size() ) */
/*         _rw->phrases.resize( n + 10 ); */

//    MESSAGE( "inserting %d at %d", pn, n );

    _rw->phrases.insert( _find( n ), pn );
    _rw->num++;

    unlock();
}

vector <int>::iterator
sequence::_find ( int n )
{
    // boy I hate C++/STL.. So lame.
    int i = 0;
    for ( vector <int>::iterator e = _rw->phrases.begin(); e != _rw->phrases.end(); e++ )
    {
        if ( i == n )
            return e;
        i++;
    }

    return _rw->phrases.end();
}

void
sequence::remove ( int n )
{
    lock();

    _rw->phrases.erase( _find( n ) );
    _rw->num--;

    unlock();
}

/** return the number of phrases in this sequence */
int
sequence::phrases ( void ) const
{
    return _rd->num;
}

void
sequence::_swap ( int n1, int n2 )
{
    int x = _rw->phrases[ n1 ];
    _rw->phrases[ n1 ] = _rw->phrases[ n2 ];
    _rw->phrases[ n2 ] = x;
}

void
sequence::move ( int n, int dir )
{
    lock();

    switch ( dir )
    {
        case UP:
        {
            if ( n - 1 >= 0 )
                _swap( n - 1, n );
            break;
        }
        case DOWN:
        {
            if ( n + 1 < _rw->num )
                _swap( n + 1, n );
            break;
        }

    }

    unlock();
}

/* Render sequence to a string.. suitable for display in the UI */
char *
sequence::dump ( void )
{
    char *s = (char *)malloc( 256 );
    s[0] = '\0';
    size_t siz = 256;

    int start = 1;
    for ( int i = 0; i < _rd->num; i++ )
    {
        const int len = 256;

        char line[len];

        int x = _rd->phrases[ i ];

        phrase *p = phrase::phrase_by_number( x );

        if ( ! p )
            return NULL;

        snprintf( line, len, "%d\t%d\t%s\n", start, p->number(), p->name() );

        start += p->bars();

        s = (char *)realloc( s, siz += strlen( line ) + 1 );

        strcat( s, line );
    }
    return s;
}


void
sequence::play ( tick_t start, tick_t end ) const
{
    // keep our own copy.
    data *d = _rd;

    tick_t offset = 0;
    for ( int i = 0; i < d->num; i++ )
    {
        phrase *p = phrase::phrase_by_number( d->phrases[ i ] );
        if ( p )
        {
            tick_t pstart = offset;
            tick_t pend = offset + p->length();

            // this phrase seems to be current.
            if ( pend > start && pstart <= end )
            {
                p->trigger( pstart, pend );
          
                _playing = p->number();

                _index = start;
                
                p->play( start, end );
            }

            offset = pend;
        }
        else
            WARNING( "programming error: no such phrase." );
    }
}

/** return the number of the currently playing phrase, or 0 if none. */
int
sequence::playing ( void ) const
{
    return _playing;
}

/** return the location of the playhead for this sequence */
tick_t
sequence::index ( void ) const
{
    return _index;
}

/** return the total length of the sequence in ticks */
tick_t
sequence::length ( void ) const
{
    tick_t l = 0;

    for ( int i = 0; i < _rd->num; i++ )
    {
        phrase *p = phrase::phrase_by_number( _rd->phrases[ i ] );

        if ( ! p )
            break;

        l += p->length();
    }

    return l;
}

/** return to a blank slate */
void
sequence::reset ( void )
{
//    MESSAGE( "reseting" );

    lock();

    _rw->num = 0;

    phrase::reset();
    pattern::reset();

    unlock();
}

/** load entire sequence from file, replacing everything */
bool
sequence::load ( const char *name )
{
    smf f;

    if ( ! f.open( name, smf::READ ) )
    {
        WARNING( "error opening file: %s", strerror( errno ) );
        return false;
    }

    f.read_header();

    if ( f.format() != 2 )
    {
        WARNING( "not a Non song file" );
        return false;
    }

    f.next_track();

    DMESSAGE( "reading song info" );

    /* read song info */
    int mode = PATTERN;
    int phrases = 0;
    int patterns = 0;
    char *sname = NULL;
    char *notes = NULL;

    if ( ! f.read_song_info( &mode, &phrases, &patterns, &sname, &notes ) )
    {
        WARNING( "not a Non song file" );
        return false;
    }

    song.play_mode = (play_mode_e)mode;

    if ( sname )
        this->name( sname );

    if ( notes )
        this->notes( notes );

    /* tear it down */
    reset();

    DMESSAGE( "reading playlist" );

//    f.read_playlist( this );

    lock();

    char *s;
    while ( (s = f.read_cue_point() ) )
    {
        int n;

        sscanf( s, "%d:", &n );

        _rw->phrases.insert( _find( _rw->num++ ), n );
    }

    /* read playlist */

    DMESSAGE( "reading phrases" );

    while ( phrases-- && f.next_track() )
    {
        phrase *p = new phrase;

        p->load( &f );
    }

    DMESSAGE( "reading patterns" );

    while ( patterns-- && f.next_track() )
    {
        pattern *p = new pattern;

        p->load( &f );
    }

    unlock();

    signal_new_song();

    return true;
}

/** save entire sequence to file */
void
sequence::save ( const char *name ) const
{
    smf f;

    /* open for writing */
    f.open( name, smf::WRITE );

    f.write_header( 2 );

    DMESSAGE( "saving playlist" );

    f.open_track( NULL, -1 );

    DMESSAGE( "saving song info" );

    f.write_song_info( song.play_mode, phrase::phrases(), pattern::patterns(), this->name(), notes() );

    for ( int i = 0; i < _rd->num; ++i )
    {
        char pat[256];

        phrase *p = phrase::phrase_by_number( _rd->phrases[ i ] );

        snprintf( pat, 256, "%d: %s", p->number(), p->name() );

        f.write_meta_event( smf::CUEPOINT, pat );
    }

    f.close_track( 0 );

    DMESSAGE( "saving phrases" );

    for ( int i = 0; i < phrase::phrases(); i++ )
    {
        phrase *p = phrase::phrase_by_number( i + 1 );

        p->dump( &f );
    }

    DMESSAGE( "saving patterns" );

    for ( int i = 0; i < pattern::patterns(); i++ )
    {
        pattern *p = pattern::pattern_by_number( i + 1 );

        p->dump( &f );
    }
}


/*************/
/* Accessors */
/*************/

char *
sequence::name ( void ) const
{
    return _name;
}

void
sequence::name ( const char *s )
{
    if ( _name ) free( _name );

    _name = strdup( s );

    song.set_dirty();
}

char *
sequence::notes ( void ) const
{
    return _notes;
}

void
sequence::notes ( const char *s )
{
    if ( _notes ) free( _notes );

    _notes = strdup( s );

    song.set_dirty();
}
