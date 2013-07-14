
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
#include <assert.h>

#include "Endpoint.H"

#include "Thread.H"

#pragma GCC diagnostic ignored "-Wunused-parameter"

namespace OSC
{

    /**********/
    /* Method */
    /**********/

    Method::Method ( )
    {
        _path = _typespec = _documentation = 0;
    }
    
    Method::~Method ( )
    {
        if ( _path )
            free( _path );
        if ( _typespec )
            free( _typespec );
    }

    /**********/
    /* Signal */
    /**********/

    Signal::Signal ( const char *path, Direction dir )
    { 
        _direction = dir;
        _path = NULL;
        if ( path )
            _path = strdup( path );
        _value = 0.0f;
        _endpoint = NULL;
        _peer = NULL;
        _documentation = 0;
        _user_data = 0;
        _connection_state_callback = 0;
        _connection_state_userdata = 0;
    }

    Signal::~Signal ( )
    {
        if ( _endpoint )
        {
            _endpoint->del_signal( this );
        }
                                              
        if ( _path )
            free( _path );
        _path = NULL;

        _endpoint = NULL;
    }

    void
    Signal::rename ( const char *path )
    {
        char *new_path;
        asprintf( &new_path, "%s%s", _endpoint->name(), path );

        DMESSAGE( "Renaming signal %s to %s", this->path(), new_path );

        if ( _direction == Signal::Input )
        {
            lo_server_del_method( _endpoint->_server, _path, NULL );
            lo_server_add_method( _endpoint->_server, new_path, NULL, _endpoint->osc_sig_handler, this );
        }

        for ( std::list<Peer*>::iterator i = _endpoint->_peers.begin(); 
              i != _endpoint->_peers.end();
              ++i )
        {
            _endpoint->send( (*i)->addr, "/signal/renamed", _path, new_path );
        }

        _endpoint->rename_translation_destination( _path, new_path );

        free( _path );
        _path = new_path;
    }

    void
    Signal::value ( float f )
    {
        if ( f == _value )
            return;
        
        _value = f;
        
        if ( direction() == Output )
        {
            for ( std::list<Peer*>::iterator i = _endpoint->_peers.begin(); 
                  i != _endpoint->_peers.end();
                  ++i )
            {
                _endpoint->send( (*i)->addr, 
                                 path(),
                                 f );
            }

            // free(s);   
        }
        /* else if ( direction() == Input ) */
        /* { */
        /*     DMESSAGE( "Sending value feedback for signal %s...", path() ); */
        /*     for ( std::list<Signal*>::iterator i = _incoming.begin(); */
        /*           i != _incoming.end(); */
        /*           ++i ) */
        /*     { */
        /*         DMESSAGE( "Sending value feedback to %s %s %f", lo_address_get_url( (*i)->_peer->addr), (*i)->path() , f); */
        /*         _endpoint->send( (*i)->_peer->addr,  */
        /*                          (*i)->path(), */
        /*                          f ); */
        /*     } */
        /* } */
    }

    /* char * */
    /* Signal::get_output_connection_peer_name_and_path ( int n ) */
    /* { */
    /*     Signal *t = NULL; */

    /*     int j = 0; */
    /*     for ( std::list<Signal*>::const_iterator i = _outgoing.begin(); */
    /*           i != _outgoing.end(); */
    /*           ++i, ++j ) */
    /*     { */
    /*         if ( j == n ) */
    /*         { */
    /*             t = *i; */
    /*             break; */
    /*         } */
    /*     } */

    /*     if ( t ) */
    /*     { */
    /*         char *r; */
    /*         asprintf( &r, "%s%s", t->_peer->name, t->path() ); */

    /*         return r; */
    /*     } */
    /*     else */
    /*         return NULL; */
    /* } */
    /*  */


    void
    Endpoint::error_handler(int num, const char *msg, const char *path)
    {
        WARNING( "LibLO server error %d in path %s: %s\n", num, path, msg);
    }

