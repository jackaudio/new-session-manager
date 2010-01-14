
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

#pragma once

#include "Loggable.H"

#include "types.h"

class Log_Entry
{
//    vector <Pair> _sa;
    char **_sa;
    int _i;

    /* not permitted */
    Log_Entry ( const Log_Entry &rhs );
    Log_Entry & operator= ( const Log_Entry &rhs );

    static char ** parse_alist ( const char *s );
    static bool log_diff (  char **sa1, char **sa2 );

public:

    Log_Entry ( );
    Log_Entry ( char **sa );
    Log_Entry ( const char *s );
    ~Log_Entry ( );

/****************/
/* Construction */
/****************/

    void grow (  );

#define ADD( type, format, exp )                                \
    void add ( const char *name, type v )                       \
        {                                                       \
            grow();                                             \
            asprintf( &_sa[ _i ], "%s " format, name, (exp) );  \
            strtok( _sa[ _i++ ], " " );                         \
        }

    void add_raw ( const char *name, const char *v )
        {
            grow();
            asprintf( &_sa[ _i ], "%s %s", name, v );
            strtok( _sa[ _i++ ], " " );
        }

/***************/
/* Examination */
/***************/

    static bool diff ( Log_Entry *e1, Log_Entry *e2 );

    int size ( void ) const;

    void get ( int n, const char **name, const char **value ) const;
    char **sa ( void );

    char *print ( void ) const;

/* #define ADD ( type, format, exp )             \ */
/*     void add ( const char *name, type v ) \ */
/*         { \ */
/*             char pat[ 256 ]; \ */
/*             Pair p; \ */
/*             p.name = strdup( name ); \ */
/*             snprintf( pat, sizeof( pat ), format, exp ); \ */
/*             p.value = strdup( pat ); \ */
/*             _sa.push( p ); \ */
/*         } \ */

    ADD( int, "%d", v );
    ADD( nframes_t, "%lu", (unsigned long)v );
    ADD( unsigned long, "%lu", v );
    ADD( const char *, "\"%s\"", v ? Loggable::escape( v ) : "" );
    ADD( Loggable * , "0x%X", v ? v->id() : 0 );
    ADD( float, "%f", v );
    ADD( double, "%f", v );

#undef ADD

};
