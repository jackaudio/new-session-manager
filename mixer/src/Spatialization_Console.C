
/*******************************************************************************/
/* Copyright (C) 2009 Jonathan Moore Liles                                     */
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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <FL/fl_draw.H>

#include "Module.H"
#include "Spatialization_Console.H"
#include "Controller_Module.H"
#include "Chain.H"
#include "Panner.H"
#include "Mixer_Strip.H"
#include "Mixer.H"

#include "debug.h"
#include <FL/Fl_Menu_Bar.H>



Spatialization_Console::Spatialization_Console ( void ) : Fl_Double_Window( 850, 850 )
{
    _resized = false;
  
    label( "Spatialization Console" );

    labelfont( FL_HELVETICA );
    labelsize( 14 );

    int padding = 48;
    int S = 802;
    
    if ( fl_display )
        /* don't open the display in noui mode... */
    {
        int sx, sy, sw, sh;
        
        Fl::screen_xywh( sx, sy, sw, sh );

        if ( sw < 850 || sh < 850 )
        {
            /* if screen isn't big enough, use smaller version of control */
            S = 502;
        }
    }

    panner = new Panner( 25,25, S, S );
    panner->callback( cb_panner_value_handle, this );
    panner->when( FL_WHEN_CHANGED );

    size( S + padding, S + padding );

    callback( cb_window, this );
    end();

    make_controls();

    mixer->spatialization_console = this;
}

Spatialization_Console::~Spatialization_Console ( )
{
//    controls_by_port.clear();
    mixer->spatialization_console = NULL;
    
}


void 
Spatialization_Console::get ( Log_Entry &e ) const
{
    e.add( ":range", panner->range() );
    e.add( ":projection", panner->projection() );
    e.add( ":shown", ((const Fl_Double_Window*)this)->shown() );
}

void
Spatialization_Console::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! ( strcmp( s, ":range" ) ) )
            panner->range( atoi( v ) );
        if ( ! ( strcmp( s, ":projection" ) ) )
            panner->projection( atoi( v ) );
        else if ( ! ( strcmp( s, ":shown" ) ) )
        {
            if ( atoi( v ) )
            {
                if ( fl_display )
                {
                show();
                }
            }
            else
                hide();
        }
    }
}



void
Spatialization_Console::cb_window ( Fl_Widget *w, void *v )
{
    ((Spatialization_Console*)v)->cb_window(w);
}

void
Spatialization_Console::cb_window ( Fl_Widget *w )
{
    w->hide();
    mixer->update_menu();
}



void
Spatialization_Console::make_controls ( void )
{
    panner->clear_points();

    for ( int i = 0; i < mixer->nstrips(); i++ )
    {
        Mixer_Strip *o = mixer->track_by_number( i );
        
        if ( o->spatializer() )
        {
            Panner::Point p;

            p.color = o->color();
            p.userdata = o->spatializer();
            p.label = o->name();

            if ( o->spatializer()->is_controlling() )
            {
                p.visible = true;

                p.azimuth( o->spatializer()->control_output[0].control_value() );
                p.elevation( o->spatializer()->control_output[1].control_value() );
                if ( o->spatializer()->control_output[2].connected() )
                {
                    p.radius_enabled = true;
                    p.radius( o->spatializer()->control_output[2].control_value() );
                }
            }
            else
                p.visible = false;

            panner->add_point(p);
        }
    }

    panner->redraw();
}

void
Spatialization_Console::cb_panner_value_handle ( Fl_Widget *w, void *v )
{
//    callback_data *cd = (callback_data*)v;
    
    Spatialization_Console *sc = (Spatialization_Console*)v;

    Panner::Point *p = sc->panner->pushed();
 
    Controller_Module *cm = (Controller_Module*)p->userdata;

    cm->control_output[0].control_value( p->azimuth() );
    cm->control_output[1].control_value( p->elevation() );
    if ( p->radius_enabled )
        cm->control_output[2].control_value( p->radius() );
}

/* Display changes initiated via automation or from other parts of the GUI */
void
Spatialization_Console::handle_control_changed ( Controller_Module *m )
{
    if ( Fl::pushed() == panner )
        return;

    for ( int i = 0; i < panner->points(); i++ )
    {
        Panner::Point *p = panner->point(i);

        if ( p->userdata == m )
        {
            p->azimuth( m->control_output[0].control_value() );
            p->elevation( m->control_output[1].control_value() );
            if ( p->radius_enabled )
                p->radius( m->control_output[2].control_value() );

            if ( panner->visible_r() )
                panner->redraw();

            break;
        }
    }
}

void
Spatialization_Console::update ( void )
{
    make_controls();
}
