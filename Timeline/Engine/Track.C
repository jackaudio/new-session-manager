
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

void
Track::update_port_names ( void )
{
    for ( unsigned int i = 0; i < output.size(); ++i )
        output[ i ].name( name(), i );

    for ( unsigned int i = 0; i < input.size(); ++i )
        input[ i ].name( name(), i );

    for ( unsigned int i = 0; i < control_out.size(); ++i )
        control_out[ i ]->name( name(), i, "cv" );


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

        ds->shutdown();
        delete ds;
    }

    if ( n > on )
    {
        for ( int i = on; i < n; ++i )
        {
            Port p( Port::Output, name(), i );

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

        ds->shutdown();
        delete ds;
    }

    if ( n > on )
    {
        for ( int i = on; i < n; ++i )
        {
            Port p( Port::Input, name(), i );

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

/* THREAD: RT */
nframes_t
Track::process ( nframes_t nframes )
{
    if ( ! transport->rolling )
    {
        for ( int i = output.size(); i--; )
            output[ i ].silence( nframes );

        for ( int i = input.size(); i--; )
            input[ i ].silence( nframes );

        /* FIXME: is this really the right thing to do for control ports? */
        for ( int i = control_out.size(); i--; )
            control_out[ i ]->silence( nframes );

        return 0;
    }

    for ( int i = control->children(); i--; )
        ((Control_Sequence*)control->child( i ))->process( nframes );

    if ( playback_ds )
    {
        record_ds->process( nframes );
        return playback_ds->process( nframes );
    }
    else
        return 0;
}

/* THREAD: RT */
void
Track::seek ( nframes_t frame )
{
    if ( playback_ds )
        return playback_ds->seek( frame );
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

/* #include "Audio_Region.H" */


#include <time.h>

/** very cheap UUID generator... */
unsigned long long
uuid ( void )
{
    time_t t = time( NULL );

    return (unsigned long long) t;
}


/* THREAD: IO */
/** create capture region and prepare to record */
void
Track::record ( nframes_t frame )
{
    assert( ! _capture );
    assert( ! _capture_af );

    char pat[256];

    snprintf( pat, sizeof( pat ), "%s-%llu", name(), uuid() );


    _capture_af = Audio_File_SF::create( pat, engine->sample_rate(), input.size(), Track::capture_format );

    if ( ! _capture_af )
    {
        /* ERROR */

    }

    /* open it again for reading in the GUI thread */
    Audio_File *af = Audio_File::from_file( _capture_af->name() );

    _capture = new Audio_Region( af, sequence(), frame );

    _capture->prepare();
}

/* THREAD: IO */
/** write a block to the (already opened) capture file */
void
Track::write ( sample_t *buf, nframes_t nframes )
{
    nframes_t l = _capture_af->write( buf, nframes );

    _capture->write( l );
}

#include <stdio.h>

/* THREAD: IO */
void
Track::stop ( nframes_t frame )
{
    _capture->finalize( frame );

    _capture = NULL;

    _capture_af->finalize();

    delete _capture_af;
    _capture_af = NULL;
}
