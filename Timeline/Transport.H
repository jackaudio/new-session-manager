
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

#include <jack/transport.h>
#include "types.h"

#include "Timeline.H"

#include <FL/Fl_Pack.H>
#include <FL/Fl_Button.H>

#include <stdio.h>

struct Transport : public jack_position_t, public Fl_Pack
{

private:

    /* not permitted */
    Transport ( const Transport &rhs );
    Transport & operator = ( const Transport &rhs );

    Fl_Button *_home_button;
    Fl_Button *_end_button;
    Fl_Button *_play_button;
    Fl_Button *_record_button;

    static
    void
    cb_button ( Fl_Widget *w, void *v )
        {
            ((Transport*)v)->cb_button( w );
        }

    void
    cb_button ( Fl_Widget *w )
        {
            if ( w == _home_button )
                locate( 0 );
            if ( w == _end_button )
                locate( timeline->length() );
            else if ( w == _play_button )
                toggle();
            else if ( w == _record_button )
            {
                if ( _record_button->value() )
                    w->labelcolor( FL_RED );
                else
                    w->labelcolor( fl_color_average( FL_RED, FL_WHITE, 0.25f ) );

                redraw();

                if ( rolling )
                    if ( _record_button->value() )
                        timeline->record();
                    else
                        timeline->stop();
            }
        }

public:

    void toggle_record ( void )
        {
            if ( _record_button->value() )
                _record_button->value( 0 );
            else
                _record_button->value( 1 );

            _record_button->do_callback();
        }


    Transport ( int X, int Y, int W, int H, const char *L=0 )
        : Fl_Pack( X, Y, W, H, L )
        {
            recording = false;
            rolling = false;

            const int bw = W / 3;

            type( HORIZONTAL );

            Fl_Button *o;

            _home_button = o = new Fl_Button( 0, 0, bw, 0, "@|<" );
            o->labeltype( FL_EMBOSSED_LABEL );
            o->callback( cb_button, this );
            o->shortcut( FL_Home );
            o->box( FL_UP_BOX );

            _end_button = o = new Fl_Button( 0, 0, bw, 0, "@>|" );
            o->labeltype( FL_EMBOSSED_LABEL );
            o->callback( cb_button, this );
            o->shortcut( FL_End );

            _play_button = o = new Fl_Button( 0, 0, bw, 0, "@>" );
            o->labeltype( FL_EMBOSSED_LABEL );
            o->callback( cb_button, this );
            o->shortcut( ' ' );
            o->box( FL_UP_BOX );

            _record_button = o = new Fl_Button( 0, 0, bw, 0, "@circle" );
            o->type( FL_TOGGLE_BUTTON );
            o->labeltype( FL_EMBOSSED_LABEL );
            o->labelcolor( fl_color_average( FL_RED, FL_WHITE, 0.25f ) );
            o->shortcut( 'R' );
            o->callback( cb_button, this );
            o->when( FL_WHEN_CHANGED );
            o->box( FL_UP_BOX );

            end();
        }

    int
    handle ( int m )
        {
            /* FIXME: hack to avoid stealing focus */

            if ( m == FL_FOCUS  )
                return 0;
            else
                return Fl_Pack::handle( m );

        }

    bool rolling;
    bool recording;

    void poll ( void );
    void locate ( nframes_t frame );
    void start ( void );
    void stop ( void );
    void toggle ( void );
};

extern Transport* transport;
