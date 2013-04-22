
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

#include "const.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <getopt.h>

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Tooltip.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Pack.H>
#include "Thread.H"
#include "debug.h"

#include "Mixer.H"
#include "Project.H"

#include "Loggable.H"

/* for registration */
#include "Module.H"
#include "Gain_Module.H"
#include "Plugin_Module.H"
#include "JACK_Module.H"
#include "Meter_Module.H"
#include "Meter_Indicator_Module.H"
#include "Controller_Module.H"
#include "Mono_Pan_Module.H"
#include "Chain.H"
#include "Mixer_Strip.H"
#include "AUX_Module.H"
#include "NSM.H"

#include <signal.h>
#include <unistd.h>

#include "FL/Fl.H"
#include "FL/x.H"
#include "FL/Fl_PNG_Image.H"

/* TODO: put these in a header */
#define USER_CONFIG_DIR ".non-mixer/"

const double NSM_CHECK_INTERVAL = 0.25f;

const char COPYRIGHT[]  = "Copyright (c) 2008-2013 Jonathan Moore Liles";

char *user_config_dir;
Mixer *mixer;
NSM_Client *nsm;

char *instance_name;

#include <errno.h>

static int
ensure_dirs ( void )
{
    asprintf( &user_config_dir, "%s/%s", getenv( "HOME" ), USER_CONFIG_DIR );

    int r = mkdir( user_config_dir, 0777 );

    return r == 0 || errno == EEXIST;
}

#include <signal.h>

static void cb_main ( Fl_Double_Window *, void *)
{
    if ( Fl::event() == FL_SHORTCUT && Fl::event_key() == FL_Escape )
	return;

    mixer->command_quit();
}

void
check_nsm ( void * v )
{
    nsm->check();
    Fl::repeat_timeout( NSM_CHECK_INTERVAL, check_nsm, v );
}

static volatile int got_sigterm = 0;

void
sigterm_handler ( int )
{
    got_sigterm = 1;
}

void
check_sigterm ( void * )
{
    if ( got_sigterm )
    {
        MESSAGE( "Got SIGTERM, quitting..." );
        mixer->quit();
    }
    Fl::repeat_timeout( 0.1f, check_sigterm );
}


int
main ( int argc, char **argv )
{
    bool no_ui = false;

    printf( "%s %s %s -- %s\n", APP_TITLE, VERSION, "", COPYRIGHT );

    Thread::init();

    Thread thread( "UI" );
    thread.set();

    ensure_dirs();

    signal( SIGTERM, sigterm_handler );
    signal( SIGHUP, sigterm_handler );
    signal( SIGINT, sigterm_handler );

    Fl_Tooltip::color( FL_BLACK );
    Fl_Tooltip::textcolor( FL_YELLOW );
    Fl_Tooltip::size( 14 );
    Fl_Tooltip::hoverdelay( 0.1f );


    LOG_REGISTER_CREATE( Mixer_Strip );
    LOG_REGISTER_CREATE( Chain );
    LOG_REGISTER_CREATE( Plugin_Module );
    LOG_REGISTER_CREATE( Gain_Module );
    LOG_REGISTER_CREATE( Meter_Module );
    LOG_REGISTER_CREATE( JACK_Module );
    LOG_REGISTER_CREATE( Mono_Pan_Module );
    LOG_REGISTER_CREATE( Meter_Indicator_Module );
    LOG_REGISTER_CREATE( Controller_Module );
    LOG_REGISTER_CREATE( AUX_Module );

    signal( SIGPIPE, SIG_IGN );


    const char *osc_port = NULL;

    nsm = new NSM_Client;

    instance_name = strdup( APP_NAME );
    bool instance_override = false;

    static struct option long_options[] = 
        {
            { "help", no_argument, 0, '?' },
            { "instance", required_argument, 0, 'i' },
            { "osc-port", required_argument, 0, 'p' },
            { "no-ui", no_argument, 0, 'u' },
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
                DMESSAGE( "Using OSC port %s", optarg );
                free( instance_name );
                instance_name = strdup( optarg );
                instance_override = true;
                break;
            case 'u':
                DMESSAGE( "Disabling user interface" );
                no_ui = true;
                break;
            case '?':
                printf( "\nUsage: %s [--instance instance_name] [--osc-port portnum] [path_to_project]\n\n", argv[0] );
                exit(0);
                break;
        }
    }

    {
        char *name = strdup( argv[0] );
        char *n = basename( name );

        if ( ! strcmp( n, "non-mixer-noui" ) )
            no_ui = true;
        
        free( name );
    }

    if ( ! no_ui )
    {
        Fl::visual( FL_DOUBLE | FL_RGB );
        
        Fl::visible_focus( 0 );

        fl_register_images();
    }

    Fl::lock();

    Fl_Double_Window *main_window;

    
    {
        Fl_Double_Window *o = main_window = new Fl_Double_Window( 800, 600, "Non Mixer" );
        {
            main_window->xclass( APP_NAME );

            { 
                Fl_Widget *o = mixer = new Mixer( 0, 0, main_window->w(), main_window->h(), NULL );
                Fl_Group::current()->resizable(o);
            }
        }
        o->end();

        o->size_range( main_window->w(), mixer->min_h(), 0, 0 );

        o->callback( (Fl_Callback*)cb_main, main_window );

        if ( ! no_ui )
        {
            o->show( 0,0 );
        }
                      
    }

    Plugin_Module::spawn_discover_thread();
    
    mixer->init_osc( osc_port );

    char *nsm_url = getenv( "NSM_URL" );
        
    if ( nsm_url )
    {
        if ( ! nsm->init( nsm_url ) )
        {
            if ( instance_override )
                WARNING( "--instance option is not available when running under session management, ignoring." );
                
            if ( optind < argc )
                WARNING( "Loading files from the command-line is incompatible with session management, ignoring." );

            nsm->announce( APP_NAME, ":switch:dirty:", argv[0] );

            /* if ( ! no_ui ) */
            /* { */
                // poll so we can keep OSC handlers running in the GUI thread and avoid extra sync
                Fl::add_timeout( NSM_CHECK_INTERVAL, check_nsm, NULL );
            /* } */
        }
    }
    else
    {
        if ( optind < argc )
        {
            MESSAGE( "Waiting for plugins..." );
            Plugin_Module::join_discover_thread();

            MESSAGE( "Loading \"%s\"", argv[optind] );
                
            if ( ! mixer->command_load( argv[optind] ) )
            {
                fl_alert( "Error opening project specified on commandline" );
            }
        }
    }

    Fl::add_timeout( 0.1f, check_sigterm );
    Fl::dnd_text_ops( 0 );

    if ( ! no_ui )
    {
        DMESSAGE( "Running UI..." );

        Fl::run();
    }
    else
    {
        DMESSAGE( "Not Running UI..." );
        while ( ! got_sigterm )
        {
            Fl::check();
            usleep( 200 * 1000 );
        }
    }

    delete main_window;
    main_window = NULL;

    MESSAGE( "Your fun is over" );
}
