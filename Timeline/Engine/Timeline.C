
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

#include "../Timeline.H"

#include "../Transport.H" // for .rolling
#include "../Track.H"

#include "Record_DS.H"
#include "Playback_DS.H"

/** Initiate recording for all armed tracks */
bool
Timeline::record ( void )
{
    /* FIXME: right place for this? */
    transport->recording = true;

    Loggable::block_start();

    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( t->armed() && t->record_ds )
            t->record_ds->start( transport->frame );
    }

    deactivate();

    return true;
}

/** stop recording for all armed tracks */
void
Timeline::stop ( void )
{
    nframes_t frame = transport->frame;

    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( t->armed() && t->record_ds )
            t->record_ds->stop( frame );
    }

    Loggable::block_end();

    activate();

    transport->recording = false;
}


/**********/
/* Engine */
/**********/

/** call process() on each track header */
nframes_t
Timeline::process ( nframes_t nframes )
{
    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        t->process( nframes );
    }

    /* FIXME: BOGUS */
    return nframes;
}

/* THREAD: RT */
void
Timeline::seek ( nframes_t frame )
{
    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        t->seek( frame );
    }
}

/* THREAD: RT (non-RT) */
void
Timeline::resize_buffers ( nframes_t nframes )
{
    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        t->resize_buffers( nframes );
    }
}

/* THREAD: RT */
int
Timeline::seek_pending ( void )
{
    int r = 0;

    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( t->playback_ds )
            r += t->playback_ds->buffer_percent() < 50;
    }

    return r;
}


/* FIXME: shouldn't these belong to the engine? */
int
Timeline::total_input_buffer_percent ( void )
{
    int r = 0;

    int cnt = 0;

    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( t->record_ds )
        {
            ++cnt;
            r += t->record_ds->buffer_percent();
        }
    }

    if ( ! cnt )
        return 0;

    return r / cnt;
}

int
Timeline::total_output_buffer_percent ( void )
{

    int r = 0;

    int cnt = 0;

    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( t->playback_ds )
        {
            ++cnt;
            r += t->playback_ds->buffer_percent();
        }
    }

    if ( ! cnt )
        return 0;

    return r / cnt;
}

/** wait for I/O threads to fill their buffers */
void
Timeline::wait_for_buffers ( void )
{
    while ( total_output_buffer_percent() + total_input_buffer_percent() < 200 )
        usleep( 5000 );
}


int
Timeline::total_playback_xruns ( void )
{
    int r = 0;

    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( t->playback_ds )
            r += t->playback_ds->xruns();
    }

    return r;
}

int
Timeline::total_capture_xruns ( void )
{
    int r = 0;

    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( t->record_ds )
            r += t->record_ds->xruns();
    }

    return r;
}
