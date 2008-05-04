
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

#define _LOGGABLE_C
#include "Loggable.H"
#undef _LOGABLE_C

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

FILE *Loggable::_fp;
int Loggable::_log_id = 0;
int Loggable::_level = 0;
int Loggable::_undo_index = 1;

vector <Loggable *> Loggable::_loggables;
map <string, create_func*> Loggable::_class_map;
queue <char *> Loggable::_transaction;

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

    for ( int i = _log_id; i--; )
    {
        Loggable ** l = &_loggables[ i ];

        if ( *l )
            delete *l;

        *l = NULL;
    }

    return true;
}

/** sigh. parse a string of ":name value :name value" pairs into an
 * array of strings, one per pair */
// FIXME: doesn't handle the case of :name ":foo bar", nested quotes
// or other things it should.
static
char **
parse_alist( const char *s )
{

// FIXME: bogus over allocation...

    int tl = strlen( s );
    char **r = (char**)malloc( sizeof( char* ) * tl );

//    const char *e = s + tl;

    const char *c = NULL;
    int i = 0;
    for ( ; ; s++ )
    {

/*         if ( *s == '\n' ) */
/*             break; */

//        if ( *s == ':' || s == e )
        if ( *s == ':' || *s == '\0' )
        {
            if ( c )
            {
                int l = s - c;

                char *pair = (char*)malloc( l + 1 );

                /* remove trailing space */
                if ( c[ l  - 1 ] == ' ' )
                    --l;

                strncpy( pair, c, l );

                pair[ l ] = '\0';

                r[ i++ ] = pair;

                /* split */

                strtok( pair, " " );

                /* remove quotes */
                char *v = pair + strlen( pair ) + 1;

                if  ( *v == '"' )
                {
//                    v++;
                    if ( v[ strlen( v ) - 1 ] != '"' )
                        WARNING( "invalid quoting in log entry!" );
                    else
                    {
                        v[ strlen( v ) - 1 ] = '\0';
                        memmove( v, v + 1, strlen( v ) + 1 );
                    }
                }
            }

            c = s;

            if ( *s == '\0' )
                break;
        }
    }

    r[ i ] = NULL;

    return r;
}


static
void free_sa ( char **sa )
{
    char **a = sa;
    for ( ; *a; a++ )
        free( *a );

    free( sa );
}

/** 'do' a message like "Region 0xF1 set :r 123" */
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
    char *arguments;

    const char *create, *destroy;

    if ( reverse )
    {
        sscanf( s, "%s %*X %s %*[^\n<]<< %a[^\n]", classname, command, &arguments );
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
        {
            int id = l->id();
            delete l;
            _loggables[ id ] = NULL;
        }
    }
    else if ( ! strcmp( command, "set" ) )
    {
//        printf( "got set command (%s).\n", arguments );

        char **sa  = parse_alist( arguments );

        Log_Entry e( sa );

        l->log_start();
        l->set( e );
        l->log_end();
    }
    else if ( ! strcmp( command, create ) )
    {
        char **sa = NULL;

        if ( arguments )
            sa = parse_alist( arguments );

        Log_Entry e( sa );





        assert( _class_map[ string( classname ) ] );

/*         if ( ! _class_map[ string( classname ) ] ) */
/*             printf( "error class %s is unregistered!\n", classname ); */
/*         else */

        {
            /* create */
            Loggable *l = _class_map[ string( classname ) ]( e );
            l->update_id( id );
            l->log_create();
        }

    }

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

        DMESSAGE( "undoing \"%s\"\n", s );

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

    block_start();

    for ( int i = 0; i < _log_id; ++i )
    {
        const Loggable * l = _loggables[ i ];

        if ( l && _class_map[ string( l->class_name() ) ] )
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

/** compare elements of dumps s1 and s2, removing those elements
    of dst which are not changed from src */
static
bool
log_diff (  char **sa1, char **sa2 )
{
    if ( ! sa1 )
        return true;

    int w = 0;
    for ( int i = 0; sa1[ i ]; ++i )
    {
        const char *v1 = sa1[ i ] + strlen( sa1[ i ] ) + 1;
        const char *v2 = sa2[ i ] + strlen( sa2[ i ] ) + 1;

        if ( ! strcmp( sa1[ i ], sa2[ i ] ) && ! strcmp( v1, v2 ) )
        {
            free( sa2[ i ] );
            free( sa1[ i ] );
        }
        else
        {
            sa2[ w ] = sa2[ i ];
            sa1[ w ] = sa1[ i ];

            w++;
        }
    }

    sa1[ w ] = NULL;
    sa2[ w ] = NULL;

    return w == 0 ? false : true;
}

void
Loggable::log_start ( void )
{
    if ( ! _old_state )
    {
        Log_Entry e;

        get( e );

        _old_state = e.sa();
    }
    ++_nest;
}

void
Loggable::log_end ( void )
{

    if ( --_nest > 0 )
        return;

    char **_new_state;

    {
        Log_Entry e;

        get( e );

        _new_state = e.sa();
    }

    if ( log_diff( _old_state, _new_state ) )
    {
//        indent();
        log( "%s 0x%X set ", class_name(), _id );

        log_print( _old_state, _new_state );
    }

    if ( _new_state )
        free_sa( _new_state );

    if ( _old_state )
        free_sa( _old_state );

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
        free_sa( sa );
    }
    else
        log( "\n" );

    if ( Loggable::_level == 0 )
        Loggable::flush();
}

void
Loggable::log_destroy ( void ) const
{
//    indent();
    log( "%s 0x%X destroy (nothing) << ", class_name(), _id );

   char **sa;

    Log_Entry e;

    get( e );

    sa = e.sa();

//    log_print( sa, NULL );
    log_print( NULL, sa );

    free_sa( sa );

    if ( Loggable::_level == 0 )
        Loggable::flush();
}
