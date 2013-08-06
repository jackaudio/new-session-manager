
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

#include <JACK/Client.H>
#include <JACK/Port.H>
#include <OSC/Endpoint.H>
#include <MIDI/midievent.H>
#include "debug.h"

#include <sys/stat.h>
#include <sys/types.h>

using namespace MIDI;

#include <jack/ringbuffer.h>
#include <jack/thread.h>

#include <stdlib.h>
#include <stdio.h>

#include <map>
#include <string>

#include <unistd.h>                                             /* usleep */
/* simple program to translate from MIDI<->OSC Signals using a fixed mapping  */

#include <nsm.h>

#undef APP_NAME
const char *APP_NAME = "non-midi-mapper";
#undef APP_TITLE
const char *APP_TITLE = "Non-MIDI-Mapper";
#undef VERSION
const char *VERSION = "1.0";

nsm_client_t *nsm;
char *instance_name;


OSC::Endpoint *osc = 0;
/* const double NSM_CHECK_INTERVAL = 0.25f; */


void
handle_hello ( lo_message msg )
{
    int argc = lo_message_get_argc( msg );
    lo_arg **argv = lo_message_get_argv( msg );

    if ( argc >= 4 )
    {
        const char *url = &argv[0]->s;
        const char *name = &argv[1]->s;
        const char *version = &argv[2]->s;
        const char *id = &argv[3]->s;
                
        MESSAGE( "Discovered NON peer %s (%s) @ %s with ID \"%s\"", name, version, url, id );
                
        /* register peer */
        osc->handle_hello( id, url );
    }
}

void
check_nsm ( void )
{
    nsm_check_nowait( nsm );
    //   Fl::repeat_timeout( NSM_CHECK_INTERVAL, &check_nsm, v );
}

static int
osc_non_hello ( const char *, const char *, lo_arg **, int , lo_message msg, void * )
{
    handle_hello( msg );
    return 0;
}


static     void
say_hello ( void )
{
    if ( nsm_is_active( nsm ) )
    {
        lo_message m = lo_message_new();
        
        lo_message_add( m, "sssss",
                        "/non/hello",
                        osc->url(),
                        APP_TITLE,
                        VERSION,
                        instance_name );
        
        nsm_send_broadcast( nsm, m );
    }
}

class Engine : public JACK::Client
{
public:
    jack_ringbuffer_t *input_ring_buf;
    jack_ringbuffer_t *output_ring_buf;
    JACK::Port *midi_input_port;
    JACK::Port *midi_output_port;
    
    Engine ( )
        {
            input_ring_buf = jack_ringbuffer_create( 16 * 16 * sizeof( jack_midi_event_t ));
            jack_ringbuffer_reset( input_ring_buf );
            output_ring_buf = jack_ringbuffer_create( 16 * 16 * sizeof( jack_midi_event_t ));
            jack_ringbuffer_reset( output_ring_buf );

            midi_input_port = 0;
            midi_output_port = 0;
        }

     
    int process ( nframes_t nframes )
        {
            /* process input */
            {
                if ( !midi_input_port )
                    return 0;

                void *buf = midi_input_port->buffer( nframes );
            
                jack_midi_event_t ev;

                jack_nframes_t count = jack_midi_get_event_count( buf );
  
                /* place MIDI events into ringbuffer for non-RT thread */
            
                for ( uint i = 0; i < count; ++i )
                {
//            MESSAGE( "Got midi input!" );
                
                    jack_midi_event_get( &ev, buf, i );
                
                    /* /\* time is frame within cycle, convert to absolute tick *\/ */
                    /* e.timestamp( ph + (ev.time / transport.frames_per_tick) ); */
                    /* e.status( ev.buffer[0] ); */
                    /* e.lsb( ev.buffer[1] ); */
                    /* if ( ev.size == 3 ) */
                    /* e.msb( ev.buffer[2] ); */
                
                    if ( jack_ringbuffer_write( input_ring_buf, (char*)&ev, sizeof( jack_midi_event_t ) ) != sizeof( jack_midi_event_t ) )
                        WARNING( "input buffer overrun" );
                }
            }

            /* process output */
            {
                void *buf = midi_output_port->buffer(nframes);
                
                jack_midi_clear_buffer( buf );

                jack_midi_event_t ev;

                nframes_t frame = 0;

                while ( true )
                {
                    /* jack_ringbuffer_data_t vec[2]; */
                    /* jack_ringbuffer_get_read_vector( output_ring_buf, vec ); */

                    if ( jack_ringbuffer_peek( output_ring_buf, (char*)&ev, sizeof( jack_midi_event_t )) <= 0 )
                        break;

                    unsigned char *buffer = jack_midi_event_reserve( buf, frame, ev.size );
                    if ( !buffer )
                    {
                        WARNING("Output buffer overrun, will send later" );
                        break;
                    }

                    memcpy( buffer, &ev, ev.size );
                    
                    jack_ringbuffer_read_advance( output_ring_buf, sizeof( jack_midi_event_t ) );
                }
            }
            
            return 0;
        }


