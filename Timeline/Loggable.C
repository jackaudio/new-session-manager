
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

#include <algorithm>
using std::min;
using std::max;



FILE *Loggable::_fp;
int Loggable::_log_id = 0;
int Loggable::_level = 0;
int Loggable::_undo_index = 1;

size_t Loggable::_loggables_size = 0;
Loggable ** Loggable::_loggables;
std::map <std::string, create_func*> Loggable::_class_map;
std::queue <char *> Loggable::_transaction;

bool
Loggable::open ( const char *filename )
{
    FILE *fp;

    if ( ! ( fp = fopen( filename, "a+" ) ) )
    {
        WARNING( "Could not open log file for writing!" );
        return false;
    }

    /* replay log */
    {
        char buf[BUFSIZ];

        while ( fscanf( fp, "%[^\n]\n", buf ) == 1 )
        {
            if ( ! ( ! strcmp( buf, "{" ) || ! strcmp( buf, "}" ) ) )
            {
                if ( *buf == '\t' )
                    do_this( buf + 1, false );
                else
                    do_this( buf, false );
            }
        }
    }


    Loggable::_fp = fp;

    return true;
}

/** close journal and delete all loggable objects */
bool
Loggable::close ( void )
{

    DMESSAGE( "closing journal and destroying all journaled objects" );

    if ( ! _fp )
        return true;

    fclose( _fp );

    _fp = NULL;

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

/** write a snapshot of the state of all loggable objects, sufficient
 * for later reconstruction, to /fp/ */
bool
Loggable::snapshot( FILE *fp )
{
    FILE *ofp = _fp;

    if ( ! ( _fp = fp ) )
    {
        _fp = ofp;
        return false;
    }

    /* first, make all ids consecutive */
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

void
Loggable::compact ( void )
{

    fseek( _fp, 0, SEEK_SET );
    ftruncate( fileno( _fp ), 0 );

    snapshot( _fp );

    _undo_index = 1;
}


void
Loggable::log ( const char *fmt, ... )
{
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

void
Loggable::log_print(  char **o, char **n ) const
{
    if ( n )
        for ( ; *n; n++ )
            log( "%s %s%s", *n, *n + strlen( *n ) + 1, *(n + 1) ? " " : ""  );

    if ( o && *o )
    {
        if ( n ) log( " << " );
        for ( ; *o; o++ )
            log( "%s %s%s", *o, *o + strlen( *o ) + 1, *(o + 1) ? " " : ""  );
    }

    log( "\n" );
}


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

void
Loggable::log_end ( void )
{

    if ( --_nest > 0 )
        return;

    Log_Entry *_new_state;

    {
        _new_state = new Log_Entry;

        get( *_new_state );
    }

    if ( *_old_state != *_new_state )
    {
//        indent();
        log( "%s 0x%X set ", class_name(), _id );

        log_print( _old_state->sa(), _new_state->sa() );
    }

    if ( _new_state )
        delete _new_state;

    if ( _old_state )
        delete _old_state;

    _old_state = NULL;

    if ( Loggable::_level == 0 )
        Loggable::flush();
}

void
Loggable::log_create ( void ) const
{
//    indent();

/*     if ( Loggable::_level ) */
/*     { */
/*         /\* defer until the block ends--this is really only required for capturing... *\/ */



/*     } */

    log( "%s 0x%X create ", class_name(), _id );

    char **sa;

    Log_Entry e;

    get( e );

    sa = e.sa();

    if ( sa )
    {
        log_print( NULL, sa );
    }
    else
        log( "\n" );

    if ( Loggable::_level == 0 )
        Loggable::flush();
}

void
Loggable::log_destroy ( void ) const
{
    if ( ! _fp )
        /* tearing down... don't bother */
        return;

    log( "%s 0x%X destroy << ", class_name(), _id );

    Log_Entry e;

    get( e );

    log_print( NULL, e.sa() );

    if ( Loggable::_level == 0 )
        Loggable::flush();
}
