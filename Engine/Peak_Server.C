
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

   The Peak Server streams peak data to any Timeline Editors or other
   clients that ask for it.

   Peak request looks like (in ASCII)

   >  read_peaks "foo.wav" fpp start end

   Where "foo.wav" is the base name. (actual filenames may differ if
   the channels of the source are 'broken out')

   Response looks like (in binary)

   >  (int)channels (int)length (float)min max min max min max
   >                            min max min max

   Were length specifies the number of Peaks (min/max pairs) (for each
   channel) The first channel is transmitted first, and any others
   follow.
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

    enum { GET_INFO, READ_PEAKS } request;

    if ( 1 == sscanf( buf, "get_info \"%[^\"]\"", source ) )
    {
        request = GET_INFO;
    }
    else if ( 4 == sscanf( buf, "read_peaks \"%[^\"]\" %f %lu %lu", source, &fpp, &start, &end ) )
    {
        request = READ_PEAKS;
    }
    else
    {
        const char *err = "error: malformed request\n";
        fprintf( stderr, err );
        send( s, err, strlen( err ), 0 );
        return;
    }

    Audio_File *af = Audio_File::from_file( source );

    if ( ! af )
    {
        const char *err = "error: could not open source\n";
        send( s, err, strlen( err ), 0 );
        return;
    }

    switch ( request )
    {
        case GET_INFO:
        {
            char buf[128];

            snprintf( buf, sizeof( buf ), "length=%lu channels=%d\n", af->length(), af->channels() );

            send( s, buf, strlen( buf ), 0 );

            break;
        }
        case READ_PEAKS:
        {

            int data[2];
            int peaks;

            data[0] = af->channels();
            data[1] = peaks = (end - start) / fpp;

            send( s, &data, sizeof( data ), 0 );

            const Peaks *pk = af->peaks();

            int npeaks = pk->fill_buffer( fpp, start, end );

            int channels = af->channels();

            Peak *pbuf = new Peak[ npeaks * channels ];

            /* deinterlace */
            int k = 0;
            for ( int i = 0; i < channels; i++ )
                for ( int j = i; j < npeaks * channels; j += channels )
                    pbuf[ k++ ] = pk->peakbuf()[ j ];

            /* transmit */
            send( s, pbuf, sizeof( Peak ) * npeaks * channels, 0 );

            delete pbuf;

            break;
        }
    }

//    delete af;
}
