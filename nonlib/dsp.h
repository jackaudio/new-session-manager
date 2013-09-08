
/*******************************************************************************/
/* Copyright (C) 2008 Jonathan Moore Liles                                     */
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

#pragma once

#include "JACK/Client.H"
#include <math.h>


sample_t *buffer_alloc ( nframes_t size );
void buffer_apply_gain ( sample_t *buf, nframes_t nframes, float g );
void buffer_apply_gain_unaligned ( sample_t *buf, nframes_t nframes, float g );
void buffer_apply_gain_buffer ( sample_t *buf, const sample_t *gainbuf, nframes_t nframes );
void buffer_copy_and_apply_gain_buffer ( sample_t *dst, const sample_t *src, const sample_t *gainbuf, nframes_t nframes );
void buffer_mix ( sample_t *dst, const sample_t *src, nframes_t nframes );
void buffer_mix_with_gain ( sample_t *dst, const sample_t *src, nframes_t nframes, float g );
void buffer_interleave_one_channel ( sample_t *dst, const sample_t *src, int channel, int channels, nframes_t nframes );
void buffer_interleave_one_channel_and_mix ( sample_t *dst, const sample_t *src, int channel, int channels, nframes_t nframes );
void buffer_deinterleave_one_channel ( sample_t *dst, const sample_t *src, int channel, int channels, nframes_t nframes );
void buffer_interleaved_mix ( sample_t *__restrict__ dst, const sample_t * __restrict__ src, int dst_channel, int src_channel, int dst_channels, int src_channels, nframes_t nframes );
void buffer_interleaved_copy ( sample_t *__restrict__ dst, const sample_t * __restrict__ src, int dst_channel, int src_channel, int dst_channels, int src_channels, nframes_t nframes );
void buffer_fill_with_silence ( sample_t *buf, nframes_t nframes );
bool buffer_is_digital_black ( const sample_t *buf, nframes_t nframes );
float buffer_get_peak ( const sample_t *buf, nframes_t nframes );
void buffer_copy ( sample_t *dst, const sample_t *src, nframes_t nframes );
void buffer_copy_and_apply_gain ( sample_t *dst, const sample_t *src, nframes_t nframes, float gain );

class Value_Smoothing_Filter
{
    float w, g1, g2;
    
    float _cutoff;

public:

    Value_Smoothing_Filter ( )
    {
        g1 = g2 = 0;
        _cutoff = 10.0f;
    }

    void cutoff ( float v ) { _cutoff = v; }

    void sample_rate ( nframes_t v );
    
    inline bool target_reached ( float gt ) const { return gt == g2; }
 
    bool apply ( sample_t *dst, nframes_t nframes, float target );

};

static inline float interpolate_cubic ( const float fr, const float inm1, const float in, const float inp1, const float inp2)
{
    return in + 0.5f * fr * (inp1 - inm1 +
                             fr * (4.0f * inp1 + 2.0f * inm1 - 5.0f * in - inp2 +
                                   fr * (3.0f * (in - inp1) - inm1 + inp2)));
}

// from SWH plugins.
// Convert a value in dB's to a coefficent
#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)
#define CO_DB(v) (20.0f * log10f(v))

#define DEG2RAD       0.01745329251f
#define ONEOVERSQRT2  0.70710678118f

#ifndef likely
#define likely(x) __builtin_expect(x,1)
#endif
#ifndef unlikely
#define unlikely(x) __builtin_expect(x,0)
#endif
