
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

#include "Sequence.H"
#include "Timeline.H"

#include "Audio_Region.H"

#include <FL/fl_draw.H>

#include "Track.H"

#include "../FL/Boxtypes.H"

queue <Sequence_Widget *> Sequence::_delete_queue;

Sequence::Sequence ( Track *track ) : Fl_Widget( 0, 0, 0, 0 ), Loggable( true  )
{
    init();

    _track = track;

//    log_create();
}

Sequence::Sequence ( int X, int Y, int W, int H ) : Fl_Widget( X, Y, W, H ), Loggable( false )
{
    init();
}

void
Sequence::init ( void )
{
    _track = NULL;

    _name = NULL;

    box( FL_DOWN_BOX );
    color(  FL_BACKGROUND_COLOR );
    align( FL_ALIGN_LEFT );
}

Sequence::~Sequence (  )
{
    if ( _name )
        free( _name );

    for ( std::list <Sequence_Widget*>::iterator i = _widgets.begin();
          i != _widgets.end(); ++i )
    {
        Sequence_Widget *w = *i;

        *i = NULL;

        delete w;
    }

    _widgets.clear();

    parent()->redraw();
    parent()->remove( this );
//    log_destroy();
}

nframes_t
Sequence::x_to_offset ( int X )
{
    return timeline->xoffset + timeline->x_to_ts( X - x() );
}

void
Sequence::sort ( void )
{
    _widgets.sort( Sequence_Widget::sort_func );
}

/** return a pointer to the widget that /r/ overlaps, or NULL if none. */
Sequence_Widget *
Sequence::overlaps ( Sequence_Widget *r )
{
    for ( list <Sequence_Widget *>::const_iterator i = _widgets.begin(); i != _widgets.end(); i++ )
    {
        if ( *i == r ) continue;
        if ( ! ( (*i)->start() > r->start() + r->length() || (*i)->start() + (*i)->length() < r->start() ) )
            return *i;
    }

    return NULL;
}


void
Sequence::draw ( void )
{

    if ( ! fl_not_clipped( x(), y(), w(), h() ) )
        return;

    fl_push_clip( x(), y(), w(), h() );

    /* draw the box with the ends cut off. */
    draw_box( box(), x() - Fl::box_dx( box() ) - 1, y(), w() + Fl::box_dw( box() ) + 2, h(), color() );

    int X, Y, W, H;

    fl_clip_box( x(), y(), w(), h(), X, Y, W, H );

    if ( Sequence_Widget::pushed() && Sequence_Widget::pushed()->sequence() == this )
    {
        /* make sure the Sequence_Widget::pushed widget is above all others */
        remove( Sequence_Widget::pushed() );
        add( Sequence_Widget::pushed() );
    }

//    printf( "track::draw %d,%d %dx%d\n", X,Y,W,H );

    timeline->draw_measure_lines( X, Y, W, H, color() );

    for ( list <Sequence_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); ++r )
        (*r)->draw_box();


    for ( list <Sequence_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); ++r )
        (*r)->draw();

    fl_pop_clip();

}

void
Sequence::remove ( Sequence_Widget *r )
{
//    Logger _log( this );

    _widgets.remove( r );
}


void
Sequence::remove_selected ( void )
{
    Loggable::block_start();

    for ( list <Sequence_Widget *>::iterator r = _widgets.begin(); r != _widgets.end(); )
        if ( (*r)->selected() )
        {
            Sequence_Widget *t = *r;
            _widgets.erase( r++ );
            delete t;
        }
        else
            ++r;

    Loggable::block_end();
}


Sequence_Widget *
Sequence::event_widget ( void )
{
    nframes_t ets = timeline->xoffset + timeline->x_to_ts( Fl::event_x() - x() );
    for ( list <Sequence_Widget *>::const_reverse_iterator r = _widgets.rbegin();  r != _widgets.rend(); r++ )
        if ( ets > (*r)->start() && ets < (*r)->start() + (*r)->length()
             && Fl::event_y() >= (*r)->y() && Fl::event_y() <= (*r)->y() + (*r)->h() )
            return (*r);

    return NULL;
}

