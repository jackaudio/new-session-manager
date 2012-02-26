
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
#include "Mixer.H"
#include "NSM.H"
#include "Project.H"

#include "OSC/Endpoint.H"

extern char *instance_name;
extern Mixer *mixer;

extern NSM_Client *nsm;

NSM_Client::NSM_Client ( )
{
}

int command_open ( const char *name, const char *display_name, const char *client_id, char **out_msg );
int command_save ( char **out_msg );

int
NSM_Client::command_broadcast ( const char *path, lo_message msg )
{
    int argc = lo_message_get_argc( msg );
    lo_arg **argv = lo_message_get_argv( msg );

    if ( argc == 1 && !strcmp( path, "/non/finger" ) )
    {
        mixer->reply_to_finger( msg );
        return 0;
    }
    else 
        return -1;

}


int
NSM_Client::command_save ( char **out_msg )
{
    Fl::lock();

    int r = ERR_OK;

    if ( ! mixer->command_save() )
    {
        *out_msg = strdup( "Failed to save for unknown reason");
        return r = ERR_GENERAL;
    }

    Fl::unlock();
    
    return r;
}

int
NSM_Client::command_open ( const char *name, const char *display_name, const char *client_id, char **out_msg )
{
    Fl::lock();

    if ( instance_name )
        free( instance_name );
    
    instance_name = strdup( client_id );

    mixer->osc_endpoint->name( client_id );

    int r = ERR_OK;

    if ( Project::validate( name ) )
    {
        if ( ! mixer->command_load( name, display_name ) )
        {
            *out_msg = strdup( "Failed to load for unknown reason" );
            r = ERR_GENERAL;
        }
    }
    else
    {
        if ( ! mixer->command_new( name, display_name ) )
        {
            *out_msg = strdup( "Failed to load for unknown reason" );
            r = ERR_GENERAL;
        }
    }

    mixer->discover_peers();

    Fl::unlock();

    return r;
}

void
NSM_Client::command_active ( bool active )
{
    Fl::lock();

    mixer->sm_active( active );

    Fl::unlock();
}
