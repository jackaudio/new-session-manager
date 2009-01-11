
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

/* This class handles all journaling. All journaled objects must
   inherit from Loggable as well as define a few special methods (via
   macros), get and set methods, and have contructors and destructors
   that call log_create() and log_destroy() in the appropriate
   order. Any action that might affect multiple loggable objects
   *must* be braced by calls to Loggable::block_start() and
   Loggable::block_end() in order for Undo to work properly. */

#include "Loggable.H"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "util/file.h"

#include "const.h"
#include "util/debug.h"

#include <algorithm>
using std::min;
using std::max;



FILE *Loggable::_fp;
unsigned int Loggable::_log_id = 0;
int Loggable::_level = 0;

off_t Loggable::_undo_offset = 0;

std::map <unsigned int, Loggable *> Loggable::_loggables;
std::map <unsigned int, Log_Entry *> Loggable::_loggables_unjournaled;

std::map <std::string, create_func*> Loggable::_class_map;
std::queue <char *> Loggable::_transaction;

progress_func *Loggable::_progress_callback = NULL;
void *Loggable::_progress_callback_arg = NULL;

snapshot_func *Loggable::_snapshot_callback = NULL;
void *Loggable::_snapshot_callback_arg = NULL;



Loggable::~Loggable ( )
{
    _loggables[ _id ] = NULL;
}




void
Loggable::block_start ( void )
{
    ++Loggable::_level;
}

void
Loggable::block_end ( void )
{
    --Loggable::_level;

    ASSERT( Loggable::_level >= 0, "Programming error" );

    if ( Loggable::_level == 0 )
        flush();
}

Loggable *
Loggable::find ( unsigned int id )
{
    return _loggables[ id ];
}


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

    load_unjournaled_state();

    if ( newer( "snapshot", filename ) )
    {
        MESSAGE( "Loading snapshot" );

        FILE *fp = fopen( "snapshot", "r" );

        replay( fp );

        fclose( fp );
    }
    else
    {
        MESSAGE( "Replaying journal" );

        replay( fp );
    }

    fseek( fp, 0, SEEK_END );
    _undo_offset = ftell( fp );

    Loggable::_fp = fp;

    return true;
}

bool
Loggable::load_unjournaled_state ( void )
{
    FILE *fp;

    fp = fopen( "unjournaled", "r" );

    if ( ! fp )
        return false;

    unsigned int id;
    char buf[BUFSIZ];

    while ( fscanf( fp, "%X set %[^\n]\n", &id, buf ) == 2 )
    {
        Log_Entry *e = new Log_Entry( buf );

        _loggables_unjournaled[ id ] = e;

        Loggable *l = Loggable::find( id );
    }

    fclose( fp );

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

    if ( ! snapshot( "snapshot" ) )
        WARNING( "Failed to create snapshot" );


    for ( std::map <unsigned int, Loggable *>::iterator i = _loggables.begin();
          i != _loggables.end(); ++i )
    {
        if ( i->second )
        {
            delete i->second;
        }
    }

    save_unjournaled_state();

    return true;
}


/** save out unjournaled state for all loggables */
bool
Loggable::save_unjournaled_state ( void )
{

    /* FIXME: check for errors */
    FILE *fp = fopen( "unjournaled", "w" );

    /* write out the unjournaled state of all currently active
     * loggables */
    for ( std::map <unsigned int, Log_Entry *>::iterator i = _loggables_unjournaled.begin();
          i != _loggables_unjournaled.end(); ++i )
    {
        if ( i->second )
        {
            char *s = i->second->print();

            fprintf( fp, "0x%X set %s\n", i->first, s );

            free( s );
        }
    }

    /* write out the remembered state of inactive loggables. */

    fclose( fp );

    return true;
}

