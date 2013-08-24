
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

#include "jack.H"
#include "non.H"
#include "transport.H"
#include "pattern.H"
#include "phrase.H"
#include <MIDI/event_list.H>
#include <MIDI/midievent.H>

using namespace MIDI;

#ifdef JACK_MIDI_PROTO_API
/* correct for prototype version of API */
#define jack_midi_event_reserve( p, f, l )  jack_midi_event_reserve( p, f, l, nframes )
#define jack_midi_event_get( e, b, f )      jack_midi_event_get( e, b, f, nframes )
#define jack_midi_get_event_count( b )      jack_midi_get_event_count( b, nframes )
#define jack_midi_clear_buffer( b )         jack_midi_clear_buffer( b, nframes )
#define jack_midi_event_write( b, f, d, s ) jack_midi_event_write( b, f, d, s, nframes )
#endif


/* MIDI channel to listen for pattern control changes on */
int pattern_control_channel = 0;

/* which control change number to use for pattern control */
int pattern_control_cc = 20;

jack_client_t *client;

int sample_rate;

const int MAX_PORT = 16;

const int subticks_per_tick = 4096;

/* timers for notes on all channels and ports. When a note is played,
 * the respective value in this array is set to the note duraction in
 * subticks (an arbitrary division of the tick used only for this
 * purpose). Decremented in each process cycle, when this value
 * reaches zero, a note off is generated--regardless of the state of
 * the transport */
int note_duration[MAX_PORT][16][128];

/* tracks the number of concurrent note ons for the same note so that
 * we can be sure to emit the correct number of note offs */
int notes_on[MAX_PORT][16][128];

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

bool
midi_is_active ( void )
{
    return client != NULL;
}

/** get next recorded event, if any--runs in UI thread */
bool
midi_input_event ( int port, midievent *me )
{
    if ( ! midi_is_active() )
        return NULL;

    if ( jack_ringbuffer_read_space( input[ port ].ring_buf ) >= sizeof( midievent ) )
    {
        if ( jack_ringbuffer_read( input[ port ].ring_buf, (char *)me, sizeof( midievent ) ) )
            return true;
    }
    return false;
}


/**
 * Queue an event for output. /tick/ is relative to the current cycle! */
void
midi_output_event ( int port, const midievent *e )
{
    if ( ! midi_is_active() )
        return;

    event *fe = freelist.first();

    if ( ! fe )
    {
        WARNING( "output buffer underrun" );
    }
    else
    {
        if ( e->is_note_on() )
        {
            if ( notes_on[ port ][ e->channel() ][ e->note() ] == 0 )
            {
                freelist.unlink( fe );
                *fe = *e;

                output[ port ].events.insert( fe );
                ++notes_on[ port ][ e->channel() ][ e->note() ];
            }
            else
            {
                DMESSAGE( "Dropping extra Note ON" );
            }
        }
        else if ( e->is_note_off() )
        {
            if ( notes_on[ port ][ e->channel() ][ e->note() ] == 0 )
            {
                DMESSAGE( "Dropping extra Note OFF" );
            }
            else
            {
                freelist.unlink( fe );
                *fe = *e;

                output[ port ].events.insert( fe );
                --notes_on[ port ][ e->channel() ][ e->note() ];
            }
        }
        else
        {
            freelist.unlink( fe );
            *fe = *e;

            output[ port ].events.insert( fe );
        }
    }
}

