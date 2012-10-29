
/*******************************************************************************/
/* Copyright (C) 2012 Jonathan Moore Liles                                     */
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
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Single_Window.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Choice.H>
#include <FL/fl_draw.H>
#include <sys/time.h>
#include <stdio.h>

static Fl_Boxtype boxtype = FL_UP_BOX;

#include <unistd.h>

unsigned long long tv_to_ts ( timeval *tv )
{
    return tv->tv_sec * 1e6 + tv->tv_usec;
}

unsigned long long get_ts ( void )
{
    struct timeval then;
    gettimeofday( &then, NULL );

    return tv_to_ts( &then );
}

class PerfTest : public Fl_Widget
{
public:

    PerfTest ( int X, int Y, int W, int H, const char *L=0 ) : Fl_Widget( X, Y, W, H, L )
        {
            align(FL_ALIGN_TOP | FL_ALIGN_RIGHT |FL_ALIGN_INSIDE);
            box(FL_UP_BOX);
            labelcolor( FL_WHITE );
            use_cairo = false;
        }

    
    bool use_cairo;

   void draw ( void )
        {          
            if ( use_cairo )
                fl_push_use_cairo(true);

            fl_rectf( x(), y(), w(), h(), FL_BLACK );

            unsigned long long then = get_ts();

            fl_push_clip( x(), y(), w(), h() );

            int count = 400;

            /* draw stuff */
            int i = 0;
            for ( ; i < count; ++i )
                fl_draw_box( boxtype, x(), y(), w(), h(), fl_lighter( FL_BLACK ) );

            fl_pop_clip();

            unsigned long long now = get_ts();

            double elapsedms = (now - then) / 1000.0;

            static char text[256];
            sprintf( text, "Drew %i boxes in in %fms", i, elapsedms );

            fl_color( FL_RED );
            fl_draw( text, x(), y(), w(), h(), FL_ALIGN_CENTER | FL_ALIGN_INSIDE );

            draw_label();

            if ( use_cairo )
                fl_pop_use_cairo();
        }
};


void
boxtype_cb ( Fl_Widget *w, void *v )
{
    const char *picked = ((Fl_Choice*)w)->mvalue()->label();

    if ( !strcmp( picked, "UP_BOX" ) )
        boxtype = FL_UP_BOX;
    else if ( !strcmp( picked, "FLAT_BOX" ) )
        boxtype = FL_FLAT_BOX;
    else if ( !strcmp( picked, "ROUNDED_BOX" ) )
        boxtype = FL_ROUNDED_BOX;
    else if ( !strcmp( picked, "OVAL_BOX" ) )
        boxtype = FL_OVAL_BOX;
    
    w->window()->redraw();
}

int
main ( int argc, char **argv )
{
    {
    Fl_Single_Window *w = new Fl_Single_Window( 800, 600 );
    
    { Fl_Choice *o = new Fl_Choice( 0, 0, 200, 24, "Boxtype" );
        o->align( FL_ALIGN_RIGHT );

        o->callback( boxtype_cb, NULL );

        o->add( "UP_BOX" );
        o->add( "FLAT_BOX" );
        o->add( "ROUNDED_BOX" );
        o->add( "OVAL_BOX" );
    }

    {
        Fl_Pack *o = new Fl_Pack( 0, 24, 800, 600 - 24 );
        o->type( 0 );
        
        {
            PerfTest *o = new PerfTest( 0,0, 800, 400, "Xlib" );
        }

        {
            PerfTest *o = new PerfTest( 0,0, 800, 400, "Cairo" );
            o->use_cairo = true;
        }

        o->end();
    }
    
    w->end();
    w->show();
    }

    /* { */
    /* Fl_Single_Window *w = new Fl_Single_Window( 800, 600 ); */

    /* PerfTest *o = new PerfTest( 0,0, 800, 600 ); */
    
    /* w->end(); */
    /* w->show(); */
    /* } */

    Fl::run();
}
