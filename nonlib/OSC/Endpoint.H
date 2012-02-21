
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
#include "Thread.H"
#include <list>
#include <stdlib.h>
#include <string.h>

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

    struct Parameter_Limits
    {
        float min;
        float max;
        float default_value;
    };

    class Endpoint;
    class Signal;
    struct Peer
    {
     
        bool _scanning;

        char *name;
        lo_address addr;

        std::list<Signal*> _signals;
    };

    struct Target
    {
        Peer *peer;
//        char *path;
        int signal_id;
        float value;
    };

    typedef int (*signal_handler) ( float value, void *user_data );

    class Signal
    {
        static int next_id;

    public:

        enum Direction {
            Input,
            Output,
            Bidirectional
        };

    private:

        Endpoint *_endpoint;

        int _id;

        char *_path;
        char *_documentation;
        
        float _value;

        std::list<Target*> _outgoing;

        Direction _direction;

        /* FIXME:
           In order to support signal mixing, the receiver must track
           each connected signal channel separately, and add the
           values together before invoking the handler. 
         */
        std::list<Target*> _incoming;

        signal_handler _handler;
        void *_user_data;
        
    public:

        Parameter_Limits _parameter_limits;

        Signal ( const char *path, Direction dir )
            { 
                _direction = dir;
                _path = strdup( path );
                _id = ++next_id;
                _value = 0.0f;
            }

        ~Signal ( )
            {
                free( _path );
            }

        bool connected ( void ) { return _outgoing.size() + _incoming.size(); }

        int id ( void ) { return _id; }

        Direction direction ( void ) { return _direction; }

        void parameter_limits ( float min, float max, float default_value )
            {
                _parameter_limits.min = min;
                _parameter_limits.max = max;
                _parameter_limits.default_value = default_value;
                _value = default_value;
            }

        Parameter_Limits& parameter_limits ( void ) { return _parameter_limits; }

        const char *path ( void ) { return _path; }

        /* publishes value to targets */
        void value ( float v );
        /* get current value */
        float value ( void ) { return _value; }

        friend class Endpoint;
    };

    class Method
    {
        char *_path;
        char *_typespec;
        char *_documentation;
        struct Parameter_Limits *_parameter_limits;

    public:

        const char *path ( void ) { return _path; }
        const char *typespec ( void ) { return _typespec; }

        Method ( );
        ~Method ( );

        friend class Endpoint;
    };


    class Endpoint
    {
        static void error_handler(int num, const char *msg, const char *path);


        static int osc_reply ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data );

        static int osc_signal_lister ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data );
        static int osc_generic ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data );
        static int osc_sig_handler ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data );

        Thread _thread;

//        lo_server_thread _st;
        lo_server _server;

        std::list<Peer*> _peers;
        std::list<Signal*> _signals;
        std::list<Method*> _methods;

        static void *osc_thread ( void *arg );
        void osc_thread ( void );
            
        OSC::Signal *find_signal_by_id ( int id );

        Peer *find_peer_by_name ( const char *name );
        Peer *find_peer_by_address ( lo_address addr );
        static bool address_matches ( lo_address addr1, lo_address addr2 );

        static Target *find_target_by_peer_address ( std::list<Target*> *l, lo_address addr );

    public:

        void list_peers ( void (*callback) (const char *, const char *, int, void * ), void *v );

        int init ( const char *port = 0 );
        Endpoint ( );

        ~Endpoint ( );

        bool connect_signal( OSC::Signal *s, const char *peer_name, int signal_id );


        Signal *add_signal ( const char *path, Signal::Direction dir, signal_handler handler, void *user_data );
        Method *add_method ( const char *path, const char *typespec, lo_method_handler handler, void *user_data, const char *argument_description );
        void del_method ( const char *path, const char *typespec );
        void del_method ( Method* method );
        void del_signal ( Signal *signal );
        void start ( void );
        void stop ( void );
        int port ( void ) const;
        char * url ( void ) const;

        void scan_peer ( const char *name, const char *url );

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
        int send ( lo_address to, const char *path, int v1, int v2 );
        int send ( lo_address to, const char *path, int v1, float v2 );
        int send ( lo_address to, const char *path, const char *v );
        int send ( lo_address to, const char *path, const char *v1, float v2 );
        int send ( lo_address to, const char *path, const char *v1, int v2, int v3 );
        int send ( lo_address to, const char *path, const char *v1, const char *v2 );
        int send ( lo_address to, const char *path, const char *v1, const char *v2, const char *v3 );
        int send ( lo_address to, const char *path, const char *v1, int v2, int v3, int v4 );
        int send ( lo_address to, const char *path, const char *v1, const char *v2, int v3, int v4, int v5 );
        
        int send ( lo_address to, const char *path, const char *v1, int v2 );
        int send ( lo_address to, const char *path, int v1, const char *v2 );

        int send ( lo_address to, const char *path, const char *v1, const char *v2, const char *v3, int v4, int v5, int v6 );
        int send ( lo_address to, const char *path, const char *v1, int v2, const char *v3 );
        int send ( lo_address to, const char *path, int v1, const char *v2, const char *v3, const char *v4 );
        int send ( lo_address to, const char *path, const char *v1, int v2, const char *v3, const char *v4, const char *v5 );
        int send ( lo_address to, const char *path, const char *v1, const char *v2, const char *v3, const char *v4, const char *v5 );
        int send ( lo_address to, const char *path, const char *v1, const char *v2, const char *v3, const char *v4 );

        int send ( lo_address to, const char *path, lo_message msg );

        int send ( lo_address to, const char *path, const char *v1, const char *v2, int v3, float v4, float v5, float v6 );

        // can be used to point back to owning object.
        void *owner;
    };

};

/* helper macros for defining OSC handlers */
#define OSC_NAME( name ) osc_ ## name
#define OSC_DMSG() DMESSAGE( "Got OSC message: %s", path );
#define OSC_HANDLER( name ) static int OSC_NAME( name ) ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )


#define OSC_REPLY_OK() ((OSC::Endpoint*)user_data)->send( lo_message_get_source( msg ), path, "ok" )
#define OSC_REPLY( value ) ((OSC::Endpoint*)user_data)->send( lo_message_get_source( msg ), path, value )
#define OSC_REPLY_ERR() ((OSC::Endpoint*)user_data)->send( lo_message_get_source( msg ), path, "err" )
#define OSC_ENDPOINT() ((OSC::Endpoint*)user_data)
