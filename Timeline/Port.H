
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

#pragma once

#include <jack/jack.h>

#include "types.h"

class Port
{
    jack_port_t *_port;
    const char *_name;

public:

    enum type_e { Output, Input };

    Port ( jack_port_t *port );
    Port ( const char *name, type_e dir );
    Port ( type_e dir, const char *base, int n, const char *type=0 );

//    Port ( );
    ~Port ( );

/*     Port ( const Port & rhs ) */
/*         { */
/*             _port = rhs.port; */
/*             _name = rhs.name; */
/*         } */


    bool valid ( void ) const { return _port; }
    bool connected ( void ) const { return jack_port_connected( _port ); }
    type_e type ( void ) const
        {
            return jack_port_flags( _port ) == JackPortIsOutput ? Output : Input;
        }
    const char * name ( void ) const { return _name; }
    bool name ( const char *name );
    bool name ( const char *base, int n, const char *type=0 );


    void activate ( const char *name, type_e dir );
    void shutdown ( void );
    void write ( sample_t *buf, nframes_t nframes );
    void read ( sample_t *buf, nframes_t nframes );
    void *buffer ( nframes_t nframes );

};
