
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

/* Wrapper for a JACK audio port */

#include "Port.H"

#include <string.h>
#include <stdio.h> // sprintf
#include <errno.h>

namespace JACK
{

    static const char *name_for_port ( Port::type_e dir, const char *base, int n, const char *type );

    int
    Port::max_name ( void )
    {
        return jack_port_name_size() - jack_client_name_size() - 6;
    }


    Port::Port ( const Port &rhs )
    {
        _freezer = rhs._freezer;
        _client = rhs._client;
        _port = rhs._port;
        _direction = rhs._direction;
        _name = strdup( rhs._name );

        _client->port_added( this );
    }

/* nframes is the number of frames to buffer */
    Port::Port ( JACK::Client *client, jack_port_t *port )
    {
        _freezer = NULL;
        _client = client;
        _port = port;
        _name = strdup( jack_port_name( port ) );
        _direction = jack_port_flags( _port ) == JackPortIsOutput ? Output : Input;
    }

    Port::Port ( JACK::Client *client, const char *name, type_e dir )
    {
        _name = NULL;
        _freezer = NULL;
        _client = client;
        _direction = dir;

        _name = strdup( name );
    }

    Port::Port ( JACK::Client *client, type_e dir, const char *base, int n, const char *type )
    {
        _name = NULL;
        _freezer = NULL;
        _client = client;

        _name = strdup( name_for_port( dir, base, n, type ) );
        _direction = dir;
    }

    Port::Port ( JACK::Client *client, type_e dir, int n, const char *type )
    {
        _name = NULL;
        _freezer = NULL;
        _client = client;

        _name = strdup( name_for_port( dir, NULL, n, type ) );
        _direction = dir;

    }

    Port::~Port ( )
    {
        if ( _name )
            free( _name );

        _client->port_removed( this );
/*         if ( _freezer ) */
/*         { */
/*             delete _freezer; */
/*             _freezer = NULL; */
/*         } */

/*    if ( _port ) */
/*         jack_port_unregister( _client, _port ); */

    }

    /* sort input before output and then by alpha */
    bool
    Port::operator < ( const Port & rhs ) const
    {
        if ( type() == rhs.type() )
            return strcmp( name(), rhs.name() );
        else
            return type() == Port::Input;
    }


    static const char *
    name_for_port ( Port::type_e dir, const char *base, int n, const char *type )
    {
        static char pname[ 512 ];

        const char *dir_s = dir == Port::Output ? "out" : "in";

        pname[0] = '\0';

        if ( base )
        {
            strncpy( pname, base, Port::max_name() );
            strcat( pname, "/" );
        }

        pname[ Port::max_name() - 1 ] = '\0';

        int l = strlen( pname );

        if ( type )
            snprintf( pname + l, sizeof( pname ) - l, "%s-%s-%d", type, dir_s, n + 1 );
        else
            snprintf( pname + l, sizeof( pname ) - l, "%s-%d", dir_s, n + 1 );

        return pname;
    }

    bool
    Port::activate ( const char *name, type_e dir )
    {
        _name = strdup( name );
        _direction = dir;

        return activate();
    }

    bool
    Port::activate ( void )
    {
        _port = jack_port_register( _client->jack_client(), _name,
                                    JACK_DEFAULT_AUDIO_TYPE,
                                    _direction == Output ? JackPortIsOutput : JackPortIsInput,
                                    0 );

        if ( ! _port )
            return false;

        _client->port_added( this );

        return true;
    }

/** returns the sum of latency of all ports between this one and a
    terminal port. */
/* FIMXE: how does JACK know that input A of client Foo connects to
   output Z of the same client in order to draw the line through Z to a
   terminal port? And, if this determination cannot be made, what use is
   this function? */

    nframes_t
    Port::total_latency ( void ) const
    {
        return jack_port_get_total_latency( _client->jack_client() , _port );
    }

/** returns the number of frames of latency assigned to this port */
    nframes_t
    Port::latency ( void ) const
    {
        return jack_port_get_latency( _port );
    }

/** inform JACK that port has /frames/ frames of latency */
    void
    Port::latency ( nframes_t frames )
    {
        jack_port_set_latency( _port, frames );
    }

    void
    Port::shutdown ( void )
    {
        if ( _port )
            jack_port_unregister( _client->jack_client(), _port );

        _client->port_removed( this );
    }

/** rename port */
    bool
    Port::name ( const char *name )
    {
        _name = strdup( name );

        return 0 == jack_port_set_name( _port, name );
    }

    bool
    Port::name ( const char *base, int n, const char *type )
    {
        return name( name_for_port( this->type(), base, n, type ) );
    }

    void
    Port::write ( sample_t *buf, nframes_t nframes )
    {
        memcpy( buffer( nframes ), buf, nframes * sizeof( sample_t ) );
    }

    void
    Port::read ( sample_t *buf, nframes_t nframes )
    {
        memcpy( buf, buffer( nframes ), nframes * sizeof( sample_t ) );
    }

    void *
    Port::buffer ( nframes_t nframes )
    {
        return jack_port_get_buffer( _port, nframes );
    }

    void
    Port::silence ( nframes_t nframes )
    {
        memset( buffer( nframes ), 0, nframes * sizeof( sample_t ) );
    }

    /** Return a malloc()'d null terminated array of strings
     * representing all ports to which this port is connected. */
    const char **
    Port::connections ( void )
    {
        return jack_port_get_connections( _port );
    }

    Port::type_e
    Port::type ( void ) const
    {
        return _direction;
    }

    /** Restore the connections returned by connections() */
    bool
    Port::connections ( const char **port_names )
    {
        if ( ! port_names )
            return true;

        for ( const char **port_name = port_names; *port_name; ++port_name )
        {
            const char *src;
            const char *dst;
            const char *name = jack_port_name( _port );

            if ( type() == Output )
            {
                src = name;
                dst = *port_name;
            }
            else
            {
                src = *port_name;
                dst = name;
            }

            if ( int err = jack_connect( _client->jack_client(), src, dst ) )
            {
                if ( EEXIST == err )
                {
                    /* connection already exists, not a problem */
                }
                else
                {
                    return false;
                }
            }
        }

        return true;
    }

    int
    Port::connect ( const char *to )
    {
        const char *name = jack_port_name( _port );

        if ( _direction == Output )
        {  
            return jack_connect( _client->jack_client(), name, to );
        }
        else
        {
            return jack_connect( _client->jack_client(), to, name );
        }
    }


    int
    Port::disconnect ( const char *from )
    {
        const char *name = jack_port_name( _port );

        if ( _direction == Output )
        {  
            return jack_disconnect( _client->jack_client(), name, from );
        }
        else
        {
            return jack_disconnect( _client->jack_client(), from, name );
        }
    }

    bool
    Port::connected_to ( const char *to )
    {
        return jack_port_connected_to( _port, to );
    }

    void
    Port::freeze ( void )
    {
        if ( _freezer )
            delete _freezer;

        freeze_state *f = new freeze_state();

        f->connections = connections();
        f->name = strdup( name() );

        _freezer = f;
    }

    void
    Port::thaw ( void )
    {
        activate();

        connections( _freezer->connections );

        delete _freezer;
        _freezer = NULL;
    }
}
