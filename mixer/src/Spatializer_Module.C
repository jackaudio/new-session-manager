
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
#include <FL/Fl_Box.H>
#include "Spatializer_Module.H"
#include "dsp.h"

static const float max_distance = 15.0f;
static const float HIGHPASS_FREQ = 200.0f;
//static const float LOWPASS_FREQ = 70000.0f;
static const float LOWPASS_FREQ = 22000.0f;

#include <math.h>

class filter 
{
protected:

    float _sample_rate;
    float _w;
    float _last_output;
    float _last_cutoff;
    float _amount_of_current;
    float _amount_of_last;
    bool _bypass;

    void recalculate ( float cutoff  )
        {
            _last_cutoff = cutoff;

            if (_last_cutoff <= 10 )
            {
                _bypass = true;
            }
            else if (_last_cutoff > _sample_rate * 0.5f )
            {
                _bypass = true;
            }
            else
            {
                const float c = 2.0f - cosf(_w * _last_cutoff);
                _amount_of_last = c - sqrtf(c * c - 1.0f);
                _amount_of_current = 1 - _amount_of_last;

                _bypass = false;
            }
        }

public:
    
    void sample_rate ( nframes_t srate )
        {
            _sample_rate = srate;
            _w = (2 * M_PI) / (float)srate;
        }

    filter ()
        {
            _last_output = 0;
            _last_cutoff = 0;
            _w = 0;
            _sample_rate = 0;
            _amount_of_current = 0;
            _amount_of_last = 0;
            _bypass = false;
        }

    
    void
    run_lowpass ( float *buf, float cutoff, nframes_t nframes )
        {
            if (cutoff != _last_cutoff) 
            {
                recalculate( cutoff );
            }
            
            if ( !_bypass )
            {
                while ( nframes-- )
                {
                    *buf = _last_output = (_amount_of_current * *buf + _amount_of_last * _last_output);
                    buf++;
                }
            }
        }
    
    void
    run_highpass ( float *buf, float cutoff, nframes_t nframes )
        {
            if (cutoff != _last_cutoff) 
            {
                recalculate( cutoff );
            }
            
            if ( !_bypass )
            {
                while ( nframes-- )
                {
                    _last_output = ((_amount_of_current * *buf) + (_amount_of_last * _last_output));
                    *buf = *buf - _last_output;
                    buf++;
                }
            }
        }

};

class delay
{
    unsigned int _sample_rate;
    float *_buffer;
    long _write_index;
    unsigned int _buffer_mask;
    float _max_delay;
    nframes_t _samples_since_motion;
    nframes_t _interpolation_delay_samples;
    float _interpolation_delay_coeff;

public:

    void sample_rate ( float srate )
        {
            if ( _buffer )
                free( _buffer );

            unsigned int size, minsize;
            minsize = (unsigned long)(srate * _max_delay);

            size = 1;
            while (size < minsize)
                size <<= 1;

            _buffer = (float *)calloc(size, sizeof(float));

            _buffer_mask = size - 1;

            _sample_rate = srate;

            _write_index = 0;

            _interpolation_delay_samples = 0.2f * srate;
            _interpolation_delay_coeff = 1.0f / (float)_interpolation_delay_samples;
        }

    
    delay ( float max_delay )
        {
            _interpolation_delay_samples = 0;
            _interpolation_delay_coeff = 0;
            _samples_since_motion = 0;
            _max_delay = max_delay;
            _write_index = 0;
            _sample_rate = 0;
            _buffer = 0;
            _buffer_mask =0;
        }

    ~delay ( )
        {
            if ( _buffer )
                free( _buffer );
        }

