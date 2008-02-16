
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

#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Widget.H>

#include <stdio.h>

Region::Region ( int X, int Y, int W, int H, const char *L=0 ) : Waveform( X, Y, W, H, L )
{
    align( FL_ALIGN_INSIDE | FL_ALIGN_LEFT | FL_ALIGN_BOTTOM | FL_ALIGN_CLIP );
    labeltype( FL_SHADOW_LABEL );
    labelcolor( FL_WHITE );
    box( FL_PLASTIC_UP_BOX );

    _track = NULL;
}

void
Region::trim ( enum trim_e t, int X )
{
    switch ( t )
    {
        case LEFT:
        {
            int d = X - x();
            _start += d;
            resize( x() + d, y(), w() - d, h() );
            break;
        }
        case RIGHT:
        {
            int d = (x() + w()) - X;
            _end -= d;
            resize( x(), y(), w() - d, h() );
            break;
        }
        default:
            return;

    }

    redraw();
    parent()->redraw();

}

int
Region::handle ( int m )
{

    if ( Fl_Widget::handle( m ) )
        return 1;

    static int ox, oy;
    static enum trim_e trimming;

    switch ( m )
    {
        case FL_PUSH:
        {
            int X = Fl::event_x();
            int Y = Fl::event_y();

            if ( Fl::event_state() & FL_CTRL )
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
                ox = x() - Fl::event_x();
                oy = y() - Fl::event_y();

                if ( Fl::event_button() == 2 )
                {
//                    _track->add( new Region( *this ) );
                }

                return 1;
            }
            return 0;
            break;
        }
        case FL_RELEASE:
            fl_cursor( FL_CURSOR_DEFAULT );
            return 1;
        case FL_DRAG:

            if ( Fl::event_state() & FL_CTRL )
                if ( trimming )
                {
                    trim( trimming, Fl::event_x() );
                    return 1;
                }
                else
                    return 0;

            if ( ox + Fl::event_x() >= _track->x() )
                position( ox + Fl::event_x(), y() );

            if ( Fl::event_y() > y() + h() )
            {
                if ( _track->next() )
                    _track->next()->add( this );
            }
            else
                if ( Fl::event_y() < y() )
                {
                    if ( _track->prev() )
                        _track->prev()->add( this );
                }
//            if ( Fl::event_y() - oy >= h() )

            parent()->redraw();

            fl_cursor( FL_CURSOR_MOVE );
            return 1;
        default:
            return 0;
            break;
    }
}



void
Region::draw ( void )
{
    draw_box();

//    fl_push_clip( x() + Fl::box_dx( box() ), y(), w() - Fl::box_dw( box() ), h() );

    Waveform::draw();

//    fl_pop_clip();

    draw_label();
}
