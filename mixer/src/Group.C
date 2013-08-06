
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

#include <Mixer.H>
#include "Group.H"
#include "Chain.H"
#include "Mixer_Strip.H"
#include "Module.H"

extern char *instance_name;

Group::Group ( )
{
    _single =false;
    _name = NULL;
}

Group::Group ( const char *name, bool single ) : Loggable ( !single )
{
    _single = single;
    _name = strdup(name);

    // this->name( name );
    
    /* FIXME: handle client creation error */
/*     if ( ! jack_name ) */
/*     { */
/*         _engine = NULL; */
        
/* //            fl_alert( "Could not create JACK client. Perhaps the sound device already in use. In any event, now I'll die." ); */
/*         exit( 1 ); */
/* //            return false; */
/*     } */       
}

Group::~Group ( )
{
    DMESSAGE( "Destroying group" );

    if ( _name )
        free( _name );

    deactivate();
}


void 
Group::get ( Log_Entry &e ) const
{
    e.add( ":name", name() );
}

void
Group::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! ( strcmp( s, ":name" ) ) )
        {
            bool add = false;
            if (!_name )
                add = true;

            _name = strdup(v);

            if ( add )
                mixer->add_group(this);
        }
    }
}



/*************/
/* Callbacks */
/*************/

/* THREAD: RT */
/** This is the jack xrun callback */
int
Group::xrun ( void )
{
    return 0;
}

/* THREAD: RT */
void
Group::freewheel ( bool starting )
{
    if ( starting )
        DMESSAGE( "entering freewheeling mode" );
    else
        DMESSAGE( "leaving freewheeling mode" );
}

/* THREAD: RT (non-RT) */
int
Group::buffer_size ( nframes_t nframes )
{
    /* JACK calls this in the RT thread, even though it's a
     * non-realtime operation. This mucks up our ability to do
     * THREAD_ASSERT, so just lie and say this is the UI thread... */

    _thread.set( "UI" );

    for ( std::list<Mixer_Strip*>::iterator i = strips.begin();
          i != strips.end();
          i++ )
    {
        if ( (*i)->chain() )
            (*i)->chain()->buffer_size(nframes);
    }
    
    _thread.set( "RT" );

    return 0;
}

/* THREAD: ?? */
void
Group::port_connect( jack_port_id_t a, jack_port_id_t b, int connect )
{
    for ( std::list<Mixer_Strip*>::iterator i = strips.begin();
          i != strips.end();
          i++ )
    {
        if ( (*i)->chain() )
            (*i)->chain()->port_connect( a, b, connect);
    }
}


/* THREAD: RT */
int
Group::process ( nframes_t nframes )
{
    /* FIXME: wrong place for this */
    _thread.set( "RT" );

    if ( ! trylock() )
    {
        /* the data structures we need to access here (tracks and
         * their ports, but not track contents) may be in an
         * inconsistent state at the moment. Just punt and drop this
         * buffer. */
        ++_buffers_dropped;
        return 0;
    }

    /* since feedback loops are forbidden and outputs are
     * summed, we don't care what order these are processed
     * in */
    for ( std::list<Mixer_Strip*>::iterator i = strips.begin();
          i != strips.end();
          i++ )
    {
        if ( (*i)->chain() )
            (*i)->chain()->process(nframes);
    }

    unlock();

    return 0;
}

int
Group::sample_rate_changed ( nframes_t srate )
{
    for ( std::list<Mixer_Strip*>::iterator i = strips.begin();
          i != strips.end();
          i++ )
    {
        if ( (*i)->chain() )            
            (*i)->chain()->sample_rate_change(srate);
    }
    
    return 0;
}

/* TRHEAD: RT */
void
Group::thread_init ( void )
{
    _thread.set( "RT" );
}

/* THREAD: RT */
void
Group::shutdown ( void )
{
}

/*******************/
/* Group interface */
/*******************/

void
Group::name ( const char *n )
{
    if ( _name )
        free( _name );

    char ename[512];

    _name = strdup( n );

    if ( _single )
        snprintf( ename, sizeof(ename), "%s/%s", instance_name, n );
    else
        snprintf( ename, sizeof(ename), "%s (%s)", instance_name, n );

    if ( !active() )
    {
        const char *jack_name = Client::init( ename );
        Module::set_sample_rate( sample_rate() );
    }
    else
    {
        Client::name( ename );
    }
}

void
Group::add ( Mixer_Strip *o )
{
    lock();
    if ( ! active() )
    {
        /* to call init */
        char *n = strdup(name());
        name(n);
        free(n);
    }
    if ( o->chain() )
        o->chain()->thaw_ports();

    strips.push_back(o);
    unlock();
}

void
Group::remove ( Mixer_Strip *o )
{
    lock();
    strips.remove(o);
    if ( o->chain() )
        o->chain()->freeze_ports();
    if ( strips.size() == 0 && active() )
    {
        Client::close();
    }
    unlock();
}

