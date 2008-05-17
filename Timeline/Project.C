
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

/* Routings for opening/closing/creation of projects. All the actual
project state belongs to Timeline and other classes. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "Loggable.H"
#include "Project.H"

#include "Timeline.H" // for sample_rate();

#include "TLE.H" // all this just for load and save...

#include <FL/filename.H>

extern TLE *tle;

/* FIXME: wrong place for this */
#define APP_TITLE "Non-DAW"

#define PROJECT_VERSION "0.28.0"

#include "debug.h"
char Project::_name[256];
char Project::_path[512];
bool Project::_is_open = false;

void
Project::set_name ( const char *name )
{
    strcpy( Project::_name, name );

    if ( Project::_name[ strlen( Project::_name ) - 1 ] == '/' )
        Project::_name[ strlen( Project::_name ) - 1 ] = '\0';

    char *s = rindex( Project::_name, '/' );

    s = s ? s + 1 : Project::_name;

    memmove( Project::_name, s, strlen( s ) + 1 );

    for ( s = Project::_name; *s; ++s )
        if ( *s == '_' || *s == '-' )
            *s = ' ';
}

static int
exists ( const char *name )
{
    struct stat st;

    return 0 == stat( name, &st );
}

#include <errno.h>

bool
Project::close ( void )
{
    if ( ! open() )
        return true;

    tle->save_timeline_settings();

    Loggable::close();

    write_info();

    _is_open = false;

    *Project::_name = '\0';

    return true;
}

bool
Project::write_info ( void )
{

    if ( ! open() )
        return true;

    FILE *fp;

    if ( ! ( fp = fopen( "info", "w" ) ) )
    {
        WARNING( "could not open project info file for writing." );
        return false;
    }

    fprintf( fp, "created by\n\t%s\nversion\n\t%s\nsample rate\n\t%lu\n",
             APP_TITLE " " VERSION,
             PROJECT_VERSION,
             (unsigned long)timeline->sample_rate() );

    fclose( fp );

    return true;
}

bool
Project::read_info ( void )
{
    FILE *fp;

    if ( ! ( fp = fopen( "info", "r" ) ) )
    {
        WARNING( "could not open project info file for reading." );
        return false;
    }

    /* TODO: something */

    fclose( fp );

    return true;
}

/** ensure a project is valid before opening it... */
bool
Project::validate ( const char *name )
{
    bool r = true;

    char pwd[512];

    fl_filename_absolute( pwd, sizeof( pwd ), "." );

    if ( chdir( name ) )
    {
        WARNING( "Cannot change to project dir \"%s\"", name );
        return false;
    }

    if ( ! exists( "info" ) ||
         ! exists( "history" ) ||
         ! exists( "sources" ) )
//         ! exists( "options" ) )
    {
        WARNING( "Not a Non-DAW project: \"%s\"", name );
        r = false;
    }

    chdir( pwd );

    return r;
}

bool
Project::open ( const char *name )
{
    if ( ! validate( name ) )
        return false;

    close();

    chdir( name );

    if ( ! Loggable::open( "history" ) )
        FATAL( "error opening journal" );

    set_name( name );

    *_path = '\0';
    fl_filename_absolute( _path, sizeof( _path ), "." );

    read_info();

    _is_open = true;

    tle->load_timeline_settings();

    return true;
}

bool
Project::create ( const char *name, const char *template_name )
{
    if ( exists( name ) )
    {
        WARNING( "Project already exists" );
        return false;
    }

    close();

    if ( mkdir( name, 0777 ) )
    {
        WARNING( "Cannot create project directory" );
        return false;
    }

    if ( chdir( name ) )
        FATAL( "WTF? Cannot change to new project directory" );

    mkdir( "sources", 0777 );

    creat( "info", 0666 );
    creat( "history", 0666 );

    /* TODO: copy template */

    if ( open( name ) )
    {
        write_info();

        /* add the bare essentials */
        timeline->beats_per_minute( 0, 120 );
        timeline->time( 0, 4, 4 );

        return true;
    }
    else
        return false;
}
