
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

/* Project management routines. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <unistd.h>

#include "Loggable.H"
#include "Project.H"

#include <FL/filename.H>

#include "const.h"
#include "debug.h"
#include "file.h"

#include "Mixer.H"

const int PROJECT_VERSION = 1;



const char *Project::_errstr[] =
{
    "Not a Non-Mixer project",
    "Locked by another process",
    "Access denied",
    "Incompatible project version"
};

char Project::_name[256];
char Project::_created_on[40];
char Project::_path[512];
bool Project::_is_open = false;
bool Project::_is_opening = false;
int Project::_lockfd = 0;



/***********/
/* Private */
/***********/

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

void
Project::name ( const char *name )
{
    strcpy( Project::_name, name );
}

bool
Project::write_info ( void )
{
    FILE *fp;

    if ( ! ( fp = fopen( "info", "w" ) ) )
    {
        WARNING( "could not open project info file for writing." );
        return false;
    }

    char s[40];

    if ( ! *_created_on )
    {
        time_t t = time( NULL );
        ctime_r( &t, s );
        s[ strlen( s ) - 1 ] = '\0';
    }
    else
        strcpy( s, _created_on );

    fprintf( fp, "created by\n\t%s\ncreated on\n\t%s\nversion\n\t%d\n",
             APP_TITLE " " VERSION,
             s,
             PROJECT_VERSION );

    fclose( fp );

    return true;
}

bool
Project::read_info ( int *version, char **creation_date, char **created_by )
{
    FILE *fp;

    if ( ! ( fp = fopen( "info", "r" ) ) )
    {
        WARNING( "could not open project info file for reading." );
        return false;
    }

    *version = 0;
    *creation_date = 0;
    *created_by = 0;

    char *name, *value;

    while ( fscanf( fp, "%a[^\n]\n\t%a[^\n]\n", &name, &value ) == 2 )
    {
        MESSAGE( "Info: %s = %s", name, value );

        if ( ! strcmp( name, "version" ) )
            *version = atoi( value );
        else if ( ! strcmp( name, "created on" ) )
            *creation_date = strdup( value );
        else if ( ! strcmp( name, "created by" ) )
            *created_by = strdup( value );

        free( name );
        free( value );
    }

    fclose( fp );

    return true;
}

/**********/
/* Public */
/**********/

/** Save out any settings and unjournaled state... */
bool
Project::save ( void )
{
    if ( ! open() )
        return true;

//    tle->save_timeline_settings();

    int r = mixer->save();

//    Loggable::clear_dirty();

    return r;
//    return Loggable::save_unjournaled_state();
}


/** Close the project (reclaiming all memory) */
bool
Project::close ( void )
{
    if ( ! open() )
        return true;

    if ( ! save() )
        return false;

    Loggable::close();
/* //    write_info(); */

    _is_open = false;

    *Project::_name = '\0';
    *Project::_created_on = '\0';

    release_lock( &_lockfd, ".lock" );

    return true;
}

/** Ensure a project is valid before opening it... */
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
         ! exists( "snapshot" ))
    {
        WARNING( "Not a Non-Mixer project: \"%s\"", name );
        r = false;
    }

    chdir( pwd );

    return r;
}

/** Try to open project /name/. Returns 0 if sucsessful, an error code
 * otherwise */
int
Project::open ( const char *name )
{
    if ( ! validate( name ) )
        return E_INVALID;

    close();

    chdir( name );

    if ( ! acquire_lock( &_lockfd, ".lock" ) )
        return E_LOCKED;

    int version;
    char *creation_date;
    char *created_by;

    if ( ! read_info( &version, &creation_date, &created_by ) )
        return E_INVALID;

    if ( strncmp( created_by, APP_TITLE, strlen( APP_TITLE ) ) )
        return E_INVALID;

    if ( version != PROJECT_VERSION )
        return E_VERSION;

    _is_opening = true;
    
    if ( ! Loggable::replay( "snapshot" ) )
        return E_INVALID;

    if ( creation_date )
    {
        strcpy( _created_on, creation_date );
        free( creation_date );
    }
    else
        *_created_on = 0;


    getcwd( _path, sizeof( _path ) );

    set_name( _path );

    _is_open = true;

    _is_opening = false;
//    tle->load_timeline_settings();

//    timeline->zoom_fit();

//    Loggable::clear_dirty();

    MESSAGE( "Loaded project \"%s\"", name );

    return 0;
}

/** Create a new project /name/ from existing template
 * /template_name/ */
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
        WARNING( "Cannot create project directory: %s", name );
        return false;
    }

    if ( chdir( name ) )
    {
        FATAL( "WTF? Cannot change to new project directory" );
        return false;
    }

//    mkdir( "sources", 0777 );
    creat( "snapshot", 0666 );

    /* TODO: copy template */

    write_info();

    if ( open( name ) == 0 )
    {
//        /* add the bare essentials */
//        timeline->beats_per_minute( 0, 120 );
//        timeline->time( 0, 4, 4 );

        MESSAGE( "Created project \"%s\" from template \"%s\"", name, template_name );
        return true;
    }
    else
    {
        WARNING( "Failed to open newly created project" );
        return false;
    }
}

/** Replace the journal with a snapshot of the current state */
void
Project::compact ( void )
{
    Loggable::compact();
}
