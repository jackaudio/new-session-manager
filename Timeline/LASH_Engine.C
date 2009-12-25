
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

/* Actual implementation of our side of the LASH_Engine protocol */

/* NOTES: Since LASH doesn't provide us with the information we
 * need--when we need it--we just punt and only use LASH to save and
 * load the path to the *real* project data. */

#include "LASH_Engine.H"
#include "Project.H"
#include "TLE.H" // all this just for quit()

#include <FL/Fl.H>

extern TLE *tle;

#include "const.h"
#include "util/debug.h"

const float lash_poll_interval = 0.2f;

void
LASH_Engine::timer_cb ( void *v )
{
    ((LASH_Engine*)v)->poll();
    Fl::repeat_timeout( lash_poll_interval, &LASH_Engine::timer_cb, v );
}

LASH_Engine::LASH_Engine ( )
{
    Fl::add_timeout( lash_poll_interval, &LASH_Engine::timer_cb, this );
}

LASH_Engine::~LASH_Engine ( )
{
    Fl::remove_timeout( &LASH_Engine::timer_cb );
}

bool
LASH_Engine::handle_save_file ( const char *path )
{
    MESSAGE( "LASH wants us to save \"%s\"", path );

    char *name;

    asprintf( &name, "%s/project-path", path );

    FILE *fp;

    if ( ! ( fp = fopen( name, "w" ) ) )
    {
        free( name );
        return false;
    }
    else
        free( name );

    char project_path[ 512 ];

    fl_filename_absolute( project_path, sizeof( project_path ), "." );

    fwrite( project_path, strlen( project_path ), 1, fp );

    fclose( fp );

    return Project::save();
}

bool
LASH_Engine::handle_restore_file ( const char *path )
{
    MESSAGE( "LASH wants us to load \"%s\"", path );

    char *name;

    asprintf( &name, "%s/project-path", path );

    FILE *fp;

    if ( ! ( fp = fopen( name, "r" ) ) )
    {
        free( name );
        return false;
    }
    else
        free( name );

    char project_path[ 512 ];

    fgets( project_path, sizeof( project_path ), fp );

    fclose( fp );

    return Project::open( project_path ) == 0;
}

void
LASH_Engine::handle_quit ( void )
{
    MESSAGE( "LASH wants us to quit" );
    tle->quit();
}
