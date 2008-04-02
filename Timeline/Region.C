
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
    _offset    = rhs._offset;
    _track     = rhs._track;
//    _track     = NULL;
    _clip      = rhs._clip;
    _start     = rhs._start;
    _end       = rhs._end;
    _scale     = rhs._scale;
    _box_color = rhs._box_color;
    _color     = rhs._color;

    log_create();
}

Track_Widget *
Region::clone ( const Track_Widget *r )
{
    return new Region( *(Region*)r );
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

    int sum = 0;
    const char *s = rindex( _clip->name(), '/' );
    for ( int i = strlen( s ); i--; )
        sum += s[ i ];

    while ( sum >> 8 )
        sum = (sum & 0xFF) + (sum >> 8);

    _color = (Fl_Color)sum;

/*     _color = fl_color_average( FL_YELLOW, (Fl_Color)sum, 0.80 ); */

//    _color = FL_YELLOW;

    _box_color = FL_WHITE;

    log_create();
}

void
Region::trim ( enum trim_e t, int X )
{

    X -= _track->x();
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
    static int ox, oy;
    static enum trim_e trimming;

    static bool copied = false;
    static nframes_t os;

//    int X = Fl::event_x() - _track->x();
    int X = Fl::event_x();
    int Y = Fl::event_y();

    int ret;

    Logger _log( this );
//log_start();

    switch ( m )
    {
        case FL_ENTER:
            Track_Widget::handle( m );
            redraw();
            break;
        case FL_LEAVE:
            Track_Widget::handle( m );
            redraw();
            break;
        case FL_PUSH:
        {

            /* trimming / splitting  */
            if ( Fl::event_shift() && ! Fl::event_ctrl() )
            {
                switch ( Fl::event_button() )
                {
                    case 1:
                        trim( trimming = LEFT, X );
                        _drag = new Drag( x() - X, y() - Y );
                        _log.hold();
                        break;
                    case 3:
                        trim( trimming = RIGHT, X );
                        _drag = new Drag( x() - X, y() - Y );
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
                            return 0;
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
                /* for panning */
                os = _start;

                /* normalization and selection */
                if ( Fl::event_button2() )
                {
                    if ( Fl::event_ctrl() )
                        normalize();
                    else
                    {
                        if ( Track_Widget::current() == this )
                        {
                            if ( selected() )
                                deselect();
                            else
                                select();
                        }
                    }

                    redraw();
                    goto changed;
                }
                if ( Fl::event_button1() && Fl::event_ctrl() )
                {
                    /* duplication */
                    return 1;
                }
                else
                    return Track_Widget::handle( m );
            }
            break;
        }
        case FL_RELEASE:

        {
            Track_Widget::handle( m );

            copied = false;
            if ( trimming != NO )
                trimming = NO;

            goto changed;
        }
        case FL_DRAG:

            if ( ! _drag )
            {
                _drag = new Drag( x() - X, y() - Y );
                _log.hold();
            }

            /* panning */
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

            /* trimming */
            if ( Fl::event_state() & FL_SHIFT )
                if ( trimming )
                {
                    trim( trimming, X );
                    return 1;
                }
                else
                    return 0;

            /* duplication */
            if ( Fl::event_state() & FL_CTRL )
            {
                if ( _drag->state == 0 )
                {
                    _track->add( new Region( *this ) );
                    _drag->state = 1;
                    return 1;
                }
            }

            /* track jumping */
            if ( ! selected() )
            {
                if ( Y > y() + h() )
                {
                    Fl::copy( class_name(), strlen( class_name() ), 0 );
                    Fl::dnd();
                }
                else
                    if ( Y < y() )
                    {
                        Fl::copy( class_name(), strlen( class_name() ), 0 );
                        Fl::dnd();
                    }
            }

            ret = Track_Widget::handle( m );
            return ret | 1;
        default:
            return Track_Widget::handle( m );
            break;
    }

changed:

    return 1;

}

void
Region::draw_box( int X, int Y, int W, int H )
{
    if ( ! shown() )
        return;

    /* dirty hack to keep the box from flipping to vertical at small sizes */

    fl_push_clip( x(), Y, w(), H );

    if ( selected() )
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
    if ( ! shown() )
        return;

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

    /* get actual peak data */
    int channels;
    int peaks;
    Peak *pbuf;

    const nframes_t start = _start + offset + timeline->x_to_ts( X - rx );
    _clip->read_peaks( timeline->fpp(),
                       start,
                       start + timeline->x_to_ts( W ),
                       &peaks, &pbuf, &channels );

    assert( pbuf );

    int ch = (h() - Fl::box_dh( box() ))  / channels;

    for ( int i = 0; i < channels; ++i )
    {
        Peak *pb = pbuf + (peaks * i);

        /* scale it */
        for ( int j = peaks; j--; )
        {
            pb[ j ].min *= _scale;
            pb[ j ].max *= _scale;
        }

        Waveform::draw( X, (y() + Fl::box_dy( box() )) + (i * ch), W, ch,
                        pb, peaks,
                        selected() ? fl_invert_color( _color ) : _color );
    }

    delete pbuf;

/*     for ( int i = _clip->channels(); i--; ) */
/*         Waveform::draw( rx, X, (y() + Fl::box_dy( box() )) + (i * ch), W, */
/*                         ch, _clip, i, timeline->fpp(), */
/*                        _start + offset, min( (_end - _start) - offset, _end), */
/*                        _scale, selected() ? fl_invert_color( _color ) : _color ); */


    timeline->draw_measure_lines( rx, Y, rw, H, _box_color );

    fl_color( FL_BLACK );
    fl_line( rx, Y, rx, Y + H );
    fl_line( rx + rw - 1, Y, rx + rw - 1, Y + H );

    draw_label( _clip->name(), align() );

    if ( current() )
    {
        /* draw length bubble */

        char pat[40];

        snprintf( pat, sizeof( pat ), "%dm:%.1fs", (int)(length() / timeline->sample_rate()) / 60, (double)length() / timeline->sample_rate() );

        draw_label( pat, (Fl_Align)(FL_ALIGN_INSIDE | FL_ALIGN_CENTER), FL_GREEN );
    }

    fl_pop_clip();

}


void
Region::normalize ( void )
{
    printf( "normalize: start=%lu end=%lu\n", _start, _end );

    /* FIXME: figure out a way to do this via the peak server */
/*     _scale = _clip->peaks( 0 )->normalization_factor( timeline->fpp(), _start, _end );  */

}