    Endpoint::Endpoint ( )
    {  
        _learning_path = NULL;
        _peer_signal_notification_callback = 0;
        _peer_signal_notification_userdata = 0;
        _peer_scan_complete_callback = 0;
        _peer_scan_complete_userdata = 0;
        _server = 0;
        _name = 0;
        owner = 0;
    }

    int
    Endpoint::init ( int proto, const char *port )
    {
        DMESSAGE( "Creating OSC server" );

        _server = lo_server_new_with_proto( port, proto, error_handler );

        char *url = lo_server_get_url( _server );
        _addr = lo_address_new_from_url( url );
        free( url );
        
        if ( ! _server )
        {
            WARNING( "Error creating OSC server" );
            return -1;
        }

        add_method( "/signal/hello", "ss", &Endpoint::osc_sig_hello, this, "" );
        add_method( "/signal/connect", "ss", &Endpoint::osc_sig_connect, this, "" );
        add_method( "/signal/disconnect", "ss", &Endpoint::osc_sig_disconnect, this, "" );
        add_method( "/signal/renamed", "ss", &Endpoint::osc_sig_renamed, this, "" );
        add_method( "/signal/removed", "s", &Endpoint::osc_sig_removed, this, "" );
        add_method( "/signal/created", "ssfff", &Endpoint::osc_sig_created, this, "" );
        add_method( "/signal/list", NULL, &Endpoint::osc_signal_lister, this, "" );
        add_method( "/reply", NULL, &Endpoint::osc_reply, this, "" );
        add_method( NULL, NULL, &Endpoint::osc_generic, this, "" );

        return 0;
    }


    Endpoint::~Endpoint ( )
    {
//    lo_server_thread_free( _st );
        if ( _server )
        {
            lo_server_free( _server );
            _server = 0;
        }
    }

    OSC::Signal *
    Endpoint::find_target_by_peer_address ( std::list<Signal*> *l, lo_address addr )
    {

        for ( std::list<Signal*>::iterator i = l->begin();
              i != l->end();
              ++i )
        {
            if ( address_matches( addr, (*i)->_peer->addr ) )
            {
                return *i;
            }
        }

        return NULL;
    }
  

    OSC::Signal *
    Endpoint::find_peer_signal_by_path ( Peer *p, const char *path )
    {
        for ( std::list<Signal*>::iterator i = p->_signals.begin();
              i != p->_signals.end();
              ++i )
        {
            if ( !strcmp( (*i)->path(), path ) )
                return *i;
        }

        return NULL;
    }
   
    OSC::Signal *
    Endpoint::find_signal_by_path ( const char *path )
    {
        for ( std::list<Signal*>::iterator i = _signals.begin();
              i != _signals.end();
              ++i )
        {
            if ( !strcmp( (*i)->path(), path ) )
                return *i;
        }

        return NULL;
    }

    void
    Endpoint::hello ( const char *url )
    {
        assert( name() );

        lo_address addr = lo_address_new_from_url ( url );

        char *our_url = this->url();
        send( addr, "/signal/hello", name(), our_url );
        free( our_url );
        
        lo_address_free( addr );
    }

    void
    Endpoint::handle_hello ( const char *peer_name, const char *peer_url )
    {
        DMESSAGE( "Got hello from %s", peer_name );

        Peer *p = find_peer_by_name( peer_name ); 
      
        if ( ! p )
        {
            scan_peer( peer_name, peer_url );
        }
        else
        {
            /* maybe the peer has a new URL */
        
            /* update address */
            lo_address addr = lo_address_new_from_url( peer_url );

            if ( address_matches( addr, p->addr ) )
            {
                free( addr );
                return;
            }
            
            if ( p->addr )
                free( p->addr );

            p->addr = addr;

            /* scan it while we're at it */
            p->_scanning = true;
            
            DMESSAGE( "Scanning peer %s", peer_name );

            send( p->addr, "/signal/list" );
        }
         
        if ( name() )
        {
            hello( peer_url );
        }
        else
        {
            DMESSAGE( "Not sending hello because we don't have a name yet!" );
        }
    }

