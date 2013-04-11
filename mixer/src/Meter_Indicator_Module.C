
/*******************************************************************************/
/* Copyright (C) 2010 Jonathan Moore Liles                                     */
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

#include "Meter_Indicator_Module.H"

#include <stdio.h>

#include <FL/Fl.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Light_Button.H>

#include "FL/Fl_Dial.H"
#include "FL/Fl_Labelpad_Group.H"
#include "FL/Fl_Scalepack.H"

#include "Engine/Engine.H"
#include "Chain.H"
#include "DPM.H"

#include "FL/test_press.H"



const float CONTROL_UPDATE_FREQ = 0.1f;



Meter_Indicator_Module::Meter_Indicator_Module ( bool is_default )
    : Module ( is_default, 50, 100, name() )
{
    box( FL_FLAT_BOX );
    color( FL_BLACK );

    _disable_context_menu = false;
    _pad = true;
    control_value = 0;

    add_port( Port( this, Port::INPUT, Port::CONTROL ) );

    dpm_pack = new Fl_Scalepack( x(), y(), w(), h() );
    dpm_pack->color( FL_BACKGROUND_COLOR );
    dpm_pack->box( FL_FLAT_BOX );
    dpm_pack->type( FL_HORIZONTAL );

    end();

    control_value = new float[1];
    *control_value = -70.0f;

    align( (Fl_Align)(FL_ALIGN_CENTER | FL_ALIGN_INSIDE ) );

    clear_visible_focus();

    Fl::add_timeout( CONTROL_UPDATE_FREQ, update_cb, this );
}

Meter_Indicator_Module::~Meter_Indicator_Module ( )
{
    if ( control_value )
    {
        delete[] control_value;
        control_value = NULL;
    }

    Fl::remove_timeout( update_cb, this );

    log_destroy();
}



void
Meter_Indicator_Module::get ( Log_Entry &e ) const
{

    Port *p = control_input[0].connected_port();
    Module *m = p->module();

    e.add( ":module", m );
    e.add( ":port", m->control_output_port_index( p ) );

    Module::get( e );
}

void
Meter_Indicator_Module::set ( Log_Entry &e )
{
    Module::set( e );

    int port = -1;
    Module *module = NULL;

    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":port" ) )
        {
            port = atoi( v );
        }
        else if ( ! strcmp( s, ":module" ) )
        {
            int i;
            sscanf( v, "%X", &i );
            Module *t = (Module*)Loggable::find( i );

            assert( t );

            module = t;
        }
    }

    if ( port >= 0 && module )
        control_input[0].connect_to( &module->control_output[port] );
}



void
Meter_Indicator_Module::update_cb ( void *v )
{
    ((Meter_Indicator_Module*)v)->update_cb();
}

void
Meter_Indicator_Module::update_cb ( void )
{
    Fl::repeat_timeout( CONTROL_UPDATE_FREQ, update_cb, this );

    if ( control_input[0].connected() )
    {
        // A little hack to detect that the connected module's number
        // of control outs has changed.
        Port *p = control_input[0].connected_port();

        if ( dpm_pack->children() != p->hints.dimensions )
        {
/*             engine->lock(); */

            dpm_pack->clear();

            control_value = new float[p->hints.dimensions];

            for ( int i = p->hints.dimensions; i--; )
            {

                DPM *dpm = new DPM( x(), y(), w(), h() );
                dpm->type( FL_VERTICAL );

                dpm_pack->add( dpm );

                control_value[i] = -70.0f;
                dpm->value( -70.0f );
            }

/*             engine->unlock(); */
        }
        else
        {
            for ( int i = 0; i < dpm_pack->children(); ++i )
            {
                ((DPM*)dpm_pack->child( i ))->value( control_value[i] );
            }
        }
    }
}

void
Meter_Indicator_Module::connect_to ( Port *p )
{
    control_input[0].connect_to( p );

    /* DPM *o = new DPM( 10, 10, 10, 10 ); */
    /* o->type( FL_VERTICAL ); */

    /* dpm_pack->add( o ); */

    redraw();
}



int
Meter_Indicator_Module::handle ( int m )
{
    switch ( m )
    {
        case FL_PUSH:
        {
            if ( Fl::event_button3() && _disable_context_menu )
                return 0;

            if ( Fl::event_button1() )
            {
                /* don't let Module::handle eat our click */
                return Fl_Group::handle( m );
            }
        }
    }

    return Module::handle( m );
}



void
Meter_Indicator_Module::handle_control_changed ( Port *p )
{
    THREAD_ASSERT( UI );

    /* The engine is already locked by the UI thread at this point in
     the call-graph, so we can be sure that process() won't be
     executed concurrently. */
    if ( p->connected() )
    {
        p = p->connected_port();

        if ( dpm_pack->children() != p->hints.dimensions )
        {
            dpm_pack->clear();

            control_value = new float[p->hints.dimensions];

            for ( int i = p->hints.dimensions; i--; )
            {
                DPM *dpm = new DPM( x(), y(), w(), h() );
                dpm->type( FL_VERTICAL );
                align( (Fl_Align)(FL_ALIGN_CENTER | FL_ALIGN_INSIDE ) );

                dpm_pack->add( dpm );
                dpm_pack->redraw();

                control_value[i] = -70.0f;
                dpm->value( -70.0f );
            }

            redraw();
        }
    }
}

/**********/
/* Engine */
/**********/

void
Meter_Indicator_Module::process ( nframes_t )
{
    if ( control_input[0].connected() )
    {
        Port *p = control_input[0].connected_port();

        for ( int i = 0; i < p->hints.dimensions; ++i )
	  {
            control_value[i] = ((float*)control_input[0].buffer())[i];
	  }
    }
}