    void freewheel ( bool starting )
        {
        }

    int xrun ( void )
        {
            return 0;
        }

    int buffer_size ( nframes_t nframes )
        {
            return 0;
        }

    void shutdown ( void )
        {
        }

    void thread_init ( void )
        {
        }

};

Engine *engine;

const float MAX_NRPN = 16383.0f;

static char
get_lsb( int i )
{
    return i & 0x7F;
}

static char
get_msb( int i )
{
    return ( i >> 7 ) & 0x7F;
}

static int 
get_14bit ( char msb, char lsb )
        {
            return msb * 128 + lsb;
        }

class signal_mapping
{
public:

    bool is_nrpn;
    // int nrpn;

    midievent event;

    std::string signal_name;

    OSC::Signal *signal;

    signal_mapping ( )
        {
            is_nrpn = false;
            signal = NULL;
        }

    ~signal_mapping ( )
        {
            if ( signal )
                delete signal;
            signal = NULL;
        }

    char *serialize ( void ) const
        {
            char *s;
            const char *opcode = 0;
            int v1 = 0;
            
            if ( is_nrpn )
            {
                opcode = "NRPN";
                v1 = get_14bit( event.msb(), event.lsb() );
            }
            else
                switch ( event.opcode() )
                {
                    case MIDI::midievent::CONTROL_CHANGE:
                        opcode = "CC";
                        v1 = event.lsb();
                        break;
                    case MIDI::midievent::NOTE_ON:
                        opcode = "NOTE_ON";                        
                        v1 = event.note();
                        break;
                    default:
                        // unsupported
                        break;
                }
       
            asprintf( &s, "%s %d %d", opcode, event.channel(), v1 );

            return s;
        }

    void deserialize ( const char *s )
        {
            int channel;
            char *opcode;
            int control;

            if ( 3 == sscanf( s, "%as %d %d", &opcode, &channel, &control ) )
            {
                event.channel( channel );
                event.opcode( MIDI::midievent::CONTROL_CHANGE );

                is_nrpn = 0;

                if ( !strcmp( opcode, "NRPN" ) )
                {
                    is_nrpn = 1;
                    
                    event.lsb( get_lsb( control ));
                    event.msb( get_msb( control ));
                }
                else if ( !strcmp( opcode, "CC" ) )
                {
                    event.lsb( control );
                }

                free(opcode);
            }
        }

};

