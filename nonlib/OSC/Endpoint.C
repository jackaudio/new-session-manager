
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

    int Signal::next_id = 0;
    
    void
    Signal::value ( float f )
    {
        for ( std::list<Target*>::const_iterator i = _outgoing.begin();
              i != _outgoing.end();
              ++i )
        {
//            DMESSAGE( "Sending signal value %i %f", (*i)->signal_id, f );

            if ( (*i)->value != f )
            {
                (*i)->value = f;

                _endpoint->send( (*i)->peer->addr, 
                                 "/signal/change",
                                 (*i)->signal_id,
                                 f );
            }
        }
    }

    void
    Signal::get_connected_peer_name_and_path ( char **peer_name, char **path )
    {
        Target *t = _outgoing.back();

        Signal *s = get_peer_signal_by_id( t->peer, t->signal_id );

        *peer_name = strdup( t->peer->name );
        *path = strdup( s->path() );
    }

    void
    Endpoint::error_handler(int num, const char *msg, const char *path)
    {
        WARNING( "LibLO server error %d in path %s: %s\n", num, path, msg);
    }

    Endpoint::Endpoint ( )
    {
    }

    int
    Endpoint::init ( int proto, const char *port )
    {
        DMESSAGE( "Creating OSC server" );

        _server = lo_server_new_with_proto( port, proto, error_handler );

        if ( ! _server )
        {
            WARNING( "Error creating OSC server" );
            return -1;
        }

        add_method( "/signal/change", "if", &Endpoint::osc_sig_handler, this, "" );
        add_method( NULL, "", &Endpoint::osc_generic, this, "" );
        add_method( NULL, NULL, &Endpoint::osc_signal_lister, this, "" );
        add_method( "/reply", NULL, &Endpoint::osc_reply, this, "" );

        return 0;
    }


    Endpoint::~Endpoint ( )
    {
//    lo_server_thread_free( _st );
        lo_server_free( _server );
    }

    OSC::Target *
    Endpoint::find_target_by_peer_address ( std::list<Target*> *l, lo_address addr )
    {

        for ( std::list<Target*>::iterator i = l->begin();
              i != l->end();
              ++i )
        {
            if ( address_matches( addr, (*i)->peer->addr ) )
            {
                return *i;
            }
        }

        return NULL;
    }

    OSC::Signal *
    Endpoint::find_signal_by_id ( int id )
    {
        for ( std::list<Signal*>::iterator i = _signals.begin();
              i != _signals.end();
              ++i )
        {
            if ( (*i)->id() == id )
                return *i;
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

    int
    Endpoint::osc_sig_handler ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
    {
        Signal *o;
        float f = 0.0;
       
        if ( !strcmp( path, "/signal/change" ) && !strcmp( types, "if" ) )
        {
            /* accept a value for numbered signal */
            int id = argv[0]->i;
            f = argv[1]->f;
            o = ((Endpoint*)user_data)->find_signal_by_id( id );
            if ( ! o )
            {
                WARNING( "Unknown signal id %i", id );
                return 0;
            }
        }
        else if ( ! strcmp( types, "f" ) )
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
        }
        else
        {
            return -1;
        }

        Target *t = NULL;

        if ( 0 == o->_incoming.size() || 
             ! ( t = find_target_by_peer_address( &o->_incoming, lo_message_get_source( msg ) ) ) )
        {
            /* message came from an unconnected peer, just set the value exactly */
        }
        else
        {
            /* message is from a connected source, do mixing. */

            t->value = f;

            f = 0.0;

            for ( std::list<Target*>::const_iterator i = o->_incoming.begin();
                  i != o->_incoming.end();
                  ++i )
            {
                f += (*i)->value;
            }
        }

        o->_value = f;

        o->_handler( f, o->_user_data );
        
        return 0;
    }

    int
    Endpoint::osc_generic ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
    {
//        OSC_DMSG();

        if ( argc || path[ strlen(path) - 1 ] != '/' )
            return -1;

        Endpoint *ep = (Endpoint*)user_data;

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

        Signal::Direction dir;

        if ( ! strcmp( path, "/signal/list_inputs" ) )
        {
            dir = Signal::Input;
        }
        else if ( ! strcmp( path, "/signal/list_outputs" ) )
        {
            dir = Signal::Output;
        }
        else
            return -1;

        const char *prefix = NULL;

        if ( argc )
            prefix = &argv[0]->s;

        Endpoint *ep = (Endpoint*)user_data;

        for ( std::list<Signal*>::const_iterator i = ep->_signals.begin(); i != ep->_signals.end(); ++i )
        {
            Signal *o = *i;

            if ( dir == Signal::Bidirectional ||
                 dir == o->_direction )
            {
                if ( ! prefix || ! strncmp( o->path(), prefix, strlen(prefix) ) )
                {
                    ep->send( lo_message_get_source( msg ),
                              "/reply", 
                              path,
                              o->path(),
                              o->id(),
                              o->parameter_limits().min,
                              o->parameter_limits().max,
                              o->parameter_limits().default_value
                        );
                }
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
    Endpoint::list_peers ( void (*callback) (const char *, const char *, int, void * ), void *v )
    {
        for ( std::list<Peer*>::iterator i = _peers.begin(); 
              i != _peers.end();
              ++i )
        {
            for ( std::list<Signal*>::iterator j = (*i)->_signals.begin(); 
                  j != (*i)->_signals.end();
                  ++j )
            {
//                DMESSAGE( "Running callback" );
                callback( (*i)->name, (*j)->path(), (*j)->id(), v );
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
    Endpoint::connect_signal( OSC::Signal *s, const char *peer_name, const char *signal_path )
    {
        if ( s->_direction == Signal::Output )
        {
            Peer *p = find_peer_by_name( peer_name );
            
            if ( ! p )
                return false;

            Signal *ps = find_peer_signal_by_path( p, signal_path );
            
            if ( ! ps )
                return false;

            MESSAGE( "Connecting signal output \"%s\" to %s:%i", s->path(), peer_name, s->_id );

            if ( p )
            {
                Target *t = new Target();

                t->peer = p;
                t->signal_id = ps->_id;
                
                s->_outgoing.push_back( t );
                
                send( p->addr, "/signal/connect", 
                      s->_id, /* our signal id */
                      t->signal_id /* their signal id */ );
                
                return true;
            }
        }

        return false;
    }

    bool
    Endpoint::connect_signal( OSC::Signal *s, const char *peer_name, int signal_id )
    {
        if ( s->_direction == Signal::Output )
        {
            Peer *p = find_peer_by_name( peer_name );

            if ( !p )
                return false;
            
            MESSAGE( "Connecting signal output \"%s\" to %s:%i", s->path(), peer_name, signal_id );

            if ( p )
            {
                Target *t = new Target();
                
                t->peer = p;
                t->signal_id = signal_id;
                
                s->_outgoing.push_back( t );
                
                send( p->addr, "/signal/connect", 
                      s->_id, /* our signal id */
                      t->signal_id /* their signal id */ );
                
                return true;
            }
        }            
    
        return false;
    }

    Signal *
    Signal::get_peer_signal_by_id ( Peer *p, int signal_id )
    {
        for ( std::list<Signal *>::iterator i = p->_signals.begin();
              i != p->_signals.end();
              ++i )
        {
            if ( (*i)->_id == signal_id )
                return *i;
        }
        
        return NULL;
    }

    int
    Endpoint::osc_reply ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
        
    {
        Endpoint *ep = (Endpoint*)user_data;

        if ( argc && !strcmp( &argv[0]->s, "/signal/list_inputs" ) )
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
            }
            else if ( p->_scanning )
            {
                DMESSAGE( "Peer %s has signal %s", p->name, &argv[1]->s );

                Signal *s = new Signal( &argv[1]->s, Signal::Input );
                
                s->_id = argv[2]->i;
                s->parameter_limits().min = argv[3]->f;
                s->parameter_limits().max = argv[4]->f;
                s->parameter_limits().default_value = argv[4]->f;

                p->_signals.push_back( s );
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
        
        if ( typespec )
            md->_parameter_limits = new Parameter_Limits[strlen(typespec)];
            
        _methods.push_back( md );

        return md;
    }

    Signal *
    Endpoint::add_signal ( const char *path, Signal::Direction dir, signal_handler handler, void *user_data )
    {
        Signal *md = new Signal( path, dir );
        
        if ( path )
            md->_path = strdup( path );
        
        md->_handler = handler;
        md->_user_data = user_data;
        md->_endpoint = this;
        
        _signals.push_back( md );
        
        if ( dir == Signal::Input )
        {
            lo_server_add_method( _server, path, NULL, osc_sig_handler, md );
        }

        return md;
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

        delete o;

        _signals.remove( o );
    }

    void
    Endpoint::scan_peer ( const char *name, const char *url )
    {
        Peer *p = new Peer;

        p->name = strdup( name );
        p->addr = lo_address_new_from_url( url );
        p->_scanning = true;

        _peers.push_back( p );

        send( p->addr, "/signal/list_inputs" );
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
                    DMESSAGE( "Adding float %f", ((OSC_Float*)ov)->value() );
                    lo_message_add_float( m, ((OSC_Float*)ov)->value() );
                    break;
                case 'i':
                    DMESSAGE( "Adding int %i", ((OSC_Int*)ov)->value() );
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
}
