
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

#include "Mono_Pan_Module.H"

#include <math.h>
#include <dsp.h>

Mono_Pan_Module::Mono_Pan_Module ( )
    : Module ( 50, 24, name() )
{
    Port p( this, Port::INPUT, Port::CONTROL, "Pan" );
    p.hints.type = Port::Hints::LOGARITHMIC;
    p.hints.ranged = true;
    p.hints.minimum = -1.0f;
    p.hints.maximum = 1.0f;
    p.hints.default_value = 0.0f;

    p.connect_to( new float );
    p.control_value( p.hints.default_value );

    add_port( p );

    add_port( Port( this, Port::INPUT, Port::AUDIO ) );
    add_port( Port( this, Port::OUTPUT, Port::AUDIO ) );
    add_port( Port( this, Port::OUTPUT, Port::AUDIO ) );

    end();

    log_create();
}

Mono_Pan_Module::~Mono_Pan_Module ( )
{
    delete (float*)control_input[0].buffer();
    log_destroy();
}



bool
Mono_Pan_Module::configure_inputs ( int n )
{
    return true;
}



void
Mono_Pan_Module::process ( void )
{
    const float g = control_input[0].control_value();

// this is obviously wrong, but it produces a strange false stereo effect.
//    const float lg = (0.0f - g);
//    const float rg = g;

    const float lg = (0.0f - g) + 1.0f;
    const float rg = g + 1.0f;

    if ( audio_input[0].connected() &&
         audio_output[0].connected() &&
         audio_output[1].connected() )
    {
        buffer_copy_and_apply_gain( (sample_t*)audio_output[1].buffer(), (sample_t*)audio_input[0].buffer(), nframes(), rg );

        buffer_apply_gain( (sample_t*)audio_output[0].buffer(), nframes(), lg );
    }
}
