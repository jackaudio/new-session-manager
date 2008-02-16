
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
#include <FL/Fl_Pack.H>
#include <FL/Fl_Group.H>

#include "Waveform.H"
#include "Region.H"

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>

Fl_Color velocity_colors[128];

#include "Track.H"
#include "Timeline.H"

void
init_colors ( void )
{
    for ( int i = 128; i--; )
        velocity_colors[i] = fl_rgb_color( 23, 255 - i * 2, 32 );
}

Timeline timeline;

int
main ( int argc, char **argv )
{

    init_colors();

    Fl_Double_Window *main_window = new Fl_Double_Window( 0, 0, 800, 600 );

    timeline.scroll = new Fl_Scroll( 0, 0, 800, 600 );
    timeline.fpp = 100;

    Fl_Pack *tracks = new Fl_Pack( 0, 0, 5000, 5000 );
    tracks->type( Fl_Pack::VERTICAL );

    Fl::get_system_colors();
    Fl::scheme( "plastic" );

//    Fl_Group *pack = new Fl_Group( 0, 0, 5000, 600 );

    Track *track1 = new Track( 40, 0, 5000, 100 );

//    pack->type( Fl_Pack::VERTICAL );
//    pack->box( FL_DOWN_BOX );

    Region *wave = new Region( 0, 0, 5000, 100, "foo" );

    FILE *fp;

    fp = fopen( "peaks", "r" );

    struct stat st;

    fstat( fileno( fp ), &st );

    size_t len = st.st_size;

/*     float chunk_size; */
/*     fread( &chunk_size, sizeof( chunk_size ), 1, fp ); */

/*     printf( "%f\n", chunk_size ); */

    float *peaks = new float[ len / sizeof( float ) ];

    fread( peaks, len, 1, fp );

    wave->peaks( peaks );
    wave->start( 0 );
    wave->end( (len / sizeof( float )) / 2 );

    wave->color( FL_CYAN );
    wave->selection_color( fl_darker( FL_GRAY ) );
    wave->selection_color( FL_GREEN );

    track1->add( wave );

    track1->end();

    Track *track2 = new Track( 40, 0, 5000, 100 );

    // Region *wave2 = new Region( 0, 0, 350, 100, "bar" );

    Region *wave2 = new Region( *wave );

/*     wave2->peaks( peaks ); */
/*     wave2->start( 300 ); */
/*     wave2->end( len ); */

    track2->add( wave2 );

    track2->end();

    track1->next( track2 );
    track2->prev( track1 );

    tracks->end();
    timeline.scroll->end();

    main_window->end();
    main_window->show();

    wave->redraw();

    Fl::run();
}
