
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

#include "Track.H"

#include <FL/Fl_Input.H>

class Audio_Track : public Track
{

public:

    Audio_Track ( int X, int Y, int W, int H ) : Track( X, Y, W, H )
        {

/*             Fl_Group *o = new Fl_Group( 0, 0, 100, H ); */
/*             new Fl_Input( 0, 0, 100, 24, "name:" ); */
/*             o->end(); */

        }

    /** event handler that supports DND of audio clips */
    int
    handle ( int m )
        {
            switch ( m )
            {
                case FL_DND_DRAG:
                case FL_DND_ENTER:
                case FL_ENTER:
                    return 1;
                case FL_DND_LEAVE:
                case FL_DND_RELEASE:
                    return 1;
                case FL_PASTE:
                {
                    const char *text = Fl::event_text();

                    char *file;

                    if ( ! sscanf( text, "file://%a[^\r\n]\n", &file ) )
                    {
                        printf( "invalid drop \"%s\"\n", text );
                        return 0;
                    }

                    printf( "pasted file \"%s\"\n", file );

                    Clip *c = Clip::from_file( file );

                    if ( ! c )
                    {
                        free( file );
                        return 0;
                    }

                    Region *r = new Region( c );

                    r->offset( timeline.x_to_ts( Fl::event_x() ) );

                    this->add( r );

                    redraw();
                    return 1;
                }
                default:
                    return Track::handle( m );
            }
        }
};
