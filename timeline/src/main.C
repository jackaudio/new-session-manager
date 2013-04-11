
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
#include <getopt.h>

/* for registrations */
#include "Audio_Region.H"
#include "Sequence.H"
#include "Audio_Sequence.H"
#include "Timeline.H"
#include "Tempo_Sequence.H"
#include "Time_Sequence.H"
#include "Annotation_Sequence.H"
#include "Control_Sequence.H"
#include "Cursor_Sequence.H"
#include "Track.H"

#include "TLE.H"
#include "Timeline.H"

#include "Project.H"
#include "Transport.H"
#include "Engine/Engine.H"

#include "Thread.H"

#include <nsm.h>

extern void set_nsm_callbacks ( nsm_client_t *nsm );

Engine *engine;
Timeline *timeline;
Transport *transport;
TLE *tle;
nsm_client_t *nsm;

char *instance_name = NULL;

/* TODO: put these in a header */
#define USER_CONFIG_DIR ".non-daw/"

const char *APP_NAME = "Non-Timeline";
const char *APP_TITLE = "The Non-Timeline";

const char COPYRIGHT[]  = "Copyright (C) 2008-2013 Jonathan Moore Liles";

const double NSM_CHECK_INTERVAL = 0.25f;

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
check_nsm ( void * v )
{
    nsm_check_nowait( nsm );
    Fl::repeat_timeout( NSM_CHECK_INTERVAL, check_nsm, v );
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
    if ( !strcmp( argv[0], "non-daw" ) )
    {
        /* use old app name and title */
        APP_NAME = "Non-DAW";
        APP_TITLE = "The Non-DAW";
    }

    printf( "%s %s -- %s\n", APP_TITLE, VERSION, COPYRIGHT );

    if ( ! Fl::visual( FL_DOUBLE | FL_RGB ) )
    {
        WARNING( "Xdbe not supported, FLTK will fake double buffering." );
    }
    

    Thread::init();

    Thread thread( "UI" );
    thread.set();

    signal( SIGTERM, sigterm_handler );
    signal( SIGHUP, sigterm_handler );
    signal( SIGINT, sigterm_handler );

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
    LOG_REGISTER_CREATE( Cursor_Point        );
    LOG_REGISTER_CREATE( Cursor_Region       );
    LOG_REGISTER_CREATE( Track               );

    signal( SIGPIPE, SIG_IGN );

    if ( ! ensure_dirs() )
        FATAL( "Cannot create required directories" );

    instance_name = strdup( APP_NAME );
    bool instance_override = false;


    const char *osc_port = NULL;

    static struct option long_options[] = 
        {
            { "help", no_argument, 0, '?' },
            { "instance", required_argument, 0, 'i' },
            { "osc-port", required_argument, 0, 'p' },
            { 0, 0, 0, 0 }
        };

    int option_index = 0;
    int c = 0;

    while ( ( c = getopt_long_only( argc, argv, "", long_options, &option_index  ) ) != -1 )
    {
        switch ( c )
        {
            
            case 'p':
                DMESSAGE( "Using OSC port %s", optarg );
                osc_port = optarg;
                break;
            case 'i':
                DMESSAGE( "Using instance name %s", optarg );
                free( instance_name );
                instance_name = strdup( optarg );
                instance_override = true;
                break;
            case '?':
                printf( "\nUsage: %s [--instance instance_name] [--osc-port portnum] [path_to_project]\n\n", argv[0] );
                exit(0);
                break;
        }
    }

    /* we don't really need a pointer for this */
    // will be created on project new/open
    engine = NULL;

    tle = new TLE;

    nsm = nsm_new();
    set_nsm_callbacks( nsm );

    MESSAGE( "Starting GUI" );

    tle->run();

    timeline->init_osc( osc_port );

    tle->main_window->show( 0, NULL );
   
    char *nsm_url = getenv( "NSM_URL" );

    if ( nsm_url )
    {
        if ( ! nsm_init( nsm, nsm_url ) )
        {
            if ( instance_override )
                WARNING( "--instance option is not available when running under session management, ignoring." );
            
            if ( optind < argc )
                WARNING( "Loading files from the command-line is incompatible with session management, ignoring." );

            nsm_send_announce( nsm, APP_NAME, ":progress:switch:", argv[0] );

            /* poll so we can keep OSC handlers running in the GUI thread and avoid extra sync */
            Fl::add_timeout( NSM_CHECK_INTERVAL, check_nsm, NULL );
        }
    }
    else
    {
        if ( optind < argc )
        {
            MESSAGE( "Loading \"%s\"", argv[optind] );
            Fl::lock();
            tle->open( argv[optind] );
            Fl::unlock();
        }
    }

    Fl::add_check( check_sigterm );

    Fl::run();
    
    /* cleanup for valgrind's sake */

    if ( engine )
    {
        delete engine;
        engine = NULL;
    }

    delete timeline;
    timeline = NULL;

    delete tle;
    tle = NULL;

    nsm_free( nsm );
    nsm = NULL;
    
    MESSAGE( "Your fun is over" );
}
