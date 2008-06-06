
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


#include "Loggable.H"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "util/file.h"

#include <algorithm>
using std::min;
using std::max;

#include <FL/Fl.H> // for Fl::check()



FILE *Loggable::_fp;
int Loggable::_log_id = 0;
int Loggable::_level = 0;
int Loggable::_undo_index = 1;

size_t Loggable::_loggables_size = 0;
Loggable ** Loggable::_loggables;
std::map <std::string, create_func*> Loggable::_class_map;
std::queue <char *> Loggable::_transaction;

progress_func *Loggable::_progress_callback = NULL;
void *Loggable::_progress_callback_arg = NULL;



/** Open the journal /filename/ and replay it, bringing the end state back into RAM */
bool
Loggable::open ( const char *filename )
{
    FILE *fp;

    Loggable::_fp = NULL;

    if ( ! ( fp = fopen( filename, "a+" ) ) )
    {
        WARNING( "Could not open log file for writing!" );
        return false;
    }

    if ( newer( "snapshot", filename ) )
    {
        DMESSAGE( "Loading snapshot" );

        FILE *fp = fopen( "snapshot", "r" );

        replay( fp );

        fclose( fp );
    }
    else
    {
        DMESSAGE( "Replaying journal" );

        replay( fp );
    }

    fseek( fp, 0, SEEK_END );

    Loggable::_fp = fp;

    return true;
}

#include <sys/stat.h>
#include <unistd.h>

/** replay journal or snapshot */
bool
Loggable::replay ( FILE *fp )
{
    /* FIXME: bogus */
    char buf[BUFSIZ];

    struct stat st;
    fstat( fileno( fp ), &st );

    off_t total = st.st_size;
    off_t current = 0;

    if ( _progress_callback )
        _progress_callback( 0, _progress_callback_arg );

    while ( fscanf( fp, "%[^\n]\n", buf ) == 1 )
    {
        if ( ! ( ! strcmp( buf, "{" ) || ! strcmp( buf, "}" ) ) )
        {
            if ( *buf == '\t' )
                do_this( buf + 1, false );
            else
                do_this( buf, false );
        }

        current = ftell( fp );

        if ( _progress_callback )
            _progress_callback( current * 100 / total, _progress_callback_arg );
    }

    if ( _progress_callback )
        _progress_callback( 0, _progress_callback_arg );

    return true;
}

/** close journal and delete all loggable objects, returing the systemt to a blank slate */
bool
Loggable::close ( void )
{
    DMESSAGE( "closing journal and destroying all journaled objects" );

    if ( ! _fp )
        return true;

    fclose( _fp );

    _fp = NULL;

    snapshot( "snapshot" );

    for ( int i = 0; i < _log_id - 1; ++i )
    {
        Loggable ** l = &_loggables[ i ];

        if ( *l )
        {
            delete *l;
            *l = NULL;
        }
    }

    _log_id = 0;

    return true;
}

/** must be called after construction in create() methods */
void
Loggable::update_id ( int id )
{
    /* make sure we're the last one */
    assert( _id == _log_id );
    assert( _loggables[ _id - 1 ] == this );

    _loggables[ _id - 1 ]  = NULL;

    _log_id = max( _log_id, id );

    /* return this id number to the system */
//            --_log_id;

    _id = id;

    /* make sure it'll fit */
    ensure_size( _id );

    ASSERT( ! _loggables[ _id - 1 ], "Attempt to create object with an ID (0x%X) that already exists. The existing object is of type \"%s\", the new one is \"%s\". Corrupt journal?", _id, _loggables[ _id - 1 ]->class_name(), class_name() );

    _loggables[ _id - 1 ] = this;
}

/** return a pointer to a static copy of /s/ with all special characters escaped */
const char *
Loggable::escape ( const char *s )
{
    static char r[512];

    size_t i = 0;
    for ( ; *s && i < sizeof( r ); ++i, ++s )
    {
        if ( '\n' == *s )
        {
            r[ i++ ] = '\\';
            r[ i ] = 'n';
        }
        else if ( '"' == *s )
        {
            r[ i++ ] = '\\';
            r[ i ] = '"';
        }
        else
            r[ i ] = *s;
    }

    r[ i ] = '\0';

    return r;
}

