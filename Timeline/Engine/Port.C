
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

#include "Engine.H"

#include <stdio.h> // sprintf

static const char *name_for_port ( Port::type_e dir, const char *base, int n, const char *type );



int
Port::max_name ( void )
{
    return jack_port_name_size() - jack_client_name_size() - 6;
}

/* nframes is the number of frames to buffer */
Port::Port ( jack_port_t *port )
{
    _port = port;
    _name = jack_port_name( _port );
}

Port::Port ( const char *name, type_e dir )
{
    activate( name, dir );
}

Port::Port ( type_e dir, const char *base, int n, const char *type )
{
    const char *name = name_for_port( dir, base, n, type );

    activate( name, dir );
}

Port::~Port ( )
{
/*    if ( _port ) */
/*         jack_port_unregister( engine->client(), _port ); */

}



static const char *
name_for_port ( Port::type_e dir, const char *base, int n, const char *type )
{
    static char pname[ 512 ];

    const char *dir_s = dir == Port::Output ? "out" : "in";

    strncpy( pname, base, Port::max_name() );
    pname[ Port::max_name() - 1 ] = '\0';

    int l = strlen( pname );

    if ( type )
        snprintf( pname + l, sizeof( pname ) - l, "/%s-%s-%d", type, dir_s, n + 1 );
    else
        snprintf( pname + l, sizeof( pname ) - l, "/%s-%d", dir_s, n + 1 );

    return pname;
}

void
Port::activate ( const char *name, type_e dir )
{
    _name = name;
    _port = jack_port_register( engine->client(), _name,
                                JACK_DEFAULT_AUDIO_TYPE,
                                dir == Output ? JackPortIsOutput : JackPortIsInput,
                                0 );
}

void
Port::shutdown ( void )
{
   if ( _port )
        jack_port_unregister( engine->client(), _port );
}

/** rename port */
bool
Port::name ( const char *name )
{
    _name = name;

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
#include "const.h"
}
