
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

#pragma once

#include <lo/lo.h>
//#include "util/Thread.H"
#include <list>

namespace OSC
{

    class OSC_Value
    {

    protected:

        char _type;

        float f;
        double d;
        int i;
        const char *s;

    public:

        OSC_Value ( const OSC_Value &rhs )
            {
                _type = rhs._type;

                f =rhs.f;
                d = rhs.d;
                i = rhs.i;
                s = rhs.s;
            }

        OSC_Value ( )
            {
                _type = 0;

                f = 0;
                d = 0;
                i = 0;
                s = 0;
            }

        virtual ~OSC_Value ( ) { }
        virtual char type ( void ) const { return _type; }
    };

    class OSC_Float : public OSC_Value
    {

    public:

        float value ( void ) const { return f; }

        OSC_Float ( float v )
            {
                _type = 'f';
                f = v;
            }
    };

    class OSC_Int : public OSC_Value
    {

    public:

        int value ( void ) const { return i; }

        OSC_Int ( int v )
            {
                _type = 'i';
                i = v;
            }
    };

    class OSC_String : public OSC_Value
    {
    public:

        const char * value ( void ) const { return s; }

        OSC_String ( const char *v )
            {
                _type = 's';
                s = v;
            }
    };

    class Endpoint
    {
        static void error_handler(int num, const char *msg, const char *path);


        static int osc_generic ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data );

//    Thread _thread;

//    lo_server_thread _st;
        lo_server _server;

        std::list<char*> _path_names;

    public:

        Endpoint ( const char *port = 0 );

        ~Endpoint ( );

        char *get_paths ( const char *prefix );
        void add_method ( const char *path, const char *typespec, lo_method_handler handler, void *user_data, const char *argument_description );
        void del_method ( const char *path, const char *typespec );
        void start ( void );
        void stop ( void );
        int port ( void ) const;
        char * url ( void ) const;

        void check ( void ) const;
        void wait ( int timeout ) const;
        void run ( void ) const;

        int send ( lo_address to, const char *path, std::list< OSC_Value > values );

        /* overloads for common message formats */
        int send ( lo_address to, const char *path );
        int send ( lo_address to, const char *path, float v );
        int send ( lo_address to, const char *path, double v );
        int send ( lo_address to, const char *path, int v );
        int send ( lo_address to, const char *path, long v );
        int send ( lo_address to, const char *path, const char *v );
        int send ( lo_address to, const char *path, const char *v1, const char *v2 );

    };

};


/* helper macros for defining OSC handlers */
#define OSC_NAME( name ) osc_ ## name
#define OSC_DMSG() DMESSAGE( "Got OSC message: %s", path );
#define OSC_HANDLER( name ) static int OSC_NAME( name ) ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )


#define OSC_REPLY_OK() ((Mixer*)user_data)->osc_endpoint->send( lo_message_get_source( msg ), "/reply", path, "ok" )
#define OSC_REPLY( msg_str ) ((Mixer*)user_data)->osc_endpoint->send( lo_message_get_source( msg ), "/reply", path, msg_str )
#define OSC_REPLY_ERR() ((Mixer*)user_data)->osc_endpoint->send( lo_message_get_source( msg ), "/reply", path, "err" )
