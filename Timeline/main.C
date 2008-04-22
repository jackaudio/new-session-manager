
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


#include "Clock.H"

Engine *engine;
Timeline *timeline;
Transport *transport;

void cb_undo ( Fl_Widget *w, void *v )
{
    Loggable::undo();
}


const float UPDATE_FREQ = 0.05f;

static void
clock_update_cb ( void *w )
{
    Fl::repeat_timeout( UPDATE_FREQ, clock_update_cb, w );

    ((Clock *)w)->set( transport->frame );
}

int
main ( int argc, char **argv )
{
    Fl_Window *main_window = new Fl_Window( 0, 0, 1024, 768 );

    Fl::visual( FL_RGB8 );
    Fl::visible_focus( 0 );

    Fl::get_system_colors();
    Fl::scheme( "plastic" );
//    Fl::scheme( "gtk+" );

    /* welcome to C++ */
    LOG_REGISTER_CREATE( Region );
    LOG_REGISTER_CREATE( Time_Point );
    LOG_REGISTER_CREATE( Tempo_Point );
    LOG_REGISTER_CREATE( Control_Point );
    LOG_REGISTER_CREATE( Track );
    LOG_REGISTER_CREATE( Audio_Sequence );
    LOG_REGISTER_CREATE( Control_Sequence );

    /* TODO: change to seesion dir */

    transport = new Transport( 0, 0, 300, 24 );

    /* we don't really need a pointer for this */
    engine = new Engine;
    engine->init();

    timeline = new Timeline( 0, 100, main_window->w(), main_window->h() - 24, "Timeline" );

    Loggable::open( "history" );

    Fl_Button *o = new Fl_Button( 0, 0, 50, 24, "undo" );
    o->shortcut( FL_CTRL + 'z' );
    o->callback( cb_undo, 0 );

    {
        Clock *o = new Clock( 400, 0, 170, 40, "PLAYHEAD" );
        o->color( fl_darker( FL_GRAY ) );

        Fl::add_timeout( UPDATE_FREQ, clock_update_cb, o );
    }

    main_window->end();
    main_window->show( argc, argv );

    Fl::run();
}
