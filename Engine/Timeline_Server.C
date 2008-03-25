
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

// #include "objects.H"

/* #include <sys/socket.h> */
/* #include <sys/types.h> */
/* #include <sys/select.h> */

#include "Timeline_Server.H"

#include <stdio.h>

/* Timeline Server.


   The timeline server runs in its own thread and manages communication
   between the engine and any connected timeline editors.

   Its duties are as follows:

   * Listen for connections from Timeline Editors on a TCP port.

   * Accept 'do' messages from each connection, echoing the change to
   all others.

   * Keep global engine state in sync with recieved messages (in a
   lock-free manner)

   * Write do+undo messages to journal.

   * Translate "undo" command into the appropriate 'do' messages.

   */


#define TIMELINE_PORT 6100

void
Timeline_Server::handle_new ( int s )
{
    printf( "new connection\n" );
}

void
Timeline_Server::handle_hang_up ( int s )
{
    printf( "hangup\n" );
}

void
Timeline_Server::handle_request ( const char *s, int l )
{
    printf( "request: %s", s );
}