/** 'do' a message like "Audio_Region 0xF1 set :r 123" */
bool
Loggable::do_this ( const char *s, bool reverse )
{
    int id;
    if ( ! ( sscanf( s, "%*s %X ", &id ) > 0 ) )
        return false;

    Loggable *l = find( id );
//    assert( l );

    char classname[40];
    char command[40];
    char *arguments = NULL;

    const char *create, *destroy;

    if ( reverse )
    {
//        sscanf( s, "%s %*X %s %*[^\n<]<< %a[^\n]", classname, command, &arguments );
        sscanf( s, "%s %*X %s%*[^\n<]<< %a[^\n]", classname, command, &arguments );
        create = "destroy";
        destroy = "create";
    }
    else
    {
        sscanf( s, "%s %*X %s %a[^\n<]", classname, command, &arguments );
        create = "create";
        destroy = "destroy";
    }

    if ( ! strcmp( command, destroy ) )
    {
        /* deleting eg. a track, which contains a list of other
           widgets, causes destroy messages to be emitted for all those
           widgets, but when replaying the journal the destroy message
           causes the children to be deleted also... This is a temporary
           hack. Would it be better to queue up objects for deletion
           (when?) */
        if ( l )
            delete l;
    }
    else if ( ! strcmp( command, "set" ) )
    {
//        printf( "got set command (%s).\n", arguments );

        Log_Entry e( arguments );

        l->log_start();
        l->set( e );
        l->log_end();
    }
    else if ( ! strcmp( command, create ) )
    {
        Log_Entry e( arguments );

        ASSERT( _class_map[ std::string( classname ) ], "Journal contains an object of class \"%s\", but I don't know how to create such objects.", classname );

        {
            /* create */
            Loggable *l = _class_map[ std::string( classname ) ]( e );
            l->update_id( id );
            l->log_create();
        }

    }

    if ( arguments )
        free( arguments );

    return true;
}

/** Reverse the last journal transaction */
void
Loggable::undo ( void )
{
    char *buf = new char[ BUFSIZ ];

//    fflush( _fp );

    /* FIXME: handle more than the first block!!! */

    fseek( _fp, 0, SEEK_END );
    size_t len = ftell( _fp );

    fseek( _fp, 0 - (BUFSIZ > len ? len : BUFSIZ), SEEK_END );

    len = fread( buf, 1, BUFSIZ, _fp );

    char *s = buf + len - 1;

    int i = 1;

    /* move back _undo_index items from the end */
    for ( int j = _undo_index; j-- ; )
        for ( --s; *s && s >= buf; --s, ++i )
        {
            if ( *s == '\n' )
            {
                if ( *(s + 1) == '\t' )
                    continue;

                if ( *(s + 1) == '}' )
                {
                    *(s + 1) = NULL;
                    continue;
                }

                break;
            }
        }
    s++;

    buf[ len ] = NULL;

    if ( ! strlen( s ) )
    {
        WARNING( "corrupt undo file or no undo entries." );
        return;
    }

    char *b = s;

    s += strlen( s ) - 1;

    if ( strtok( b, "\n" ) == NULL )
        FATAL( "empty undo transaction!\n" );

    int n = 1;
    while ( strtok( NULL, "\n" ) )
        ++n;

    int ui = _undo_index;

    block_start();

    if ( strcmp( b, "{" ) )
    {
        /* It's a single undo, get rid of trailing messages in this block */

        n = 1;

        s = b + 2;
        s += strlen( s ) - 1;
    }

    while ( n-- )
    {
        while ( s >= b && *(--s) );

        s++;

        if ( ! strcmp( s, "{" ) )
            break;

        if ( *s == '\t' )
            s++;

        DMESSAGE( "undoing \"%s\"", s );

        do_this( s, true );

        s -= 2;
    }

    block_end();

    _undo_index = ui + 2;

    delete[] buf;
}

void
Loggable::compact_ids ( void )
{
    int id = 0;
    for ( int i = 0; i < _log_id; ++i )
        if ( _loggables[ i ] )
        {
            ++id;

            if ( _loggables[ id - 1 ] )
                continue;

            _loggables[ id - 1 ] = _loggables[ i ];
            _loggables[ i ] = NULL;
            _loggables[ id - 1 ]->_id = id;
        }

    _log_id = id;
}

/* FIXME: we need a version of this that is fully const, right? */
/** write a snapshot of the state of all loggable objects, sufficient
 * for later reconstruction, to /fp/ */
bool
Loggable::snapshot ( FILE *fp )
{
    FILE *ofp = _fp;

    if ( ! ( _fp = fp ) )
    {
        _fp = ofp;
        return false;
    }

    block_start();

    for ( int i = 0; i < _log_id; ++i )
    {
        const Loggable * l = _loggables[ i ];

        if ( l && _class_map[ std::string( l->class_name() ) ] )
            l->log_create();
    }

    block_end();

    _fp = ofp;

    return true;
}

