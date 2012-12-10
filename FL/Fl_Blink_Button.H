
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

#include <FL/Fl_Button.H>
#include <FL/Fl.H>

/* Kind of like Fl_Light_Button except that the whole thing is the
 * indicator and it can optionally blink */

class Fl_Blink_Button : public Fl_Button
{
    bool _on;
    int _blink_interval;
    bool _blinking;
    bool _ignore_input;

    static void
    update_cb ( void *v )
        {
            ((Fl_Blink_Button*)v)->update_cb();
        }


    float 
    blink_interval_as_fraction_of_seceond ( void ) const
        {
            return (float)_blink_interval / 1000;
        }

    void
    update_cb ( void )
        {
            Fl::repeat_timeout( blink_interval_as_fraction_of_seceond(), update_cb, this );

            _on = ! _on;

            redraw();
        }
    
public:

    enum 
    {
        SLOW=500,
        MEDIUM=300,
        FAST=100,
        DEFAULT=500
    };

    Fl_Blink_Button ( int X, int Y, int W, int H, const char *L=0 )
        : Fl_Button( X, Y, W, H, L )
        {
            _blinking = true;
            _on = false;
            _ignore_input = false;
            _blink_interval = DEFAULT;

            type( FL_TOGGLE_BUTTON );
        }

    virtual
    ~Fl_Blink_Button ()
        {
            if ( value() )
                Fl::remove_timeout( update_cb, this );
        }
    
    void ignore_input ( bool b )
        {
            _ignore_input = b;
        }

    bool ignore_input ( void ) const
        {
            return _ignore_input;
        }

    void blink ( bool b )
        {
            _blinking = b;
            if ( ! b )
                _on = true;
        }

    bool blink ( void ) const
        {
            return _blinking;
        }

    void
    blink_interval ( float v )
        {
            _blink_interval = v * 1000;
            if ( value() )
            {
                Fl::remove_timeout( update_cb, this );
                Fl::add_timeout( blink_interval_as_fraction_of_seceond(), update_cb, this );
            }
        }

    virtual void value ( float v )
        {
            if ( v != value() )
            {
                if ( v )
                {
                    if ( _blinking )
                        Fl::add_timeout( blink_interval_as_fraction_of_seceond(), update_cb, this );
                    Fl_Button::value( v );
                    redraw();
                }
                else
                {
                    Fl_Button::value( v );
                    Fl::remove_timeout( update_cb, this );
                    redraw();
                }
            }
        }

    virtual float value ( void ) { return Fl_Button::value(); }

    virtual void
    draw ( void )
        {
            draw_box( value() ? box() : down_box(), x(), y(), w(), h(),
                      ( value() != 0 && _on ) ? selection_color() : color() );
            draw_label();
        }

    virtual int handle ( int m )
        {
            if ( _ignore_input )
                return 0;
            else
                return Fl_Button::handle( m );
        }
};
