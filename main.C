
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

void
cb_scroll ( Fl_Widget *w, void *v )
{
    Scalebar *sb = (Scalebar*)w;

    if ( sb->zoom_changed() )
    {
//        timeline->fpp = sb->zoom() * 256;
        timeline->fpp = sb->zoom() * 1;

/*         timeline->fpp = max( min( timeline->fpp, 4096.0f ), (float)2 ); */

        int maxx = timeline->ts_to_x( timeline->length );
        sb->range( 0, maxx );

//        timeline->redraw();
        timeline->redraw();
    }
    else
    {
        timeline->position( sb->value() );
    }

/*     timeline->xoffset = timeline->x_to_ts( sb->value() ); */
/*     //  timeline->tracks->redraw(); */
/*     timeline->scroll->redraw(); */

    printf( "%lu\n", timeline->xoffset );

/*     for ( int i = timeline->tracks->children(); i-- ; ) */
/*     { */
/*         Fl_Group *track = (Fl_Group*)timeline->tracks->child( i ); */
/*         track->damage( FL_DAMAGE_SCROLL ); */
/*     } */

/*     for ( int i = timeline->rulers->children(); i-- ; ) */
/*     { */
/*         Fl_Group *track = (Fl_Group*)timeline->rulers->child( i ); */
/*         track->damage( FL_DAMAGE_SCROLL ); */
/*     } */



/* /\*         for ( int j = track->children(); j-- ; ) *\/ */
/* /\*             ((Region*)(track->child( j )))->resize(); *\/ */
/* /\*     } *\/ */





}


int
main ( int argc, char **argv )
{
    Fl_Window *main_window = new Fl_Window( 0, 0, 800, 600 );

    Fl::get_system_colors();
    Fl::scheme( "plastic" );

    Loggable::open( "history" );

    timeline = new Timeline( 0, 0, 800, 600, "Timeline" );

//    Region *wave = new Region( Clip::from_file( "streambass8.wav" ) );

/*     track1->next( track2 ); */
/*     track2->prev( track1 ); */

    timeline->scrollbar = new Scalebar( 0, 600 - 24, 800, 24 );
    timeline->scrollbar->range( 0, 48000 * 2 );
    timeline->scrollbar->zoom_range( 2, 8192 );
    timeline->scrollbar->zoom( 256 );
    timeline->scrollbar->type( 1 );
    timeline->scrollbar->callback( cb_scroll, 0 );

    main_window->end();
    main_window->show();

/*     wave->redraw(); */

    Fl::run();
}
