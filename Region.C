
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
#include "Region.H"
#include "Timeline.H"
#include "Waveform.H"

#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Box.H>

#include <stdio.h>

#include <algorithm>
//using  std::algorithm;
using namespace std;

extern Timeline *timeline;

Fl_Boxtype Region::_box = FL_UP_BOX;

Fl_Color Region::_selection_color = FL_MAGENTA;

static Fl_Color fl_invert_color ( Fl_Color c )
{
    unsigned char r, g, b;

    Fl::get_color( c, r, g, b );

    return fl_rgb_color( 255 - r, 255 - g, 255 - b );
}


#if 0
/* perhaps use map? */
map_PRIM ( set )
{
/*     if ( narg % 2 != 0 ) */
/*         printf( "invalid number of arguments\n" ); */

    int id = atoi( arg );

    map_ARG_NEXT( arg, end );

    Logable *l = Loggable::find( id );

    char **sa = malloc( sizeof( char * ) * narg + 1 );

    for ( int i = 0; i < narg; ++i )
        sa[ i ] = strdup( map_ARG_NEXT( arg, end ) );

    l->set( sa );

    map_RESULT( "" );
}
#endif


void
Region::init ( void )
{

    _track = NULL;
    _offset = 0;
    _start = 0;
    _end = 0;
    _scale = 1.0f;
    _clip = NULL;


    _box_color = FL_CYAN;
    _color = FL_BLUE;
}

/* copy constructor */
Region::Region ( const Region & rhs )
{
    _offset = rhs._offset;
    _track  = rhs._track;
    _clip   = rhs._clip;
    _start  = rhs._start;
    _end    = rhs._end;
    _scale  = rhs._scale;

    log_create();
}

/*  */
Region::Region ( Audio_File *c )
{
    init();
    _clip = c;
    _end = _clip->length();

    log_create();
}


/* used when DND importing */
Region::Region ( Audio_File *c, Track *t, nframes_t o )
{
    init();
    _clip = c;
    _end = _clip->length();
    _track = t;
    _offset = o;

    _track->add( this );

    log_create();
}

void
Region::trim ( enum trim_e t, int X )
{

    redraw();

    switch ( t )
    {
        case LEFT:
        {
/*             if ( d < 0 ) */
/* //                _track->damage( FL_DAMAGE_EXPOSE, x() + d, y(), 1 - d, h() ); */
/*                 _track->damage( FL_DAMAGE_EXPOSE, x(), y(), w(), h() ); */
/*             else */
/*                 _track->damage( FL_DAMAGE_EXPOSE, x(), y(), d, h() ); */

            int d = X - ( abs_x() - scroll_x() );

            long td = timeline->x_to_ts( d );

            if ( td < 0 && _start < 0 - td )
                td = 0 - _start;

            if ( _start + td >= _end )
                td = (_end - _start) - timeline->x_to_ts( 1 );

            _start += td;
            _offset += td;
            break;
        }
        case RIGHT:
        {
            int d = (( abs_x() - scroll_x() ) + abs_w() ) - X;

/*             _track->damage( FL_DAMAGE_EXPOSE, x() + w(), y(), d, h() ); */

            long td = timeline->x_to_ts( d );

//            printf( "%li %li\n", td, _end - _start );

            if ( td >= 0 && _end - _start < td )
                _end = _start + timeline->x_to_ts( 1 );
            else
                _end -= td;

            break;
        }
        default:
            return;

    }
}

int
Region::handle ( int m )
{
    static bool dragging = false;

    static int ox, oy;
    static enum trim_e trimming;

    static bool copied = false;
    static nframes_t os;

    int X = Fl::event_x() - _track->x();
    int Y = Fl::event_y();

    int ret;

    Logger _log( this );
//log_start();

    switch ( m )
    {
        case FL_PUSH:
        {

            if ( Fl::event_state() & FL_SHIFT &&
                 ! ( Fl::event_state() & FL_CTRL ))
            {
                switch ( Fl::event_button() )
                {
                    case 1:
                        trim( trimming = LEFT, X );
                        _log.hold();
                        break;
                    case 3:
                        trim( trimming = RIGHT, X );
                       _log.hold();
                        break;
                    case 2:
                    {
                        /* split */
                        if ( ! copied )
                        {
                            Loggable::block_start();

                            Region *copy = new Region( *this );

                            trim( RIGHT, X );
                            copy->trim( LEFT, X );

                            _track->add( copy );

                            log_end();

                            Loggable::block_end();
                            return 1;
                        }
                    }
                    default:
                        return 0;
                        break;
                }

                fl_cursor( FL_CURSOR_WE );
                return 1;
            }
            else
            {
                ox = x() - X;
                oy = y() - Y;

                if ( Fl::event_state() && FL_CTRL )
                {
                    os = _start;
//                    Fl::local_grab( this );
                }


                if ( Fl::event_button() == 2 )
                {
                    if ( Fl::event_state() & FL_CTRL )
                        normalize();
                    else
                        _selected = ! _selected;

                    redraw();
                    goto changed;
                }


                ret = Track_Widget::handle( m );

                return ret | 1;

            }
            break;
        }
        case FL_RELEASE:
            Track_Widget::handle( m );
            copied = false;
            if ( trimming != NO )
            {
                trimming = NO;
                _log.release();
            }
            if ( dragging )
                _log.release();

            dragging = false;
            goto changed;
        case FL_DRAG:

            if ( ! dragging )
            {
                _log.hold();
                dragging = true;
            }

            if ( Fl::event_state() & FL_SHIFT &&
                 Fl::event_state() & FL_CTRL )
            {
                int d = (ox + X) - x();
                long td = timeline->x_to_ts( d );

                nframes_t W = _end - _start;

                if ( td > 0 && os < td )
                    _start = 0;
                else
                    _start = os - td;

                _end = _start + W;

                _track->redraw();
                return 1;
            }

            if ( Fl::event_state() & FL_SHIFT )
                if ( trimming )
                {
                    trim( trimming, X );
                    return 1;
                }
                else
                    return 0;

            if ( Fl::event_state() & FL_CTRL )
            {
                if ( ! copied )
                {
                    _track->add( new Region( *this ) );
                    copied = true;
                    return 1;
                }
            }

            if ( Y > y() + h() )
            {
                if ( _track->next() )
                    if ( Y > _track->next()->y() )
                        _track->next()->add( this );
            }
            else
                if ( Y < y() )
                {
                    if ( _track->prev() )
                        if ( Y < _track->prev()->y() + _track->prev()->h() )
                            _track->prev()->add( this );
                }

            //                  _track->damage( FL_DAMAGE_EXPOSE, x(), y(), w(), h() );

            ret = Track_Widget::handle( m );
            return ret | 1;
        default:
            return Track_Widget::handle( m );
            break;
    }

changed:

//    log_end();
    return 1;

}

