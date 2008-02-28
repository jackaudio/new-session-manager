
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

#include "Track.H"
#include "Audio_Track.H"
#include "Timeline.H"
#include "Tempo_Track.H"
#include "Time_Track.H"

#include "Loggable.H"

#include "const.h"

Timeline *timeline;

void cb_undo ( Fl_Widget *w, void *v )
{
/*     static char pat[20]; */


    Loggable::undo();

/*     sprintf( pat, "undo %d", Loggable::undo_index() ); */
/*     w->label( pat ); */

}

int
main ( int argc, char **argv )
{
    Fl_Window *main_window = new Fl_Window( 0, 0, 800, 600 );

    Fl::get_system_colors();
    Fl::scheme( "plastic" );

    Loggable::open( "history" );
    Loggable::register_create( "Region",      &Region::create      );
    Loggable::register_create( "Tempo_Point", &Tempo_Point::create );
    Loggable::register_create( "Time_Point",  &Time_Point::create  );


    timeline = new Timeline( 0, 24, 800, 600 - 24, "Timeline" );

//    Region *wave = new Region( Clip::from_file( "streambass8.wav" ) );

/*     track1->next( track2 ); */
/*     track2->prev( track1 ); */

//    timeline->scrollbar = new Scalebar( 0, 600 - 24, 800, 24 );

    Fl_Button *o = new Fl_Button( 0, 0, 50, 24, "undo" );
    o->shortcut( FL_CTRL + 'z' );
    o->callback( cb_undo, 0 );

    main_window->end();
    main_window->show();

/*     wave->redraw(); */

    Fl::run();
}
