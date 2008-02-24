
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

#include <stdio.h>
#include <stdarg.h>

FILE *Loggable::_fp;

int Loggable::_log_id = 0;
int Loggable::_level = 0;

vector <Loggable *> Loggable::_loggables;

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

/* void */
/* Loggable::log ( const char *module, const char *action, const char *fmt, ... ) */
/* { */
/*     va_list args; */

/*     fprintf( _fp, "%-15s %-8s %p ", module, action, _id ); */

/*     if ( fmt ) */
/*     { */
/*         va_start( args, fmt ); */
/*         vfprintf( _fp, fmt, args ); */
/*         va_end( args ); */
/*     } */

/*     fprintf( _fp, "\n" ); */
/* } */


static
void free_sa ( char **sa )
{
    char **a = sa;
    for ( ; *a; a++ )
        free( *a );

    free( sa );
}


static
void
log_print(  char **o, char **n )
{
    if ( n )
        for ( ; *n; n++ )
            printf( "%s%s", *n, *(n + 1) ? " " : ""  );

    if ( o && *o )
    {
        if ( n ) printf( " << " );
        for ( ; *o; o++ )
            printf( "%s%s", *o, *(o + 1) ? " " : ""  );
    }

    printf( "\n" );
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
//    if ( _old_state )
    //      log_end();
    if ( _old_state )
        return;

    _old_state = log_dump();
}

void
Loggable::log_end ( void )
{
    char **_new_state = log_dump();

    // if ( _old_state )

    if ( log_diff( _old_state, _new_state ) )
    {
        indent();
        printf( "%s 0x%X set ", class_name(), _id );

        log_print( _old_state, _new_state );
    }

    free_sa( _old_state );
    if ( _new_state )
        free_sa( _new_state );

    _old_state = NULL;
}

void
Loggable::log_create ( void )
{
    indent();
    printf( "%s 0x%X new ", class_name(), _id );

    char **sa = log_dump();

    if ( sa )
    {
        log_print( NULL, sa );
        free_sa( sa );
    }
    else
        printf( "\n" );
}

void
Loggable::log_destroy ( void )
{
    indent();
    printf( "%s 0x%X destroy ", class_name(), _id );

    char **sa = log_dump();

    log_print( sa, NULL );

    free_sa( sa );
}
