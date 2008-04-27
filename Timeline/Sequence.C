
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

#include "Region.H"

#include <FL/fl_draw.H>

#include "Track.H"

#include "../FL/Boxtypes.H"

queue <Sequence_Widget *> Sequence::_delete_queue;

Sequence::Sequence ( int X, int Y, int W, int H, Track *track ) : Fl_Widget( X, Y, W, H )
{
    _name = NULL;
    _track = track;

/*     if ( track ) */
/*         track->add( this ); */

//    box( FL_DOWN_BOX );
    box( FL_CRYSTAL_DOWN_BOX );
    color( fl_darker( FL_GRAY ) );
    align( FL_ALIGN_LEFT );

//    log_create();
}

Sequence::~Sequence (  )
{
    /* FIXME: what to do with regions? */
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
        if ( ! ( (*i)->offset() > r->offset() + r->length() || (*i)->offset() + (*i)->length() < r->offset() ) )
            return *i;
    }

    return NULL;
}


#include "Waveform.H"

void
Sequence::draw ( void )
{

    if ( ! fl_not_clipped( x(), y(), w(), h() ) )
        return;

    fl_push_clip( x(), y(), w(), h() );

    /* draw the box with the ends cut off. */
    draw_box( box(), x() - Fl::box_dx( box() ), y(), w() + Fl::box_dw( box() ) + 1, h(), color() );

    int X, Y, W, H;

    fl_clip_box( x(), y(), w(), h(), X, Y, W, H );


    if ( Sequence_Widget::pushed() && Sequence_Widget::pushed()->track() == this )
    {
        /* make sure the Sequence_Widget::pushed widget is above all others */
        remove( Sequence_Widget::pushed() );
        add( Sequence_Widget::pushed() );
    }

    int xfades = 0;

//    printf( "track::draw %d,%d %dx%d\n", X,Y,W,H );

    timeline->draw_measure_lines( x(), y(), w(), h(), color() );

    for ( list <Sequence_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); r++ )
        (*r)->draw_box();


    for ( list <Sequence_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); r++ )
        (*r)->draw();


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

//    printf( "There are %d xfades\n", xfades );

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

/*                 Fl_Color oc = o->color(); */
/*                 Fl_Color rc = (*r)->color(); */

/*                 /\* give each region a different color *\/ */
/*                 o->color( FL_RED ); */
/*                 (*r)->color( FL_GREEN ); */

                fl_push_clip( b.x, b.y, b.w, b.h );

                o->draw();
                (*r)->draw();

                fl_pop_clip();

                Waveform::fill = t;


/*                 o->color( oc ); */
/*                 (*r)->color( rc ); */

/*                 fl_color( FL_BLACK ); */
/*                 fl_line_style( FL_DOT, 4 ); */

/*                 b.x = (*r)->line_x(); */
/*                 b.w = min( 32767, (*r)->abs_w() ); */

/*                 fl_line( b.x, b.y, b.x + b.w, b.y + b.h ); */

/*                 fl_line( b.x, b.y + b.h, b.x + b.w, b.y ); */

/*                 fl_line_style( FL_SOLID, 0 ); */

//                fl_pop_clip();

            }
        }
    }


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
        if ( ets > (*r)->offset() && ets < (*r)->offset() + (*r)->length() )
            return (*r);

    return NULL;
}

void
Sequence::select_range ( int X, int W )
{
    nframes_t sts = timeline->xoffset + timeline->x_to_ts( X - x() );
    nframes_t ets = sts + timeline->x_to_ts( W );

    for ( list <Sequence_Widget *>::const_reverse_iterator r = _widgets.rbegin();  r != _widgets.rend(); r++ )
        if ( ! ( (*r)->offset() > ets || (*r)->offset() + (*r)->length() < sts ) )
            (*r)->select();
}

void
Sequence::add ( Sequence_Widget *r )
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
Sequence::snap ( Sequence_Widget *r )
{
    const int snap_pixels = 10;

    const int rx1 = r->x();
    const int rx2 = r->x() + r->w();


    for ( list <Sequence_Widget*>::iterator i = _widgets.begin(); i != _widgets.end(); i++ )
    {
        const Sequence_Widget *w = (*i);

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
Sequence::handle ( int m )
{

    switch ( m )
    {
        case FL_FOCUS:
            return 1;
        case FL_UNFOCUS:
            return 1;
        case FL_ENTER:
        case FL_LEAVE:
            return 1;
        case FL_DND_ENTER:
            printf( "enter\n" );
            if ( Sequence_Widget::pushed() && Sequence_Widget::pushed()->track()->class_name() == class_name() )
            {
                add( Sequence_Widget::pushed() );
                redraw();
            }
        case FL_DND_LEAVE:
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
