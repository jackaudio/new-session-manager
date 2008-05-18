
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

#include "LASH_Client.H"


#include "debug.h"

LASH_Client::LASH_Client ( )
{
    _void = NULL;
}

LASH_Client::~LASH_Client ( )
{
    /* TODO: anything? */
}

#ifdef HAVE_LASH

#include <lash/lash.h>

#define _client (static_cast<lash_client_t*>(_void))

bool
LASH_Client::init ( const char *jack_name, const char *long_name, int *argc, char ***argv )
{
    if ( ! ( _void = lash_init( lash_extract_args( argc, argv ), jack_name,
                                  LASH_Config_File, LASH_PROTOCOL( 2, 0 ) ) ) )
        return false;

    /* register name */
    lash_jack_client_name( _client, jack_name );

    lash_event_t *e = lash_event_new_with_type( LASH_Client_Name );
    lash_event_set_string( e, long_name );
    lash_send_event( _client, e );

    return true;
}

bool
LASH_Client::enabled ( void )
{
    return lash_enabled( _client );
}

/** process any queued events */
void
LASH_Client::poll ( void )
{
    if ( ! _client )
        return;

    lash_event_t *e;

    while ( ( e = lash_get_event( _client ) ) )
    {
        const char *name = lash_event_get_string( e );

        switch ( lash_event_get_type( e )  )
        {
            case LASH_Save_File:
                handle_save_file( name );

                lash_send_event( _client, lash_event_new_with_type( LASH_Save_File ) );

                break;
            case LASH_Restore_File:
                if ( ! handle_restore_file( name ) )
                    /* FIXME: should we tell lash that we couldn't load the song? */;

                lash_send_event( _client, lash_event_new_with_type( LASH_Restore_File ) );

                break;
            case LASH_Quit:
                handle_quit();
                break;
            default:
                WARNING( "unhandled LASH event" );
                break;
        }

        lash_event_destroy( e );
    }
}

#else

bool
LASH_Client::init ( const char *jack_name, const char *long_name, int *argc, char ***argv )
{
    return true;
}

bool
LASH_Client::enabled ( void )
{
    return false;
}

void
LASH_Client::poll ( void )
{
}

#endif
