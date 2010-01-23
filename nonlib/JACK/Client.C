
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

#include "Client.H"



namespace JACK
{

    Client::Client ( )
    {
        _freewheeling = false;
        _zombified = false;
        _client = NULL;
        _xruns = 0;
    }

    Client::~Client ( )
    {
        jack_client_close( _client );
    }

    /** Tell JACK to calling process callback. This MUST be called in
     * an inheriting class' destructor */
    void
    Client::deactivate ( )
    {
        jack_deactivate( _client );
    }


/*******************/
/* Static Wrappers */
/*******************/

    int
    Client::process ( nframes_t nframes, void *arg )
    {
        return ((Client*)arg)->process( nframes );
    }

    int
    Client::sync ( jack_transport_state_t state, jack_position_t *pos, void *arg )
    {
        return ((Client*)arg)->sync( state, pos );
    }

    int
    Client::xrun ( void *arg )
    {
        ++((Client*)arg)->_xruns;
        return ((Client*)arg)->xrun();
    }

    void
    Client::timebase ( jack_transport_state_t state, jack_nframes_t nframes, jack_position_t *pos, int new_pos, void *arg )
    {
        ((Client*)arg)->timebase( state, nframes, pos, new_pos );
    }

    void
    Client::freewheel ( int starting, void *arg )
    {
        ((Client*)arg)->_freewheeling = starting;
        ((Client*)arg)->freewheel( starting );
    }

    int
    Client::buffer_size ( nframes_t nframes, void *arg )
    {
        return ((Client*)arg)->buffer_size( nframes );
    }

    void
    Client::thread_init ( void *arg )
    {
        ((Client*)arg)->thread_init();
    }

    void
    Client::shutdown ( void *arg )
    {
        ((Client*)arg)->_zombified = true;
        ((Client*)arg)->shutdown();
    }



/** Connect to JACK using client name /client_name/. Return a static
 * pointer to actual name as reported by JACK */
    const char *
    Client::init ( const char *client_name )
    {
        if (( _client = jack_client_open ( client_name, (jack_options_t)0, NULL )) == 0 )
            return NULL;

#define set_callback( name ) jack_set_ ## name ## _callback( _client, &Client:: name , this )

        set_callback( thread_init );
        set_callback( process );
        set_callback( xrun );
        set_callback( freewheel );
        set_callback( buffer_size );

        /* FIXME: should we wait to register this until after the project
           has been loaded (and we have disk threads running)? */
        set_callback( sync );

        jack_set_timebase_callback( _client, 0, &Client::timebase, this );

        jack_on_shutdown( _client, &Client::shutdown, this );

        jack_activate( _client );

        _sample_rate = frame_rate();

        return jack_get_client_name( _client );
    }



/* THREAD: RT */
/** enter or leave freehweeling mode */
    void
    Client::freewheeling ( bool yes )
    {
        if ( jack_set_freewheel( _client, yes ) )
            ;
//            WARNING( "Unkown error while setting freewheeling mode" );
    }

}