    void run ( float *buf, float *delaybuf, float delay, nframes_t nframes )
        {
            const nframes_t min_delay_samples = 4;


            if ( delaybuf )
            {
                for (nframes_t i = 0; i < nframes; i++ ) 
                {
                    float delay_samples = delaybuf[i] * _sample_rate;

                    if ( delay_samples > _buffer_mask + 1 )
                        delay_samples = _buffer_mask;
                    else  if ( delay_samples < min_delay_samples )
                        delay_samples = min_delay_samples;

                    long idelay_samples = (long)delay_samples;
                    const float frac = delay_samples -  idelay_samples;
                    const long read_index = _write_index  - idelay_samples;

                    _buffer[_write_index++ & _buffer_mask] = buf[i];

                    const float read = interpolate_cubic (frac,
                                                          _buffer[(read_index-1) & _buffer_mask],
                                                          _buffer[read_index & _buffer_mask],
                                                          _buffer[(read_index+1) & _buffer_mask],
                                                          _buffer[(read_index+2) & _buffer_mask]);

                    buf[i] = read;
                }

                _samples_since_motion = 0;
            }
            else
            {
                float delay_samples = delay * _sample_rate;

                if ( delay_samples > _buffer_mask + 1 )
                    delay_samples = _buffer_mask;
                else  if ( delay_samples < min_delay_samples )
                    delay_samples = min_delay_samples;

                long idelay_samples = (long)delay_samples;
         
                if ( _samples_since_motion >= _interpolation_delay_samples )
                {
                    /* switch to non-interpolating mode */
                    for (nframes_t i = 0; i < nframes; i++ ) 
                    {
                        const long read_index = _write_index - idelay_samples;

                        _buffer[_write_index++ & _buffer_mask] = buf[i];
                        
                        const float read = _buffer[read_index & _buffer_mask];
                        
                        buf[i] = read;
                    }
                }
                else
                {
                    /* linearly interpolate our way to an integer sample delay */

                    float frac = delay_samples - idelay_samples;

                    const float scale = 1.0f - (_samples_since_motion * _interpolation_delay_coeff);

                    for (nframes_t i = 0; i < nframes; i++ ) 
                    {
                        const long read_index = _write_index - idelay_samples;

                        _buffer[_write_index++ & _buffer_mask] = buf[i];

                        frac *= scale;
                        
                        const float read = interpolate_cubic (frac,
                                                              _buffer[(read_index-1) & _buffer_mask],
                                                              _buffer[read_index & _buffer_mask],
                                                              _buffer[(read_index+1) & _buffer_mask],
                                                              _buffer[(read_index+2) & _buffer_mask]);
                        
                        buf[i] = read;
                    }

                    _samples_since_motion += nframes;
                }
            }
        }    
};

class ambisonic_panner 
{
    /* last values */
    float _x, _y, _z;

    /* for stereo */
    float _xr, _yr;
    
    static inline void spherical_to_cartesian (float a, float e, float &x, float &y, float &z )
        {
            a *= DEG2RAD;
            e *= DEG2RAD;
          
            z = sinf(e);
            const float ce = cosf(e);
            x = ce * cosf(-a);
            y = ce * sinf(-a);
        }

public:

    ambisonic_panner ( )
        {
            _x = _y = _z = _xr = _yr = 1.0f;
        }

    void
    run_mono ( float *in,
               float *out_w, float *out_x, float *out_y, float *out_z, 
               float a, float e, 
               nframes_t nframes )
        {
            float x = _x;
            float y = _y;
            float z = _z;

            spherical_to_cartesian( a, e, _x, _y, _z );

            const float c = 1.0f / (float)nframes;

            /* calculate increment for linear interpolation */
            const float dx = (_x - x) * c;
            const float dy = (_y - y) * c;
            const float dz = (_z - z) * c;

            while ( nframes-- )
            {
                x += dx;
                y += dy;
                z += dz;

                const float t = *in++;

                *out_w++ = ONEOVERSQRT2 * t;
                *out_x++ = x * t;
                *out_y++ = y * t;
                *out_z++ = z * t;
            }
        }

    void
    run_stereo ( float *in_l, float *in_r, 
                 float *out_w, float *out_x, float *out_y, float *out_z, 
                 float a, float e, float w,
                 nframes_t nframes )
        {
            float x = _x;
            float y = _y;
            float z = _z;
            float xr = _xr;
            float yr = _yr;

            w *= 0.5f;

            spherical_to_cartesian( a - w, e, _x, _y, _z );
            spherical_to_cartesian( a + w, e, _xr, _yr, _z );

            const float c = 1.0f / (float)nframes;

            /* calculate increment for linear interpolation */
            const float dx = (_x - x) * c;
            const float dy = (_y - y) * c;
            const float dz = (_z - z) * c;
            const float dxr = (_xr - xr) * c;
            const float dyr = (_yr - yr) * c;

            while ( nframes-- )
            {
                x += dx;
                y += dy;
                z += dz;
                xr += dxr;
                yr += dyr;

                const float L = *in_l++;
                const float R = *in_r++;

                const float LR = L + R;

                *out_w++ = ONEOVERSQRT2 * LR;
                *out_x++ = x * L + xr * R;
                *out_y++ = y * L + yr * R;
                *out_z++ = z * LR;
            }
        }
};



