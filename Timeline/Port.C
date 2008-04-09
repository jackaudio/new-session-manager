
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

Port::Port ( const char *name )
{
    _name = name;

    _port = jack_port_register( engine->client(), _name, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );
}

Port::~Port ( )
{
    /* close port? */
//    jack_port_unregister( engine->client(), _port );
}

void
Port::write ( sample_t *buf, nframes_t nframes )
{
    memcpy( buffer( nframes ), buf, nframes * sizeof( sample_t ) );
}

sample_t *
Port::buffer ( nframes_t nframes )
{
    return (sample_t*)jack_port_get_buffer( _port, nframes );
}
