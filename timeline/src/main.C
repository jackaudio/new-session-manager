
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
#include <stdlib.h>

/* for registrations */
#include "Audio_Region.H"
#include "Sequence.H"
#include "Audio_Sequence.H"
#include "Timeline.H"
#include "Tempo_Sequence.H"
#include "Time_Sequence.H"
#include "Annotation_Sequence.H"
#include "Control_Sequence.H"
#include "Track.H"

#include "TLE.H"
#include "Timeline.H"
#include "../FL/Boxtypes.H"

#include "Project.H"
#include "Transport.H"
#include "Engine/Engine.H"

#include "Thread.H"

#include "NSM.H"

Engine *engine;
Timeline *timeline;
Transport *transport;
TLE *tle;
NSM_Client *nsm;

char *instance_name = NULL;

/* TODO: put these in a header */
#define USER_CONFIG_DIR ".non-daw/"

const char APP_NAME[]   = "Non-DAW";
const char APP_TITLE[]  = "The Non-DAW";
const char COPYRIGHT[]  = "Copyright (C) 2008-2010 Jonathan Moore Liles";

#define OSC_INTERVAL 0.2f

#define PACKAGE "non"


#include "const.h"
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

#include <FL/Fl_Shared_Image.H>

#include <signal.h>

void
shift ( char **argv, int *argc, int n )
{
    int i;

    for ( i = 0; i < *argc; ++i )
        argv[ i ] = argv[ i + n ];

    argv[ i ] = 0;
    argc -= n;
}

extern Timeline *timeline;

void
check_osc ( void * v )
{
    nsm->check();
    Fl::repeat_timeout( OSC_INTERVAL, check_osc, v );
}

static int got_sigterm = 0;

void
sigterm_handler ( int )
{
    got_sigterm = 1;
    Fl::awake();
}

void
check_sigterm ( void * )
{
    if ( got_sigterm )
    {
        MESSAGE( "Got SIGTERM, quitting..." );
        timeline->command_quit();
    }
}

int
main ( int argc, char **argv )
{
    Thread::init();

    Thread thread( "UI" );
    thread.set();

    signal( SIGTERM, sigterm_handler );

    fl_register_images();

    /* welcome to C++ */
    LOG_REGISTER_CREATE( Annotation_Point    );
    LOG_REGISTER_CREATE( Annotation_Region   );
    LOG_REGISTER_CREATE( Annotation_Sequence );
    LOG_REGISTER_CREATE( Audio_Region        );
    LOG_REGISTER_CREATE( Audio_Sequence      );
    LOG_REGISTER_CREATE( Control_Point       );
    LOG_REGISTER_CREATE( Control_Sequence    );
    LOG_REGISTER_CREATE( Tempo_Point         );
    LOG_REGISTER_CREATE( Time_Point          );
    LOG_REGISTER_CREATE( Track               );

    init_boxtypes();

    signal( SIGPIPE, SIG_IGN );

    if ( ! ensure_dirs() )
        FATAL( "Cannot create required directories" );

    printf( "%s %s -- %s\n", APP_TITLE, VERSION, COPYRIGHT );

    tle = new TLE;

    instance_name = strdup( APP_NAME );

    /* we don't really need a pointer for this */
    // will be created on project new/open
    engine = NULL;

    nsm = new NSM_Client;

    const char *osc_port = NULL;

    {
        int r = argc - 1;
        int i = 1;
        for ( ; i < argc; ++i, --r )

            if ( !strcmp( argv[i], "--osc-port" ) )
            {
                if ( r > 1 )
                {
                    MESSAGE( "Using OSC port \"%s\"", argv[i+1] );
                    osc_port = argv[i+1];
                    --r;
                    ++i;
                }
                else
                {
                    FATAL( "Missing OSC port" );
                }
            }
    

        MESSAGE( "Starting GUI" );

        tle->run();

        char *nsm_url = getenv( "NSM_URL" );

        if ( nsm_url )
        {
            if ( ! nsm->init() );
            {
                nsm->announce( nsm_url, APP_NAME, ":progress:switch:", argv[0] );
            }
        }
        else
        {
            if ( r >= 1 )
            {
                MESSAGE( "Loading \"%s\"", argv[i] );
            
                tle->open( argv[i] );

                /* ) */
                /*            { */
                /*                fl_alert( "Error opening project specified on commandline" ); */
                /*            } */
            }
        }
    
    }

    /* poll so we can keep OSC handlers running in the GUI thread and avoid extra sync */
    Fl::add_timeout( OSC_INTERVAL, check_osc, NULL );

    Fl::add_check( check_sigterm );

    Fl::run();
    
    if ( engine )
    {
        delete engine;
        engine = NULL;
    }

    delete tle;
    tle = NULL;

    delete nsm;
    nsm = NULL;
    
    MESSAGE( "Your fun is over" );
}
