
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

#include <jack/jack.h>
#include <jack/midiport.h>
#ifdef HAVE_JACK_METADATA
#	include <jack/metadata.h>
#endif
#include <Mutex.H>

typedef jack_nframes_t nframes_t;
typedef jack_default_audio_sample_t sample_t;

#include <list>

namespace JACK
{
    class Port;
    class Client
    {
        std::list <JACK::Port*> _active_ports;

        Mutex _frozen;

        jack_client_t *_client;

//        nframes_t _sample_rate;
        volatile int _xruns;
        volatile bool _freewheeling;
        volatile bool _zombified;
        volatile bool _active;

        static int sample_rate_changed ( nframes_t srate, void *arg );
        virtual int sample_rate_changed ( nframes_t srate ) { return 0; }
        static void port_connect ( jack_port_id_t a, jack_port_id_t b, int connect, void *arg );
        virtual void port_connect ( jack_port_id_t a, jack_port_id_t b, int connect ) { }
        static void shutdown ( void *arg );
        virtual void shutdown ( void ) = 0;
        static int process ( nframes_t nframes, void *arg );
        virtual int process ( nframes_t nframes ) = 0;
        static int sync ( jack_transport_state_t state, jack_position_t *pos, void *arg );
        virtual int sync ( jack_transport_state_t, jack_position_t * ) { return 1; }
        static int xrun ( void *arg );
        virtual int xrun ( void ) = 0;
        static void timebase ( jack_transport_state_t state, jack_nframes_t nframes, jack_position_t *pos, int new_pos, void *arg );
        virtual void timebase ( jack_transport_state_t, jack_nframes_t, jack_position_t *, int ) { }
        static void freewheel ( int yes, void *arg );
        virtual void freewheel ( bool yes ) = 0;
        static int buffer_size ( nframes_t nframes, void *arg );
        virtual int buffer_size ( nframes_t nframes ) = 0;
        static void thread_init ( void *arg );
        virtual void thread_init ( void ) = 0;

        static void latency ( jack_latency_callback_mode_t mode, void *arg );
        virtual void latency ( jack_latency_callback_mode_t mode ) { }
        
        Client ( const Client &rhs );
        Client & operator = ( const Client &rhs );

        void freeze_ports ( void );
        void thaw_ports ( void );

    protected:
        
        bool active ( void ) const { return _active; }
        void deactivate ( void );
        void activate ( void );

    private:

        friend class Port;
        friend class Transport;

    public:

        enum options { DEFAULT = 0,
                       SLOW_SYNC = 1 << 0,
                       TIMEBASE_MASTER = 1 << 1 };

        jack_client_t * jack_client ( void ) const { return _client; }

        void port_added ( JACK::Port * p );
        void port_removed ( JACK::Port *p );

        Client ( );
        virtual ~Client ( );

        const char * init ( const char *client_name, unsigned int opts = 0 );
        const char * name ( const char * );

        const char *jack_name ( void ) const;

        void close ( void );
        nframes_t nframes ( void ) const { return jack_get_buffer_size( _client ); }
//        float frame_rate ( void ) const { return jack_get_sample_rate( _client ); }
        nframes_t sample_rate ( void ) const { return jack_get_sample_rate( _client ); }
        int xruns ( void ) const { return _xruns; };
        bool freewheeling ( void ) const { return _freewheeling; }
        void freewheeling ( bool yes );
        bool zombified ( void ) const { return _zombified; }
        float cpu_load ( void ) const { return jack_cpu_load( _client ); }

        void transport_stop ( void );
        void transport_start ( void );
        void transport_locate ( nframes_t frame );
        jack_transport_state_t transport_query ( jack_position_t *pos );
        
        void recompute_latencies ( void );

        static int maximum_name_length ( void ) { return jack_client_name_size(); }
    };
}
