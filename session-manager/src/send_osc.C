
/*******************************************************************************/
/* Copyright (C) 2010 Jonathan Moore Liles                                     */
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

// #include <lo/lo.h>
#include <stdio.h>
#include <stdlib.h>
#include <OSC/Endpoint.H>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static bool got_response = false;

/************************/
/* OSC Message Handlers */
/************************/

static int osc_reply ( const char *path, const char *types, lo_arg **argv, int argc, lo_message, void * )
{
//    OSCDMSG();

    printf( "%s : ", path );

    for ( int i = 0; i < argc; ++i )
    {
        switch ( types[i] )
        {
            case 's':
                printf( "\"%s\" ", &argv[i]->s );
                break;
            case 'f':
                printf( "%f ", argv[i]->f );
                break;
            case 'i':
                printf( "%i ", argv[i]->i );
                break;
        }
    }

    printf( "\n" );

    got_response = true;

    return 0;
}



int main(int argc, char *argv[])
{
    OSC::Endpoint s;

    s.init( LO_UDP );

    s.add_method( NULL, NULL, osc_reply, 0, "");

    std::list<OSC::OSC_Value> args;

    for ( int i = 3; i < argc; ++i )
    {
        const char *s = argv[i];

        if ( strspn( s, "+-0123456789" ) == strlen( s ) )
        {
            args.push_back( OSC::OSC_Int( atol( s ) ) );
        }
        else if ( strspn( s, ".+-0123456789" ) == strlen( s ) )
            args.push_back( OSC::OSC_Float( atof( s ) ) );
        else
        {
            args.push_back( OSC::OSC_String( s ) );
        }
    }

    lo_address t = lo_address_new_from_url( argv[1] );

    fprintf( stderr, "Sending to %s\n", argv[1] );

    s.send( t, argv[2], args );

    printf( "Waiting for reply...\n" );

    while ( ! got_response )
         s.wait( 1000 * 30 );

    return 0;
}
