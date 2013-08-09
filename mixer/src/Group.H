
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

#pragma once

#include <list>
class Mixer_Strip;

#include "Mutex.H"

class Port;

#include "JACK/Client.H"

#include "Thread.H"
#include "Loggable.H"

class Group : public Loggable, public JACK::Client, public Mutex
{
    bool _single;
    char *_name;

    Thread _thread;                                            /* only used for thread checking */

    int _buffers_dropped;                                       /* buffers dropped because of locking */
/*     int _buffers_dropped;                                       /\* buffers dropped because of locking *\/ */

    volatile float _dsp_load;
    float _load_coef;

    int sample_rate_changed ( nframes_t srate );
    void shutdown ( void );
    int process ( nframes_t nframes );
    int xrun ( void );
    void freewheel ( bool yes );
    int buffer_size ( nframes_t nframes );
    void thread_init ( void );
    void port_connect ( jack_port_id_t a, jack_port_id_t b, int connect );

    /* not allowed */
    Group ( const Group &rhs );
    Group & operator = ( const Group &rhs );

    void request_locate ( nframes_t frame );

    void recal_load_coef ( void );

protected:

    virtual void get ( Log_Entry &e ) const;
    virtual void set ( Log_Entry &e );

private:

    friend class Port;
    friend class Transport;

public: 

    LOG_CREATE_FUNC( Group );

    float dsp_load ( void ) const { return _dsp_load; }
    int nstrips ( void ) const { return strips.size(); }
    int dropped ( void ) const { return _buffers_dropped; }

    Group ( );
    Group ( const char * name, bool single );
    virtual ~Group ( );

    bool single ( void ) const { return _single; }

    const char * name ( void ) const { return _name; }
    void name ( const char *n );

    std::list<Mixer_Strip*> strips;

    /* static void process ( nframes_t nframes, void *v ); */
    /* void process ( nframes_t nframes ); */

    void add (Mixer_Strip*);
    void remove (Mixer_Strip*);

    /* Engine *engine ( void ) { return _engine; } */
};

