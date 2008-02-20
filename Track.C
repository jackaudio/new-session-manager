
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

#include "Track.H"
#include "Timeline.H"

#include "Region.H"

void
Track::draw ( void )
{
    Fl_Group::draw();
    for ( list <Region *>::iterator r = _regions.begin();  r != _regions.end(); r++ )
    {
        (*r)->draw( timeline.xoffset + x(), y(), w(), h() );
    }
}

void
Track::remove_region ( Region *r )
{
    _regions.remove( r );
}


Region *
Track::event_region ( void )
{
// FIXME: doesn't handle overlap!

    for ( list <Region *>::iterator r = _regions.begin();  r != _regions.end(); r++ )
    {
        int X = timeline.ts_to_x( (*r)->offset() ) - timeline.xoffset;
        int W = timeline.ts_to_x( (*r)->length() );

        if ( Fl::event_x() > X && Fl::event_x() < X + W )
            return (*r);
    }

    return NULL;
}

void
Track::add ( Region *r )
{
    if ( r->track() )
    {
        r->track()->remove_region( r );
        r->track()->redraw();
    }

    _regions.push_back( r );

    r->track( this );

    // Fl_Group::add( r );
//            add( r );

//    r->position( r->x(), y() );
//    r->redraw();
}

/** snap /r/ to nearest edge */
/* void */
/* Track::snap ( Region *r ) */
/* { */
/*     const int snap_pixels = 10; */

/*     int rx1 = r->x(); */
/*     int rx2 = r->x() + r->w(); */

/*     for ( int i = children(); i-- ; ) */
/*     { */
/*         const Region *w = (Region*)child( i ); */

/*         if ( w == r ) */
/*             continue; */

/*         int wx1 = w->x(); */
/*         int wx2 = w->x() + w->w(); */

/*         if ( abs( rx1 - wx2 ) < snap_pixels ) */
/*         { */
/*             r->offset( w->offset() + w->length() + 1 ); */

/*             printf( "snap: %lu | %lu\n", w->offset() + w->length(), r->offset() ); */

/*             goto done; */
/*         } */

/*         if ( abs( rx2 - wx1 ) < snap_pixels ) */
/*         { */
/*             r->offset( ( w->offset() - r->length() ) - 1 ); */

/*             printf( "snap: %lu | %lu\n", r->offset() + r->length(), w->offset() ); */

/*             goto done; */
/*         } */
/*     } */

/*     r->offset( timeline.x_to_ts( r->x() ) ); */

/* done: */

/*     r->resize(); */
/* //            r->position( rx1, y() ); */
/* } */

int
Track::handle ( int m )
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
//            r->position( Fl::event_x(), r->y() );

            this->add( r );

            return 1;
        }
        case FL_MOVE:
            /* these aren't used, so don't bother doing lookups for them */
            return 1;
        default:
        {
            Region *r = event_region();
            if ( r )
                return r->handle( m );
            else
                return Fl_Group::handle( m );
        }
    }
}