void
Region::draw_box( int X, int Y, int W, int H )
{
    /* dirty hack to keep the box from flipping to vertical at small sizes */

    fl_push_clip( x(), Y, w(), H );

    if ( _selected )
        fl_draw_box( fl_down( box() ), x() - 10, y(), w() + 50, h(), _selection_color );
//        fl_draw_box( fl_down( box() ), x() - 10, Y, w() + 50, H, fl_invert_color( _box_color ) );
    else
        fl_draw_box( box(), x() - 10, y(), w() + 50, h(), _box_color );

    fl_pop_clip();
}

/* Draw (part of) region. OX is pixel offset from start of timeline, X
   Y W and H are the portion of the widget to draw (arrived at by
   intersection of the clip and relative to OX) */
void
Region::draw ( int X, int Y, int W, int H )
{
    if ( ! ( W > 0 && H > 0 ) )
        return;

    int OX = scroll_x();
    int ox = timeline->ts_to_x( _offset );

    if ( ox > OX + _track->w() ||
         ox < OX && ox + abs_w() < OX )
        return;

    int rw = timeline->ts_to_x( _end - _start );

    nframes_t end = _offset + ( _end - _start );

    /* calculate waveform offset due to scrolling */
    nframes_t offset = 0;
    if ( ox < OX )
    {
        offset = timeline->x_to_ts( OX - ox );

        rw = timeline->ts_to_x( (_end - _start) - offset );
    }

    rw = min( rw, _track->w() );

    int rx = x();

    fl_push_clip( rx, Y, rw, H );

    /* dirty hack to keep the box from flipping to vertical at small sizes */
//    fl_draw_box( box(), rx - 10, Y, rw + 50, H, _box_color );


//    fl_push_clip( x() + Fl::box_dx( box() ), y(), w() - Fl::box_dw( box() ), h() );

    int ch = (h() - Fl::box_dh( box() ))  / _clip->channels();
    for ( int i = _clip->channels(); i--; )
//        draw_waveform( rx, y() + (i * ch), rw, ch, _clip, i,
//                       _start + offset, min( (_end - _start) - offset, _end),
//                       _scale, _selected ? _color : fl_invert_color( _color )  );
        draw_waveform( rx, X, (y() + Fl::box_dy( box() )) + (i * ch), W, ch, _clip, i,
                       _start + offset, min( (_end - _start) - offset, _end),
                       _scale, _selected ? _color : fl_invert_color( _color )  );


    timeline->draw_measure_lines( rx, Y, rw, H, _box_color );

    fl_color( FL_BLACK );
    fl_line( rx, Y, rx, Y + H );
    fl_line( rx + rw - 1, Y, rx + rw - 1, Y + H );

    draw_label( _clip->name(), align() );



/*     if ( _selected ) */
/*     { */
/*         fl_color( selection_color() ); */

/*         fl_line_style( FL_SOLID, 4 ); */
/*         fl_rect( x(), y(), w(), h() ); */
/*         fl_line_style( FL_SOLID, 0 ); */
/*     } */

    fl_pop_clip();

}


void
Region::normalize ( void )
{
    printf( "normalize: start=%lu end=%lu\n", _start, _end );

    /* FIXME: punt */
    _scale = _clip->peaks( 0 )->normalization_factor( _start, _end );
}


void
Region::dump ( void )
{
//    printf( "Region %p %lu { \"%s\" %lu %lu }\n", this, _offset, _clip->name(),  _start, _end );
    /* how about in STD?  */
    printf( "Region\n\t%p\n\toffset\n\t\t%lu\n\tranage\n\t\t%lu\n\t\t%lu\n\tsource\n\t\t\"%s\"\n\n", this, _offset, _start, _end, _clip->name() );
}
