
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

#include <jack/jack.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "transport.H"
#include "common.h"
#include "const.h"

extern jack_client_t *client;

/* FIXME: use JackSyncCallback instead? (sync-callback) */

Transport transport;

static volatile bool _done;

/** callback for when we're Timebase Master, mostly taken from
 * transport.c in Jack's example clients. */
/* FIXME: there is a subtle interaction here between the tempo and
 * JACK's buffer size. Inflating ticks_per_beat (as jack_transport
 * does) diminishes the effect of this correlation, but does not
 * eliminate it... This is caused by the accumulation of a precision
 * error, and all timebase master routines I've examined appear to
 * suffer from this same tempo distortion (and all use the magic
 * number of 1920 ticks_per_beat in an attempt to reduce the magnitude
 * of the error. Currently, we keep this behaviour. */
void
Transport::timebase ( jack_transport_state_t, jack_nframes_t nframes, jack_position_t *pos, int new_pos, void * )
{

    if ( new_pos || ! _done )
    {
        pos->valid = JackPositionBBT;
        pos->beats_per_bar = transport._master_beats_per_bar;
        pos->ticks_per_beat = 1920.0;                           /* magic number means what? */
        pos->beat_type = transport._master_beat_type;
        pos->beats_per_minute = transport._master_beats_per_minute;

        double wallclock = (double)pos->frame / (pos->frame_rate * 60);

        unsigned long abs_tick = wallclock * pos->beats_per_minute * pos->ticks_per_beat;
        unsigned long abs_beat = abs_tick / pos->ticks_per_beat;

        pos->bar = abs_beat / pos->beats_per_bar;
        pos->beat = abs_beat - (pos->bar * pos->beats_per_bar) + 1;
        pos->tick = abs_tick - (abs_beat * pos->ticks_per_beat);
        pos->bar_start_tick = pos->bar * pos->beats_per_bar * pos->ticks_per_beat;
        pos->bar++;

        _done = true;
    }
    else
    {
        pos->tick += nframes * pos->ticks_per_beat * pos->beats_per_minute / (pos->frame_rate * 60);

        while ( pos->tick >= pos->ticks_per_beat )
        {
            pos->tick -= pos->ticks_per_beat;

            if ( ++pos->beat > pos->beats_per_bar )
            {
                pos->beat = 1;

                ++pos->bar;

                pos->bar_start_tick += pos->beats_per_bar * pos->ticks_per_beat;
            }
        }
    }
}


Transport::Transport ( void )
{
    _master_beats_per_bar    = 4;
    _master_beat_type        = 4;
    _master_beats_per_minute = 120;
    _done                    = false;
}

void
Transport::poll ( void )
{
    jack_transport_state_t ts;
    jack_position_t pos;

    ts = jack_transport_query( client, &pos );

    rolling = ts == JackTransportRolling;

    valid = pos.valid & JackPositionBBT;

    bar = pos.bar;
    beat = pos.beat;
    tick = pos.tick;

    /* bars and beats start at 1.. */
    pos.bar--;
    pos.beat--;

    /* FIXME: these probably shouldn't be called from the RT
       thread... Anyway, it happens infrequently. */
    if ( pos.beats_per_minute != beats_per_minute )
        signal_tempo_change( pos.beats_per_minute );

    if ( pos.beats_per_bar != beats_per_bar )
        signal_bpb_change( pos.beats_per_bar );

    if ( pos.beat_type != beat_type )
        signal_beat_change( pos.beat_type );

    ticks_per_beat = pos.ticks_per_beat;
    beats_per_bar = pos.beats_per_bar;
    beat_type = pos.beat_type;
    beats_per_minute = pos.beats_per_minute;

    frame = pos.frame;
    frame_rate = pos.frame_rate;

    /* FIXME: this only needs to be calculated if bpm or framerate changes  */
    {
        const double frames_per_beat = frame_rate * 60 / beats_per_minute;

        frames_per_tick = frames_per_beat / (double)PPQN;
        ticks_per_period = nframes / frames_per_tick;
    }

    tick_t abs_tick = (pos.bar * pos.beats_per_bar + pos.beat) * pos.ticks_per_beat + pos.tick;
//    tick_t abs_tick = pos.bar_start_tick + (pos.beat * pos.ticks_per_beat) + pos.tick;

    /* scale Jack's ticks to our ticks */

    const double pulses_per_tick = PPQN / pos.ticks_per_beat;

    ticks = abs_tick * pulses_per_tick;
    tick = tick * pulses_per_tick;

    ticks_per_beat = PPQN;
}

void
Transport::start ( void )
{
    MESSAGE( "Starting transport" );
    jack_transport_start( client );
}

void
Transport::stop ( void )
{
    MESSAGE( "Stopping transport" );
    jack_transport_stop( client );
}

void
Transport::toggle ( void )
{
    if ( rolling )
        stop();
    else
        start();
}

void
Transport::locate ( tick_t ticks )
{
    jack_nframes_t frame = trunc( ticks * transport.frames_per_tick );

    MESSAGE( "Relocating transport to %f, %lu", ticks, frame );

    jack_transport_locate( client, frame );
}

void
Transport::set_beats_per_minute ( double n )
{
    _master_beats_per_minute = n;
    _done = false;
}

void
Transport::set_beats_per_bar ( int n )
{
    if ( n < 2 )
        return;

    _master_beats_per_bar = n;
    _done = false;
}

void
Transport::set_beat_type ( int n )
{
    if ( n < 4 )
        return;

    _master_beat_type = n;
    _done = false;
}
