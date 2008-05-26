
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

#include <FL/fl_draw.H>

#include "Sequence_Widget.H"

using namespace std;



list <Sequence_Widget *> Sequence_Widget::_selection;
Sequence_Widget * Sequence_Widget::_current = NULL;
Sequence_Widget * Sequence_Widget::_pushed = NULL;
Sequence_Widget * Sequence_Widget::_belowmouse = NULL;
Fl_Color Sequence_Widget::_selection_color = FL_MAGENTA;



void
Sequence_Widget::get ( Log_Entry &e ) const
{
    e.add( ":start", _r->start );
//    e.add( ":offset", _r->offset );
//    e.add( ":length", _r->length );
    e.add( ":sequence", _sequence );
    e.add( ":selected", selected() );
}

void
Sequence_Widget::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":start" ) )
            _r->start = atoll( v );
//        else if ( ! strcmp( s, ":offset" ) )
//            _r->offset = atoll( v );
//        else if ( ! strcmp( s, ":length" ) )
//            _r->length = atoll( v );
        else if ( ! strcmp( s, ":selected" ) )
        {
            if ( atoi( v ) )
                select();
            else
                deselect();
        }
        else if ( ! strcmp( s, ":sequence" ) )
        {
            int i;
            sscanf( v, "%X", &i );
            Sequence *t = (Sequence*)Loggable::find( i );

            assert( t );

            t->add( this );
        }
//                else
//                    e.erase( i );
    }

    if ( _sequence )
        _sequence->redraw();

}

void
Sequence_Widget::begin_drag ( const Drag &d )
{
    _drag = new Drag( d );

    timeline->rdlock();

    _r = new Range( _range );

    timeline->unlock();
}

void
Sequence_Widget::end_drag ( void )
{
    timeline->wrlock();

    /* swap in the new value */
    _range = *_r;

    timeline->unlock();

    delete _r;
    _r = &_range;

    delete _drag;
    _drag = NULL;

    sequence()->handle_widget_change( _r->start, _r->length );
}

/** set position of widget on the timeline. */
void
Sequence_Widget::start ( nframes_t where )
{
    /* this is pretty complicated because of selection and snapping */

    if  ( ! selected() )
    {
        redraw();
        _r->start = where;
    }
    else
    {
        if ( this != Sequence_Widget::_current )
            return;

        long d = where - _r->start;

        if ( d < 0 )
        {
            /* first, make sure we stop at 0 */
            nframes_t m = (nframes_t)-1;

            for ( list <Sequence_Widget *>::iterator i = _selection.begin(); i != _selection.end(); ++i )
                m = min( m, (*i)->_r->start );

            d = 0 - d;

            if ( m <= (nframes_t)d )
                d = m;

            for ( list <Sequence_Widget *>::iterator i = _selection.begin(); i != _selection.end(); ++i )
            {
                (*i)->redraw();
                (*i)->_r->start -= d;
            }
        }
        else
        {
            /* TODO: do like the above and disallow wrapping */
            for ( list <Sequence_Widget *>::iterator i = _selection.begin(); i != _selection.end(); ++i )
            {
                (*i)->redraw();
                (*i)->_r->start += d;
            }
        }
    }
}

void
Sequence_Widget::draw_label ( const char *label, Fl_Align align, Fl_Color color )
{
    int X, Y;

    X = x();
    Y = y();

    /* FIXME: why do we have to do this here? why doesn't Fl_Label::draw take care of this stuff? */
    if ( ! (align & FL_ALIGN_INSIDE) )
    {
        if ( align & FL_ALIGN_RIGHT )
        {
            X += abs_w();
            align = (Fl_Align)((align & ~FL_ALIGN_RIGHT) | FL_ALIGN_LEFT);
        }
        if ( align & FL_ALIGN_BOTTOM  )
        {
            Y += h();
            align = (Fl_Align)((align & ~FL_ALIGN_BOTTOM) | FL_ALIGN_TOP);
        }
    }


    Fl_Label lab;

    lab.color = color;
//    lab.type = FL_SHADOW_LABEL;
    lab.type = FL_NORMAL_LABEL;
    lab.value = label;
    lab.font = FL_HELVETICA;
    lab.size = 14;

    int lw = 0, lh = 0;

    fl_font( lab.font, lab.size );
    fl_measure( lab.value, lw, lh );

    int W = w();
    int H = h();

    if ( align & FL_ALIGN_INSIDE )
    {
        X += Fl::box_dx( box() );
        Y += Fl::box_dy( box() );
        W -= Fl::box_dw( box() );
        H -= Fl::box_dh( box() );
    }

    if ( align & FL_ALIGN_CLIP ) fl_push_clip( X, Y, W, H );

    int dx = 0;

    /* adjust for scrolling */
    if ( abs_x() < scroll_x() )
        dx = min( 32767, scroll_x() - abs_x() );

//        const Fl_Boxtype b = FL_ROUND_UP_BOX;
    const Fl_Boxtype b = FL_ROUNDED_BOX;
    const int bx = Fl::box_dx( b ) + 1;
    const int bw = Fl::box_dw( b ) + 1;

    if ( align & FL_ALIGN_BOTTOM )
        fl_draw_box( b, X - dx - bx, Y + H - lh, lw + bw, lh, FL_GRAY );
    else if ( align == FL_ALIGN_LEFT )
        fl_draw_box( b, X - dx, Y + ((H >> 1) - (lh >> 1)), lw + bw, lh, FL_GRAY );
    else if ( align & FL_ALIGN_TOP )
        fl_draw_box( b, X - dx - bx + ((W >> 1) - (lw >> 1)), Y + ((H >> 1) - (lh >> 1)), lw + bw, lh, FL_GRAY );

//    lab.draw( X - dx, Y, W, H, align );
    fl_color( color );
    fl_draw( label, ( X - dx ) + bx, Y, W, H, align );

    if ( align & FL_ALIGN_CLIP ) fl_pop_clip();

}

