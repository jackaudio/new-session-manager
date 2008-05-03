
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

/* Master class for journaling. */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <vector>
#include <map>
#include <string>
#include <queue>
using namespace std;

#include "types.h"

class Log_Entry;
class Loggable;
typedef Loggable *(create_func)(Log_Entry &);


#define LOG_REGISTER_CREATE( class ) \
    Loggable::register_create( #class, & class ::create  );

#define LOG_NAME_FUNC( class ) \
    virtual const char *class_name ( void ) const { return #class ; }

#define LOG_CREATE_FUNC( class )                                        \
    static Loggable *                                                   \
    create ( Log_Entry &e )                                             \
    {                                                                   \
        class *r = new class;                                           \
        r->set( e );                                                    \
        return (Loggable *)r;                                           \
    }                                                                   \
    LOG_NAME_FUNC( class );                                             \


class Logger;
class Loggable
{

    static FILE *_fp;
    static int _log_id;
    static int _level;
    static int _undo_index;

    static vector <Loggable *> _loggables;

    static map <string, create_func*> _class_map;

    static queue <char *> _transaction;

private:

    int _id;

    char **_old_state;
    char **_new_state;

    int _nest;

    void log_print(  char **o, char **n ) const;
    static void log ( const char *fmt, ... );

    static void flush ( void );
    static
    void indent ( void )
        {
            int n = Loggable::_level;

            while ( n-- )
                log( "\t" );
        }

    static bool snapshot( FILE * fp );

public:

    static bool open ( const char *filename );
    static void undo ( void );
    static int undo_index ( void ) { return _undo_index; }
    static void compact ( void );

    static
    void
    block_start ( void )
        {
            ++Loggable::_level;
        }

    static
    void
    block_end ( void )
        {
            assert( --Loggable::_level >= 0 );

            if ( Loggable::_level == 0 )
                flush();
        }

    static
    Loggable *
    find ( int id )
        {
            if ( id > _log_id )
                return NULL;

            return _loggables[ id - 1 ];
        }

    Loggable ( )
        {
            _id = ++_log_id;
            _old_state = NULL;
            _nest = 0;

            _loggables.push_back( this );
        }

    /** must be called after construction in create() methods */
    void
    update_id ( int id )
        {
            assert( _id == _log_id );

            assert( _loggables[ _id - 1 ] == this );

            _loggables[ _id - 1 ]  = NULL;

//            --_log_id;

            _id = id;

            /* make sure it'll fit */
            _loggables.reserve( _id );

            assert( ! _loggables[ _id - 1 ] );

            _loggables[ _id - 1 ] = this;
        }

    virtual ~Loggable (  )
        {
            _loggables[ _id - 1 ] = NULL;
        }

    static
    void
    register_create ( const char *name, create_func *func )
        {
//            printf( "registering %s to %p\n", name, func );

            _class_map[ string( name ) ] = func;
        }

    /* log messages for journal */
    virtual void get ( Log_Entry &e ) const = 0;
    virtual void set ( Log_Entry &e ) = 0;

    static bool do_this ( const char *s, bool reverse );

protected:

    void log_start ( void );
    void log_end ( void );
    void log_create  ( void ) const;
    void log_destroy ( void ) const;

public:

    virtual const char *class_name ( void ) const = 0;

    int id ( void ) { return _id; }


    friend class Logger;
};


class Logger
{

    Loggable *_this;
    Logger ( ) {}

public:

    Logger ( Loggable *l ) : _this( l )
        {
            _this->log_start();

        }

    ~Logger ( )
        {
            _this->log_end();
        }

    void hold ( void )
        {
            printf( "hold\n" );
            _this->_nest++;
        }

    void release ( void )
        {
            printf( "release\n" );
            _this->_nest--;
            assert( _this->_nest );
        }
};



class Log_Entry
{
//    vector <Pair> _sa;
    char **_sa;
    int _i;

public:

    struct Pair
    {
        const char *name;
        const char *value;
    };

    Log_Entry ( )
        {
            _sa = (char**)malloc( sizeof( char * ) );
            *_sa = NULL;
            _i = 0;
        }

    Log_Entry ( char **sa )
        {
            _sa = sa;
            _i = 0;

            if ( _sa )
                while ( _sa[ _i ] ) ++_i;

        }

    ~Log_Entry ( )
        {
            if ( ! _sa )
                return;

            for ( _i = 0; _sa[ _i ]; ++_i )
            {
                free( _sa[ _i ] );
            }

            free( _sa );
        }

/****************/
/* Construction */
/****************/

    void grow (  )
        {
            _sa = (char**)realloc( _sa, sizeof( char * ) * (_i + 2) );
            _sa[ _i + 1 ] = NULL;
        }

#define ADD( type, format, exp )                                        \
    void add ( const char *name, type v )                               \
        {                                                               \
            grow();                                                     \
            asprintf( &_sa[ _i ], "%s " format, name, (exp) );          \
            strtok( _sa[ _i++ ], " " );                                 \
        }                                                               \

/***************/
/* Examination */
/***************/

    int size ( void ) const
        {
            return _i;
        }

    void get ( int n, const char **name, const char **value )
        {
            *name = _sa[ n ];
            *value = *name + strlen( *name ) + 1;
        }


    char **sa ( void )
        {
            char **sa = _sa;

            _sa = NULL;

            return sa;
        }

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
    ADD( const char *, "\"%s\"", v ? v : "" );
    ADD( Loggable *, "0x%X", v ? v->id() : 0 );
    ADD( float, "%f", v );
    ADD( double, "%f", v );

#undef ADD

};
