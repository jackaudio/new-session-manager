
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

#include "const.h"

#include <string.h>

#include <FL/fl_ask.H>

#include "dsp.h"

#include "Engine/Engine.H"
#include "Chain.H"

#include "JACK_Module.H"



JACK_Module::JACK_Module ( )
    : Module ( 50, 24, name() )
{
    /* FIXME: how do Controls find out that a connected value has changed? How does this work in ladspa? */
    {
        Port p( this, Port::INPUT, Port::CONTROL, "Inputs" );
        p.hints.type = Port::Hints::INTEGER;
        p.hints.minimum = 0;
        p.hints.maximum = 6;
        p.hints.ranged = true;

        p.connect_to( new float );
        p.control_value_no_callback( 0 );

        add_port( p );
    }

    {
        Port p( this, Port::INPUT, Port::CONTROL, "Outputs" );
        p.hints.type = Port::Hints::INTEGER;
        p.hints.minimum = 0;
        p.hints.maximum = 6;
        p.hints.ranged = true;

        p.connect_to( new float );
        p.control_value_no_callback( 0 );

        add_port( p );
    }

    end();

    log_create();
}

JACK_Module::~JACK_Module ( )
{
    log_destroy();
    configure_inputs( 0 );
    configure_outputs( 0 );
}



int
JACK_Module::can_support_inputs ( int n )
{
    return audio_output.size();
}

bool
JACK_Module::configure_inputs ( int n )
{
    int on = audio_input.size();

    if ( n > on )
    {
        for ( int i = on; i < n; ++i )
        {
            JACK::Port po( chain()->engine(), JACK::Port::Output, i );

            if ( ! po.activate() )
            {
                jack_port_activation_error( &po );
                return false;
            }

            if ( po.valid() )
            {
                add_port( Port( this, Port::INPUT, Port::AUDIO ) );
                jack_output.push_back( po );
            }
        }
    }
    else
    {
        for ( int i = on; i > n; --i )
        {
            audio_input.back().disconnect();
            audio_input.pop_back();
            jack_output.back().shutdown();
            jack_output.pop_back();
        }
    }

    control_input[0].control_value_no_callback( n );

    return true;
}

void
JACK_Module::jack_port_activation_error ( JACK::Port *p )
{
    fl_alert( "Could not activate JACK port \"%s\"", p->name() );
}

bool
JACK_Module::configure_outputs ( int n )
{
   int on = audio_output.size();

    if ( n > on )
    {
        for ( int i = on; i < n; ++i )
        {
            JACK::Port po( chain()->engine(), JACK::Port::Input, i );

            if ( ! po.activate() )
            {
                jack_port_activation_error( &po );
                return false;
            }

            if ( po.valid() )
            {
                add_port( Port( this, Port::OUTPUT, Port::AUDIO ) );
                jack_input.push_back( po );
            }
        }
    }
    else
    {
        for ( int i = on; i > n; --i )
        {
            audio_output.back().disconnect();
            audio_output.pop_back();
            jack_input.back().shutdown();
            jack_input.pop_back();
        }
    }

    control_input[1].control_value_no_callback( n );

    return true;
}

bool
JACK_Module::initialize ( void )
{
    return true;
}

void
JACK_Module::handle_control_changed ( Port *p )
{
    THREAD_ASSERT( UI );

    if ( 0 == strcmp( p->name(), "Inputs" ) )
    {
        DMESSAGE( "Adjusting number of inputs (JACK outputs)" );
        configure_inputs( p->control_value() );
        if ( chain() )
            chain()->configure_ports();
    }
    else if ( 0 == strcmp( p->name(), "Outputs" ) )
    {
        DMESSAGE( "Adjusting number of outputs (JACK inputs)" );

        if ( ! chain() )
        {
            configure_outputs( p->control_value() );
        }
        else if ( chain()->can_configure_outputs( this, p->control_value() ) )
        {
            configure_outputs( p->control_value() );
            chain()->configure_ports();
        }
        else
        {
            p->connected_port()->control_value( noutputs() );
        }
    }
}

void
JACK_Module::handle_chain_name_changed ( void )
{
    for ( unsigned int i = 0; i < jack_output.size(); ++i )
        jack_output[ i ].name( NULL, i  );

    for ( unsigned int i = 0; i < jack_input.size(); ++i )
        jack_input[ i ].name( NULL, i );
}



/**********/
/* Engine */
/**********/

void
JACK_Module::process ( nframes_t nframes )
{
    for ( unsigned int i = 0; i < audio_input.size(); ++i )
        if ( audio_input[i].connected() )
            buffer_copy( (sample_t*)jack_output[i].buffer( nframes ), (sample_t*)audio_input[i].buffer(), nframes );

    for ( unsigned int i = 0; i < audio_output.size(); ++i )
        if ( audio_output[i].connected() )
            buffer_copy( (sample_t*)audio_output[i].buffer(), (sample_t*)jack_input[i].buffer( nframes ), nframes );
}