int
Sequence_Widget::dispatch ( int m )
{
    Sequence_Widget::_current = this;

    if ( selected() )
    {
        Loggable::block_start();

        int r = 0;

        for ( list <Sequence_Widget *>::iterator i = _selection.begin(); i != _selection.end(); i++ )
            if ( *i != this )
                r |= (*i)->handle( m );

        r |= handle( m );

        Loggable::block_end();

        return r;
    }
    else
        return handle( m );
}

void
Sequence_Widget::draw ( void )
{
    draw_box();
}

void
Sequence_Widget::draw_box ( void )
{
    fl_draw_box( box(), x(), y(), w(), h(), selected() ? FL_MAGENTA : _box_color );
}


/* base hanlde just does basic dragging */
int
Sequence_Widget::handle ( int m )
{
    int X = Fl::event_x();
    int Y = Fl::event_y();

    Logger _log( this );


    switch ( m )
    {
        case FL_ENTER:
            fl_cursor( FL_CURSOR_HAND );
            return 1;
        case FL_LEAVE:
//            DMESSAGE( "leave" );
            fl_cursor( sequence()->cursor() );
            return 1;
        case FL_PUSH:
        {
            /* deletion */
            if ( Fl::test_shortcut( FL_CTRL + FL_BUTTON3 ) && ! Fl::event_shift() )
            {
                redraw();
                sequence()->queue_delete( this );
                return 1;
            }
            else if ( Fl::test_shortcut( FL_BUTTON1 ) && ! Fl::event_shift() )
            {
                fl_cursor( FL_CURSOR_MOVE );

                /* movement drag */
                return 1;
            }

            return 0;
        }
        case FL_RELEASE:
            if ( _drag )
            {
                end_drag();
                _log.release();
            }

            fl_cursor( FL_CURSOR_HAND );

            return 1;
        case FL_DRAG:
        {
            if ( ! _drag )
            {
                begin_drag ( Drag( x() - X, y() - Y, x_to_offset( X ) ) );
                _log.hold();
            }

            if ( ( Fl::test_shortcut( FL_BUTTON1 + FL_CTRL ) ||
                   Fl::test_shortcut( FL_BUTTON1 ) )  && ! Fl::event_shift() )
            {
//                fl_cursor( FL_CURSOR_MOVE );

                redraw();

                const nframes_t of = timeline->x_to_offset( X );

                if ( of >= _drag->start )
                    start( of - _drag->start );
                else
                    start( 0 );

                if ( Sequence_Widget::_current == this )
                    sequence()->snap( this );

                if ( X >= sequence()->x() + sequence()->w() ||
                     X <= sequence()->x() )
                {
                    /* this drag needs to scroll */

                    nframes_t pos = timeline->xoffset;

                    nframes_t d = timeline->x_to_ts( 100 );

                    if ( X <= sequence()->x() )
                    {

                        if ( pos > d )
                            pos -= d;
                        else
                            pos = 0;
                    }
                    else
                        pos += d;

                    timeline->xposition( timeline->ts_to_x(  pos ) );

                    /* FIXME: why isn't this enough? */
//                sequence()->redraw();
                    timeline->redraw();
                }

                return 1;
            }
            else
                return 0;
        }
        default:
            return 0;
    }
}