bool
Loggable::snapshot ( const char *name )
{
    FILE *fp;

    if ( ! ( fp = fopen( name, "w" ) ))
        return false;

    snapshot( fp );

    fclose( fp );

    return true;
}

/** Replace the journal with a snapshot of the current state */
void
Loggable::compact ( void )
{
    fseek( _fp, 0, SEEK_SET );
    ftruncate( fileno( _fp ), 0 );

    compact_ids();
    if ( ! snapshot( _fp ) )
        FATAL( "Could not write snapshot!" );

    _undo_index = 1;
}

/** Buffered sprintf wrapper */
void
Loggable::log ( const char *fmt, ... )
{
    if ( ! _fp )
        return;

    /* FIXME: bogus limit  */
    static char buf[1024];
    static int i = 0;

    va_list args;

    if ( fmt )
    {
        va_start( args, fmt );
        i += vsprintf( buf + i, fmt, args );
        va_end( args );
    }

    if ( rindex( buf, '\n' ) )
    {
        _transaction.push( strdup( buf ) );
        i = 0;
    }
}

/** End the current transaction and commit it to the journal */
void
Loggable::flush ( void )
{
    if ( ! _fp )
    {
//        printf( "error: no log file open!\n" );

        while ( ! _transaction.empty() )
        {
            free( _transaction.front() );
            _transaction.pop();
        }

        return;
    }

    int n = _transaction.size();

    if ( n )
        /* something done, reset undo index */
        _undo_index = 1;

    if ( n > 1 )
        fprintf( _fp, "{\n" );

    while ( ! _transaction.empty() )
    {
        char *s = _transaction.front();

        _transaction.pop();

        if ( n > 1 )
            fprintf( _fp, "\t" );

        fprintf( _fp, "%s", s );

        free( s );
    }

    if ( n > 1 )
        fprintf( _fp, "}\n" );


    fflush( _fp );
}

/** Print bidirectional journal entry */
void
Loggable::log_print( const Log_Entry *o, const Log_Entry *n ) const
{
    if ( ! _fp )
        return;

    if ( n )
        for ( int i = 0; i < n->size(); ++i )
        {
            const char *s, *v;

            n->get( i, &s, &v );

            log( "%s %s%s", s, v, n->size() == i + 1 ? "" : " " );
        }

    if ( o && o->size() )
    {
        if ( n ) log( " << " );

        for ( int i = 0; i < o->size(); ++i )
        {
            const char *s, *v;

            o->get( i, &s, &v );

            log( "%s %s%s", s, v, o->size() == i + 1 ? "" : " " );
        }
    }

    log( "\n" );
}


/** Remember current object state for later comparison. *Must* be
 * called before any user action that might change one of the object's
 * journaled properties.  */
void
Loggable::log_start ( void )
{
    if ( ! _old_state )
    {
        _old_state = new Log_Entry;

        get( *_old_state );
    }
    ++_nest;
}

/** Log any change to the object's state since log_start(). */
void
Loggable::log_end ( void )
{
    if ( --_nest > 0 )
        return;

    Log_Entry *new_state;

    new_state = new Log_Entry;

    get( *new_state );

    if ( *_old_state != *new_state )
    {
        log( "%s 0x%X set ", class_name(), _id );

        log_print( _old_state, new_state );
    }

    if ( new_state )
        delete new_state;

    if ( _old_state )
        delete _old_state;

    _old_state = NULL;

    if ( Loggable::_level == 0 )
        Loggable::flush();
}

/** Log object creation. *Must* be called at the end of all public
 * constructors for leaf classes  */
void
Loggable::log_create ( void ) const
{
    if ( ! _fp )
        /* replaying, don't bother */

        return;
    log( "%s 0x%X create ", class_name(), _id );

    Log_Entry e;

    get( e );

    if ( e.size() )
        log_print( NULL, &e );
    else
        log( "\n" );

    if ( Loggable::_level == 0 )
        Loggable::flush();
}

/** Log object destruction. *Must* be called at the beginning of the
 * destructors of leaf classes */
void
Loggable::log_destroy ( void ) const
{
    if ( ! _fp )
        /* tearing down... don't bother */
        return;

    log( "%s 0x%X destroy << ", class_name(), _id );

    Log_Entry e;

    get( e );

    log_print( NULL, &e );

    if ( Loggable::_level == 0 )
        Loggable::flush();
}