    int
    Endpoint::osc_sig_hello ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
    {
        Endpoint *ep = (Endpoint*)user_data;

        const char *peer_name = &argv[0]->s;
        const char *peer_url = &argv[1]->s;
        
        ep->handle_hello( peer_name, peer_url );

        return 0;
    }

    int
    Endpoint::osc_sig_disconnect ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
    {
        const char *their_name = &argv[0]->s;
        const char *our_name = &argv[1]->s;
     
        Endpoint *ep = (Endpoint*)user_data;

        Signal *s = ep->find_signal_by_path( our_name );

        if ( ! s )
            return 0;

        if ( s->_direction == Signal::Input )
        {
            DMESSAGE( "Peer %s has disconnected from signal %s", our_name, their_name );
 
            ep->del_translation( their_name );

            if ( s->_connection_state_callback )
                s->_connection_state_callback( s, s->_connection_state_userdata );

            return 0;
        }

        return 0;
    }

    int
    Endpoint::osc_sig_connect ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
    {
        const char *src_path = &argv[0]->s;
        const char *dst_path = &argv[1]->s;
     
        Endpoint *ep = (Endpoint*)user_data;

        Signal *dst_s = ep->find_signal_by_path( dst_path );

        if ( ! dst_s )
        {
            WARNING( "Unknown destination signal in connection attempt: \"%s\"", dst_path );
            return 0;
        }

        if ( dst_s->_endpoint != ep )
        {
            WARNING( "Got connection request for a destination signal we don't own" );
            return 0;
        }

        DMESSAGE( "Has requested signal connection %s |> %s", src_path, dst_s->path() );

        ep->add_translation( src_path, dst_s->path() );

        /* if ( dst_s->_connection_state_callback ) */
        /*     dst_s->_connection_state_callback( dst_s, dst_s->_connection_state_userdata ); */

        return 0;
    }

    int
    Endpoint::osc_sig_removed ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
    {
        const char *name = &argv[0]->s;
        
        Endpoint *ep = (Endpoint*)user_data;
        
        Signal *o = ep->find_signal_by_path( name );

        if ( ! o )
        {
            WARNING( "Unknown signal: %s", name  );
            return 0;
        }

        DMESSAGE( "Signal %s:%s was removed", o->_peer->name, o->path() );


        if ( ep->_peer_signal_notification_callback )
            ep->_peer_signal_notification_callback( o, Signal::Removed, ep->_peer_signal_notification_userdata );
        
        ep->_signals.remove( o );

        delete o;

        return 0;
    }

    int
    Endpoint::osc_sig_created ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
    {
        Endpoint *ep = (Endpoint*)user_data;
        
        const char *name = &argv[0]->s;
        const char *direction = &argv[1]->s;
        const float min = argv[2]->f;
        const float max = argv[3]->f;
        const float default_value = argv[4]->f;

        Peer *p = ep->find_peer_by_address( lo_message_get_source( msg ) );

        if ( ! p )
        {
            WARNING( "Got signal creation notification from unknown peer" );
            return 0;
        }
       
        Signal::Direction dir = Signal::Input;
        
        if ( !strcmp( direction, "in" ) )
            dir = Signal::Input;
        else if ( !strcmp( direction, "out" ) )
            dir = Signal::Output;
        
        Signal *s = new Signal( name, dir );
        
        s->_peer = p;
        s->parameter_limits( min, max, default_value );
        
        p->_signals.push_back( s );

        DMESSAGE( "Peer %s has created signal %s (%s %f %f %f)", p->name, 
                  name, direction, min, max, default_value );
       
        if ( ep->_peer_signal_notification_callback )
            ep->_peer_signal_notification_callback( s, Signal::Created, ep->_peer_signal_notification_userdata );
        
        return 0;
    }

