/**********************************************************************************/
/* Copyright (C) 2007,2008 Jonathan Moore Liles                                   */
/*                                                                                */
/* This program is free software; you can redistribute it and/or modify it        */
/*  under the terms of the GNU General Public License as published by the         */
/*  Free Software Foundation; either version 2 of the License, or (at your        */
/*  option) any later version.                                                    */
/*                                                                                */
/*  This program is distributed in the hope that it will be useful, but WITHOUT   */
/*  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or ;       */
/*  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for     */
/*  more details.                                                                 */
/*                                                                                */
/*  You should have received a copy of the GNU General Public License along       */
/*  with This program; see the file COPYING.  If not,write to the Free Software ; */
/*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.    */
/**********************************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "non.H"
// #include "gui/input.H"
#include "gui/ui.H"
#include "jack.H"
#include "NSM.H"
#include "transport.H"
#include "pattern.H"
#include "phrase.H"
#include <signal.h>
#include <MIDI/midievent.H>
using namespace MIDI;

// extern const char *BUILD_ID;
// extern const char *VERSION;

const double NSM_CHECK_INTERVAL = 0.25f;

sequence *playlist;

global_settings config;
song_settings song;

NSM_Client *nsm;

char *instance_name;

/* default to pattern mode */

UI *ui;

void
quit ( void )
{
    /* clean up, only for valgrind's sake */
    ui->save_settings();

    delete ui;

    midi_all_sound_off();

    // wait for it...
    sleep( 1 );

    midi_shutdown();


    MESSAGE( "Your fun is over" );

    exit( 0 );
}

void
clear_song ( void )
{
//    song.filename = NULL;

    ui->pattern_canvas_widget->grid( NULL );
    ui->phrase_canvas_widget->grid( NULL );

    playlist->reset();
    playlist->insert( 0, 1 );

    ui->pattern_canvas_widget->grid( new pattern );
    ui->phrase_canvas_widget->grid( new phrase );

    song.dirty( false );
}

void
init_song ( void )
{
    if ( ! midi_is_active() )
        setup_jack();

    if ( !( nsm && nsm->is_active() ) )
        song.filename = NULL;

    clear_song();

    if ( nsm && nsm->is_active() )
        save_song( song.filename );
}

void
handle_midi_input ( void )
{
    midievent e;
    while ( ( midi_input_event( PERFORMANCE, &e ) ) )
    {
        pattern::record_event( &e );
    }
}

bool
load_song ( const char *name )
{
    if ( ! midi_is_active() )
        setup_jack();

    MESSAGE( "loading song \"%s\"", name );

    Grid *pattern_grid = ui->pattern_canvas_widget->grid();
    Grid *phrase_grid = ui->phrase_canvas_widget->grid();

    ui->pattern_canvas_widget->grid( NULL );
    ui->phrase_canvas_widget->grid( NULL );

    if ( ! playlist->load( name ) )
    {
        WARNING( "failed to load song file" );
        goto failed;
    }

    ui->pattern_canvas_widget->grid( pattern::pattern_by_number( 1 ) );
    ui->phrase_canvas_widget->grid( phrase::phrase_by_number( 1 ) );

    song.filename = strdup( name );

    song.dirty( false );

    return true;

failed:

    ui->pattern_canvas_widget->grid( pattern_grid );
    ui->phrase_canvas_widget->grid( phrase_grid );

    return false;
}

bool
save_song ( const char *name )
{
    playlist->save( name );

    song.filename = strdup( name );
    song.dirty( false );

    return true;
}


void
setup_jack ( )
{
   const char *jack_name;

    jack_name = midi_init( instance_name );
    if ( ! jack_name )
        ASSERTION( "Could not initialize MIDI system! (is Jack running and with MIDI ports enabled?)" );

    if ( ! transport.valid )
    {
        if ( transport.master )
            ASSERTION( "The version of JACK you are using does not appear to be capable of passing BBT positional information." );
        else
            ASSERTION( "Either the version of JACK you are using does pass BBT information, or the current timebase master does not provide it." );
    }
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
        quit();
    }
}


void
check_nsm ( void * v )
{
    nsm->check();
    Fl::repeat_timeout( NSM_CHECK_INTERVAL, check_nsm, v );
}

int
main ( int argc, char **argv )
{

    printf( "%s %s %s -- %s\n", APP_TITLE, VERSION, "", COPYRIGHT );

    if ( ! Fl::visual( FL_DOUBLE | FL_RGB ) )
    {
        WARNING( "Xdbe not supported, FLTK will fake double buffering." );
    }

    ::signal( SIGTERM, sigterm_handler );
    ::signal( SIGHUP, sigterm_handler );
    ::signal( SIGINT, sigterm_handler );

    config.follow_playhead = true;
    config.record_mode = MERGE;
    song.play_mode = PATTERN;

    asprintf( &config.user_config_dir, "%s/%s", getenv( "HOME" ), USER_CONFIG_DIR );
    mkdir( config.user_config_dir, 0777 );

    playlist = new sequence;

    nsm = new NSM_Client;

    song.filename = NULL;

    ui = new UI;

    pattern::signal_create_destroy.connect( mem_fun( ui->phrase_canvas_widget,  &Canvas::v_zoom_fit ) );
    pattern::signal_create_destroy.connect( mem_fun( song, &song_settings::set_dirty ) );
    phrase::signal_create_destroy.connect( mem_fun( song, &song_settings::set_dirty ) );

    song.dirty( false );

    clear_song();

#ifdef HAVE_XPM
    ui->main_window->icon((char *)p);
#endif
    ui->main_window->show( 0, 0 );

    instance_name = strdup( APP_NAME );

    const char *nsm_url = getenv( "NSM_URL" );
    
    if ( nsm_url )
    {
        if ( ! nsm->init( nsm_url ) )
        {
            nsm->announce( APP_NAME, ":switch:dirty:", argv[0] );

            song.signal_dirty.connect( sigc::mem_fun( nsm, &NSM_Client::is_dirty ) );
            song.signal_clean.connect( sigc::mem_fun( nsm, &NSM_Client::is_clean ) );

            // poll so we can keep OSC handlers running in the GUI thread and avoid extra sync
            Fl::add_timeout( NSM_CHECK_INTERVAL, check_nsm, NULL );
        }       
        else
            WARNING( "Error initializing NSM" );
    }
    else
    {
        setup_jack();
        if ( argc > 1 )
        {
            /* maybe a filename on the commandline */
            if ( ! load_song( argv[ 1 ] ) )
                ASSERTION( "Could not load song \"%s\" specified on command line", argv[ 1 ] );
        }
    }

    MESSAGE( "Initializing GUI" );

    Fl::add_check( check_sigterm );

    ui->load_settings();
    ui->run();

    return 0;
}