/** same as above, but only for note-on + duration */
void
midi_output_event ( int port, const midievent *e, tick_t duration )
{
    if ( ! midi_is_active() )
        return;

    if ( duration )
    {
        note_duration[ port ][ e->channel() ][ e->note() ] = (duration + e->timestamp()) * subticks_per_tick;
        midi_output_event( port, e );
    }
    else
    {
        /* We allow duplicate notes on and pass notes off through as
         * is in order to support poly synths. */
        midi_output_event( port, e );
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
    if ( ! midi_is_active() )
        return;

    if ( jack_ringbuffer_write( output[ port ].ring_buf, (const char *)e, sizeof( midievent ) ) != sizeof( midievent ) )
        WARNING( "output ringbuffer overrun" );
    else
        if ( e->is_note_on() )
        {
            /* use timestamp as duration */
            note_duration[ port ][ e->channel() ][ e->note() ] = e->timestamp() * subticks_per_tick;
        }
}

/** stop all notes on all channels of all ports */
void
midi_all_sound_off ( void )
{
    if ( ! midi_is_active() )
        return;

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

static void
stop_all_patterns ( void )
{
    for ( uint i = pattern::patterns(); i--; )
    {
        pattern *p = pattern::pattern_by_number( i + 1 );

        p->stop();
    }
}

static int
sync ( jack_transport_state_t state, jack_position_t *pos, void * )
{
    switch ( state )
    {
        case JackTransportStopped:           /* new position requested */
            /* JACK docs lie. This is only called when the transport
               is *really* stopped, not when starting a slow-sync
               cycle */
            stop_all_patterns();
            return 1;
        case JackTransportStarting:          /* this means JACK is polling slow-sync clients */
        {
            stop_all_patterns();
            return 1;
        }
        case JackTransportRolling:           /* JACK's timeout has expired */
            /* FIXME: what's the right thing to do here? */
//            request_locate( pos->frame );
            return 1;
            break;
        default:
            WARNING( "unknown transport state" );
    }

    return 0;
}


static int
process ( jack_nframes_t nframes, void *arg )
{
    static tick_t oph = 0;
    static int old_play_mode = PATTERN;

    static int not_dropped = 0;

    ::nframes = nframes;

    transport.nframes = nframes;
    transport.poll();

    /* ph-nph is exclusive. It is important that in normal continuous playback each tick is covered exactly once! */
    const tick_t ph = transport.ticks;
    const tick_t nph = transport.ticks + transport.ticks_per_period;

    if ( ! transport.valid )
        goto schedule;

    if ( ( ! transport.rolling ) || ph == oph )
        goto schedule;

    /* if ( ph != onph ) */
    /* { */
    /*     if ( onph > ph ) */
    /*         DWARNING( "duplicated %lu ticks (out of %d)", onph - ph, (int)(not_dropped * transport.ticks_per_period) ); */
    /*     else */
    /*         DWARNING( "dropped %lu ticks (out of %d), ticks per period = %f", ph - onph, (int)(not_dropped * transport.ticks_per_period) ); */

    /*     not_dropped = 0; */
    /* } */

    ++not_dropped;

//    onph = nph;

    if ( old_play_mode != song.play_mode )
    {
        switch ( old_play_mode )
        {
            case PATTERN:
            case TRIGGER:
            case QUEUE:
                DMESSAGE( "Stopping all patterns" );
                stop_all_patterns();
                break;
        }
        old_play_mode = song.play_mode;
    }

//  DMESSAGE( "tpp %f %lu-%lu", transport.ticks_per_period, ph, nph );

    /* now handle control input */
    {
        int j = CONTROL;

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

            /* no need to pass it to the GUI, we can trigger patterns here */

            if ( e.channel() == pattern_control_channel &&
                 e.opcode() == midievent::CONTROL_CHANGE &&
                 e.lsb() == pattern_control_cc )
            {
                if ( e.msb() < pattern::patterns() )
                {
                    
                    pattern *p = pattern::pattern_by_number( e.msb() + 1 );
                    
                    if ( TRIGGER == song.play_mode )
                    {
                        if ( p->playing() )
                        {
                            DMESSAGE( "Untriggering pattern %i ph=%lu, ts=%lu", e.msb(), ph, e.timestamp() );
                            
                            p->trigger( ph, e.timestamp() );
                        }
                        else
                        {
                            DMESSAGE( "Triggering pattern %i ph=%lu, ts=%lu", e.msb(), ph, e.timestamp() );
                            
                            p->trigger( e.timestamp(), INFINITY );
                        }
                    }
                    else
                    {
                        if ( p->mode() == PLAY )
                        {
                            DMESSAGE( "Dequeuing pattern %i ph=%lu, ts=%lu", e.msb(), ph, e.timestamp() );
                            p->mode( MUTE );
                        }
                        else
                        {
                            DMESSAGE( "Queuing pattern %i ph=%lu, ts=%lu", e.msb(), ph, e.timestamp() );
                            
                            p->mode( PLAY );
                        }
                    }
                }
            }
        }
    }
    
            
    switch ( song.play_mode )
    {
        case SEQUENCE:
            playlist->play( ph, nph );
            break;
        case QUEUE:
        case PATTERN:
        {
            for ( uint i = pattern::patterns(); i--; )
            {
                pattern *p = pattern::pattern_by_number( i + 1 );

                p->trigger( 0, INFINITY );

                p->play( ph, nph );
            }
            break;
        }
        case TRIGGER:
        {
            for ( uint i = pattern::patterns(); i--; )
            {
                pattern *p = pattern::pattern_by_number( i + 1 );

                p->play( ph, nph );
            }
            break;
        }
    }

    oph = ph;

    /* handle midi input */
//    for ( int j = transport.recording ? 2 : 1; j--; )
    
    if ( transport.recording )
    {
        int j = PERFORMANCE;

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

            /* handle scheduled note offs */
            for ( uint j = 16; j-- ; )
            {
                register int *note = &note_duration[ i ][ j ][ 0 ];

                for ( register uint k = 0; k < 128; ++note, ++k )
                    if ( *note > 0 )
                        if ( ( *note -= subticks_per_period ) <= 0 )
                        {
                            while ( notes_on[ i ][ j ][ k] > 0 )
                            {
                                static midievent e;
                                
                                e.status( midievent::NOTE_OFF );
                                e.channel( j );
                                e.note( k );
                                e.note_velocity( 64 );
                                
                                e.timestamp( (subticks_per_period + *note) / subticks_per_tick );
                                
                                midi_output_event( i, &e );
                            }

                            *note = 0;
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

const char *
midi_init ( const char *name )
{
    MESSAGE( "Initializing Jack MIDI" );

    if (( client = jack_client_open ( name, (jack_options_t)0, NULL )) == 0 )
        return NULL;

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

    DMESSAGE( "allocated output buffer space for %lu events", freelist.size() );

    /* clear notes */
    for ( int p = MAX_PORT; p--; )
    {
        for ( int c = 16; c-- ; )
            for ( int n = 128; n-- ; )
            {
                note_duration[ p ][ c ][ n ] = 0;
                notes_on[ p ][ c ][ n ] = 0;
            }
    }

//1    jack_set_buffer_size_callback( client, bufsize, 0 );
    jack_set_process_callback( client, process, 0 );
    jack_set_sync_callback( client, sync, 0 );

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

    return (const char *) jack_get_client_name(client);
}

void
midi_shutdown ( void )
{
    // TODO: wait for all queued events to play.
    if ( client )
    {
        jack_deactivate( client );
        jack_client_close( client );
        client = NULL;
    }
}
