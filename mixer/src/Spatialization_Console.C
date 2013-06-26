
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



Spatialization_Console::Spatialization_Console ( void ) : Fl_Double_Window( 565, 565 )
{
    _resized = false;
    _min_width = 100;
  
    label( "Spatialization Console" );

    fl_font( FL_HELVETICA, 14 );

    panner = new Panner( 25,25, 512, 512 );

    panner->callback( cb_panner_value_handle, this );
    panner->when( FL_WHEN_CHANGED );

    end();

    make_controls();
}

Spatialization_Console::~Spatialization_Console ( )
{
//    controls_by_port.clear();
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
