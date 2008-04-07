
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

class Port
{
    jack_ringbuffer_t _rb;
    jack_port_t *_port;
    const char *_name;

public:

    Port ( jack_port_t *port, nframes_t nframes );
    ~Port ( );

    bool connected ( void ) const { return jack_port_connected( _port ); }
    const char * name ( void ) const { return _name; }

    nframes_t write ( sample_t *buf, nframes_t nframes );
    void process ( nframes_t nframes );

};
