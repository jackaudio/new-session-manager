
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

FILE *Loggable::_fp;
int Loggable::_log_id = 0;
int Loggable::_level = 0;
int Loggable::_undo_index = 1;

vector <Loggable *> Loggable::_loggables;
map <string, create_func*> Loggable::_class_map;

bool
Loggable::open ( const char *filename )
{
    if ( ! ( Loggable::_fp = fopen( filename, "a+" ) ) )
    {
        printf( "Could not open log file for writing!" );
        return false;
    }

    return true;
}

/** sigh. parse a string of ":name value :name value" pairs into an array of strings, one per pair */
// FIXME: doesn't handle the case of :name ":foo bar". Also, quotes should be removed here, not in client code.
static
char **
parse_alist( const char *s )
{

// FIXME: bogus over allocation...

    int tl = strlen( s );
    char **r = (char**)malloc( sizeof( char* ) * tl );

    const char *e = s + tl;

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

                strncpy( pair, c, l );

                pair[ l ] = '\0';

                r[ i++ ] = pair;
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


void
Loggable::undo ( void )
{
    char *buf = new char[ BUFSIZ ];

//    fflush( _fp );

    fseek( _fp, 0, SEEK_END );
    size_t len = ftell( _fp );

    fseek( _fp, 0 - (BUFSIZ > len ? len : BUFSIZ), SEEK_END );

    len = fread( buf, 1, BUFSIZ, _fp );

    char *s = buf + len - 1;

// FIXME: handle blocks
    int i = 1;

    /* move back _undo_index items from the end */
    for ( int j = _undo_index; j-- ; )
        for ( --s; *s && s >= buf; --s, ++i )
        {
            if ( *s == '\n' )
            {
                if ( *(s + 1) == '\t' )
                    continue;

                break;
            }
        }
    s++;

    strtok( s, "\n" );

    buf[ len ] = NULL;

//    fsync( fileno( _fp ) );
    /* pop the entry off the end */

/*     fseek( _fp, 0 - i, SEEK_END ); */
/*     ftruncate( fileno( _fp ), ftell( _fp ) ); */

    if ( ! strlen( s ) )
    {
        printf( "corrupt undo file or no undo entries.\n" );
        return;
    }

    printf( "undoing \"%s\"\n", s );

    int id;
    sscanf( s, "%*s %X ", &id );
    Loggable *l = find( id );
//    assert( l );

    char classname[40];
    char command[40];
    char *arguments;

    sscanf( s, "%s %*X %s %*[^\n<] << %a[^\n]", classname, command, &arguments );


    int ui = _undo_index;


    if ( ! l )
    {
        printf( "corrupt undo?\n" );
        abort();
    }



    if ( ! strcmp( command, "destroy" ) )
    {
        printf( "should create new %s here\n", classname );

        char **sa = parse_alist( arguments );

        _class_map[ string( classname ) ]( sa );
    }
    else
        if ( ! strcmp( command, "set" ) )
        {

            printf( "got set command.\n" );

            char **sa  = parse_alist( arguments );

            l->log_start();
            l->set( sa );
            l->log_end();

        }
        else
            if ( ! strcmp( command, "create" ) )
            {
                int id = l->id();
                delete l;
                _loggables[ id ] = NULL;
            }


// FIXME: bogus... needs to account for multiple events.
    _undo_index = ui + 1;

    ++_undo_index;

    delete buf;
}


void
Loggable::log ( const char *fmt, ... )
{
    va_list args;

    if ( fmt )
    {
        va_start( args, fmt );
        vfprintf( _fp, fmt, args );
        va_end( args );
    }

    fflush( _fp );
}


void
Loggable::log_print(  char **o, char **n )
{
    if ( n )
        for ( ; *n; n++ )
            log( "%s%s", *n, *(n + 1) ? " " : ""  );

    if ( o && *o )
    {
        if ( n ) log( " << " );
        for ( ; *o; o++ )
            log( "%s%s", *o, *(o + 1) ? " " : ""  );
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
        if ( ! strcmp( sa1[ i ], sa2[ i ] ) )
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
        _old_state = log_dump();

    ++_nest;

    _undo_index = 1;
}

void
Loggable::log_end ( void )
{

    if ( --_nest > 0 )
        return;

//    assert( _old_state );

    char **_new_state = log_dump();

    // if ( _old_state )

    if ( log_diff( _old_state, _new_state ) )
    {
        indent();
        log( "%s 0x%X set ", class_name(), _id );

        log_print( _old_state, _new_state );
    }

    if ( _new_state )
        free_sa( _new_state );

    if ( _old_state )
        free_sa( _old_state );

    _old_state = NULL;

/*     if ( _old_state ) */
/*     { */
/*         free_sa( _old_state ); */
/*         _old_state = NULL; */
/*     } */

//    _old_state = NULL;
}

void
Loggable::log_create ( void )
{
    indent();
    log( "%s 0x%X create ", class_name(), _id );

    char **sa = log_dump();

    if ( sa )
    {
        log_print( NULL, sa );
        free_sa( sa );
    }
    else
        log( "\n" );
}

void
Loggable::log_destroy ( void )
{
    indent();
    log( "%s 0x%X destroy (nothing) << ", class_name(), _id );

    char **sa = log_dump();

//    log_print( sa, NULL );
    log_print( NULL, sa );

    free_sa( sa );
}
