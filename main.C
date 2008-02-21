
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

#include "const.h"


Timeline timeline;


void
cb_scroll ( Fl_Widget *w, void *v )
{
    Scalebar *sb = (Scalebar*)w;

    timeline.fpp = sb->zoom() * 256;
    timeline.fpp = max( min( timeline.fpp, 4096.0f ), (float)2 );

    int maxx = timeline.ts_to_x( timeline.length );
    sb->range( 0, maxx );
//    sb->value( sb->value(), maxx
//    sb->slider_size( sb->w() / maxx );
//    ((Fl_Scrollbar*)sb)->value( sb->value(), 60, 10, maxx );

    timeline.xoffset = timeline.x_to_ts( sb->value() );
    //  timeline.tracks->redraw();
    timeline.scroll->redraw();

    printf( "%lu\n", timeline.xoffset );

    for ( int i = timeline.tracks->children(); i-- ; )
    {
        Fl_Group *track = (Fl_Group*)timeline.tracks->child( i );
        track->damage( FL_DAMAGE_SCROLL );
    }

/* /\*         for ( int j = track->children(); j-- ; ) *\/ */
/* /\*             ((Region*)(track->child( j )))->resize(); *\/ */
/* /\*     } *\/ */





}


int
main ( int argc, char **argv )
{

    Fl_Double_Window *main_window = new Fl_Double_Window( 0, 0, 800, 600 );

    timeline.scroll = new Fl_Scroll( 0, 0, 800, 600 - 24 );
    timeline.scroll->type( Fl_Scroll::VERTICAL );
    timeline.fpp = 256;
    timeline._beats_per_minute = 120;
    timeline.length = 48000 * 60 * 2;


    timeline.beats_per_minute( 0, 120 );
    timeline.beats_per_minute( 48000, 250 );
    timeline.beats_per_minute( 48000 * 4, 60 );


    timeline.sample_rate = 44100;

    timeline.tracks = new Fl_Pack( 0, 0, 800, 5000 );
    timeline.tracks->type( Fl_Pack::VERTICAL );
    timeline.tracks->spacing( 20 );

    Fl::get_system_colors();
    Fl::scheme( "plastic" );

//    Fl_Group *pack = new Fl_Group( 0, 0, 5000, 600 );

    {
        Track *tempo_track = new Track( 100, 0, 800, 24 );

        tempo_track->label( "tempo map" );
        tempo_track->add( new Tempo_Point( 0, 120 ) );

        tempo_track->add( new Tempo_Point( 56000, 250 ) );

        tempo_track->end();
    }

    Track *track1 = new Audio_Track( 40, 0, 800, 100 );

//    pack->type( Fl_Pack::VERTICAL );
//    pack->box( FL_DOWN_BOX );

    //  Region *wave = new Region( 0, 0, 5000, 100, "foo" );

    Region *wave = new Region( Clip::from_file( "streambass8.wav" ) );

//    wave->resize( 0, 0, 500, 100 );

    wave->offset( 1024 );
    wave->end( 3000 );

/*     wave->color( FL_CYAN ); */
/*     wave->selection_color( fl_darker( FL_GRAY ) ); */
/*     wave->selection_color( FL_GREEN ); */

    track1->add( wave );

    Region *wave3 = new Region( *wave );
    wave3->offset( 4000 );
    track1->add( wave3 );

    track1->end();

    Track *track2 = new Audio_Track( 40, 0, 5000, 100 );

    // Region *wave2 = new Region( 0, 0, 350, 100, "bar" );

    Region *wave2 = new Region( *wave );

/*     wave2->peaks( peaks ); */
/*     wave2->start( 300 ); */
/*     wave2->end( len ); */

    track2->add( wave2 );


    track2->end();

    track1->next( track2 );
    track2->prev( track1 );

    timeline.tracks->end();
    timeline.scroll->end();

    timeline.scrollbar = new Scalebar( 0, 600 - 24, 800, 24 );
    timeline.scrollbar->range( 0, 48000 * 2 );
    timeline.scrollbar->type( 1 );
    timeline.scrollbar->callback( cb_scroll, 0 );

    main_window->end();
    main_window->show();

/*     wave->redraw(); */

    Fl::run();
}
