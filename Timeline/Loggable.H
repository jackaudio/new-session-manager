
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

#include <map>
#include <string>
#include <queue>

#include "types.h"

typedef void (progress_func)( int, void * );
typedef void (snapshot_func)( void * );

class Log_Entry;
class Loggable;
typedef Loggable *(create_func)(Log_Entry &, unsigned int id);

#define LOG_REGISTER_CREATE( class ) \
    Loggable::register_create( #class, & class ::create  );

#define LOG_NAME_FUNC( class ) \
    virtual const char *class_name ( void ) const { return #class ; }

#define LOG_CREATE_FUNC( class )                                        \
    static Loggable *                                                   \
    create ( Log_Entry &e, unsigned int id )                                     \
    {                                                                   \
        class *r = new class;                                           \
        r->update_id( id );                                             \
        r->set( e );                                                    \
        return (Loggable *)r;                                           \
    }                                                                   \
    LOG_NAME_FUNC( class );                                             \


#define LOG_NOT_LOGGABLE_FUNC( class ) \
    virtual const char *class_name ( void ) const { return #class ; } \

class Logger;
class Loggable
{

    static FILE *_fp;
    static unsigned int _log_id;
    static int _level;

    static off_t _undo_offset;

    static size_t _loggables_size;
    static Loggable ** _loggables;
    static std::map <unsigned int, Log_Entry *> _loggables_unjournaled;

    static std::map <std::string, create_func*> _class_map;

    static std::queue <char *> _transaction;

    static progress_func *_progress_callback;
    static void *_progress_callback_arg;

    static snapshot_func *_snapshot_callback;
    static void *_snapshot_callback_arg;

private:

    unsigned int _id;

    Log_Entry *_old_state;

    int _nest;

    static void ensure_size ( size_t n );

    void log_print ( const Log_Entry *o, const Log_Entry *n ) const;
    static void log ( const char *fmt, ... );

    static void flush ( void );

    static bool snapshot ( FILE * fp );
    static bool snapshot ( const char *name );
    static bool save_unjournaled_state ( void );
    static bool replay ( FILE *fp );

    void init ( bool loggable=true )
        {
            // _new_state
            _old_state = NULL;
            _nest = 0;

            if ( loggable )
            {
                _id = ++_log_id;

                ensure_size( _id );

                _loggables[ _id - 1 ] = this;
            }
            else
                _id = 0;

        }

    /* not implemented */
    const Loggable & operator= ( const Loggable &rhs );

    void record_unjournaled ( void ) const;
    static bool load_unjournaled_state ( void );

public:

    static void snapshot_callback ( snapshot_func *p, void *arg ) { _snapshot_callback = p; _snapshot_callback_arg = arg; }
    static void progress_callback ( progress_func *p, void *arg ) { _progress_callback = p; _progress_callback_arg = arg;}
    static const char *escape ( const char *s );

    unsigned int id ( void ) const { return _id; }

    static bool open ( const char *filename );
    static bool close ( void );
    static void undo ( void );

    static void compact ( void );

    static void block_start ( void );
    static void block_end ( void );

    static Loggable * find ( unsigned int id );

    Loggable ( bool loggable=true )
        {
            init( loggable );
        }

    void update_id ( unsigned int id );

    virtual ~Loggable (  );

    static
    void
    register_create ( const char *name, create_func *func )
        {
            _class_map[ std::string( name ) ] = func;
        }

    /* log messages for journal */
    virtual void get ( Log_Entry &e ) const = 0;
    virtual void get_unjournaled ( Log_Entry & ) const
        {
            /* implementation optional */
        }
    virtual void set ( Log_Entry &e ) = 0;

    virtual const char *class_name ( void ) const = 0;

    virtual void log_children ( void ) const { return; }

    static bool do_this ( const char *s, bool reverse );

    void log_create  ( void ) const;

protected:

    void log_start ( void );
    void log_end ( void );

    void log_destroy ( void ) const;

    /* leaf subclasses *must* call log_create() at the end of their copy contructors */
    Loggable ( const Loggable & )
        {
            init( true );
        }

public:

    friend class Logger;
};


class Logger
{

    Loggable *_this;
    Logger ( ) {}

    /* not permitted */
    Logger ( const Logger &rhs );
    const Logger & operator= ( const Logger &rhs );

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
            _this->_nest++;
        }

    void release ( void )
        {
            _this->_nest--;
            assert( _this->_nest );
        }
};

#include "Log_Entry.H"