Spatializer_Module::Spatializer_Module ( ) : JACK_Module ( false )
{
    is_default( false );
    
    _panner = 0;

    {
        Port p( this, Port::INPUT, Port::CONTROL, "Azimuth" );
        p.hints.type = Port::Hints::LINEAR;
        p.hints.ranged = true;
        p.hints.minimum = -180.0f;
        p.hints.maximum = 180.0f;
        p.hints.default_value = 0.0f;
        
        p.connect_to( new float );
        p.control_value( p.hints.default_value );

        add_port( p );
    }

    {
        Port p( this, Port::INPUT, Port::CONTROL, "Elevation" );
        p.hints.type = Port::Hints::LINEAR;
        p.hints.ranged = true;
        p.hints.minimum = -90.0f;
        p.hints.maximum = 90.0f;
        p.hints.default_value = 0.0f;
        
        p.connect_to( new float );
        p.control_value( p.hints.default_value );

        add_port( p );
    }

    {
        Port p( this, Port::INPUT, Port::CONTROL, "Radius" );
        p.hints.type = Port::Hints::LINEAR;
        p.hints.ranged = true;
        p.hints.minimum = 0.0f;
        p.hints.maximum = max_distance;
        p.hints.default_value = 1.0f;
        
        p.connect_to( new float );
        p.control_value( p.hints.default_value );

        add_port( p );
    }

   
    {
        Port p( this, Port::INPUT, Port::CONTROL, "Highpass (Hz)" );
        p.hints.type = Port::Hints::LINEAR;
        p.hints.ranged = true;
        p.hints.minimum = 0.0f;
        p.hints.maximum = 600.0f;
        p.hints.default_value = 200.0f;
        
        p.connect_to( new float );
        p.control_value( p.hints.default_value );

        add_port( p );
    }
   
    {
        Port p( this, Port::INPUT, Port::CONTROL, "Width" );
        p.hints.type = Port::Hints::LINEAR;
        p.hints.ranged = true;
        p.hints.minimum = -90.0f;
        p.hints.maximum = 90.0f;
        p.hints.default_value = 90.0f;
        p.hints.visible = false;
        p.connect_to( new float );
        p.control_value( p.hints.default_value );

        add_port( p );
    }

    log_create();

    _panner = new ambisonic_panner();

    labelsize(9);

    color( FL_DARK1 );

    copy_label( "Spatializer" );
    align(FL_ALIGN_LEFT|FL_ALIGN_TOP|FL_ALIGN_INSIDE);

    gain_smoothing.sample_rate( sample_rate() );
    delay_smoothing.cutoff( 0.5f );
    delay_smoothing.sample_rate( sample_rate() );
}

Spatializer_Module::~Spatializer_Module ( )
{
    configure_inputs(0);
    delete _panner;
    delete (float*)control_input[0].buffer();
    delete (float*)control_input[1].buffer();
    delete (float*)control_input[2].buffer();
    delete (float*)control_input[3].buffer();
    delete (float*)control_input[4].buffer();
}




void
Spatializer_Module::handle_sample_rate_change ( nframes_t n )
{
    gain_smoothing.sample_rate( n );
    delay_smoothing.sample_rate( n );

    for ( unsigned int i = 0; i < audio_input.size(); i++ )
    {
        _lowpass[i]->sample_rate( n );
        _highpass[i]->sample_rate( n );
        _delay[i]->sample_rate( n );
    }
}

void
Spatializer_Module::draw ( void )
{
    int W = 5;

    child(0)->size( w() - W, h() );
    Module::draw_box(x(),y(),w() - W,h());
    Module::draw_label(x() + 4,y(),w() - W,h());

    Module *m = this;
    
    fl_color( fl_darker( FL_FOREGROUND_COLOR ) );

    int spacing, offset;

    int ni = aux_audio_output.size();
  
    spacing = h() / ni;
    offset = spacing / 2;
    for ( int i = ni; i--; )
    {
        int xi = offset + ( spacing * i );
        fl_rectf( m->x() + m->w() - W, m->y() + xi, W, 2 );
    }
}

