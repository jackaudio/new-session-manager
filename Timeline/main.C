
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

#include <FL/Fl.H>


#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#include "Region.H"
#include "Sequence.H"
#include "Audio_Sequence.H"
#include "Timeline.H"
#include "Tempo_Sequence.H"
#include "Time_Sequence.H"
#include "Annotation_Sequence.H"
#include "Control_Sequence.H"

#include "Transport.H"

#include "Loggable.H"
#include "Track.H"

#include "Engine.H"

#include "TLE.H"

#include "../FL/Boxtypes.H"

#include <stdlib.h>
#include <sys/stat.h>

#include "Session.H"

Engine *engine;
Timeline *timeline;
Transport *transport;

/* TODO: put these in a header */
#define USER_CONFIG_DIR ".non-daw/"

const char APP_NAME[]   = "Non-DAW";
const char APP_TITLE[]  = "The Non-DAW (Digital Audio Workstation)";
const char COPYRIGHT[]  = "Copyright (C) 2008 Jonathan Moore Liles";

#define PACKAGE "non"


#include "debug.h"

char *user_config_dir;

#include <errno.h>

static int
ensure_dirs ( void )
{
    asprintf( &user_config_dir, "%s/%s", getenv( "HOME" ), USER_CONFIG_DIR );

    int r = mkdir( user_config_dir, 0777 );

    return r == 0 || errno == EEXIST;
}


int
main ( int argc, char **argv )
{
    /* welcome to C++ */
    LOG_REGISTER_CREATE( Region );
    LOG_REGISTER_CREATE( Time_Point );
    LOG_REGISTER_CREATE( Tempo_Point );
    LOG_REGISTER_CREATE( Annotation_Point );
    LOG_REGISTER_CREATE( Control_Point );
    LOG_REGISTER_CREATE( Track );
    LOG_REGISTER_CREATE( Audio_Sequence );
    LOG_REGISTER_CREATE( Control_Sequence );
    LOG_REGISTER_CREATE( Annotation_Sequence );

    init_boxtypes();

    if ( ! ensure_dirs() )
        FATAL( "Cannot create required directories" );

    printf( "%s %s -- %s\n", APP_TITLE, VERSION, COPYRIGHT );

    TLE tle;

    MESSAGE( "Initializing JACK" );

    /* we don't really need a pointer for this */
    engine = new Engine;
    engine->init();

    /* always start stopped (please imagine for me a realistic
     * scenario requiring otherwise */
    transport->stop();

    if ( argc > 1 )
        if ( ! Session::open( argv[ 1 ] ) )
            FATAL( "Could not open session specified on command line" );

    /* FIXME: open session in /tmp if none is given? */

    MESSAGE( "Starting GUI" );
//    tle.main_window->show( argc, argv );
    tle.main_window->show();

    Fl::run();
}