/** must be called after construction in create() methods */
void
Loggable::update_id ( unsigned int id )
{
    /* make sure we're the last one */
    assert( _id == _log_id );
    assert( _loggables[ _id ] == this );

    _loggables[ _id ] = NULL;

    _log_id = max( _log_id, id );

    /* return this id number to the system */
//            --_log_id;

    _id = id;

    if ( _loggables[ _id ] )
        FATAL( "Attempt to create object with an ID (0x%X) that already exists. The existing object is of type \"%s\", the new one is \"%s\". Corrupt journal?", _id, _loggables[ _id ]->class_name(), class_name() );

    _loggables[ _id ] = this;
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
    unsigned int id = 0;

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

        DMESSAGE( "undoing \"%s\"", s );
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
            Loggable *l = _class_map[ std::string( classname ) ]( e, id );
            l->log_create();

            /* we're now creating a loggable. Apply any unjournaled
             * state it may have had in the past under this log ID */

            Log_Entry *e = _loggables_unjournaled[ id ];

            if ( e )
                l->set( *e );
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
    const int bufsiz = 1024;
    char buf[bufsiz];

    block_start();

    long here = ftell( _fp );

    fseek( _fp, _undo_offset, SEEK_SET );

    backwards_fgets( buf, bufsiz, _fp );

    if ( ! strcmp( buf, "}\n" ) )
    {
        DMESSAGE( "undoing block" );
        for ( ;; )
        {
            backwards_fgets( buf, bufsiz, _fp );

            char *s = buf;
            if ( *s != '\t' )
                break;
            else
                ++s;

            do_this( s, true );
        }
    }
    else
        do_this( buf, true );

    off_t uo = ftell( _fp );

    ASSERT( _undo_offset <= here, "WTF?" );

    block_end();

    _undo_offset = uo;
}

/** write a snapshot of the current state of all loggable objects to
 * file handle /fp/ */
bool
Loggable::snapshot ( FILE *fp )
{
    FILE *ofp = _fp;

    if ( ! Loggable::_snapshot_callback )
    {
        DWARNING( "No snapshot callback defined" );
        return false;
    }

    if ( ! ( _fp = fp ) )
    {
        _fp = ofp;
        return false;
    }

    block_start();

    Loggable::_snapshot_callback( _snapshot_callback_arg );

    block_end();

    _fp = ofp;

    return true;
}

/** write a snapshot of the current state of all loggable objects to
 * file /name/ */
bool
Loggable::snapshot ( const char *name )
{
    FILE *fp;

    if ( ! ( fp = fopen( name, "w" ) ))
        return false;

    bool r = snapshot( fp );

    fclose( fp );

    return r;
}

/** Replace the journal with a snapshot of the current state */
void
Loggable::compact ( void )
{
    fseek( _fp, 0, SEEK_SET );
    ftruncate( fileno( _fp ), 0 );

    if ( ! snapshot( _fp ) )
        FATAL( "Could not write snapshot!" );

    fseek( _fp, 0, SEEK_END );
}

#include <stdarg.h>

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

    if ( n )
        /* something done, reset undo index */
        _undo_offset = ftell( _fp );

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
    ASSERT( _old_state, "Programming error: log_end() called before log_start()" );

    if ( --_nest > 0 )
        return;

    Log_Entry *new_state;

    new_state = new Log_Entry;

    get( *new_state );

    if ( Log_Entry::diff( _old_state, new_state ) )
    {
        log( "%s 0x%X set ", class_name(), _id );

        log_print( _old_state, new_state );
    }

    delete new_state;
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

/** record this loggable's unjournaled state in memory */
void
Loggable::record_unjournaled ( void ) const
{
    Log_Entry *e = new Log_Entry();

    get_unjournaled( *e );

    Log_Entry *le = _loggables_unjournaled[ _id ];

    if ( le )
        delete le;

    if ( e->size() )
    {
        _loggables_unjournaled[ _id ] = e;
        DMESSAGE( "logging %s", e->print() );
    }
    else
        /* don't waste space on loggables with no unjournaled properties */
        _loggables_unjournaled[ _id ] = NULL;
}

/** Log object destruction. *Must* be called at the beginning of the
 * destructors of leaf classes */
void
Loggable::log_destroy ( void ) const
{
    /* the unjournaled state may have changed: make a note of it. */
    record_unjournaled();

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
