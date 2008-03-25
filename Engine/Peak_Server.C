
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

#include "Peak_Server.H"

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

/* Peak Server

   The peak server streams peak data to any timeline editors or other clients that ask for it.

   Peak request looks like:

   >  read_peaks "foo.wav" fpp start end

   Response looks like (in binary floats):

   >  channels length min max min max min max
               length ...

  */

#include "Audio_File.H"
#include "Peaks.H"

typedef unsigned long tick_t;


#define PEAK_PORT 6100

void
Peak_Server::handle_new ( int s )
{
    printf( "new connection\n" );
}

void
Peak_Server::handle_hang_up ( int s )
{
    printf( "hangup\n" );
}

void
Peak_Server::handle_request ( int s, const char *buf, int l )
{
    printf( "request: %s", buf );

    char source[512];
    float fpp;
    tick_t start, end;

    if ( 4 != sscanf( buf, "read_peaks \"%[^\"]\" %f %lu %lu", source, &fpp, &start, &end ) )
      fprintf( stderr, "error: malformed peak request!\n" );

    Audio_File *af = Audio_File::from_file( source );

    int channels = af->channels();

    send( s, &channels, sizeof( int ), 0 );

    for ( int i = 0; i < af->channels(); ++i )
    {
        const Peaks *pk = af->peaks( i );

        int peaks = pk->fill_buffer( fpp, start, end );

        send( s, &peaks, sizeof( int ), 0 );

        send( s, pk->peakbuf(), peaks * sizeof( Peak ), 0 );
    }

    delete af;
}
