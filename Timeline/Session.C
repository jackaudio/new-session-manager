
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

/* Routings for opening/closing/creation of sessions. All the actual
session state belongs to Timeline and other classes. */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include "Loggable.H"
#include "Session.H"

#include "debug.h"
char Session::_name[256];

void
Session::set_name ( const char *name )
{
    char *s = rindex( name, '/' );

    strcpy( Session::_name, s ? s + 1 : name );

    for ( s = Session::_name; *s; ++s )
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
Session::close ( void )
{
    Loggable::close();
}

bool
Session::open ( const char *name )
{
    if ( chdir( name ) )
    {
        WARNING( "Cannot change to session dir \"%s\"", name );
        return false;
    }

    if ( ! exists( "history" ) ||
         ! exists( "sources" ) )
//         ! exists( "options" ) )
    {
        WARNING( "Not a Non-DAW session: \"%s\"", name );
        return false;
    }

    if ( ! Loggable::open( "history" ) )
        FATAL( "error opening journal" );

    set_name( name );

    return true;
}

bool
Session::create ( const char *name, const char *template_name )
{
    if ( exists( name ) )
    {
        WARNING( "Session already exists" );
        return false;
    }

    if ( mkdir( name, 0777 ) )
    {
        WARNING( "Cannot create session directory" );
        return false;
    }

    if ( chdir( name ) )
        FATAL( "WTF? Cannot change to new session directory" );

    mkdir( "sources", 0777 );

    creat( "history", 0666 );

    /* TODO: copy template */

    close();

    return open( name );
}
