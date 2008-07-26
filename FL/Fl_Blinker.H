
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

#include <FL/Fl_Button.H>
#include <FL/Fl.H>

const float BLINK_FREQ = 0.5f;

class Fl_Blinker : public Fl_Button
{

    bool _on;

    static void
    update_cb ( void *v )
        {
            ((Fl_Blinker*)v)->update_cb();
        }

    void
    update_cb ( void )
        {
            Fl::repeat_timeout( BLINK_FREQ, update_cb, this );

            _on = ! _on;

            redraw();
        }

public:

    Fl_Blinker ( int X, int Y, int W, int H, const char *L )
        : Fl_Button( X, Y, W, H, L )
        {
            _on = false;
            Fl::add_timeout( BLINK_FREQ, update_cb, this );

            type( FL_TOGGLE_BUTTON );
        }

    virtual
    ~Fl_Blinker ()
        {
            Fl::remove_timeout( update_cb, this );
        }

    virtual void
    draw ( void )
        {
            draw_box( value() ? box() : down_box(), x(), y(), w(), h(), ( value() != 0 && _on ) ? selection_color() : color() );
            draw_label();
        }
};
