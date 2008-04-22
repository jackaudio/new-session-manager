
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

#include "Transport.H"
#include "Engine.H"

Transport transport;

#define client engine->client()

void
Transport::poll ( void )
{
    jack_transport_state_t ts;

    ts = jack_transport_query( client, this );

    rolling = ts == JackTransportRolling;
}

void
Transport::locate ( nframes_t frame )
{
    jack_transport_locate( client, frame );
}


void
Transport::start ( void )
{
//    MESSAGE( "Starting transport" );
    jack_transport_start( client );
}

void
Transport::stop ( void )
{
//    MESSAGE( "Stopping transport" );
    jack_transport_stop( client );
}

void
Transport::toggle ( void )
{
    if ( rolling )
        stop();
    else
        start();
}