    int
    Endpoint::osc_sig_renamed ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
    {
        DMESSAGE( "Got renamed message." );

        const char *old_name = &argv[0]->s;
        const char *new_name = &argv[1]->s;

        Endpoint *ep = (Endpoint*)user_data;

        Peer *p = ep->find_peer_by_address( lo_message_get_source( msg ) );
        
        if ( ! p )
        {
            WARNING( "Got signal rename notification from unknown peer." );
            return 0;
        }

        Signal *o = ep->find_peer_signal_by_path( p, old_name );

        if ( ! o )
        {
            WARNING( "Unknown signal: %s", old_name );
            return 0;
        }

        DMESSAGE( "Signal %s was renamed to %s", o->_path, new_name );
        
        ep->rename_translation_source( o->_path, new_name );

        free( o->_path );
        o->_path = strdup( new_name );

        return 0;
    }

    int
    Endpoint::osc_sig_handler ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
    {
        Signal *o;
        float f = 0.0;

        if ( ! strcmp( types, "f" ) )
        {
            /* accept a value for signal named in path */
            o = (Signal*)user_data;
            f = argv[0]->f;
        }
        else if ( ! types || 0 == types[0] )
        {
            /* reply with current value */
            o = (Signal*)user_data;
            o->_endpoint->send( lo_message_get_source( msg ), "/reply", path, o->value() );
            return 0;
        }
        else
        {
            return -1;
        }

        o->_value = f;

        if ( o->_handler )
            o->_handler( f, o->_user_data );
        
        return 1;
    }

    const char**
    Endpoint::get_connections ( const char *path )
    {
        const char **  conn = NULL;

        int j = 0;
        for ( std::map<std::string,TranslationDestination>::iterator i = _translations.begin();
              i != _translations.end();
              i++ )
        {
            if ( !strcmp( i->second.path.c_str(), path ) )
            {
                conn = (const char**)realloc( conn, sizeof( char * ) * (j+2));
                conn[j++] = i->first.c_str();
            }
        }
        
        if ( conn )
            conn[j] = 0;

        return conn;
    }

    void
    Endpoint::clear_translations ( void )
    {
        _translations.clear();
    }

    void
    Endpoint::add_translation ( const char *a, const char *b )
    {
        _translations[a].path = b;
    }

    void
    Endpoint::del_translation ( const char *a )
    {
        std::map<std::string,TranslationDestination>::iterator i = _translations.find( a );

        if ( i != _translations.end() )
            _translations.erase( i );
    }

    void
    Endpoint::rename_translation_destination ( const char *a, const char *b )
    {
        
        for ( std::map<std::string,TranslationDestination>::iterator i = _translations.begin();
              i != _translations.end();
              i++ )
        {
            if ( !strcmp( i->second.path.c_str(), a ) )
            {
                i->second.path = b;
            }
        }
    }

    void
    Endpoint::rename_translation_source ( const char *a, const char *b )
    {
        std::map<std::string,TranslationDestination>::iterator i = _translations.find( a );

        if ( i != _translations.end() )
        {
            _translations[b] = _translations[a];

            _translations.erase( i );
        }
    }

    int
    Endpoint::ntranslations ( void )
    {
        return _translations.size();
    }

    bool
    Endpoint::get_translation ( int n, const char **from, const char **to )
    {
        int j = 0;
        for ( std::map<std::string,TranslationDestination>::const_iterator i = _translations.begin();
              i != _translations.end();
              i++, j++)
        {
            if ( j == n )
            {
                *from = i->first.c_str();
                *to = i->second.path.c_str();
                return true;
            }
        }

        return false;
    }

