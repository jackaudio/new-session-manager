
/*******************************************************************************/
/* Copyright (C) 2007-2008 Jonathan Moore Liles                                */
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <math.h>
/* jack */
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>
#include <jack/thread.h>

#include "non.H"
#include "transport.H"
#include "pattern.H"
#include "phrase.H"
#include "event_list.H"

#ifdef JACK_MIDI_PROTO_API
/* correct for prototype version of API */
#define jack_midi_event_reserve( p, f, l )  jack_midi_event_reserve( p, f, l, nframes )
#define jack_midi_event_get( e, b, f )      jack_midi_event_get( e, b, f, nframes )
#define jack_midi_get_event_count( b )      jack_midi_get_event_count( b, nframes )
#define jack_midi_clear_buffer( b )         jack_midi_clear_buffer( b, nframes )
#define jack_midi_event_write( b, f, d, s ) jack_midi_event_write( b, f, d, s, nframes )
#endif

jack_client_t *client;

int sample_rate;

const int MAX_PORT = 16;

const int subticks_per_tick = 2048;

/* timers for notes on all channels and ports. When a note is played,
 * the respective value in this array is set to the note duraction in
 * subticks (an arbitrary division of the tick used only for this
 * purpose). Decremented in each process cycle, when this value
 * reaches zero, a note off is generated--regardless of the state of
 * the transport */
int notes_on[MAX_PORT][16][128];

/* number of notes currently playing on each port */
int port_notes_on[MAX_PORT];

typedef unsigned char byte_t;

int num_output_ports = 1;

event_list freelist;

typedef struct {
    void *buf;
    jack_ringbuffer_t *ring_buf;                                /* for realtime output and recording */
    event_list events;                                          /* events to be output this cycle */
    jack_port_t *port;
} port_t;

static port_t output[MAX_PORT];
static port_t input[2];                                                /* control, performance */

jack_nframes_t nframes;                                         /* for compatibility with older jack */

/** get next recorded event, if any--runs in UI thread */
midievent *
midi_input_event ( int port )
{
    if ( jack_ringbuffer_read_space( input[ port ].ring_buf ) >= sizeof( midievent ) )
    {
        midievent *me = new midievent;

//        MESSAGE( "passing midi input to non-RT thread" );

        if ( jack_ringbuffer_read( input[ port ].ring_buf, (char *)me, sizeof( midievent ) ) )
            return me;
    }
    return NULL;
}

/**
 * Queue an event for output. /tick/ is relative to the current cycle! */
void
midi_output_event ( int port, const midievent *e )
{
    event *fe = freelist.first();

    if ( ! fe )
        WARNING( "output buffer underrun" );
    else
    {
        freelist.unlink( fe );

        *fe = *e;

        output[ port ].events.insert( fe );
    }
}

/** same as above, but only for note-on + duration */
void
midi_output_event ( int port, const midievent *e, tick_t duration )
{
    if ( duration )
    {
        if ( notes_on[ port ][ e->channel() ][ e->note() ] > transport.ticks_per_period * subticks_per_tick )
            WARNING( "duplicate note on?" );
        else
        {
            notes_on[ port ][ e->channel() ][ e->note() ] = (duration + e->timestamp()) * subticks_per_tick;

            ++port_notes_on[ port ];

            midi_output_event( port, e );
        }
    }
    else
    {

/*     if ( notes_on[ port ][ e->channel() ][ e->note() ] ) */
/*             WARNING( "note still on when note-off came" ); */
    }
}

void
midi_write_event ( int port, const midievent *e )
{
    byte_t *buffer;

    // what I want here is to translate a PPQN tick into the
    // current period.
    jack_nframes_t frame = transport.frames_per_tick * e->timestamp();

    int l = e->size();

    buffer = jack_midi_event_reserve( output[ port ].buf, frame, l );

    if ( ! buffer )
    {
        WARNING( "could not reserve buffer at frame %d, note event dropped!", frame );
        return;
    }

#ifdef DEBUG_EVENTS
    e->pretty_print();
#endif

    e->raw( buffer, l );
}