void
Sequence::select_range ( int X, int W )
{
    nframes_t sts = x_to_offset( X );
    nframes_t ets = sts + timeline->x_to_ts( W );

    for ( list <Sequence_Widget *>::const_reverse_iterator r = _widgets.rbegin();  r != _widgets.rend(); r++ )
        if ( ! ( (*r)->start() > ets || (*r)->start() + (*r)->length() < sts ) )
            (*r)->select();
}

void
Sequence::add ( Sequence_Widget *r )
{
//    Logger _log( this );

    if ( r->sequence() )
    {
        r->redraw();
        r->sequence()->remove( r );
//        r->track()->redraw();
    }

    r->sequence( this );
    _widgets.push_back( r );

    sort();
}

static nframes_t
abs_diff ( nframes_t n1, nframes_t n2 )
{
    return n1 > n2 ? n1 - n2 : n2 - n1;
}

/* snap /r/ to nearest edge */
void
Sequence::snap ( Sequence_Widget *r )
{
    const int snap_pixels = 10;
    const int snap_frames = timeline->x_to_ts( snap_pixels );

    /* snap to other widgets */
    if ( Timeline::snap_magnetic )
    {
        const int rx1 = r->start();
        const int rx2 = r->start() + r->length();

        for ( list <Sequence_Widget*>::const_iterator i = _widgets.begin(); i != _widgets.end(); i++ )
        {
            const Sequence_Widget *w = (*i);

            if ( w == r )
                continue;

            const int wx1 = w->start();
            const int wx2 = w->start() + w->length();

            if ( abs_diff( rx1, wx2 ) < snap_frames )
            {
                r->start( w->start() + w->length() + 1 );

                return;
            }

            if ( abs_diff( rx2, wx1 ) < snap_frames )
            {
                r->start( ( w->start() - r->length() ) - 1 );

                return;
            }
        }
    }

    nframes_t f;

    /* snap to beat/bar lines */
    if ( timeline->nearest_line( r->start(), &f ) )
        r->start( f );
}

int
Sequence::handle ( int m )
{

    switch ( m )
    {
        case FL_FOCUS:
        case FL_UNFOCUS:
        case FL_LEAVE:
            fl_cursor( FL_CURSOR_DEFAULT );
            return 1;
        case FL_DND_DRAG:
            return 1;
        case FL_ENTER:
            if ( Sequence_Widget::pushed() )
            {

                if ( Sequence_Widget::pushed()->sequence()->class_name() == class_name() )
                {
                    /* accept objects dragged from other sequences of this type */
                    add( Sequence_Widget::pushed() );
                    redraw();

                    fl_cursor( FL_CURSOR_MOVE );
                }
                else
                    fl_cursor( (Fl_Cursor)1 );
            }
            else
                fl_cursor( cursor() );

            return 1;
        case FL_DND_ENTER:
        case FL_DND_LEAVE:
        case FL_DND_RELEASE:
            return 1;
        case FL_MOVE:
        {
            Sequence_Widget *r = event_widget();

            if ( r != Sequence_Widget::belowmouse() )
            {
                if ( Sequence_Widget::belowmouse() )
                    Sequence_Widget::belowmouse()->handle( FL_LEAVE );
                Sequence_Widget::belowmouse( r );

                if ( r )
                    r->handle( FL_ENTER );
            }

            return 0;
        }
        default:
        {
            Sequence_Widget *r = Sequence_Widget::pushed() ? Sequence_Widget::pushed() : event_widget();

            if ( r )
            {
                int retval = r->dispatch( m );

                if ( retval && m == FL_PUSH )
                {
                    take_focus();

                    Sequence_Widget::pushed( r );
                }

                if ( retval && m == FL_RELEASE )
                    Sequence_Widget::pushed( NULL );

                Loggable::block_start();

                while ( _delete_queue.size() )
                {

                    Sequence_Widget *t = _delete_queue.front();
                    _delete_queue.pop();


                    if ( Sequence_Widget::pushed() == t )
                        Sequence_Widget::pushed( NULL );
                    if ( Sequence_Widget::belowmouse() == t )
                    {
                        Sequence_Widget::belowmouse()->handle( FL_LEAVE );
                        Sequence_Widget::belowmouse( NULL );
                    }

                    timeline->wrlock();

                    delete t;

                    timeline->unlock();
                }

                Loggable::block_end();

                return retval;
            }
            else
                return Fl_Widget::handle( m );
        }
    }
}
