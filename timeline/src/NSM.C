
/*******************************************************************************/
/* Copyright (C) 2012 Jonathan Moore Liles                                     */
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

#include "const.h"
#include "debug.h"
#include "Timeline.H"
#include "TLE.H"
#include "Project.H"
#include "OSC/Endpoint.H"

#include <nsm.h>

#define OSC_INTERVAL 0.2f

extern char *instance_name;
extern Timeline *timeline;

// extern NSM_Client *nsm;



static int 
command_save ( char **out_msg, void *userdata )
{
   if ( timeline->command_save() )
       return ERR_OK;
   else
   {
       *out_msg = strdup( "Failed to save for unknown reason");
       return ERR_GENERAL;
   }
}

static int 
command_open ( const char *name, const char *display_name, const char *client_id, char **out_msg, void *userdata )
{
    if ( instance_name )
        free( instance_name );
    
    instance_name = strdup( client_id );

    timeline->osc->name( client_id );

    int r = 0;

    if ( Project::validate( name ) )
    {
        if ( timeline->command_load( name, display_name ) )
            r = ERR_OK;
        else
        {
            *out_msg = strdup( "Failed to load for unknown reason" );
            r = ERR_GENERAL;
        }
    }
    else
    {
        if ( timeline->command_new( name, display_name ) )
            r =ERR_OK;
        else
        {
            *out_msg = strdup( "Failed to load for unknown reason" );
            r = ERR_GENERAL;
        }
    }

    timeline->discover_peers();

    return r;
}

static void
command_session_is_loaded ( void *userdata )
{
    MESSAGE( "NSM says session is loaded." );

    timeline->discover_peers();
}

static int
command_broadcast ( const char *path, lo_message msg, void *userdata )
{
    int argc = lo_message_get_argc( msg );
//    lo_arg **argv = lo_message_get_argv( msg );

    if ( !strcmp( path, "/non/hello" ) )
    {
        timeline->reply_to_finger( msg );
        return 0;
    }
    else 
        return -1;

}

void
set_nsm_callbacks ( nsm_client_t *nsm )
{
    nsm_set_open_callback( nsm, command_open, 0 );
    nsm_set_save_callback( nsm, command_save, 0 );
    nsm_set_broadcast_callback( nsm, command_broadcast, 0 );
    nsm_set_session_is_loaded_callback( nsm, command_session_is_loaded, 0 );
}
