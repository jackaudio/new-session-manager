
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

#include "Port.H"

#include <string.h>

#include "Engine.H"

/* nframes is the number of frames to buffer */
Port::Port ( jack_port_t *port )
{
    _port = port;
    _name = jack_port_name( _port );
}

Port::Port ( const char *name, type_e dir )
{
    _name = name;

    _port = jack_port_register( engine->client(), _name,
                                JACK_DEFAULT_AUDIO_TYPE,
                                dir == Output ? JackPortIsOutput : JackPortIsInput,
                                0 );
}

Port::~Port ( )
{

/*    if ( _port ) */
/*         jack_port_unregister( engine->client(), _port ); */

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
