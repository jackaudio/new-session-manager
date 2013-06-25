
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

#pragma once

/* Just like an Fl_Input, except that when not being edited it
 * displays just like a label. */

#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Input.H>

class Fl_Sometimes_Input : public Fl_Input
{
    Fl_Boxtype _up_box;

public:

    Fl_Sometimes_Input ( int X, int Y, int W, int H, const char *L=0 )
        : Fl_Input( X, Y, W, H, L )
        {
            clear_visible_focus();
            up_box( FL_NO_BOX );
            when(FL_WHEN_ENTER_KEY);
        }

    void up_box ( Fl_Boxtype b ) { _up_box = b;  }
    Fl_Boxtype up_box ( void ) const { return _up_box; }

    virtual void
    draw ( void )
        {
            if ( this == Fl::focus() )
                Fl_Input::draw();
            else
            {
                fl_draw_box( up_box(), x(), y(), w(), h(), color() );

                Fl_Color c = fl_contrast( textcolor(), color() );

                fl_color( active_r() ? c : fl_inactive( c ) );

                fl_font( textfont(), textsize() );
                fl_draw( value(), x(), y(), w(), h(), (Fl_Align)(FL_ALIGN_CENTER | FL_ALIGN_CLIP) );
            }
        }

    virtual void
    take_focus ( void )
        {
            set_visible_focus();
            Fl_Widget::take_focus();
            clear_visible_focus();
        }

    virtual int
    handle ( int m )
        {
            int r = 0;

            switch ( m )
            {
                case FL_KEYDOWN:
                {
                    if (  ( Fl::event_key() == FL_Enter ||
                            Fl::event_key() == FL_Tab ) ) 
                    {
                        Fl::focus( NULL );
                        r = 1;
                    }
                    break;
                }
                case FL_FOCUS:
                    redraw();
                    r = 1;
                    break;
                case FL_UNFOCUS:
                    do_callback();
                    r = 1;
                    break;
                case FL_PUSH:
                    take_focus();
                    redraw();
                    r = 1;
                    break;
                case FL_DND_ENTER:
                    return 0;
                case FL_PASTE:
                    return 0;
                default:
                    break;
            }
            
            return Fl_Input::handle( m ) | r;         
        }
};
