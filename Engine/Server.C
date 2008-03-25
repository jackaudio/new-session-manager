
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

#include "Server.H"

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

/* Generic TCP server class */

/*  */


#define MAX_HOST_NAME 512

/** open a socket listening on TCP port /port/. Returns -1 in case of error. */
int
Server::listen_on_port ( int port )
{
    int s;
    char name[ MAX_HOST_NAME + 1 ];
    struct sockaddr_in sa;
    struct hostent *hp;

    memset( &sa, 0, sizeof( sa ) );
    gethostname( name, MAX_HOST_NAME );
    hp = gethostbyname( name );

    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons( port );
    sa.sin_addr.s_addr = INADDR_ANY;

    if ( ( s = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
        printf( "Could not create command socket!" );
        return -1;
    }

    int yes = 1;

    if ( setsockopt( s, SOL_SOCKET, SO_REUSEADDR, (char * )&yes, sizeof( int ) ) < 0 )
    {
        close( s );
        return -1;
    }

    if ( bind( s, (struct sockaddr *)&sa, sizeof( sa ) ) < 0 )
    {
        printf( "bind: %s", strerror( errno ) );
        close( s );
        return -1;
    }

    listen( s, 1 );

    return s;
}

Server::Server ( int port )
{
    _port = port;
}

void
Server::run ( void )
{
    int server;                                                 /* server socket */

    if ( ( server = listen_on_port( _port ) ) < 0 )
        /* error */;

    fd_set master;
    fd_set read_fds;
    int maxfd;

    FD_ZERO( &master );
    FD_ZERO( &read_fds );

    FD_SET( server, &master );

    maxfd = server;

    for ( ;; )
    {
        read_fds = master;

        if ( select( maxfd + 1, &read_fds, NULL, NULL, NULL ) == -1 )
        {
            perror( "select()" );
            /* error */
        }

        /* service connections */
        /* FIXME: there's a better way to do this than cover *all* fds. */
        for ( int i = 2; i < maxfd + 1; ++i )
        {
            if ( FD_ISSET( i, &read_fds ) )
            {
                if ( i == server )
                {
                    struct sockaddr_in ca;

                    socklen_t al = sizeof( ca );
                    int c;

                    if ( ( c = accept( server, (struct sockaddr *)&ca, &al ) ) < 0 )
                        perror( "accept()" );

                    FD_SET( c, &master );

                    if ( c > maxfd )
                        maxfd = c;

                    handle_new( c );
//                    printf( "New connection from %s on socket %d\n", inet_ntoa( ca.sin_addr ), c );
                }
                else
                {
                    char buf[ BUFSIZ ];

                    int l;

                    if ( ( l = recv( i, buf, sizeof( buf ), 0 ) ) <= 0 )
                    {
                        if ( l == 0 )
                        {
                            handle_hang_up( i );
                        }
                        else
                            perror( "recv()" );

                        close( i );
                        FD_CLR( i, &master );
                    }
                    else
                    {

                        if ( echos() )
                            /* echo to others */
                            for ( int j = maxfd; j-- ; )
                            {
                                if ( ! FD_ISSET( j, &master ) )
                                    continue;

                                if ( j != server && j != i )
                                {
                                    if ( send( j, buf, l, 0 ) < 0 )
                                        perror( "send()" );
                                }
                            }

                        buf[ l ] = '\0';
                        handle_request( i, buf, l );
                    }
                }
            }
        }
    }
}
