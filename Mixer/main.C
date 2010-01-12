
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
#include <FL/Fl_Single_Window.H>
#include <FL/Fl_Single_Window.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Pack.H>
#include "Mixer_Strip.H"


#include <stdlib.h>
#include <unistd.h>

#include  "DPM.H"

#include "Mixer.H"
#include "Engine/Engine.H"
#include "util/Thread.H"
#include "util/debug.h"
#include "Project.H"

Engine *engine;
Mixer *mixer;

Fl_Single_Window *main_window;

#include <FL/Boxtypes.H>
#include "Loggable.H"
#include <FL/Fl_Tooltip.H>
#include <FL/fl_ask.H>

/* for registration */
#include "Module.H"
#include "Gain_Module.H"
#include "Plugin_Module.H"
#include "JACK_Module.H"
#include "Meter_Module.H"
#include "Meter_Indicator_Module.H"
#include "Controller_Module.H"
#include "Chain.H"


#include <signal.h>

int
main ( int argc, char **argv )
{
    Thread::init();

    Thread thread( "UI" );
    thread.set();

    Fl_Tooltip::color( FL_BLACK );
    Fl_Tooltip::textcolor( FL_YELLOW );
    Fl_Tooltip::size( 14 );
    Fl_Tooltip::hoverdelay( 0.1f );

//    Fl::visible_focus( 0 );

    LOG_REGISTER_CREATE( Mixer_Strip );
    LOG_REGISTER_CREATE( Chain );
    LOG_REGISTER_CREATE( Plugin_Module );
    LOG_REGISTER_CREATE( Gain_Module );
    LOG_REGISTER_CREATE( Meter_Module );
    LOG_REGISTER_CREATE( JACK_Module );
    LOG_REGISTER_CREATE( Meter_Indicator_Module );
    LOG_REGISTER_CREATE( Controller_Module );

    init_boxtypes();

    signal( SIGPIPE, SIG_IGN );

    Fl::get_system_colors();
    Fl::scheme( "plastic" );
//    Fl::scheme( "gtk+" );

/*     Fl::foreground( 0xFF, 0xFF, 0xFF ); */
/*     Fl::background( 0x10, 0x10, 0x10 ); */

    MESSAGE( "Initializing JACK" );

    engine = new Engine();


    engine->init( "Non-Mixer" );

    Fl_Single_Window *o = main_window = new Fl_Single_Window( 1024, 768, "Mixer" );
    {
        Fl_Widget *o = mixer = new Mixer( 0, 0, main_window->w(), main_window->h(), NULL );
        Fl_Group::current()->resizable(o);
    }
    o->end();

    o->show( argc, argv );

    {
        engine->lock();

        if ( argc > 1 )
        {
/*             char name[1024]; */

/*             snprintf( name, sizeof( name ), "%s/history", argv[1] ); */

/*             Loggable::open( name ); */
            MESSAGE( "Loading \"%s\"", argv[1] );

            if ( int err = Project::open( argv[1] ) )
                {
                    fl_alert( "Error opening project specified on commandline: %s", Project::errstr( err ) );
                }
        }
        else
        {
            WARNING( "Running without a project--nothing will be saved." );
        }

        engine->unlock();
    }

    Fl::run();

    delete engine;

    MESSAGE( "Your fun is over" );

}
