
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
#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Overlay_Window.H>
#include <FL/Fl_Scroll.H>
// #include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Button.H>

#include "Scalebar.H"

// #include "Waveform.H"
#include "Region.H"

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

#include "Sequence.H"
#include "Audio_Sequence.H"
#include "Timeline.H"
#include "Tempo_Sequence.H"
#include "Time_Sequence.H"
#include "Control_Sequence.H"

#include "Transport.H"

#include "Loggable.H"
#include "Track.H"

#include "Engine.H"

// #include "Clock.H"

#include "TLE.H"


#include <stdlib.h>
#include <sys/stat.h>

Engine *engine;
Timeline *timeline;
Transport *transport;

/* void cb_undo ( Fl_Widget *w, void *v ) */
/* { */
/*     Loggable::undo(); */
/* } */

char *user_config_dir;

static int
ensure_dirs ( void )
{
    asprintf( &user_config_dir, "%s/.non-daw", getenv( "HOME" ) );

    return 0 == mkdir( user_config_dir, 0777 );
}

int
main ( int argc, char **argv )
{
    /* welcome to C++ */
    LOG_REGISTER_CREATE( Region );
    LOG_REGISTER_CREATE( Time_Point );
    LOG_REGISTER_CREATE( Tempo_Point );
    LOG_REGISTER_CREATE( Control_Point );
    LOG_REGISTER_CREATE( Track );
    LOG_REGISTER_CREATE( Audio_Sequence );
    LOG_REGISTER_CREATE( Control_Sequence );


    if ( ! ensure_dirs() )
        /* error */;

    /* TODO: change to seesion dir */

    TLE tle;

    /* we don't really need a pointer for this */
    engine = new Engine;
    engine->init();

    Loggable::open( "history" );

//        Fl::add_timeout( UPDATE_FREQ, clock_update_cb, o );

    tle.main_window->show( argc, argv );

    Fl::run();
}