int signal_handler ( float value, void *user_data )
{
    signal_mapping *m = (signal_mapping*)user_data;
    
    if ( m->is_nrpn )
    {
        jack_midi_event_t jev[4];
        {
            midievent e;
            e.opcode( MIDI::midievent::CONTROL_CHANGE );
            e.channel( m->event.channel() );
            e.lsb( 99 );
            e.msb( m->event.msb() );
            jev[0].size = e.size();
            e.raw( (byte_t*)&jev[0], e.size() );
//            e.pretty_print();
        }

        {
            midievent e;
            e.opcode( MIDI::midievent::CONTROL_CHANGE );
            e.channel( m->event.channel() );
            e.lsb( 98 );
            e.msb( m->event.lsb() );
            jev[1].size = e.size();
            e.raw( (byte_t*)&jev[1], e.size() );
//            e.pretty_print();
        }


        {
            midievent e;
            e.opcode( MIDI::midievent::CONTROL_CHANGE );
            e.channel( m->event.channel() );
            e.lsb( 6 );
            e.msb( int(value * MAX_NRPN ) >> 7 );
            jev[2].size = e.size();
            e.raw( (byte_t*)&jev[2], e.size() );
//            e.pretty_print();
        }

        {
            midievent e;
            e.opcode( MIDI::midievent::CONTROL_CHANGE );
            e.channel( m->event.channel() );
            e.lsb( 38 );
            e.msb( int( value * MAX_NRPN ) & 0x7F );
            jev[3].size = e.size();
            e.raw( (byte_t*)&jev[3], e.size() );
//            e.pretty_print();
        }

        for ( int i = 0; i < 4; i++ )
        {
            if ( jack_ringbuffer_write( engine->output_ring_buf, (char*)&jev[i], 
                                        sizeof( jack_midi_event_t ) ) != sizeof( jack_midi_event_t ) )
                WARNING( "output buffer overrun" );
        }
    }
    else
    {
        jack_midi_event_t ev;
        
        m->event.msb( value * 128.0f );
        ev.size = m->event.size();
        m->event.raw( (byte_t*)&ev, m->event.size() );
        
//        m->event.pretty_print();
        
        if ( jack_ringbuffer_write( engine->output_ring_buf, (char*)&ev, sizeof( jack_midi_event_t ) ) != sizeof( jack_midi_event_t ) )
            WARNING( "output buffer overrun" );
    }

    return 0;
}


std::map<std::string,signal_mapping> sig_map;

bool
save_settings ( void )
{
    FILE *fp = fopen( "signals", "w" );
    
    if ( !fp )
        return false;
    
    for ( std::map<std::string,signal_mapping>::const_iterator i = sig_map.begin();
          i != sig_map.end();
          i++ )
    {
        fprintf( fp, "[%s] %s\n", i->first.c_str(), i->second.signal_name.c_str() );
    }
    
    fclose(fp);
    
    return true;
}
    

bool
load_settings ( void )
{
    FILE *fp = fopen( "signals", "r" );
    
    if ( !fp )
        return false;
    
    sig_map.clear();
    
    char *signal_name;
    char *midi_event;

    while ( 2 == fscanf( fp, "[%a[^]]] %a[^\n]\n", &midi_event, &signal_name ) )
    {
        DMESSAGE( "%s, %s", midi_event, signal_name );

        if ( sig_map.find( midi_event ) == sig_map.end() )
        {
            signal_mapping m;

            m.deserialize( midi_event );

            sig_map[midi_event] = m;
            sig_map[midi_event].signal_name = signal_name;
            sig_map[midi_event].signal = osc->add_signal( signal_name, OSC::Signal::Output, 0, 1, 0, signal_handler, &sig_map[midi_event] );
        }
       
        free(signal_name);
        free(midi_event);

        /* if ( sig_map.find( s ) == sig_map.end() ) */
        /* {             */
        /*     int channel, control; */
                    
        /*     if ( 2 == sscanf( s, "/midi/%d/CC/%d", &channel, &control ) ) */
        /*     { */
        /*         signal_mapping m; */
                    
        /*         m.event.channel( channel ); */
        /*         m.event.opcode( MIDI::midievent::CONTROL_CHANGE ); */
        /*         m.event.lsb( control ); */
                    
        /*         MESSAGE( "creating signal %s", s ); */
        /*         sig_map[s] = m; */

        /*         sig_map[s].signal = osc->add_signal( s, OSC::Signal::Output, 0, 1, 0, signal_handler, &sig_map[s] ); */

        /*     } */
        /*     if ( 2 == sscanf( s, "/midi/%d/NRPN/%d", &channel, &control ) ) */
        /*     { */
        /*         signal_mapping m; */
                    
        /*         m.event.channel( channel ); */
        /*         m.event.opcode( MIDI::midievent::CONTROL_CHANGE ); */
        /*         m.event.lsb( get_lsb( control ) ); */
        /*         m.event.msb( get_msb( control ) ); */

        /*         m.is_nrpn = true; */
                    
        /*         MESSAGE( "creating signal %s", s ); */
        /*         sig_map[s] = m; */

        /*         sig_map[s].signal = osc->add_signal( s, OSC::Signal::Output, 0, 1, 0, signal_handler, &sig_map[s] ); */

        /*     } */
        /*     else */
        /*         WARNING( "Could not decode signal spec \"%s\"", s ); */
        /* } */
            
        /* free(s); */
    }

    return true;
}

