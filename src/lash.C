
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

#include "lash.H"
#include "config.h"
#include "common.h"
#include "non.H"

// TODO: produce Save_Project events...

#ifndef HAVE_LASH

Lash::Lash ( ) {}
bool Lash::init ( int *argc, char ***argv, const char *jack_name ) { return true; }
void Lash::process ( void ) {}

#else

Lash::Lash ( )
{
    _client = NULL;
}

bool
Lash::init ( int *argc, char ***argv, const char *jack_name )
{
    MESSAGE( "Initializing LASH" );

    if ( ! ( _client = lash_init( lash_extract_args( argc, argv ), APP_NAME,
                                  LASH_Config_File, LASH_PROTOCOL( 2, 0 ) ) ) )
        return false;

    /* register name */
    lash_jack_client_name( _client, jack_name );

    lash_event_t *e = lash_event_new_with_type( LASH_Client_Name );
    lash_event_set_string( e, APP_TITLE );
    lash_send_event( _client, e );

    return true;
}

/** process any queued events */
void
Lash::process ( void )
{
    lash_event_t *e;

    char *name;

    while ( ( e = lash_get_event( _client ) ) )
    {
        asprintf( &name, "%s/%s", lash_event_get_string( e ), "song.non" );

        const int t = lash_event_get_type ( e );

        switch ( t )
        {
            case LASH_Save_File:
            {
                MESSAGE( "LASH wants us to save \"%s\"", name );

                save_song( name );

                lash_send_event( _client, lash_event_new_with_type( LASH_Save_File ) );

                break;

            }
            case LASH_Restore_File:
            {
                MESSAGE( "LASH wants us to load \"%s\"", name );

                if ( ! load_song( name ) )
                    /* FIXME: should we tell lash that we couldn't load the song? */;

                lash_send_event( _client, lash_event_new_with_type( LASH_Restore_File ) );

                break;
            }
            case LASH_Quit:
                MESSAGE( "LASH wants us to quit" );
                quit();
                break;
            default:
                WARNING( "unhandled LASH event (%d)", t );
        }

        lash_event_destroy( e );
    }
}

#endif