    int
    Endpoint::osc_generic ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
    {
//        OSC_DMSG();
        Endpoint *ep = (Endpoint*)user_data;

        if ( ep->_learning_path )
        {
            ep->add_translation( path, ep->_learning_path );

            DMESSAGE( "Learned translation \"%s\" -> \"%s\"", path, ep->_learning_path );
            
            free(ep->_learning_path);
            ep->_learning_path = NULL;
            
            return 0;
        }

        {
            std::map<std::string,TranslationDestination>::iterator i = ep->_translations.find( path );
            
            if ( i != ep->_translations.end() )
            {
                const char *dpath = i->second.path.c_str();

//                DMESSAGE( "Translating message \"%s\" to \"%s\"", path, dpath );

                if ( !strcmp(types, "f" ))
                {
//                    DMESSAGE( "recording value %f", argv[0]->f );
                    i->second.current_value = argv[0]->f;
                }

                i->second.suppress_feedback = true;

                lo_send_message(ep->_addr, dpath, msg );
                return 0;
            }
        }

        if ( argc || path[ strlen(path) - 1 ] != '/' )
            return -1;

        for ( std::list<Method*>::const_iterator i = ep->_methods.begin(); i != ep->_methods.end(); ++i )
        {
            if ( ! (*i)->path() )
                continue;

            if (! strncmp( (*i)->path(), path, strlen(path) ) )
            {
                /* asprintf( &stored_path, "%s (%s); %s", path, typespec, argument_description ); */

                ((Endpoint*)user_data)->send( lo_message_get_source( msg ), "/reply", path, (*i)->path() );
            }
        }

        ((Endpoint*)user_data)->send( lo_message_get_source( msg ), "/reply", path );

        return 0;
    }

    int
    Endpoint::osc_signal_lister ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
    {
//        OSC_DMSG();
        
        DMESSAGE( "Listing signals." );

        const char *prefix = NULL;

        if ( argc )
            prefix = &argv[0]->s;

        Endpoint *ep = (Endpoint*)user_data;

        for ( std::list<Signal*>::const_iterator i = ep->_signals.begin(); i != ep->_signals.end(); ++i )
        {
            Signal *o = *i;

            if ( ! prefix || ! strncmp( o->path(), prefix, strlen(prefix) ) )
            {
                ep->send( lo_message_get_source( msg ),
                          "/reply", 
                          path,
                          o->path(),
                          o->_direction == Signal::Input ? "in" : "out",
                          o->parameter_limits().min,
                          o->parameter_limits().max,
                          o->parameter_limits().default_value
                    );
            }
        }
        
        ep->send( lo_message_get_source( msg ), "/reply", path );

        return 0;
    }

    bool
    Endpoint::address_matches ( lo_address addr1, lo_address addr2 )
    {
        char *purl = strdup( lo_address_get_port( addr1 ) );
        char *url = strdup( lo_address_get_port( addr2 ) );

        bool r = !strcmp( purl, url );

        free( purl );
        free( url );
        
        return r;
    }


    void
    Endpoint::list_peer_signals ( void *v )
    {
        for ( std::list<Peer*>::iterator i = _peers.begin(); 
              i != _peers.end();
              ++i )
        {
            for ( std::list<Signal*>::iterator j = (*i)->_signals.begin(); 
                  j != (*i)->_signals.end();
                  ++j )
            {
                if ( _peer_signal_notification_callback )
                    _peer_signal_notification_callback( *j, OSC::Signal::Created, v );
            }
        }
    }

    Peer *
    Endpoint::find_peer_by_address ( lo_address addr )
    {
        char *url = strdup( lo_address_get_port( addr ) );

        Peer *p = NULL;

        for ( std::list<Peer*>::iterator i = _peers.begin(); 
              i != _peers.end();
              ++i )
        {
            char *purl = strdup( lo_address_get_port( (*i)->addr ) );

            if ( !strcmp( purl, url ) )
            {
                free( purl );
                p = *i;
                break;
            }
            free(purl);
        }

        free( url );

        return p;
    }

