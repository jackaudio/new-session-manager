
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

extern Timeline timeline;

void
Region::init ( void )
{

/*     align( FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_BOTTOM | FL_ALIGN_CLIP ); */
/*     labeltype( FL_SHADOW_LABEL ); */
/*     labelcolor( FL_WHITE ); */
/*     box( FL_PLASTIC_UP_BOX ); */

    _track = NULL;
    _offset = 0;
    _start = 0;
    _end = 0;
    _scale = 1.0f;
    _clip = NULL;


    _box_color = FL_CYAN;
    _color = FL_BLUE;
}

Region::Region ( const Region & rhs )
{
    _offset = rhs._offset;
    _track  = rhs._track;
    _clip   = rhs._clip;
    _start  = rhs._start;
    _end    = rhs._end;
    _scale  = rhs._scale;
}

Region::Region ( Clip *c )
{
    init();
    _clip = c;
    _end = _clip->length();
}


void
Region::trim ( enum trim_e t, int X )
{
    switch ( t )
    {
        case LEFT:
        {
            int d = X - x();

            long td = timeline.x_to_ts( d );

            if ( td < 0 && _start < 0 - td )
                td = 0 - _start;

            _start += td;

            _offset += td;

//            resize();
            break;
        }
        case RIGHT:
        {
            int d = (x() + w()) - X;
            long td = timeline.x_to_ts( d );

            _end -= td;

//            resize();

            break;
        }
        default:
            return;

    }

    _track->redraw();
//    redraw();
//    parent()->redraw();

}

int
Region::handle ( int m )
{

    static int ox, oy;
    static enum trim_e trimming;

    static bool copied = false;
    static nframes_t os;

    int X = Fl::event_x();
    int Y = Fl::event_y();

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
                        break;
                    case 3:
                        trim( trimming = RIGHT, X );
                        break;

                    default:
                        return 0;

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

/*                 if ( Fl::event_button() == 2 ) */
/*                     normalize(); */


                return 1;
            }
            return 0;
            break;
        }
        case FL_RELEASE:
            fl_cursor( FL_CURSOR_DEFAULT );
            copied = false;
            trimming = NO;
            //          Fl::release();
            return 1;
        case FL_DRAG:

            if ( Fl::event_state() & FL_SHIFT &&
                 Fl::event_state() & FL_CTRL )
            {
                int d = (ox + X) - x();
                long td = timeline.x_to_ts( d );

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

            if ( ox + X >= _track->x() )
            {
                int nx = ox + X;

//                nx = _track->snap( this, nx );

//                _offset = timeline.x_to_ts( nx );

//                position( nx, y() );

                _offset = timeline.x_to_ts( nx ) + timeline.xoffset;

                _track->snap( this );
            }

            if ( Y > y() + h() )
            {
                if ( _track->next() )
                    _track->next()->add( this );
            }
            else
                if ( Y < y() )
                {
                    if ( _track->prev() )
                        _track->prev()->add( this );
                }

            _track->redraw();

            fl_cursor( FL_CURSOR_MOVE );

/*             if ( X >= timeline.scroll->x() + timeline.scroll->w() || */
/*                  X <= timeline.scroll->x() ) */
/*             { */
/*                 /\* this drag needs to scroll *\/ */

/*                 long pos = timeline.scroll->xposition(); */

/*                 if ( X <= timeline.scroll->x() ) */
/*                     pos -= 100; */
/*                 else */
/*                     pos += 100; */

/*                 if ( pos < 0 ) */
/*                     pos = 0; */

/*                 timeline.scroll->position( pos, timeline.scroll->yposition() ); */
/*             } */

//            _offset = timeline.x_to_ts( x() );

            return 1;
        default:
            return 0;
            break;
    }
}


/** must be called whenever zoom is adjusted */
void
Region::resize ( void )
{
    int X = timeline.ts_to_x( _offset );

    assert( _end >= _start );

    int W = timeline.ts_to_x( _end - _start );

    printf( "%dx%d\n", X, W );

//    if ( W )
//        Fl_Widget::resize( X, y(), W, h() );
}


/* X is the timeline offset, W is the width of the track */
void
Region::draw ( int X, int Y, int W, int H )
{
    if ( ! ( W > 0 && H > 0 ) )
        return;

    if ( _offset > timeline.xoffset + timeline.x_to_ts( _track->w() ) ||
         (  _offset < timeline.xoffset &&
            _offset + (_end - _start) < timeline.xoffset ) )
        return;

    int rw = timeline.ts_to_x( _end - _start );

    nframes_t end = _offset + ( _end - _start );

    /* calculate waveform offset due to scrolling */
    nframes_t offset = 0;
    if ( _offset < timeline.xoffset )
    {
        offset = timeline.xoffset - _offset;

        rw = timeline.ts_to_x( (_end - _start) - offset );
    }

    rw = min( rw, _track->w() );

    int rx = x();

    printf( "rx %d, rw %d\n", rx, rw );

    fl_push_clip( rx, Y, rw, H );

    /* dirty hack to keep the box from flipping to vertical at small sizes */
    fl_draw_box( FL_PLASTIC_UP_BOX, rx - 10, Y, rw + 50, H, _box_color );

//    fl_push_clip( x() + Fl::box_dx( box() ), y(), w() - Fl::box_dw( box() ), h() );

    draw_waveform( rx, Y, rw, H, _clip, _start + offset, min( (_end - _start) - offset, _end), _scale, _color );

    fl_color( FL_BLACK );
    fl_line( rx, Y, rx, Y + H );
    fl_line( rx + rw - 1, Y, rx + rw - 1, Y + H );

    fl_pop_clip();

    fl_font( FL_HELVETICA, 14 );
    fl_color( FL_BLACK );

    int bx = Fl::box_dx( FL_PLASTIC_UP_BOX );
    int by = Fl::box_dy( FL_PLASTIC_UP_BOX );
    int bw = Fl::box_dw( FL_PLASTIC_UP_BOX );
    int bh = Fl::box_dh( FL_PLASTIC_UP_BOX );

    int dx = min( 32767, timeline.ts_to_x( offset ) );

    Fl_Align align = (Fl_Align)(FL_ALIGN_LEFT | FL_ALIGN_BOTTOM | FL_ALIGN_CLIP);
    fl_draw( _clip->name(), (bx + rx + 1) - dx, Y + 1 + by, rw - bw, H - bh, align );
    fl_color( FL_WHITE );
    fl_draw( _clip->name(), (bx + rx) - dx, Y + by , rw - bw, H - bh, align );

//    fl_draw( _clip->name(), X, Y );

//(Fl_Align)FL_ALIGN_LEFT | FL_ALIGN_BOTTOM );


    fl_color( FL_RED );
    fl_line( x(), y(), x(), y() + h() );


}
