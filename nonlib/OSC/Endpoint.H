
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

#include <sigc++/sigc++.h>

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

        Peer *_peer;

        int _id;

        char *_path;
        char *_documentation;
        
        float _value;

        std::list<Signal*> _outgoing;
        std::list<Signal*> _incoming;

        Direction _direction;

        signal_handler _handler;
        void *_user_data;
        Parameter_Limits _parameter_limits;
        
    public:

        Signal ( const char *path, Direction dir )
            { 
                _direction = dir;
                _path = strdup( path );
                _id = ++next_id;
                _value = 0.0f;
                _endpoint = NULL;
                _peer = NULL;
            }

        sigc::signal <void,Signal*> signal_connection_state_changed;

        ~Signal ( );

        static Signal *get_peer_signal_by_id ( Peer *p, int signal_id );
        int noutput_connections() { return _outgoing.size(); }
        bool connected ( void ) const { return _outgoing.size() + _incoming.size(); }
       
        char * get_output_connection_peer_name_and_path ( int n );
     

        int id ( void ) const { return _id; }

        Direction direction ( void ) const { return _direction; }

        void parameter_limits ( float min, float max, float default_value )
            {
                _parameter_limits.min = min;
                _parameter_limits.max = max;
                _parameter_limits.default_value = default_value;
                _value = default_value;
            }

        const Parameter_Limits& parameter_limits ( void ) const { return _parameter_limits; }

        const char *path ( void ) const { return _path; }

        void rename ( const char *name );

        /* publishes value to targets */
        void value ( float v );
        /* get current value */
        float value ( void ) const { return _value; }

        bool is_connected_to ( const Signal *s ) const;

        friend class Endpoint;
    };

    class Method
    {
        char *_path;
        char *_typespec;
        char *_documentation;

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
        static int osc_sig_renamed ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data );
        static int osc_sig_removed ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data );
        static int osc_sig_created ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data );
        static int osc_sig_disconnect ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data );
        static int osc_sig_connect ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data );
        static int osc_sig_hello ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data );

        void scan_peer ( const char *name, const char *url );

        Thread _thread;

//        lo_server_thread _st;
        lo_server _server;

        std::list<Peer*> _peers;
        std::list<Signal*> _signals;
        std::list<Method*> _methods;

        static void *osc_thread ( void *arg );
        void osc_thread ( void );
            
        OSC::Signal *find_signal_by_id ( int id );
        OSC::Signal *find_peer_signal_by_path ( Peer *p, const char *path );
        OSC::Signal *find_peer_signal_by_id ( Peer *p, int id );
        
        Peer *find_peer_by_name ( const char *name );
        Peer *find_peer_by_address ( lo_address addr );
        static bool address_matches ( lo_address addr1, lo_address addr2 );

        static Signal *find_target_by_peer_address ( std::list<Signal*> *l, lo_address addr );

        char *_name;

        void del_signal ( Signal *signal );
        void send_signal_rename_notifications( Signal *s );

    public:

        void list_peer_signals ( void (*callback) (const char *, const OSC::Signal *, void * ), void *v );

        int init ( int proto, const char *port = 0 );
        Endpoint ( );

        ~Endpoint ( );



    
        bool disconnect_signal ( OSC::Signal *s, OSC::Signal *d );
        bool disconnect_signal ( OSC::Signal *s, const char *peer_name, const char *signal_path );        
        bool connect_signal ( OSC::Signal *s, OSC::Signal *d );
        bool connect_signal ( OSC::Signal *s, const char *peer_name, const char *signal_path );        
        bool connect_signal ( OSC::Signal *s, const char *peer_name, int signal_id );
        bool connect_signal ( OSC::Signal *s, const char *peer_and_path );


        Signal * add_signal ( const char *path, Signal::Direction dir, float min, float max, float default_value, signal_handler handler, void *user_data );
        Method *add_method ( const char *path, const char *typespec, lo_method_handler handler, void *user_data, const char *argument_description );
        void del_method ( const char *path, const char *typespec );
        void del_method ( Method* method );
        void start ( void );
        void stop ( void );
        int port ( void ) const;
        char * url ( void ) const;

        void check ( void ) const;
        void wait ( int timeout ) const;
        void run ( void ) const;

        void name ( const char *name ) { _name = strdup( name ); }
        const char *name ( void ) { return _name; }

        void hello ( const char *url );

        int send ( lo_address to, const char *path, std::list< OSC_Value > values );

        /* overloads for common message formats */
        int send ( lo_address to, const char *path );
        int send ( lo_address to, const char *path, float v );
        int send ( lo_address to, const char *path, double v );
        int send ( lo_address to, const char *path, int v );
        int send ( lo_address to, const char *path, long v );
        int send ( lo_address to, const char *path, int v1, int v2 );
        int send ( lo_address to, const char *path, int v1, float v2 );
        int send ( lo_address to, const char *path, int v1, int v2, float v3 );
        int send ( lo_address to, const char *path, const char *v );
        int send ( lo_address to, const char *path, const char *v1, float v2 );
        int send ( lo_address to, const char *path, const char *v1, int v2, int v3 );
        int send ( lo_address to, const char *path, const char *v1, const char *v2 );
        int send ( lo_address to, const char *path, const char *v1, const char *v2, const char *v3 );
        int send ( lo_address to, const char *path, const char *v1, int v2, int v3, int v4 );
        int send ( lo_address to, const char *path, const char *v1, const char *v2, int v3, int v4, int v5 );
        
        int send ( lo_address to, const char *path, const char *v1, int v2 );
        int send ( lo_address to, const char *path, int v1, const char *v2 );
        int send ( lo_address to, const char *path, const char *v1, int v2, int v3, float v4 );

        int send ( lo_address to, const char *path, const char *v1, const char *v2, const char *v3, int v4, int v5, int v6 );
        int send ( lo_address to, const char *path, const char *v1, int v2, const char *v3 );
        int send ( lo_address to, const char *path, int v1, const char *v2, const char *v3, const char *v4 );
        int send ( lo_address to, const char *path, const char *v1, int v2, const char *v3, const char *v4, const char *v5 );
        int send ( lo_address to, const char *path, const char *v1, const char *v2, const char *v3, const char *v4, const char *v5 );
        int send ( lo_address to, const char *path, const char *v1, const char *v2, const char *v3, const char *v4 );

        int send ( lo_address to, const char *path, lo_message msg );

        int send ( lo_address to, const char *path, const char *v1, const char *v2, int v3, float v4, float v5, float v6 );

        int send ( lo_address to, const char *path, const char *v1, const char *v2, const char *v3, int v4, float v5, float v6, float v7 );

        // can be used to point back to owning object.
        void *owner;

        sigc::signal <void> signal_peer_scan_complete;

        friend Signal::~Signal();
        friend void Signal::rename ( const char *name );
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