    Peer *
    Endpoint::find_peer_by_name ( const char *name )
    {
        for ( std::list<Peer*>::iterator i = _peers.begin(); 
              i != _peers.end();
              ++i )
        {
            if ( !strcmp( name, (*i)->name ) )
            {
                return *i;
            }
        }

        return NULL;
    }

    bool
    Endpoint::disconnect_signal ( OSC::Signal *s, const char *signal_path )
    {
        if ( s->_direction == Signal::Output )
        {
            for ( std::list<Peer*>::iterator i = _peers.begin(); 
                  i != _peers.end();
                  ++i )
            {
                send( (*i)->addr, "/signal/disconnect",
                      s->path(),
                      signal_path);
            }
                
            return true;
        }

        return false;
    }
  
    bool
    Endpoint::connect_signal( OSC::Signal *s, const char *signal_path )
    {
        if ( s->_direction == Signal::Output )
        {
            for ( std::list<Peer*>::iterator i = _peers.begin();
                  i != _peers.end();
                  i++ )
            {
            
                send( (*i)->addr, "/signal/connect",
                      s->path(),
                      signal_path );
            }
        }
        
        return true;
    }

    int
    Endpoint::osc_reply ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
        
    {
        Endpoint *ep = (Endpoint*)user_data;

        if ( argc && !strcmp( &argv[0]->s, "/signal/list" ) )
        {
            Peer *p = ep->find_peer_by_address( lo_message_get_source( msg ) );

            if ( ! p )
            {
                WARNING( "Got input list reply from unknown peer." );
                return 0;
            }

            if ( argc == 1 )
            {
                p->_scanning = false;
                DMESSAGE( "Done scanning %s", p->name );

                if ( ep->_peer_scan_complete_callback )
                    ep->_peer_scan_complete_callback(ep->_peer_scan_complete_userdata);
            }
            else if ( argc == 6 && p->_scanning )
            {
                Signal *s = ep->find_peer_signal_by_path( p, &argv[1]->s );

                if ( s )
                    return 0;

                DMESSAGE( "Peer %s has signal %s (%s)", p->name, &argv[1]->s, &argv[2]->s );

                int dir = 0;

                if ( !strcmp( &argv[2]->s, "in" ) )
                    dir = Signal::Input;
                else if ( !strcmp( &argv[2]->s, "out" ) )
                    dir = Signal::Output;

                
                s = new Signal( &argv[1]->s, (Signal::Direction)dir );
                
                s->_peer = p;
              
                s->parameter_limits( argv[3]->f, argv[4]->f, argv[5]->f );

                p->_signals.push_back( s );

                //         ep->_signals.push_back(s);

                if ( ep->_peer_signal_notification_callback )
                    ep->_peer_signal_notification_callback( s, Signal::Created, ep->_peer_signal_notification_userdata );
            }

            return 0;
        }
        else
            return -1;
    }

    Method *
    Endpoint::add_method ( const char *path, const char *typespec, lo_method_handler handler, void *user_data, const char *argument_description )
    {
//	DMESSAGE( "Added OSC method %s (%s)", path, typespec );
 
        lo_server_add_method( _server, path, typespec, handler, user_data );

        Method *md = new Method;
        
        if ( path )
            md->_path = strdup( path );
        if ( typespec )
            md->_typespec = strdup( typespec );
        if ( argument_description )
            md->_documentation = strdup( argument_description );
        
        _methods.push_back( md );

        return md;
    }

    Signal *
    Endpoint::add_signal ( const char *path, Signal::Direction dir, float min, float max, float default_value, signal_handler handler, void *user_data )
    {
        Signal *o = new Signal( path, dir );
        
        char *s;
        asprintf( &s, "%s%s", name(), path );

        if ( s )
            o->_path = s;
        
        o->_handler = handler;
        o->_user_data = user_data;
        o->_endpoint = this;

        o->parameter_limits( min, max, default_value );
        
        _signals.push_back( o );
        
        /* if ( dir == Signal::Input ) */
        /* { */
            lo_server_add_method( _server, o->_path, NULL, osc_sig_handler, o );
        /* } */

        /* tell our peers about it */
        for ( std::list<Peer*>::iterator i = _peers.begin();
              i != _peers.end();
              ++i )
        {
            send( (*i)->addr,
                  "/signal/created",
                  o->path(),
                  o->_direction == Signal::Input ? "in" : "out",
                  min,
                  max,
                  default_value
                );
        }

        return o;
    }

