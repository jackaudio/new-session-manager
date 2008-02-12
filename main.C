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
#include "lash.H"

#include "pattern.H"
#include "phrase.H"



Canvas *pattern_c, *phrase_c, *trigger_c;

sequence *playlist;

global_settings config;
song_settings song;

Lash lash;

/* default to pattern mode */

UI *ui;

void
quit ( void )
{
    /* clean up, only for valgrind's sake */

    delete ui;

    delete pattern_c;
    delete phrase_c;
    delete trigger_c;

    midi_all_sound_off();

    // wait for it...
    sleep( 1 );

    midi_shutdown();

    MESSAGE( "Your fun is over" );

    exit( 0 );
}

void
init_song ( void )
{
    song.filename = NULL;
    song.dirty( false );

    pattern_c->grid( NULL );
    phrase_c->grid( NULL );

    playlist->reset();
    playlist->insert( 0, 1 );
    pattern_c->grid( new pattern );
    phrase_c->grid( new phrase );
}

void
handle_midi_input ( void )
{
    midievent *e;
    while ( ( e = midi_input_event( PERFORMANCE ) ) )
    {
        pattern::record_event( e );
        delete e;
    }
}

bool
load_song ( const char *name )
{
    MESSAGE( "loading song \"%s\"", name );

    pattern_c->grid( NULL );
    phrase_c->grid( NULL );

    if ( ! playlist->load( name ) )
    {
        WARNING( "failed to load song file" );
        return false;
    }

    pattern_c->grid( pattern::pattern_by_number( 1 ) );
    phrase_c->grid( phrase::phrase_by_number( 1 ) );

    song.filename = strdup( name );

// TODO: signal song loaded.

    return true;
}

bool
save_song ( const char *name )
{
    playlist->save( name );

    song.filename = strdup( name );
    song.dirty( false );

    return true;
}

int
main ( int argc, char **argv )
{

    config.follow_playhead = true;
    config.record_mode = MERGE;
    song.play_mode = PATTERN;
    song.random.feel = 8;
    song.random.probability = 0.33;

    asprintf( &config.user_config_dir, "%s/%s", getenv( "HOME" ), USER_CONFIG_DIR );
    mkdir( config.user_config_dir, 0777 );

    printf( "%s %s -- %s\n", APP_TITLE, VERSION, COPYRIGHT );

    playlist = new sequence;

    pattern_c = new Canvas;
    phrase_c = new Canvas;
    trigger_c = new Canvas;

    init_song();

    if ( ! lash.init( &argc, &argv ) )
        WARNING( "error initializing LASH" );

    if ( argc > 1 )
    {
        /* maybe a filename on the commandline */
        load_song( argv[1] );
    }

    if ( ! midi_init() )
        ASSERTION( "Could not initialize MIDI system! (is Jack running and with MIDI ports enabled?)" );

    MESSAGE( "Initializing GUI" );

    init_colors();

    ui = new UI;

    ui->run();

    return 0;
}
