
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

#include <math.h>
#include <dsp.h>

#include "Gain_Module.H"



Gain_Module::Gain_Module ( )
    : Module ( 50, 24, name() )
{
    add_port( Port( this, Port::INPUT, Port::AUDIO ) );
    add_port( Port( this, Port::OUTPUT, Port::AUDIO ) );

    {
        Port p( this, Port::INPUT, Port::CONTROL, "Gain (dB)" );
        p.hints.type = Port::Hints::LOGARITHMIC;
        p.hints.ranged = true;
        p.hints.minimum = -70.0f;
        p.hints.maximum = 6.0f;
        p.hints.default_value = 0.0f;

        p.connect_to( new float );
        p.control_value( p.hints.default_value );

        add_port( p );
    }

    {
        Port p( this, Port::INPUT, Port::CONTROL, "Mute" );
        p.hints.type = Port::Hints::BOOLEAN;
        p.hints.ranged = true;
        p.hints.minimum = 0.0f;
        p.hints.maximum = 1.0f;
        p.hints.default_value = 0.0f;

        p.connect_to( new float );
        p.control_value( p.hints.default_value );

        add_port( p );
    }

    end();

    log_create();

    smoothing.sample_rate( sample_rate() );
}

Gain_Module::~Gain_Module ( )
{
    delete (float*)control_input[0].buffer();
    delete (float*)control_input[1].buffer();
    log_destroy();
}



bool
Gain_Module::configure_inputs ( int n )
{
    audio_input.clear();
    audio_output.clear();
//    control_input.clear();

    for ( int i = 0; i < n; ++i )
    {
        add_port( Port( this, Port::INPUT, Port::AUDIO ) );
        add_port( Port( this, Port::OUTPUT, Port::AUDIO ) );
    }

    return true;
}

void
Gain_Module::handle_sample_rate_change ( nframes_t n )
{
    smoothing.sample_rate( n );
}



/**********/
/* Engine */
/**********/

void
Gain_Module::process ( nframes_t nframes )
{

    if ( unlikely( bypass() ) )
    {
        /* nothing to do */
    }
    else
    {
        const float gt = DB_CO( control_input[1].control_value() ? -90.f : control_input[0].control_value() );

        sample_t gainbuf[nframes];
    
        bool use_gainbuf = smoothing.apply( gainbuf, nframes, gt );
        
        if ( unlikely( use_gainbuf ) )
        {
            for ( int i = audio_input.size(); i--; )
            {
                if ( audio_input[i].connected() && audio_output[i].connected() )
                {
                    sample_t *out = (sample_t*)audio_input[i].buffer();

                    buffer_apply_gain_buffer( out, gainbuf, nframes );
                }
            }
        }
        else
            for ( int i = audio_input.size(); i--; )
            {
                if ( audio_input[i].connected() && audio_output[i].connected() )
                {
                    buffer_apply_gain( (sample_t*)audio_input[i].buffer(), nframes, gt );
                }
            }
    }
}
