
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

#include "../Track.H"
#include "../Transport.H" // for rolling
#include "../Control_Sequence.H"

#include "Playback_DS.H"
#include "Record_DS.H"
#include "Engine.H"



/**********/
/* Engine */
/**********/

const Audio_Region *
Track::capture_region ( void ) const
{
    if ( record_ds )
        return record_ds->capture_region();
    else
        return NULL;
}

void
Track::update_port_names ( void )
{
    for ( unsigned int i = 0; i < output.size(); ++i )
        output[ i ].name( name(), i );

    for ( unsigned int i = 0; i < input.size(); ++i )
        input[ i ].name( name(), i );

/*     /\* tell any attached control sequences to do the same *\/ */
/*     for ( int i = control->children(); i-- ) */
/*         ((Control_Sequence*)control->child( i ))->update_port_names(); */

}

bool
Track::configure_outputs ( int n )
{
    int on = output.size();

    if ( n == on )
        return true;

//    engine->lock();

    if ( playback_ds )
    {
        Playback_DS *ds = playback_ds;
        playback_ds = NULL;

        delete ds;
    }

    if ( n > on )
    {
        for ( int i = on; i < n; ++i )
        {
            JACK::Port p( engine, JACK::Port::Output, name(), i );

            if ( p.valid() )
                output.push_back( p );
            else
                WARNING( "could not create output port!" );
        }
    }
    else
    {
        for ( int i = on; i > n; --i )
        {
            output.back().shutdown();
            output.pop_back();
        }
    }

    if ( output.size() )
        playback_ds = new Playback_DS( this, engine->frame_rate(), engine->nframes(), output.size() );

//    engine->unlock();
    /* FIXME: bogus */
    return true;
}

bool
Track::configure_inputs ( int n )
{
    int on = input.size();

    if ( n == on )
        return true;

//    engine->lock();

    if ( record_ds )
    {
        Record_DS *ds = record_ds;
        record_ds = NULL;

        delete ds;
    }

    if ( n > on )
    {
        for ( int i = on; i < n; ++i )
        {
            JACK::Port p( engine, JACK::Port::Input, name(), i );

            if ( p.valid() )
                input.push_back( p );
            else
                WARNING( "could not create input port!" );
        }
    }
    else
    {
        for ( int i = on; i > n; --i )
        {
            input.back().shutdown();
            input.pop_back();
        }
    }

    if ( input.size() )
        record_ds = new Record_DS( this, engine->frame_rate(), engine->nframes(), input.size() );

//    engine->unlock();

    /* FIXME: bogus */
    return true;
}

nframes_t
Track::process_input ( nframes_t nframes )
{
    THREAD_ASSERT( RT );

    if ( ! transport->rolling )
    {
        /* There is no work to do when we're not rolling. */

        return 0;
    }

    if ( record_ds )
        return record_ds->process( nframes );
    else
        return 0;

}

nframes_t
Track::process_output ( nframes_t nframes )
{
    THREAD_ASSERT( RT );

    if ( ! transport->rolling )
    {
        for ( int i = output.size(); i--; )
            output[ i ].silence( nframes );

        return 0;
    }

    /* FIXME: should we blank the control output here or leave it floating? */
    for ( int i = control->children(); i--; )
        ((Control_Sequence*)control->child( i ))->process( nframes );

    if ( playback_ds )
        return playback_ds->process( nframes );
    else
        return 0;
}



void
Track::seek ( nframes_t frame )
{
    THREAD_ASSERT( RT );

    if ( playback_ds )
        return playback_ds->seek( frame );
}

void
Track::delay ( nframes_t frames )
{
//    THREAD_ASSERT( RT );

    if ( playback_ds )
        playback_ds->delay( frames );
}

/* THREAD: RT (non-RT) */
void
Track::resize_buffers ( nframes_t nframes )
{
    if ( record_ds )
        record_ds->resize_buffers( nframes );
    if ( playback_ds )
        playback_ds->resize_buffers( nframes );
}

#include <time.h>

/** very cheap UUID generator... */
unsigned long long
uuid ( void )
{
    time_t t = time( NULL );

    return (unsigned long long) t;
}

/** create capture region and prepare to record */
void
Track::record ( Capture *c, nframes_t frame )
{
    THREAD_ASSERT( Capture );

    char pat[256];

    snprintf( pat, sizeof( pat ), "%s-%llu", name(), uuid() );

    c->audio_file = Audio_File_SF::create( pat, engine->sample_rate(), input.size(), Track::capture_format );

    if ( ! c->audio_file )
        FATAL( "Could not create file for new capture!" );

    /* open it again for reading in the GUI thread */
    Audio_File *af = Audio_File::from_file( c->audio_file->name() );

    c->region = new Audio_Region( af, sequence(), frame );

    c->region->prepare();
}

/** write a block to the (already opened) capture file */
void
Track::write ( Capture *c, sample_t *buf, nframes_t nframes )
{
    THREAD_ASSERT( Capture );

    nframes_t l = c->audio_file->write( buf, nframes );

    c->region->write( l );
}

#include <stdio.h>

void
Track::finalize ( Capture *c, nframes_t frame )
{
    THREAD_ASSERT( Capture );

    /* adjust region start for latency */
    /* FIXME: is just looking at the first channel good enough? */

    c->region->finalize( frame );
    DMESSAGE( "finalizing audio file" );
    c->audio_file->finalize();

    nframes_t capture_offset = 0;

    /* Add the system latency twice. Once for the input (usually
     * required) and again for the output latency of whatever we're
     * playing along to (should only apply when overdubbing) */

    /* Limitations in the JACK latency reporting API prevent us from
     * compensating from any software latency introduced by other
     * clients in our graph... Oh well */

    capture_offset += engine->system_latency();
    capture_offset += engine->system_latency();

    DMESSAGE( "Adjusting capture by %lu frames.", (unsigned long)capture_offset );

    c->region->offset( capture_offset );

    delete c->audio_file;
}