static bool
create_engine ( void )
{
    if ( engine ) 
    {
        delete engine->midi_input_port;
        delete engine->midi_output_port;
        delete engine;
    }

    DMESSAGE( "Creating JACK engine" );

    engine = new Engine();
    
    if ( ! engine->init( instance_name ) )
    {
        WARNING( "Failed to create JACK client" );
        return false;
    }

    engine->midi_input_port = new JACK::Port( engine, NULL, "midi-in", JACK::Port::Input, JACK::Port::MIDI );
    engine->midi_output_port = new JACK::Port( engine, NULL, "midi-out", JACK::Port::Output, JACK::Port::MIDI );

    if ( !engine->midi_input_port->activate() )
    {
        WARNING( "Failed to activate JACK port" );
        return false;
    }

    if ( !engine->midi_output_port->activate() )
    {
        WARNING( "Failed to activate JACK port" );
        return false;
    }

    return true;
}


static int 
command_open ( const char *name, const char *display_name, const char *client_id, char **out_msg, void *userdata )
{
    if ( instance_name )
        free( instance_name );
    
    instance_name = strdup( client_id );
            
    osc->name( client_id );

    mkdir( name, 0777 );
    chdir( name );

    if ( ! create_engine() )
    {
        return ERR_GENERAL;
    }

    load_settings();

    say_hello();
    
    return ERR_OK;
}

static int
command_save ( char **out_msg, void *userdata )
{
    if ( save_settings() )
    {
        nsm_send_is_clean(nsm);
        return ERR_OK;
    }
    else
        return ERR_GENERAL;
}

static int
command_broadcast ( const char *path, lo_message msg, void *userdata )
{
    lo_message_get_argc( msg );
//    lo_arg **argv = lo_message_get_argv( msg );

    if ( !strcmp( path, "/non/hello" ) )
    {
        handle_hello( msg );
        return 0;
    }
    else 
        return -1;

}

struct nrpn_state
{
    char control_msb;
    char control_lsb;
    char value_msb;
    char value_lsb;
    bool decending;
};

static
struct nrpn_state *
decode_nrpn ( nrpn_state *state, midievent e, int *take_action )
{
    nrpn_state *n = &state[e.channel()];

    *take_action = 0;

    switch ( e.lsb() )
    {
        case 6:
            if ( e.msb() < n->value_msb )
                n->value_lsb = 127;
            else if ( e.msb() > n->value_msb )
                n->value_lsb = 0;
            
            n->value_msb = e.msb();
            *take_action = 1;
            return n;
        case 38:
            n->value_lsb = e.msb();
            *take_action = 1;
            return n;
        case 99:
            n->control_msb = e.msb();
            n->control_lsb = 0;
            return n;
        case 98:
            n->control_lsb = e.msb();
            return n;
    }

    return NULL;
}

