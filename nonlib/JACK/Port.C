
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

#include <assert.h>
#include "debug.h"

namespace JACK
{

    /* static char *name_for_port ( Port::direction_e dir, const char *base, int n, const char *type ); */

    int
    Port::max_name ( void )
    {
        return jack_port_name_size() - jack_client_name_size() - 6;
    }
    
    Port::Port ( const Port &rhs )
    {
        _connections = NULL;
        _terminal = rhs._terminal;
//        _connections = rhs._connections;
        _client = rhs._client;
        _port = rhs._port;
        _direction = rhs._direction;
        _type = rhs._type;
        _name = NULL;
        _name = strdup( rhs._name );
        _trackname = NULL;
        if ( rhs._trackname )
            _trackname = strdup( rhs._trackname );
        _client->port_added( this );
    }

/* nframes is the number of frames to buffer */
    Port::Port ( JACK::Client *client, jack_port_t *port )
    {
        _terminal = 0;
        _connections = NULL;
        _client = client;
        _port = port;
        _name = strdup( jack_port_name( port ) );
        _trackname = NULL;
        _direction = ( jack_port_flags( _port ) & JackPortIsOutput ) ? Output : Input;
        const char *type = jack_port_type( _port );

        _type = Audio;
        if ( strstr( type, "MIDI") )
            _type = MIDI;

        _client->port_added( this );

    }

    Port::Port ( JACK::Client *client, const char *trackname, const char *name, direction_e dir, type_e type )
    {
        _port = 0;
        _terminal = 0;
        _name = NULL;
        _trackname = NULL;
        _connections = NULL;
        _client = client;
        _direction = dir;
        _type = type;
        _trackname = NULL;

        if ( trackname )
            _trackname = strdup( trackname );

        _name = strdup( name );

        _client->port_added( this );
    }

    /* Port::Port ( JACK::Client *client, direction_e dir, type_e type, const char *base, int n, const char *subtype ) */
    /* { */
    /*     _port = 0; */
    /*     _terminal = 0; */
    /*     _name = NULL; */
    /*     _connections = NULL; */
    /*     _client = client; */

    /*     _name = name_for_port( dir, base, n, subtype ); */
    /*     _direction = dir; */
    /*     _type = type; */

    /*     _client->port_added( this ); */
    /* } */

    /* Port::Port ( JACK::Client *client, direction_e dir, type_e type, int n, const char *subtype ) */
    /* { */
    /*     _port = 0; */
    /*     _terminal = 0; */
    /*     _name = NULL; */
    /*     _connections = NULL; */
    /*     _client = client; */

    /*     _name = name_for_port( dir, NULL, n, subtype ); */
    /*     _direction = dir; */
    /*     _type = type; */

    /*     _client->port_added( this ); */
    /* } */

    Port::~Port ( )
    {
        _client->port_removed( this );
 
        if ( _name )
        {
            free( _name );
            _name = NULL;
        }
        if ( _trackname )
        {
            free( _trackname );
            _trackname = NULL;
        }

    }

    /* sort input before output and then by alpha */
    bool
    Port::operator < ( const Port & rhs ) const
    {
        if ( type() == rhs.type() )
            return strcmp( name(), rhs.name() );
        else
            return direction() == Port::Input;
    }


    /* static char * */
    /* name_for_port ( Port::direction_e dir, const char *base, int n, const char *type ) */
    /* { */
    /*     char *pname; */

    /*     const char *dir_s = dir == Port::Output ? "out" : "in"; */

    /*     if ( type ) */
    /*         asprintf( &pname, "%s-%s%s%s-%d", type, base ? base : "", base ? "/" : "", dir_s, n + 1 ); */
    /*     else */
    /*         asprintf( &pname, "%s%s%s-%d", base ? base : "", base ? "/" : "", dir_s, n + 1 ); */

    /*     return pname; */
    /* } */

