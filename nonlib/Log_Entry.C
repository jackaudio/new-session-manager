
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

#include "Log_Entry.H"

// #include "const.h"
#include "debug.h"

Log_Entry::Log_Entry ( )
{
    _sa = (char**)malloc( sizeof( char * ) );
    *_sa = NULL;
    _i = 0;
}

Log_Entry::Log_Entry ( char **sa )
{
    _sa = sa;
    _i = 0;

    if ( _sa )
        while ( _sa[ _i ] ) ++_i;

}

Log_Entry::Log_Entry ( const char *s )
{
    _i = 0;
    _sa = s ? parse_alist( s ) : NULL;

    if ( _sa )
        while ( _sa[ _i ] ) ++_i;
}

Log_Entry::~Log_Entry ( )
{
    if ( ! _sa )
        return;

    for ( _i = 0; _sa[ _i ]; ++_i )
    {
        free( _sa[ _i ] );
    }

    free( _sa );
}


/** remove escapes from string /s/ in-place */
static void
unescape ( char *s )
{
    char *r = s;
    for ( ; *s; s++, r++ )
    {
        if ( '\\' == *s )
        {
            switch ( *(++s) )
            {
                case 'n':
                    *r = '\n';
                    break;
                case '"':
                    *r = '"';
                    break;
                default:
                    break;
            }
        }
        else
            *r = *s;
    }

    *r = '\0';
}

/** return a dynamically allocated string representing this log entry */
char *
Log_Entry::print ( void ) const
{
    /* FIXME: gross over-allocation */
    char *r = (char*)malloc( 1024 );

    r[0] = 0;

    for ( int i = 0; i < size(); ++i )
    {
        const char *s, *v;

        get( i, &s, &v );

        /* FIXME: arbitrary limit */
        char t[1024];
        snprintf( t, sizeof( t ), "%s %s%s", s, v, size() == i + 1 ? "" : " " );

        strcat( r, t );
    }

    char *r2 = (char*)malloc( strlen( r ) + 1 );

    strcpy( r2, r );

    free( r );

    return r2;
}

/** sigh. parse a string of ":name value :name value" pairs into an
 * array of strings, one per pair */
// FIXME: doesn't handle the case of :name ":foo bar", nested quotes
// or other things it should.
char **
Log_Entry::parse_alist( const char *s )
{

// FIXME: bogus over allocation...

    int tl = strlen( s );
    char **r = (char**)malloc( sizeof( char* ) * tl );

    bool quote = false;
    bool value = false;
    const char *c = NULL;
    int i = 0;
    for ( ; ; s++ )
    {
        switch ( *s )
        {
            case '\0':
            case ' ':
                if ( ! quote && c )
                {
                    if ( ! value )
                    {
                        value = true;
                        break;
                    }

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

                    unescape( v );

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

                    c = NULL;
                }
                break;
            case ':':                                           /* this is a key */
                if ( ! quote && ! c )
                {
                    c = s;
                    value = false;
                }
                break;
            case '"':
                quote = !quote;
                break;
            case '\\':
                s++;
                break;
        }

        if ( *s == '\0' )
            break;

    }

    r[ i ] = NULL;

    return r;
}

/** compare elements of dumps s1 and s2, removing those elements
    of dst which are not changed from src */
bool
Log_Entry::diff ( Log_Entry *e1, Log_Entry *e2 )
{

    if ( ! e1 )
        return true;

    char **sa1 = e1->_sa;
    char **sa2 = e2->_sa;

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

    e1->_i = w;
    e2->_i = w;

    return w == 0 ? false : true;
}

void
Log_Entry::grow (  )
{
    _sa = (char**)realloc( _sa, sizeof( char * ) * (_i + 2) );
    _sa[ _i + 1 ] = NULL;
}

int
Log_Entry::size ( void ) const
{
    return _i;
}

void
Log_Entry::get ( int n, const char **name, const char **value ) const
{
    *name = _sa[ n ];
    *value = *name + strlen( *name ) + 1;
}


char **
Log_Entry::sa ( void )
{
    return _sa;

/*   char **sa = _sa; */

/* //    _sa = NULL; */

/*     return sa; */
/* } */

}