void
Spatializer_Module::process ( nframes_t nframes )
{
    if ( !bypass() )
    {
        float azimuth = control_input[0].control_value();
        float elevation = control_input[1].control_value();
        float radius = control_input[2].control_value();
        float highpass_freq = control_input[3].control_value();
        float width = control_input[4].control_value();

        float delay_seconds = 0.0f;

        if ( radius > 1.0f )
            delay_seconds = ( radius - 1.0f ) / 340.29f;

        /* direct sound follows inverse square law */
        /* but it's just the inverse as far as SPL goes */
        
        /* let's not go nuts... */
        if ( radius < 0.01f )
            radius = 0.01f;

        float gain = 1.0f / radius;

        float cutoff_frequency = gain * LOWPASS_FREQ;

        sample_t gainbuf[nframes];
        sample_t delaybuf[nframes];
        
        bool use_gainbuf = gain_smoothing.apply( gainbuf, nframes, gain );
        bool use_delaybuf = delay_smoothing.apply( delaybuf, nframes, delay_seconds );
       
        for ( unsigned int i = 0; i < audio_input.size(); i++ )
        {
            sample_t *buf = (sample_t*) audio_input[i].buffer();
            
            /* frequency effects */
            _highpass[i]->run_highpass( buf, highpass_freq, nframes );
            _lowpass[i]->run_lowpass( buf, cutoff_frequency, nframes );

            /* send to late reverb */
            if ( i == 0 )
                buffer_copy( (sample_t*)aux_audio_output[0].jack_port()->buffer(nframes), buf, nframes );
            else
                buffer_mix( (sample_t*)aux_audio_output[0].jack_port()->buffer(nframes), buf, nframes );

            /* /\* FIXME: use smoothed value... *\/ */
            /* buffer_apply_gain( (sample_t*)jack_output[0].buffer(nframes), nframes, 1.0f / sqrt(D) ); */

            if ( use_delaybuf )
                _delay[i]->run( buf, delaybuf, 0, nframes );
            else
                _delay[i]->run( buf, 0, delay_seconds, nframes );
        }

        if ( audio_input.size() == 1 )
        {
            _panner->run_mono( (sample_t*)audio_input[0].buffer(),
                               (sample_t*)audio_output[0].buffer(),
                               (sample_t*)audio_output[1].buffer(),
                               (sample_t*)audio_output[2].buffer(),
                               (sample_t*)audio_output[3].buffer(),
                               azimuth,
                               elevation,
                               nframes );
        }
        else
        {
            _panner->run_stereo( (sample_t*)audio_input[0].buffer(),
                                 (sample_t*)audio_input[1].buffer(),
                                 (sample_t*)audio_output[0].buffer(),
                                 (sample_t*)audio_output[1].buffer(),
                                 (sample_t*)audio_output[2].buffer(),
                                 (sample_t*)audio_output[3].buffer(),
                                 azimuth,
                                 elevation,
                                 width,
                                 nframes );
        }
  
        /* send to early reverb */
        for ( int i = 4; i--; )
            buffer_copy( (sample_t*)aux_audio_output[1 + i].jack_port()->buffer(nframes),
                         (sample_t*)audio_output[0 + i].buffer(),
                         nframes );

        /* gain effects */
        if ( use_gainbuf )
        {
            for ( int i = 4; i--; )
                buffer_apply_gain_buffer( (sample_t*)audio_output[i].buffer(), gainbuf, nframes );
        }
        else
        {
            for ( int i = 4; i--; )
                buffer_apply_gain( (sample_t*)audio_output[i].buffer(), nframes, gain );
        }
    }                
}

bool
Spatializer_Module::configure_inputs ( int n )
{
    output_connection_handle->show();
    output_connection_handle->tooltip( "Late Reverb" );
    output_connection2_handle->show();
    output_connection2_handle->tooltip( "Early Reverb" );

    int on = audio_input.size();

    if ( n > on )
    {
        for ( int i = n - on; i--; )
        { 
            {    filter *o = new filter();
                o->sample_rate( sample_rate() );
                _lowpass.push_back( o );
            }

            { 
                filter *o = new filter();
                o->sample_rate( sample_rate() );
                _highpass.push_back( o );
            }
            {
                delay *o = new delay( max_distance / 340.29f );
                o->sample_rate( sample_rate() );
                _delay.push_back( o );
            }

            add_port( Port( this, Port::INPUT, Port::AUDIO ) );
        }
    }
    else if ( n < on )
    {
        
        for ( int i = on - n; i--; )
        { 
            delete _lowpass.back();
            _lowpass.pop_back();
            delete _highpass.back();
            _highpass.pop_back();
            delete _delay.back();
            _delay.pop_back();

            audio_input.pop_back();
        }
    }

    control_input[4].hints.visible = audio_input.size() == 2;
     
    
    if ( n == 0 )
    {
        remove_aux_audio_outputs();
        audio_output.clear();
        audio_input.clear();
    }
    else
    {
        if ( audio_output.size() != 4 )
        {
            for ( int i = 0; i < 4; i++ )
            {
                add_port( Port( this, Port::OUTPUT, Port::AUDIO ) );
            }
        }

        if ( aux_audio_output.size() != 5 )
        {
            add_aux_audio_output( "late reverb", 0 );
            add_aux_audio_output( "early reverb", 0 );
            add_aux_audio_output( "early reverb", 1 );
            add_aux_audio_output( "early reverb", 2 );
            add_aux_audio_output( "early reverb", 3 );
        }
    }

    _connection_handle_outputs[0][0] = 0;
    _connection_handle_outputs[0][1] = 1;
    _connection_handle_outputs[1][0] = 1;
    _connection_handle_outputs[1][1] = aux_audio_output.size();

    return true;
}
