
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

#include <FL/fl_draw.H>


queue <Track_Widget *> Track::_delete_queue;



static bool
sort_func ( Track_Widget *lhs, Track_Widget *rhs )
{
    return *lhs < *rhs;
}


void
Track::sort ( void )
{
    _widgets.sort( sort_func );
}

void
Track::draw ( void )
{

    if ( ! fl_not_clipped( x(), y(), w(), h() ) )
        return;

    fl_push_clip( x(), y(), w(), h() );

    Fl_Group::draw();

    timeline->draw_measure_lines( x(), y(), w(), h(), color() );

    int X, Y, W, H;

    fl_clip_box( x(), y(), w(), h(), X, Y, W, H );

//    printf( "track::draw %d,%d %dx%d\n", X,Y,W,H );

    for ( list <Track_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); r++ )
        (*r)->draw_box( X, Y, W, H );

    /* TODO: detect overlap and draw with transparency/crossfade */
    for ( list <Track_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); r++ )
        (*r)->draw( X, Y, W, H );

    fl_pop_clip();
}

void
Track::remove ( Track_Widget *r )
{
//    Logger _log( this );

    _widgets.remove( r );
}


void
Track::remove_selected ( void )
{
    Loggable::block_start();

    for ( list <Track_Widget *>::iterator r = _widgets.begin(); r != _widgets.end(); )
        if ( (*r)->selected() )
        {
            Track_Widget *t = *r;
            _widgets.erase( r++ );
            delete t;
        }
        else
            ++r;

    Loggable::block_end();
}


Track_Widget *
Track::event_widget ( void )
{
    nframes_t ets = timeline->xoffset + timeline->x_to_ts( Fl::event_x() - x() );
    for ( list <Track_Widget *>::const_reverse_iterator r = _widgets.rbegin();  r != _widgets.rend(); r++ )
        if ( ets > (*r)->offset() && ets < (*r)->offset() + (*r)->length() )
            return (*r);

    return NULL;
}

void
Track::select_range ( int X, int W )
{
    nframes_t sts = timeline->xoffset + timeline->x_to_ts( X - x() );
    nframes_t ets = sts + timeline->x_to_ts( W );

    for ( list <Track_Widget *>::const_reverse_iterator r = _widgets.rbegin();  r != _widgets.rend(); r++ )
        if ( ! ( (*r)->offset() > ets || (*r)->offset() + (*r)->length() < sts ) )
            (*r)->select();
}

void
Track::add ( Track_Widget *r )
{
//    Logger _log( this );

    if ( r->track() )
    {
        r->redraw();
        r->track()->remove( r );
//        r->track()->redraw();
    }

    r->track( this );
    _widgets.push_back( r );

    sort();
}

/* snap /r/ to nearest edge */
void
Track::snap ( Track_Widget *r )
{
    const int snap_pixels = 10;

    const int rx1 = r->x();
    const int rx2 = r->x() + r->w();


    for ( list <Track_Widget*>::iterator i = _widgets.begin(); i != _widgets.end(); i++ )
    {
        const Track_Widget *w = (*i);

        if ( w == r )
            continue;

        const int wx1 = w->x();
        const int wx2 = w->x() + w->w();

        if ( abs( rx1 - wx2 ) < snap_pixels )
        {
            r->offset( w->offset() + w->length() + 1 );

//            printf( "snap: %lu | %lu\n", w->offset() + w->length(), r->offset() );

            goto done;
        }

        if ( abs( rx2 - wx1 ) < snap_pixels )
        {
            r->offset( ( w->offset() - r->length() ) - 1 );

//            printf( "snap: %lu | %lu\n", r->offset() + r->length(), w->offset() );

            goto done;
        }
    }

    {
        int nx = timeline->nearest_line( r->abs_x() );

        if ( nx >= 0 )
        {
            r->offset( timeline->x_to_ts( nx ) );
            return;
        }
    }
//    r->offset( timeline->x_to_ts( r->x() ) );

done:

    return;
//    r->resize();
//            r->position( rx1, y() );
}

int
Track::handle ( int m )
{
    static Track_Widget *pushed;

    switch ( m )
    {
        case FL_MOVE:
            /* these aren't used, so don't bother doing lookups for them */
            return 1;
        default:
        {
            Track_Widget *r = pushed ? pushed : event_widget();

            if ( r )
            {
                int retval = r->dispatch( m );

                if ( retval && m == FL_PUSH )
                    pushed = r;

                if ( retval && m == FL_RELEASE )
                    pushed = NULL;

                while ( _delete_queue.size() )
                {

                    delete _delete_queue.front();
                    _delete_queue.pop();
                    pushed = NULL;
                }

                return retval;
            }
            else
                return Fl_Group::handle( m );
        }
    }
}
