
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

Track::Capture *
Track::capture ( void )
{
    if ( record_ds )
        return record_ds->capture();
    else
        return NULL;
}

void
Track::update_port_names ( void )
{
    for ( unsigned int i = 0; i < output.size(); ++i )
    {
        output[ i ].trackname( name() );
        output[ i ].rename();
    }

    for ( unsigned int i = 0; i < input.size(); ++i )
    {
        input[ i ].trackname( name() );
        input[ i ].rename();
    }
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

    DMESSAGE( "Reconfiguring outputs for track %s", name() );

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
            char s[512];
            snprintf( s, sizeof(s), "out-%i", i + 1 );

            JACK::Port p( engine, name(), s, JACK::Port::Output,  JACK::Port::Audio );

            p.terminal(true);

            if ( !p.activate() )
            {
                FATAL( "could not created output port!");
            }

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
        playback_ds = new Playback_DS( this, engine->sample_rate(), engine->nframes(), output.size() );

    /* FIXME: bogus */
    return true;
}

bool
Track::configure_inputs ( int n )
{
    int on = input.size();

    if ( n == on )
        return true;

    DMESSAGE( "Reconfiguring inputs for track %s", name() );

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
            char s[512];
            snprintf( s, sizeof(s), "in-%i", i + 1 );

            JACK::Port p( engine, name(), s, JACK::Port::Input, JACK::Port::Audio );
            
            p.terminal( true );

            if ( !p.activate() )
            {
                FATAL( "could not created output port!");
            }

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
        record_ds = new Record_DS( this, engine->sample_rate(), engine->nframes(), input.size() );

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
    for ( int i = 0; i < ((Fl_Pack*)control)->children(); i++ )
        ((Control_Sequence*)((Fl_Pack*)control)->child( i ))->process( nframes );

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

    char *pat;

    asprintf( &pat, "%s-%llu", name(), uuid() );

    c->audio_file = Audio_File_SF::create( pat, engine->sample_rate(), input.size(), Track::capture_format );

    free( pat );

    if ( ! c->audio_file )
        FATAL( "Could not create file for new capture!" );

    /* open it again for reading in the GUI thread */
    //   Audio_File *af = Audio_File::from_file( c->audio_file->name() );
    /* must acquire the FLTK lock because adding a widget might interfere with drawing */
//    Fl::lock();

    /* must acquire a write lock because the Audio_Region constructor
     * will add the region to the specified sequence, which might affect playback */
    timeline->wrlock();

    c->region = new Audio_Region( c->audio_file, sequence(), frame );

    timeline->unlock();

//    Fl::unlock();

    c->region->prepare();

    nframes_t min,max;

    input[0].get_latency( JACK::Port::Input, &min, &max );

    if ( transport->freewheel_enabled() )
    {
        /* in freewheeling mode, assume we're bouncing and only
         * compensate for capture latency */
        _capture_offset = min;
    }
    else
    {
        /* not freewheeling, so assume we're overdubbing and need to
         * compensate for both capture and playback latency */
        _capture_offset = min;
        
        /* since the track output might not be connected to
         * anything, just get the playback latency */
        
        _capture_offset += engine->playback_latency();
    }
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

    DMESSAGE( "finalizing audio file" );
    c->audio_file->finalize();

    timeline->wrlock();

    c->region->finalize( frame );

    DMESSAGE( "Adjusting capture by %lu frames.", (unsigned long)_capture_offset );

    c->region->offset( _capture_offset );
    _capture_offset = 0;

    timeline->unlock();
}
