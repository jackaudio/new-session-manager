
/*******************************************************************************/
/* Copyright (C) 2010 Jonathan Moore Liles                                     */
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

/* Scrolling group suitable for containing a single child (a
 * pack). When the Fl_Packscroller is resized, the child will be resized
 * too. No scrollbars are displayed, but the widget responds to
 * FL_MOUSEWHEEL events. */

#pragma once

#include <FL/Fl_Group.H>
#include <FL/fl_draw.H>
#include <FL/Fl.H>

/* FIXME: Optimize scroll */

class Fl_Packscroller : public Fl_Group
{

    int _increment;
    int _yposition;
    static const int sbh = 15;                                  /* scroll button height */

public:

    Fl_Packscroller ( int X, int Y, int W, int H, const char *L = 0 ) : Fl_Group( X, Y, W, H, L )
        {
            clip_children( 1 );

            _increment = 30;
            _yposition = 0;

//            color( FL_WHITE );
        }

    int increment ( void ) const { return _increment; }
    void increment ( int v ) { _increment = v; }

    void yposition ( int v )
        {
            if ( ! children() )
                return;

            int Y = v;

            if ( Y > 0 )
                Y = 0;

            const int H = h() - (sbh * 2);

            Fl_Widget *o = child( 0 );

            if ( o->h() > H &&
                 Y + o->h() < H )
                Y = H - o->h();
            else if ( o->h() < H )
                Y = 0;

            if ( _yposition != Y )
            {
                _yposition = Y;

                damage( FL_DAMAGE_SCROLL );
            }
        }

    int yposition ( void ) const
        {
            if ( children() )
                return child( 0 )->y() - (y() + sbh);

            return 0;
        }

    void bbox ( int &X, int &Y, int &W, int &H )
        {
            X = x();
            Y = y() + sbh;
            W = w();
            H = h() - (sbh * 2);
        }

    virtual void
    draw ( void )
        {
            if ( ! children() )
                return;

            if ( ! fl_not_clipped( x(), y(), w(), h() ) )
                return;

//            draw_box();

            Fl_Widget *o = child( 0 );

            o->position( x(), y() + sbh + _yposition );

            if ( damage() != FL_DAMAGE_CHILD )
            {
                fl_rectf( x(), y(), w(), h(), color() );

                fl_font( FL_HELVETICA, 12 );

                if ( o->y() != y() + sbh )
                {
                    fl_draw_box( box(), x(), y(), w(), sbh, color() );
                    fl_color( FL_BLACK );
                    fl_draw( "@2<", x(), y(), w(), sbh, FL_ALIGN_CENTER );
                }

                if ( o->h() > h() - (sbh * 2) && o->y() + o->h() != y() + h() - sbh )
                {
                    fl_draw_box( box(), x(), y() + h() - sbh, w(), sbh, color() );
                    fl_color( FL_BLACK );
                    fl_draw( "@2>", x(), y() + h() - sbh, w(), sbh, FL_ALIGN_CENTER );
                }

            }

            fl_push_clip( x(), y() + sbh, w(), h() - (sbh * 2 ) );

            draw_children();

            fl_pop_clip();
        }

    virtual int
    handle ( int m )
        {
            if ( Fl_Group::handle( m ) )
                return 1;

            switch ( m )
            {
                case FL_PUSH:
                {
                    if ( Fl::event_button1() )
                    {
                        if ( Fl::event_inside( x(), y(), w(), sbh ) )
                        {
                            yposition( yposition() + ( h() / 4 ) );
                            return 1;
                        }
                        else if ( Fl::event_inside(  x(), y() + h() - sbh, w(), sbh ) )
                        {
                            yposition( yposition() - (h() / 4 ) );
                            return 1;
                        }

                        return 0;
                    }
                }
                case FL_ENTER:
                case FL_LEAVE:
                    return 1;
                case FL_FOCUS:
                case FL_UNFOCUS:
                    return 1;
                case FL_KEYBOARD:
                {
                    if ( Fl::event_key() == FL_Up )
                    {
                        yposition( yposition() + ( h() / 4 ) );
                        return 1;
                    }
                    else if ( Fl::event_key() == FL_Down )
                    {
                        yposition( yposition() - (h() / 4 ) );
                        return 1;
                    }
                    return 0;
                }
                case FL_MOUSEWHEEL:
                {
                    yposition( yposition() - ( Fl::event_dy() * _increment ) );

                    return 1;
                }
            }

            return 0;
        }
};
