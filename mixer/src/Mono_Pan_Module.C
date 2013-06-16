
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

#include <math.h>
#include <dsp.h>

#include "Mono_Pan_Module.H"



Mono_Pan_Module::Mono_Pan_Module ( )
    : Module ( 50, 24, name() )
{
    Port p( this, Port::INPUT, Port::CONTROL, "Pan" );
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
 
    smoothing.sample_rate( sample_rate() );
}

Mono_Pan_Module::~Mono_Pan_Module ( )
{
    delete (float*)control_input[0].buffer();
    log_destroy();
}



void
Mono_Pan_Module::handle_sample_rate_change ( nframes_t n )
{
    smoothing.sample_rate( n );
}

bool
Mono_Pan_Module::configure_inputs ( int )
{
    return true;
}



/**********/
/* Engine */
/**********/

void
Mono_Pan_Module::process ( nframes_t nframes )
{

    if ( audio_input[0].connected() &&
         audio_output[0].connected() &&
         audio_output[1].connected() )
    {
        if ( bypass() )
        {
            buffer_copy( (sample_t*)audio_output[1].buffer(), (sample_t*)audio_input[0].buffer(), nframes );
        }
        else
        {
            const float gt = (control_input[0].control_value() + 1.0f) * 0.5f;

            if ( ! smoothing.target_reached( gt ) )
            {            
                sample_t gainbuf[nframes];            
            
                smoothing.apply( gainbuf, nframes, gt );
                
                /* right channel */
                
                buffer_copy_and_apply_gain_buffer( (sample_t*)audio_output[1].buffer(),
                                                   (sample_t*)audio_input[0].buffer(),
                                                   gainbuf,
                                                   nframes );
                
                /*  left channel  */
                for ( nframes_t i = 0; i < nframes; i++ )
                    gainbuf[i] = 1.0f - gainbuf[i];
                
                buffer_apply_gain_buffer( (sample_t*)audio_output[0].buffer(), gainbuf, nframes );
            }
            else
            {
                /* right channel */
                buffer_copy_and_apply_gain( (sample_t*)audio_output[1].buffer(),
                                                   (sample_t*)audio_input[0].buffer(),
                                                   nframes,
                                                   gt );
                
                /*  left channel  */
                buffer_apply_gain( (sample_t*)audio_output[0].buffer(), nframes, 1.0f - gt);
            }
        }
    }
}