/** Call this to send an event immediately from UI thread. Timestamp is meaningless */
void
midi_output_immediate_event ( int port, const midievent *e )
{
    if ( jack_ringbuffer_write( output[ port ].ring_buf, (const char *)e, sizeof( midievent ) ) != sizeof( midievent ) )
        WARNING( "output ringbuffer overrun" );
    else
        if ( e->is_note_on() )
        {
            /* use timestamp as duration */
            notes_on[ port ][ e->channel() ][ e->note() ] = e->timestamp() * subticks_per_tick;
            ++port_notes_on[ port ];
        }
}

/** stop all notes on all channels of all ports */
void
midi_all_sound_off ( void )
{
    MESSAGE( "stopping all sound" );

    midievent e;

    /* all notes off */
    e.status( midievent::CONTROL_CHANGE );
    e.data( 123, 0 );
    e.timestamp( 0 );

    for ( int p = MAX_PORT; p--; )
        for ( int c = 16; c--; )
        {
            e.channel( c );
            midi_output_immediate_event( p, &e );
        }
}

static int
process ( jack_nframes_t nframes, void *arg )
{
    static tick_t oph = 0;
    static tick_t onph = 0;
    static int old_play_mode = PATTERN;

    ::nframes = nframes;

    // init all port buffers (maybe we should only do this as needed)
    /* loop over stuff */

    transport.nframes = nframes;
    transport.poll();

    tick_t ph = trunc( transport.ticks );
    tick_t nph = trunc( transport.ticks + transport.ticks_per_period );

    if ( ! transport.valid )
        goto schedule;

    if ( ( ! transport.rolling ) || ph == oph )
        goto schedule;

    if ( ph != onph )
        WARNING( "dropped ticks" );

    onph = nph;

//  MESSAGE( "tpp %f %f-%f", transport.ticks_per_period, ph, nph );
//  MESSAGE( "tpp %f %lu-%lu", transport.ticks_per_period, ph, nph );

    switch ( old_play_mode )
    {
        case PATTERN:
        case TRIGGER:
        {
            // stop all patterns.
            for ( uint i = pattern::patterns(); i--; )
            {
                pattern *p = pattern::pattern_by_number( i + 1 );

                p->stop();
            }

            break;
        }
    }
    switch ( song.play_mode )
    {
        case SEQUENCE:
            // first handle patterns already playing
            for ( uint i = pattern::patterns(); i--; )
            {
                pattern *p = pattern::pattern_by_number( i + 1 );
                if ( p && p->playing() )
                    p->play( ph, nph );
            }

            playlist->play( ph, nph );
            break;
        case PATTERN:
        case TRIGGER:
        {
            for ( uint i = pattern::patterns(); i--; )
            {
                pattern *p = pattern::pattern_by_number( i + 1 );

                p->trigger( 0, -1 );

                p->play( ph, nph );
            }

            break;
        }
    }

    old_play_mode = song.play_mode;

    oph = ph;

    /* handle midi input */
    for ( int j = transport.recording ? 2 : 1; j--; )
    {
        static midievent e;

        input[j].buf = jack_port_get_buffer( input[j].port, nframes );

        jack_midi_event_t ev;

        jack_nframes_t count = jack_midi_get_event_count( input[j].buf );

        for ( uint i = 0; i < count; ++i )
        {
//            MESSAGE( "Got midi input!" );

            jack_midi_event_get( &ev, input[j].buf, i );

            /* time is frame within cycle, convert to absolute tick */
            e.timestamp( ph + (ev.time / transport.frames_per_tick) );
            e.status( ev.buffer[0] );
            e.lsb( ev.buffer[1] );
            if ( ev.size == 3 )
                e.msb( ev.buffer[2] );

            if ( jack_ringbuffer_write( input[j].ring_buf, (char*)&e, sizeof( midievent ) ) != sizeof( midievent ) )
                WARNING( "input buffer overrun" );
        }
    }

schedule:

    const int subticks_per_period = transport.ticks_per_period * subticks_per_tick;

    for ( uint i = MAX_PORT; i-- ; )
    {
        /* reserve and clear buffers */
        output[ i ].buf = jack_port_get_buffer( output[ i ].port, nframes );
        jack_midi_clear_buffer( output[ i ].buf );

        if ( port_notes_on[ i ] > 0 )
        {
            /* handle scheduled note offs */
            for ( uint j = 16; j-- ; )
            {
                register int *note = &notes_on[ i ][ j ][ 0 ];

                for ( register uint k = 0; k < 128; ++note, ++k )
                    if ( *note )
                        if ( ( *note -= subticks_per_period ) <= 0 )
                        {
                            static midievent e;

                            e.status( midievent::NOTE_OFF );
                            e.channel( j );
                            e.note( k );
                            e.note_velocity( 64 );

                            e.timestamp( (subticks_per_period + *note) / subticks_per_tick );

                            *note = 0;
                            --port_notes_on[ i ];

                            midi_output_event( i, &e );
                        }
            }

        }

        static midievent e;
        /* first, write any immediate events from the UI thread */
        while ( jack_ringbuffer_read( output[ i ].ring_buf, (char *)&e, sizeof( midievent ) ) )
        {
//                MESSAGE( "sending immediate event" );
            // FIXME: could we do better?
            e.timestamp( 0 );

            midi_output_event( i, &e );
        }

        /* Write queued events */
        event *n;
        for ( event *e = output[ i ].events.first(); e; e = n )
        {
            n = e->next();

            midi_write_event( i, e );

            output[ i ].events.unlink( e );

            freelist.append( e );
        }
    }

    return 0;
}

