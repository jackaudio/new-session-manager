
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

// #include <jack/jack.h>
#include "Client.H"
#include <stdlib.h>

namespace JACK
{
    class Port
    {
        jack_port_t *_port;
        char *_name;
        JACK::Client *_client;

        /* FIXME: reference count? */

/*     /\* not permitted  *\/ */
/*     Port ( const Port &rhs ); */
/*     Port & operator= ( const Port &rhs ); */

    public:

        bool operator < ( const Port & rhs ) const;

        enum type_e { Output, Input };

        static int max_name ( void );

        Port ( JACK::Client *client, jack_port_t *port );
        Port ( JACK::Client *client, const char *name, type_e dir );
        Port ( JACK::Client *client, type_e dir, const char *base, int n, const char *type=0 );
        Port ( JACK::Client *client, type_e dir, int n, const char *type=0 );

//    Port ( );
        ~Port ( );

        Port ( const Port & rhs );


        bool valid ( void ) const { return _port; }
        bool connected ( void ) const { return jack_port_connected( _port ); }
        type_e type ( void ) const;
        const char * name ( void ) const { return _name; }
        bool name ( const char *name );
        bool name ( const char *base, int n, const char *type=0 );

        nframes_t total_latency ( void ) const;
        nframes_t latency ( void ) const;
        void latency ( nframes_t frames );

        void activate ( const char *name, type_e dir );
        void shutdown ( void );
        void write ( sample_t *buf, nframes_t nframes );
        void read ( sample_t *buf, nframes_t nframes );
        void *buffer ( nframes_t nframes );
        void silence ( nframes_t nframes );

        /*  */
        const char ** connections ( void );
        bool connections ( const char **port_names );
        void freeze ( void );
        void thaw ( void );

    private:

        /* holds all we need to know about a jack port to recreate it on a
         new client */
        struct freeze_state
        {
            const char **connections;
            type_e direction;
            char *name;

            freeze_state ( )
                {
                    connections = NULL;
                    name = NULL;
                }

            ~freeze_state ( )
                {
                    if ( connections )
                    {
                        free( connections );
                        connections = NULL;
                    }
                    if ( name )
                    {
                        free( name );
                    }
                }
        };

        freeze_state *_freezer;

    };

}
