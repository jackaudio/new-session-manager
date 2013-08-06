
/*******************************************************************************/
/* Copyright (C) 2013 Jonathan Moore Liles                                     */
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

#include <FL/fl_draw.H>
#include "AUX_Module.H"
#include "dsp.h"

/* The purpose of this module is to provide auxiliary outputs, with
 * gain. This allows one to create a 'send' type topology without
 * having to use an extra strip to control the 'send' gain.  */

AUX_Module::AUX_Module ( ) : JACK_Module ( false )
{
    is_default( false );

    _number = 0;

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

    log_create();

    color( FL_DARK1 );

    copy_label( "Aux" );

    smoothing.sample_rate( sample_rate() );
}

AUX_Module::~AUX_Module ( )
{
    configure_outputs(0);
    delete (float*)control_input[0].buffer();
}



void
AUX_Module::get ( Log_Entry &e ) const
{
    e.add( ":number", number() );
    JACK_Module::get(e);
}

void
AUX_Module::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! ( strcmp( s, ":number" ) ) )
        {
            number( atoi(v) );
        }
    }

    JACK_Module::set(e);
}

void
AUX_Module::number ( int n )
{
    _number = n;

    char s[10];
    snprintf( s, sizeof(s), "aux-%c", 'A' + n ); 
    prefix( s );
    snprintf( s, sizeof(s), "Aux (%c)", 'A' + n ); 
    copy_label( s );
}

void
AUX_Module::handle_sample_rate_change ( nframes_t n )
{
    smoothing.sample_rate( n );
}

void
AUX_Module::process ( nframes_t nframes )
{
    if ( !bypass() )
    {
        float gt = DB_CO( control_input[0].control_value() );
 
        sample_t gainbuf[nframes];
    
        bool use_gainbuf = smoothing.apply( gainbuf, nframes, gt );

        if ( use_gainbuf )
        {
        
            for ( unsigned int i = 0; i < audio_input.size(); ++i )
            {
                if ( audio_input[i].connected() )
                    buffer_copy_and_apply_gain_buffer( (sample_t*)aux_audio_output[i].buffer(), (sample_t*)audio_input[i].buffer(), gainbuf, nframes );
            }

        }
        else
        {
            for ( unsigned int i = 0; i < audio_input.size(); ++i )
            {
                if ( audio_input[i].connected() )
                    buffer_copy_and_apply_gain( (sample_t*)aux_audio_output[i].buffer(), (sample_t*)audio_input[i].buffer(), nframes, gt );
            }
        }
    }
    else
    {
        for ( unsigned int i = 0; i < audio_input.size(); ++i )
        {
            if ( audio_input[i].connected() )
                buffer_fill_with_silence( (sample_t*)aux_audio_output[i].buffer(), nframes );
        }
    }
}

void
AUX_Module::draw ( void )
{
    int W = 5;

    child(0)->size( w() - W, h() );
    Module::draw_box(x(),y(),w() - W,h());
    Module::draw_label(x(),y(),w() - W,h());

    Module *m = this;
    
    fl_color( fl_darker( FL_FOREGROUND_COLOR ) );

    int spacing, offset;

    spacing = h() / m->ninputs();
    offset = spacing / 2;
    for ( int i = m->ninputs(); i--; )
    {
        int xi = offset + ( spacing * i );
        fl_rectf( m->x() + m->w() - W, m->y() + xi, W, 2 );
    }
}

bool
AUX_Module::configure_outputs ( int n )
{
  int on = audio_output.size();

    if ( n > on )
    {
        for ( int i = on; i < n; ++i )
        {
            add_port( Port( this, Port::OUTPUT, Port::AUDIO ) );
        }
    }
    else
    {
        for ( int i = on; i > n; --i )
        {
            audio_output.back().disconnect();
            audio_output.pop_back();
        }
    }

    return true;
}

bool
AUX_Module::configure_inputs ( int n )
{
    bool b = JACK_Module::configure_inputs( n );

    if ( b )
    {
        return configure_outputs( n );
    }

    return false;
}