int
midi_init ( void )
{
    MESSAGE( "Initializing Jack MIDI" );

/*     if (( client = jack_client_new ( APP_NAME )) == 0 ) */
/*         return 0; */

    if (( client = jack_client_open ( APP_NAME, (jack_options_t)0, NULL )) == 0 )
        return 0;

    /* create output ports */
    for ( int i = 0; i < MAX_PORT; i++ )
    {
        char pat[40];

        sprintf( pat, "midi_out-%d", i + 1 );
        output[i].port = jack_port_register( client, pat, JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0 );
        output[i].ring_buf = jack_ringbuffer_create( 16 * 16 * sizeof( midievent ) );       // why this value?
        jack_ringbuffer_reset( output[i].ring_buf );

    }

    /* create input ports */
    input[0].port = jack_port_register( client, "control_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0 );
    input[0].ring_buf = jack_ringbuffer_create( 128 * sizeof( midievent ) );       // why this value?
    jack_ringbuffer_reset( input[0].ring_buf );
    input[1].port = jack_port_register( client, "midi_in", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput, 0 );
    input[1].ring_buf = jack_ringbuffer_create( 128 * sizeof( midievent ) );       // why this value?
    jack_ringbuffer_reset( input[1].ring_buf );

    /* preallocate events */
    for ( int i = 32 * 16 * MAX_PORT; i-- ; )
        freelist.append( new event );

    DEBUG( "allocated output buffer space for %lu events", freelist.size() );

    /* clear notes */
    for ( int p = MAX_PORT; p--; )
    {
        port_notes_on[ p ] = 0;
        for ( int c = 16; c-- ; )
            for ( int n = 128; n-- ; )
                notes_on[ p ][ c ][ n ] = 0;
    }

//1    jack_set_buffer_size_callback( client, bufsize, 0 );
    jack_set_process_callback( client, process, 0 );


/*     /\* initialize buffer size *\/ */
/*     transport_poll(); */
/*     bufsize( jack_get_buffer_size( client ), 0 ); */

    if ( jack_set_timebase_callback( client, 1, Transport::timebase, NULL ) == 0 )
    {
        MESSAGE( "running as timebase master" );
        transport.master = true;
    }
    else
        WARNING( "could not take over as timebase master" );

    jack_activate( client );

    sample_rate = jack_get_sample_rate( client );

    /* FIXME: hack! we need to wait until jack finally calls our
     * timebase and process callbacks in order to be able to test for
     * valid transport info. */
    MESSAGE( "Waiting for JACK..." );
    usleep( 500000 );

    return 1;
}

void
midi_shutdown ( void )
{
    // TODO: wait for all queued events to play.

    jack_deactivate( client );
}
