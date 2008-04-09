
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

#include "Audio_Track.H"


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

/** event handler that supports DND of audio clips */
int
Audio_Track::handle ( int m )
{
    switch ( m )
    {

       case FL_DND_DRAG:
           return Track::handle( m ) | 1;

/*         case FL_DND_ENTER: */
/*         case FL_DND_LEAVE: */

        case FL_DND_RELEASE:
            return 1;

        case FL_PASTE:
        {
            const char *text = Fl::event_text();


            if ( ! strcmp( text, "Region" ) )
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
                printf( "could not open file\n" );
                free( file );
                return 0;
            }

//            Region *r =
            new Region( c, this, timeline->xoffset + timeline->x_to_ts( Fl::event_x() - x() ) );

            redraw();
            return 1;
        }
        default:
            return Track::handle( m );
    }
}


/**********/
/* Engine */
/**********/

/* THREAD: IO */
/** determine region coverage and fill /buf/ with interleaved samples
 * from /frame/ to /nframes/ for exactly /channels/ channels. */
nframes_t
Audio_Track::play ( sample_t *buf, nframes_t frame, nframes_t nframes, int channels )
{
    sample_t *cbuf = new sample_t[ nframes ];

    /* quick and dirty--let the regions figure out coverage for themselves */
    for ( list <Track_Widget *>::const_iterator i = _widgets.begin(); i != _widgets.end(); ++i )
    {
        const Region *r = (Region*)(*i);

        for ( int i = channels; i--;  )
        {
            memset( cbuf, 0, nframes * sizeof( sample_t ) );

            if ( ! r->read( cbuf, frame, nframes, i ) )
                /* error ? */;

            if ( channels == 1 )
                memcpy( buf, cbuf, nframes * sizeof( sample_t ) );
            else
            {
                /* interleave */
                int k = 0;
                for ( unsigned int j = i; k < nframes; j += channels )
                    buf[ j ] = cbuf[ k++ ];
            }
        }
    }

    delete[] cbuf;

    /* FIXME: bogus */
    return nframes;
}

/* /\* THREAD: RT *\/ */
/* nframes_t */
/* Audio_Track::process ( nframes_t nframes ) */
/* { */
/*     return disktream->process( nframes ); */
/* } */