    void
    Endpoint::del_method ( const char *path, const char *typespec )
    {
//	DMESSAGE( "Deleted OSC method %s (%s)", path, typespec );

        lo_server_del_method( _server, path, typespec );

        for ( std::list<Method *>::iterator i = _methods.begin(); i != _methods.end(); ++i )
        {
            if ( ! (*i)->path() )
                continue;

            if ( ! strcmp( path, (*i)->path() ) &&
                 ! strcmp( typespec, (*i)->typespec() ) )
            {
                delete *i;
                i = _methods.erase( i );

                break;
            }
        }
    }

    void
    Endpoint::del_method ( Method *meth )
    {
//	DMESSAGE( "Deleted OSC method %s (%s)", path, typespec );

        lo_server_del_method( _server, meth->path(), meth->typespec() );

        delete meth;

        _methods.remove( meth );
    }

    void
    Endpoint::del_signal ( Signal *o )
    {
//	DMESSAGE( "Deleted OSC method %s (%s)", path, typespec );

        lo_server_del_method( _server, o->path(), "f" );

        /* tell our peers about it */
        for ( std::list<Peer*>::iterator i = _peers.begin();
              i != _peers.end();
              ++i )
        {
            send( (*i)->addr,
                  "/signal/removed", 
                  o->path() );
        }

        /* FIXME: clear loopback connections first! */

        _signals.remove( o );
    }

    /* prepare to learn a translation for /path/. The next unhandled message to come through will be mapped to /path/ */
    void
    Endpoint::learn ( const char *path )
    {
        if ( _learning_path )
            free( _learning_path );

        _learning_path = NULL;

        if ( path )
            _learning_path = strdup( path );
    }

    /** if there's a translation with a destination of 'path', then send feedback for it */
    void
    Endpoint::send_feedback ( const char *path, float v )
    {
        for ( std::map<std::string,TranslationDestination>::iterator i = _translations.begin();
              i != _translations.end();
              i++ )
        {
            if ( ! strcmp( i->second.path.c_str(), path ) )
            {
                /* found it */
                if ( !i->second.suppress_feedback && i->second.current_value != v )
                {
                    const char *spath = i->first.c_str();

//                    DMESSAGE( "Sending feedback to \"%s\": %f", spath, v );

                    /* send to all peers */
                    for ( std::list<Peer*>::iterator p = _peers.begin(); 
                          p != _peers.end();
                          ++p )
                    {
                        send( (*p)->addr, spath, v );
                    }

                    i->second.current_value = v;
                }

                i->second.suppress_feedback = false;

                /* break; */

            }
        }
    }

    Peer *   
    Endpoint::add_peer ( const char *name, const char *url )
    {
        Peer *p = new Peer;

        DMESSAGE( "Adding peer %s @ %s...", name, url );

        p->name = strdup( name );
        p->addr = lo_address_new_from_url( url );
    
        _peers.push_back( p );
        
        return p;
    }

    void
    Endpoint::scan_peer ( const char *name, const char *url )
    {
        Peer *p = add_peer(name,url);

        p->_scanning = true;

        DMESSAGE( "Scanning peer %s", name );

        send( p->addr, "/signal/list" );
    }

    void *
    Endpoint::osc_thread ( void * arg )
    {
        ((Endpoint*)arg)->osc_thread();

        return NULL;
    }

    void
    Endpoint::osc_thread ( void )
    {
        _thread.name( "OSC" );

        DMESSAGE( "OSC Thread running" );
        
        run();
    }