    bool
    Port::activate ( void )
    {
        /* assert( !_port ); */

        int flags = 0;
        
        if ( _direction == Output )
            flags |= JackPortIsOutput;
        else
            flags |= JackPortIsInput;

        if ( _terminal )
            flags |= JackPortIsTerminal;

        char jackname[max_name()];

        snprintf( jackname, sizeof(jackname), "%s%s%s", _trackname ? _trackname : "", _trackname ? "/" : "", _name );

        DMESSAGE( "Activating port name %s", jackname );
        _port = jack_port_register( _client->jack_client(), jackname,
                                    _type == Audio ? JACK_DEFAULT_AUDIO_TYPE : JACK_DEFAULT_MIDI_TYPE,
                                    flags,
                                    0 );

        DMESSAGE( "Port = %p", _port );

        if ( ! _port )
            return false;

        _client->port_added( this );
 
        return true;
    }

/** returns the sum of latency of all ports between this one and a
    terminal port. */
    nframes_t
    Port::total_latency ( void ) const
    {
#ifdef HAVE_JACK_PORT_GET_LATENCY_RANGE
        jack_latency_range_t range;

        jack_port_get_latency_range( _port, _direction == Input ? JackPlaybackLatency : JackCaptureLatency, &range );   

        return range.max;
#else
        return jack_port_get_total_latency( _client->jack_client() , _port );
#endif
    }

/** returns the number of frames of latency assigned to this port */
    void
    Port::get_latency ( direction_e dir, nframes_t *min, nframes_t *max ) const
    {
#ifdef HAVE_JACK_PORT_GET_LATENCY_RANGE
        jack_latency_range_t range;

        jack_port_get_latency_range( _port, dir == Output ? JackPlaybackLatency : JackCaptureLatency, &range );   

        *min = range.min;
        *max = range.max;
#else
        *min = *max = jack_port_get_latency( _port );
#endif

    }

/** inform JACK that port has /frames/ frames of latency */
    void
    Port::set_latency ( direction_e dir, nframes_t min, nframes_t max )
    {
#ifdef HAVE_JACK_PORT_GET_LATENCY_RANGE
        jack_latency_range_t range;
//        DMESSAGE( "Setting port latency!" );

        range.max = max;
        range.min = min;

        jack_port_set_latency_range( _port, dir == Output ? JackPlaybackLatency : JackCaptureLatency, &range );   
#else
        jack_port_set_latency( _port, max );
#endif
    }

    void
    Port::shutdown ( void )
    {
        deactivate();

        _client->port_removed( this );
    }

    void 
    Port::deactivate ( void )
    {
        if ( _port )
            jack_port_unregister( _client->jack_client(), _port );

        _port = 0;
    }


    void
    Port::name ( const char *name )
    {
        if ( _name )
            free( _name );

        _name = strdup( name );
    }

    void
    Port::trackname ( const char *trackname )
    {
        if ( _trackname )
            free( _trackname );

        _trackname = NULL;

        if ( trackname )
            _trackname = strdup( trackname );
    }

    bool
    Port::rename ( void )
    {
        char jackname[max_name()];

        snprintf( jackname, sizeof(jackname), "%s%s%s", _trackname ? _trackname : "", _trackname ? "/" : "", _name );

        if ( _port )
            return 0 == jack_port_set_name( _port, jackname );
        else
            return false;
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
        ASSERT( _port, "Attempt to get connections of null port" );

        return jack_port_get_connections( _port );
    }

    /** Restore the connections returned by connections() */
    bool
    Port::connections ( const char **port_names )
    {
        if ( ! port_names )
            return true;

        for ( const char **port_name = port_names; *port_name; ++port_name )
        {
            printf( "Attempting to reconnect to %s\n", *port_name );
            connect( *port_name );
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
        if ( _connections )
            free( _connections );

//        DMESSAGE( "Freezing port %s", _name );

        _connections = connections();

        //      deactivate();
    }

    void
    Port::thaw ( void )
    {
//        DMESSAGE( "Thawing port %s", _name );

        activate();
        
        if ( _connections )
        {
            connections( _connections );
            
            free( _connections );

            _connections = NULL;
        }
    }
}
