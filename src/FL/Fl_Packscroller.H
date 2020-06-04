
/*******************************************************************************/
/* Copyright (C) 2008-2020 Jonathan Moore Liles (as "Non-Session-Manager")     */
/*                                                                             */
/* This file is part of New-Session-Manager                                    */
/*                                                                             */
/* New-Session-Manager is free software: you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by        */
/* the Free Software Foundation, either version 3 of the License, or           */
/* (at your option) any later version.                                         */
/*                                                                             */
/* New-Session-Manager is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of              */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               */
/* GNU General Public License for more details.                                */
/*                                                                             */
/* You should have received a copy of the GNU General Public License           */
/* along with New-Session-Manager. If not, see <https://www.gnu.org/licenses/>.*/
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

            const int H = h();
// - (sbh * 2);

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
            Y = y() + ( sbh * top_sb_visible() );
            W = w();
            H = h() - ( sbh * ( top_sb_visible() + bottom_sb_visible() ) );
        }

    int top_sb_visible ( void )
        {
            return children() && child(0)->y() != y() ? 1 : 0;
        }

    int bottom_sb_visible ( void )
        {
            if ( children() )
            {
                Fl_Widget *o = child( 0 );
           
                if ( o->h() > h() && o->y() + o->h() != y() + h() )
                    return 1;
            }

            return 0;
        }

    virtual void
    draw ( void )
        {
            if ( damage() & FL_DAMAGE_ALL )
            {
                fl_rectf( x(), y(), w(), h(), color() );
            }

            if ( ! children() )
                return;

            Fl_Widget *o = child( 0 );

            o->position( x(), y() + _yposition );

            const int top_sb = top_sb_visible();
            const int bottom_sb = bottom_sb_visible();
            
            fl_push_clip( x(), y() + ( sbh * top_sb ), w(), h() - (sbh * (top_sb + bottom_sb) ));
            
            draw_children();

            fl_pop_clip();
            
            fl_font( FL_HELVETICA, 12 );
            
            if ( top_sb )
            {
                fl_draw_box( box(), x(), y(), w(), sbh, color() );
                fl_color( fl_contrast( FL_FOREGROUND_COLOR, color() ) );
                fl_draw( "@2<", x(), y(), w(), sbh, FL_ALIGN_CENTER );
            }
            
            if ( bottom_sb )
            {
                fl_draw_box( box(), x(), y() + h() - sbh, w(), sbh, color() );
                fl_color( fl_contrast( FL_FOREGROUND_COLOR, color() ) );
                fl_draw( "@2>", x(), y() + h() - sbh, w(), sbh, FL_ALIGN_CENTER );
            }
        }

    virtual int
    handle ( int m )
        {
            switch ( m )
            {
                case FL_PUSH:
                    if ( top_sb_visible() &&
                         Fl::event_inside( x(), y(), w(), sbh ) )
                    {
                        return 1;
                    }
                    else if ( bottom_sb_visible() &&
                              Fl::event_inside(  x(), y() + h() - sbh, w(), sbh ) )
                    {
                        return 1;
                    }
                    break;
                case FL_RELEASE:
                {
                    if ( top_sb_visible() &&
                         Fl::event_inside( x(), y(), w(), sbh ) )
                    {
                        yposition( yposition() + ( h() / 4 ) );
                        return 1;
                    }
                    else if ( bottom_sb_visible() &&
                              Fl::event_inside(  x(), y() + h() - sbh, w(), sbh ) )
                    {
                        yposition( yposition() - (h() / 4 ) );
                        return 1;
                    }
                    break;
                }
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
                    break;
                }
                case FL_MOUSEWHEEL:
                {
                    yposition( yposition() - ( Fl::event_dy() * _increment ) );

                    return 1;
                }
            }

            return Fl_Group::handle( m );
        }
};
