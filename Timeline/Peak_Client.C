
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

#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>


#include "Peak_Client.H"

/* An interface to the peak server */

#define PEAK_PORT 6111

int
connect_to_host ( const char *host, int port )
{
    int s;
    struct sockaddr_in sa;
    struct hostent *hp;

    memset( &sa, 0, sizeof( sa ) );

    hp = gethostbyname( host );

    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons( port );
    sa.sin_addr.s_addr = INADDR_ANY;

    if ( ( s = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
        perror( "socket()" );
        return -1;
    }

    if ( connect( s, (struct sockaddr *)&sa, sizeof( sa ) ) < 0 )
    {
        perror( "connect()" );
        close( s );
        return -1;
    }

    return s;
}

bool
Peak_Client::connect ( const char *host )
{

    if ( ( _socket = connect_to_host( host, PEAK_PORT ) ) < 0 )
        return false;

    return true;
}

bool
Peak_Client::get_info ( const char *source, nframes_t *len, int *channels )
{

    /* FIXME: needs to handle errors/reconnect */
    char buf[512];

    snprintf( buf, sizeof( buf ), "get_info \"%s\"\n", source );

    send( _socket, buf, strlen( buf ), 0 );

    recv( _socket, buf, sizeof( buf ), 0 );

    if ( sscanf( buf, "length=%lu channels=%d", len, channels ) != 2 )
        return false;

    return true;
}

bool
Peak_Client::read_peaks ( const char *source, float fpp, nframes_t start, nframes_t end,
                          int *peaks, Peak **pbuf, int *channels )
{

    /* FIXME: needs to handle errors/reconnect */
    char buf[512];

    snprintf( buf, sizeof( buf ), "read_peaks \"%s\" %f %lu %lu\n", source, fpp, start, end );

    send( _socket, buf, strlen( buf ), 0 );

    if ( recv( _socket, buf, sizeof( int ) * 2, 0 ) != sizeof( int ) * 2 )
    {
        printf( "error!\n" );
        return false;
    }

    if ( ! strncmp( buf, "error", 5 ) )
        return false;

    *channels = ((int*)buf)[0];
    *peaks    = ((int*)buf)[1];

    if ( ! ( *peaks && *channels ) )
        /* unknown error */;

    printf( "reading %d peaks for %d channels\n", *peaks, *channels );

    *pbuf = new Peak[ *peaks * *channels ];

    for ( int i = 0; i < *channels; ++i )
        recv( _socket, *pbuf + (i * *peaks), *peaks * sizeof( Peak ), MSG_WAITALL );

    printf( "done.\n" );

    return true;
}