int
main ( int argc, char **argv )
{
    nrpn_state nrpn_state[16];

    nsm = nsm_new();
//    set_nsm_callbacks( nsm );
    
    nsm_set_open_callback( nsm, command_open, 0 );
    nsm_set_broadcast_callback( nsm, command_broadcast, 0 );
    nsm_set_save_callback( nsm, command_save, 0 );
   
    char *nsm_url = getenv( "NSM_URL" );

    if ( nsm_url )
    {
        if ( ! nsm_init( nsm, nsm_url ) )
        {
            nsm_send_announce( nsm, APP_TITLE, ":dirty:", basename( argv[0] ) );

            /* poll so we can keep OSC handlers running in the GUI thread and avoid extra sync */
//            Fl::add_timeout( NSM_CHECK_INTERVAL, check_nsm, NULL );
        }
    }
    
    osc = new OSC::Endpoint();

    osc->init( LO_UDP, NULL );
    
    osc->add_method( "/non/hello", "ssss", osc_non_hello, osc, "" );
    
    MESSAGE( "OSC URL = %s", osc->url() );

    /* now we just read from the MIDI ringbuffer and output OSC */
    
    DMESSAGE( "waiting for events" );

    static int max_signal = 1;

    jack_midi_event_t ev;
    midievent e;
    while ( true )
    {
        osc->wait(20);
        check_nsm();

        if ( ! engine )
            continue;

        while ( jack_ringbuffer_read( engine->input_ring_buf, (char *)&ev, sizeof( jack_midi_event_t ) ) )
        {
            e.timestamp( ev.time );
            e.status( ev.buffer[0] );
            e.lsb( ev.buffer[1] );
            if ( ev.size == 3 )
                e.msb( ev.buffer[2] );

            switch ( e.opcode() )
            {
                case MIDI::midievent::CONTROL_CHANGE:
                case MIDI::midievent::PITCH_WHEEL:
                {
                    int is_nrpn = 0;

                    struct nrpn_state *st = decode_nrpn( nrpn_state, e, &is_nrpn );
                    
                    if ( st != NULL && !is_nrpn )
                        continue;
                   
                    char *midi_event;
                    
                    if ( is_nrpn )
                    {
                        asprintf( &midi_event, "NRPN %d %d", e.channel(), st->control_msb * 128 + st->control_lsb );
                    }
                    else if ( e.opcode() == MIDI::midievent::CONTROL_CHANGE )
                        asprintf( &midi_event, "CC %d %d", e.channel(), e.lsb() );
                    /* else if ( e.opcode() == MIDI::midievent::PITCH_WHEEL ) */
                    /*     asprintf( &s, "/midi/%i/PB", e.channel() ); */
                    else
                        break;
                   
                    if ( sig_map.find( midi_event ) == sig_map.end() )
                    {
                        char *s;

                        asprintf( &s, "/control/%i", max_signal++ );
                      
                        signal_mapping m;
                        
                        m.event.opcode( e.opcode() );
                        m.event.channel( e.channel() );
                        
                        m.event.lsb( e.lsb() );
                        m.event.msb( e.msb() );

                        m.is_nrpn = is_nrpn;

                        if ( is_nrpn )
                        {
                            m.event.lsb( st->control_lsb );
                            m.event.msb( st->control_msb );
                        }

                        /* if ( is_nrpn ) */
                        /*     m.nrpn = nrpnc_msb * 127 + nrpnc_lsb; */
                        
                        MESSAGE( "creating signal %s", s );
                        sig_map[midi_event] = m;
                        sig_map[midi_event].signal_name = s;
                        sig_map[midi_event].signal = osc->add_signal( s, OSC::Signal::Output, 0, 1, 0, signal_handler, &sig_map[midi_event] );

                        nsm_send_is_dirty( nsm );

                        free(s);
                    }

                    float val = 0;

                    if ( is_nrpn )
                    {
                        val = ( st->value_msb * 128 + st->value_lsb ) / ( MAX_NRPN );
                    }
                    else if ( e.opcode() == MIDI::midievent::CONTROL_CHANGE )
                        val = e.msb() / 128.0f;
                    else if ( e.opcode() == MIDI::midievent::PITCH_WHEEL )
                        val = e.pitch() / ( MAX_NRPN );

//                    MESSAGE( "sending signal for %s = %f", s, val );
                
                    sig_map[midi_event].signal->value( val ); 

                    free( midi_event );

                    break;
                }
                default:
                    break;
            }
//            e.pretty_print();
        }
//    usleep( 500 );
    }
}
