
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

/* RT/thread-safe interface to a single jack port. */

/* nframes is the number of frames to buffer */
Port::Port ( jack_port_t *port, nframes_t nframes )
{
    _port = port;
    _rb = jack_ringbuffer_create( nframes * sizeof( stample_t ) );

    _name = jack_port_name( _port );
}

Port::~Port ( )
{
    jack_ringbuffer_free( _rb );
}

nframes_t
Port::write ( sample_t *buf, nframes_t nframes )
{
    const size_t size = nframes * sizeof( sample_t );

    return jack_ringbuffer_write( _rb, buf, size ) / sizeof( sample_t );
}

/* runs in the RT thread! */
void
Port::process ( nframes_t nframes )
{
    sample_t *buf = jack_port_get_buffer( _port, nframes );

    /* FIXME: check size */
    jack_ringbuffer_read( _rb, buf, nframes );
}
