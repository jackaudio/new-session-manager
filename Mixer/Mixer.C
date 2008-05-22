
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

Fl_Single_Window *main_window;

#include <FL/Boxtypes.H>

int
main ( int argc, char **argv )
{
    Fl::get_system_colors();
    Fl::scheme( "plastic" );

    init_boxtypes();

    Fl_Pack * mixer_strips;

    Fl_Single_Window *o = main_window = new Fl_Single_Window( 1024, 768 );
    {
        Fl_Scroll *o = new Fl_Scroll( 0, 0, main_window->w(), main_window->h() );
        main_window->resizable( o );
        {
            Fl_Pack *o = mixer_strips = new Fl_Pack( 0, 0, 1, main_window->h() );
            o->type( Fl_Pack::HORIZONTAL );
            {
                for ( int i = 16; i-- ; )
                    new Mixer_Strip( 0, 0, 120, main_window->h() + 150 );
            }
            o->end();
        }
        o->end();
    }
    o->end();
    o->show( argc, argv );

    while ( 1 )
    {

        for ( int i = mixer_strips->children(); i--; )
        {
            Meter_Pack *mp = (Meter_Pack*)((Mixer_Strip*) mixer_strips->child( i ))->meters_pack;

            for ( int j = mp->channels(); j-- ; )
            {
                Meter *o = mp->channel( j );

                float v = o->value();

                float r = ((rand() / (float)RAND_MAX) - 0.5f) * 10.0f;

                v += r;

                if ( v > 4.0f ) v = 0.0f;
                if ( v < -80.0f ) v = 0.0f;

                o->value( v );
            }

        }

        Fl::wait( 0.02f );
/*         Fl::check(); */
/*         usleep( 50000 ); */

    }

//    Fl::run();
}