    void
    Endpoint::start ( void )
    {

        if ( !_thread.clone( &Endpoint::osc_thread, this ) )
            FATAL( "Could not create OSC thread" );

/*      lo_server_thread_start( _st ); */

    }

    void
    Endpoint::stop ( void )
    {
        _thread.join();
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

/** Process any waiting events and return after timeout */
    void
    Endpoint::wait ( int timeout ) const
    {
        if ( lo_server_wait( _server, timeout ) )
            while ( lo_server_recv_noblock( _server, 0 ) ) { }
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
//                    DMESSAGE( "Adding float %f", ((OSC_Float*)ov)->value() );
                    lo_message_add_float( m, ((OSC_Float*)ov)->value() );
                    break;
                case 'i':
//                    DMESSAGE( "Adding int %i", ((OSC_Int*)ov)->value() );
                    lo_message_add_int32( m, ((OSC_Int*)ov)->value() );
                    break;
                case 's':
//                    DMESSAGE( "Adding string %s", ((OSC_String*)ov)->value() );
                    lo_message_add_string( m, ((OSC_String*)ov)->value() );
                    break;
                default:
                    FATAL( "Unknown format: %c", ov->type() );
                    break;
            }
        }

//        DMESSAGE( "Path: %s", path );

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
    Endpoint::send ( lo_address to, const char *path, const char * v1, float v2 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "sf", v1, v2 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, const char * v1, const char *v2 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "ss", v1, v2 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, const char * v1, const char *v2, const char *v3 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "sss", v1, v2, v3 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, const char *v1, int v2, int v3, int v4 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "siii", v1, v2, v3, v4 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, const char *v1, const char *v2, int v3, int v4, int v5 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "ssiii", v1, v2, v3, v4, v5 );
    }


    int
    Endpoint::send ( lo_address to, const char *path, const char *v1, const char *v2, const char *v3, int v4, int v5, int v6 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "sssiii", v1, v2, v3, v4, v5, v6 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, const char *v1, int v2 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "si", v1, v2 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, int v1, const char *v2 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "is", v1, v2 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, const char *v1, int v2, const char *v3 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "sis", v1, v2, v3 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, int v1, const char *v2, const char *v3, const char *v4 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "isss", v1, v2, v3, v4 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, const char *v1, int v2, const char *v3, const char *v4, const char *v5 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "sisss", v1, v2, v3, v4, v5 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, const char *v1, const char *v2, const char *v3, const char *v4, const char *v5 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "sssss", v1, v2, v3, v4, v5 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, const char *v1, const char *v2, const char *v3, const char *v4 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "ssss", v1, v2, v3, v4 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, lo_message msg )
    {
        return lo_send_message_from( to, _server, path, msg );
    }

    int
    Endpoint::send ( lo_address to, const char *path, const char *v1, const char *v2, int v3, float v4, float v5, float v6 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "ssifff", v1, v2, v3, v4, v5, v6 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, const char *v1, const char *v2, const char *v3, int v4, float v5, float v6, float v7 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "sssifff", v1, v2, v3, v4, v5, v6, v7 );
    }
  
    int
    Endpoint::send ( lo_address to, const char *path, const char *v1, const char *v2, const char *v3, float v4, float v5, float v6 )
    { 
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "sssfff", v1, v2, v3, v4, v5, v6 );
    }
    
    int
    Endpoint::send ( lo_address to, const char *path, const char *v1, int v2, int v3 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "sii", v1, v2, v3 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, int v1, int v2 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "ii", v1, v2 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, int v1, float v2 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "if", v1, v2 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, const char *v1, int v2, int v3, float v4 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "siif", v1, v2, v3, v4 );
    }

    int
    Endpoint::send ( lo_address to, const char *path, int v1, int v2, float v3 )
    {
        return lo_send_from( to, _server, LO_TT_IMMEDIATE, path, "iif", v1, v2, v3 );
    }
}
