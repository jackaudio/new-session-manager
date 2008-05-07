
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

#include "Audio_Sequence.H"
#include "Waveform.H"

#include "dsp.h"

#include <Fl/fl_ask.H>

#include "Track.H"

Audio_Sequence::Audio_Sequence ( Track *track ) : Sequence( track )
{

    _track = track;

    if ( track )
        track->add( this );

    log_create();

    /* FIXME: temporary  */
    labeltype( FL_NO_LABEL );

}


Audio_Sequence::~Audio_Sequence ( )
{
    log_destroy();
}


const Audio_Region *
Audio_Sequence::capture ( void ) const
{
    return track()->capture();
}

void
Audio_Sequence::get ( Log_Entry &e ) const
        {
            e.add( ":t", _track );
            e.add( ":n", name() );
        }

void
Audio_Sequence::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( ":t", s ) )
        {
            int i;
            sscanf( v, "%X", &i );
            Track *t = (Track*)Loggable::find( i );

            assert( t );

            t->track( this );
        }
        else if ( ! strcmp( ":n", s ) )
            name( v );
    }
}


static
void
deurlify ( char *url )
{
    char *r, *w;

    r = w = url;

    for ( ; *r; r++, w++ )
    {
        if ( *r == '%' )
        {
            char data[3] = { *(r + 1), *(r + 2), 0 };

            int c;

            sscanf( data, "%2X", &c );

            *w = c;

            r += 2;
        }
        else
            *w = *r;
    }

    *w = NULL;
}


void
Audio_Sequence::draw ( void )
{

    Sequence::draw();

    int xfades = 0;

    /* draw crossfades */
    for ( list <Sequence_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); r++ )
    {
        Sequence_Widget *o = overlaps( *r );

        if ( o )
        {
            if ( *o <= **r )
            {

/*                 if ( o->x() == (*r)->x() && o->w() == (*r)->w() ) */
/*                     printf( "complete superposition\n" ); */

                if ( (*r)->x() >= o->x() && (*r)->x() + (*r)->w() <= o->x() + o->w() )
                    /* completely inside */
                    continue;

                ++xfades;

                Rectangle b( (*r)->x(),
                               o->y(),
                               (o->x() + o->w()) - (*r)->x(),
                               o->h() );

                Fl_Color c = fl_color_average( o->box_color(), (*r)->box_color(), 0.50f );
                c = fl_color_average( c, FL_YELLOW, 0.30f );

                fl_push_clip( b.x, b.y, b.w, b.h );

                draw_box( FL_FLAT_BOX, b.x - 100, b.y, b.w + 200, b.h, c );
                draw_box( FL_UP_FRAME, b.x - 100, b.y, b.w + 200, b.h, c );


                fl_pop_clip();

            }
        }

    }

    for ( list <Sequence_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); r++ )
    {
        Sequence_Widget *o = overlaps( *r );

        if ( o )
        {
            if ( *o <= **r )
            {

                if ( (*r)->x() >= o->x() && (*r)->x() + (*r)->w() <= o->x() + o->w() )
                    /* completely inside */
                    continue;

                Rectangle b( (*r)->x(), o->y(), (o->x() + o->w()) - (*r)->x(), o->h() );

                /* draw overlapping waveforms in X-ray style. */
                bool t = Waveform::fill;

                Waveform::fill = false;

                fl_push_clip( b.x, b.y, b.w, b.h );

                o->draw();
                (*r)->draw();

                fl_pop_clip();

                Waveform::fill = t;

            }
        }
    }
}

/** event handler that supports DND of audio clips */
int
Audio_Sequence::handle ( int m )
{
    switch ( m )
    {
        case FL_PASTE:
        {
            const char *text = Fl::event_text();

            if ( ! strcmp( text, "Audio_Region" ) )
                return 1;

            char *file;

            if ( ! sscanf( text, "file://%a[^\r\n]\n", &file ) )
            {
                printf( "invalid drop \"%s\"\n", text );
                return 0;
            }

            deurlify( file );

            printf( "pasted file \"%s\"\n", file );

            fl_cursor( FL_CURSOR_WAIT );
            Fl::check();

            Audio_File *c = Audio_File::from_file( file );

            fl_cursor( FL_CURSOR_DEFAULT );

            if ( ! c )
            {
                fl_alert( "Could not import file \"%s\": Unsupported file type.", file );
                printf( "could not open file\n" );
                free( file );
                return 0;
            }

            free( file );

//            Audio_Region *r =
            new Audio_Region( c, this, timeline->xoffset + timeline->x_to_ts( Fl::event_x() - x() ) );

            redraw();
            return 1;
        }
        default:
            return Sequence::handle( m );
    }
}


/**********/
/* Engine */
/**********/

/* THREAD: IO */
/** determine region coverage and fill /buf/ with interleaved samples
 * from /frame/ to /nframes/ for exactly /channels/ channels. */
nframes_t
Audio_Sequence::play ( sample_t *buf, nframes_t frame, nframes_t nframes, int channels )
{
    sample_t *cbuf = new sample_t[ nframes ];

    memset( cbuf, 0, nframes * sizeof( sample_t ) );

    /* quick and dirty--let the regions figure out coverage for themselves */
    for ( list <Sequence_Widget *>::const_iterator i = _widgets.begin();
          i != _widgets.end(); ++i )
    {
        const Audio_Region *r = (Audio_Region*)(*i);

        for ( int i = channels; i--;  )
        {
            int nfr;

            if ( ! ( nfr = r->read( cbuf, frame, nframes, i ) ) )
                /* error ? */
                continue;

            if ( channels == 1 )
                buffer_mix( buf, cbuf, nframes );
            else
                buffer_interleave_one_channel_and_mix( buf, cbuf, i, channels, nframes );
        }
    }

    delete[] cbuf;

    /* FIXME: bogus */
    return nframes;
}

/* /\* THREAD: RT *\/ */
/* nframes_t */
/* Audio_Sequence::process ( nframes_t nframes ) */
/* { */
/*     return disktream->process( nframes ); */
/* } */
