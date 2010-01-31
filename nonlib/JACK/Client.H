
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

typedef jack_nframes_t nframes_t;
typedef float sample_t;

#include <list>

namespace JACK
{
    class Port;
    class Client
    {
        std::list <JACK::Port*> _active_ports;

        jack_client_t *_client;

        static nframes_t _sample_rate;
        volatile int _xruns;
        volatile bool _freewheeling;
        volatile bool _zombified;

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

        Client ( const Client &rhs );
        Client & operator = ( const Client &rhs );

        void freeze_ports ( void );
        void thaw_ports ( void );

    protected:

        void deactivate ( void );

    private:

        friend class Port;
        friend class Transport;

    public:

        enum options { DEFAULT = 0,
                       SLOW_SYNC = 1 << 0,
                       TIMEBASE_MASTER = 1 << 1 };

        jack_client_t * jack_client ( void ) { return _client; }

        void port_added ( JACK::Port * p );
        void port_removed ( JACK::Port *p );

        Client ( );
        virtual ~Client ( );

        const char * init ( const char *client_name, unsigned int opts = 0 );
        const char * name ( const char * );

        nframes_t nframes ( void ) const { return jack_get_buffer_size( _client ); }
        float frame_rate ( void ) const { return jack_get_sample_rate( _client ); }
        static nframes_t sample_rate ( void ) { return _sample_rate; }
        int xruns ( void ) const { return _xruns; };
        bool freewheeling ( void ) const { return _freewheeling; }
        void freewheeling ( bool yes );
        bool zombified ( void ) const { return _zombified; }
        float cpu_load ( void ) const { return jack_cpu_load( _client ); }

        static int maximum_name_length ( void ) { return jack_client_name_size(); }
    };
}
