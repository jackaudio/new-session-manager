
/*******************************************************************************/
/* Copyright (C) 2010 Jonathan Moore Liles                                     */
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

#include <lo/lo.h>
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Endpoint.H"

namespace OSC
{

    void
    Endpoint::error_handler(int num, const char *msg, const char *path)
    {
        WARNING( "LibLO server error %d in path %s: %s\n", num, path, msg);
    }

    Endpoint::Endpoint ( const char *port )
    {
        DMESSAGE( "Creating OSC server" );

//    _st = lo_server_thread_new( s, error_handler );
        //   _server = lo_server_thread_get_server( _st );

        _server = lo_server_new( port, error_handler );

        if ( ! _server )
            FATAL( "Error creating OSC server" );

        char *url = lo_server_get_url(_server);
        printf("OSC: %s\n",url);
        free(url);


        add_method( NULL, "", &Endpoint::osc_generic, this, "" );

//        _path_names = new std::list<const char*>();
    }

    Endpoint::~Endpoint ( )
    {
//    lo_server_thread_free( _st );
        lo_server_free( _server );
    }


    int
    Endpoint::osc_generic ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
    {
        OSC_DMSG();

        if ( path[ strlen(path) - 1 ] != '/' )
            return -1;

        char *paths = ((Endpoint*)user_data)->get_paths( path );

        ((Endpoint*)user_data)->send( lo_message_get_source( msg ), "/reply", path, paths );

        free(paths);

        return 0;
    }

    // returns a malloc()'d string containing path names beginning with /prefix/, newline separated
    char *
    Endpoint::get_paths ( const char *prefix )
    {
        char *r = (char*)malloc( 1024 );
        r[0] = 0;
        
        for ( std::list<char*>::iterator i = _path_names.begin(); i != _path_names.end(); ++i )
        {
            if ( ! *i )
                continue;

            if (! strncmp( *i, prefix, strlen(prefix) ) )
            {
                r = (char*)realloc( r, strlen( r ) + strlen( *i ) + 2 );

                strcat( r, *i );
                strcat( r, "\n" );
            }
        }

        return r;
    }

    void
    Endpoint::add_method ( const char *path, const char *typespec, lo_method_handler handler, void *user_data, const char *argument_description )
    {
	DMESSAGE( "Added OSC method %s (%s)", path, typespec );
 
        lo_server_add_method( _server, path, typespec, handler, user_data );

        char *stored_path = NULL;
        
        asprintf( &stored_path, "%s (%s); %s", path, typespec, argument_description );
        
        _path_names.push_back( stored_path );
    }

    void
    Endpoint::del_method ( const char *path, const char *typespec )
    {
	DMESSAGE( "Deleted OSC method %s (%s)", path, typespec );

        lo_server_del_method( _server, path, typespec );

        for ( std::list<char *>::iterator i = _path_names.begin(); i != _path_names.end(); ++i )
        {
            if ( ! *i )
                continue;

            if ( ! strncmp( path, *i, index( *i, ' ' ) - *i ) )
            {
                free( *i );
                i = _path_names.erase( i );
            }
        }
    }

/* void * */
/* Endpoint::osc_thread ( void * arg ) */
/* { */
/*     ((Endpoint*)arg)->osc_thread(); */

/*     return NULL; */
/* } */

/* void */
/* Endpoint::osc_thread ( void ) */
/* { */
/*     _thread.name( "OSC" ); */

/*     DMESSAGE( "OSC Thread running" ); */

/*     for ( ;; ) */
/*     { */
/*         lo_server_recv( _sever ); */
/*     } */
/* } */

    void
    Endpoint::start ( void )
    {

/*     if ( !_thread.clone( &Endpoint::osc_thread, this ) ) */
/*         FATAL( "Could not create OSC thread" ); */

/*      lo_server_thread_start( _st ); */

    }

    void
    Endpoint::stop ( void )
    {
//    lo_server_thread_stop( _st );
    }

    int
    Endpoint::port ( void ) const
    {
        return lo_server_get_port( _server );
    }

    char *
    Endpoint::url ( void ) const
    {
        return lo_server_get_url( _server );
    }

/** Process any waiting events and return immediately */
    void
    Endpoint::check ( void ) const
    {
        wait( 0 );
    }

/** Process any waiting events and return immediately */
    void
    Endpoint::wait ( int timeout ) const
    {
        lo_server_recv_noblock( _server, timeout );
    }

/** Process events forever */
    void
    Endpoint::run ( void ) const
    {
        for ( ;; )
        {
            lo_server_recv( _server );
        }
    }

    int
    Endpoint::send ( lo_address to, const char *path, std::list< OSC_Value > values )
    {

        lo_message m = lo_message_new();

        for ( std::list< OSC_Value >::const_iterator i = values.begin();
              i != values.end();
              ++i )
        {
            const OSC_Value *ov = &(*i);

            switch ( ov->type() )
            {
                case 'f':
                    lo_message_add_float( m, ((OSC_Float*)ov)->value() );
                    break;
                case 'i':
                    lo_message_add_int32( m, ((OSC_Int*)ov)->value() );
                    break;
                case 's':
                    DMESSAGE( "Adding string %s", ((OSC_String*)ov)->value() );
                    lo_message_add_string( m, ((OSC_String*)ov)->value() );
                    break;
                default:
                    FATAL( "Unknown format: %c", ov->type() );
                    break;
            }
        }

        DMESSAGE( "Path: %s", path );

        lo_bundle b = lo_bundle_new( LO_TT_IMMEDIATE );

        lo_bundle_add_message(b, path, m );

        int r = lo_send_bundle_from( to, _server, b );

//    int r = lo_send_message_from( to, _server, path, m );

//    lo_message_free( m );

        return r;
    }

    int
    Endpoint::send ( lo_address to, const char *path )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "" );
    }

    int
    Endpoint::send ( lo_address to, const char *path, int v )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "i", v );
    }

    int
    Endpoint::send ( lo_address to, const char *path, float v )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "f", v );
    }

    int
    Endpoint::send ( lo_address to, const char *path, double v )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "d", v );
    }

    int
    Endpoint::send ( lo_address to, const char *path, const char * v )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "s", v );
    }

    int
    Endpoint::send ( lo_address to, const char *path, const char * v1, const char *v2 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "ss", v1, v2 );
    }

}
