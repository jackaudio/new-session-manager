
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


#include "Transport.H"

#include "Engine/Engine.H"

// Transport transport;

#define client engine->client()

void
Transport::poll ( void )
{
    jack_transport_state_t ts;

    ts = jack_transport_query( client, this );

    rolling = ts == JackTransportRolling;
}

void
Transport::locate ( nframes_t frame )
{
    jack_transport_locate( client, frame );
}


void
Transport::start ( void )
{
//    MESSAGE( "Starting transport" );
    if ( _record_button->value() )
        timeline->record();

    jack_transport_start( client );
}

void
Transport::stop ( void )
{
//    MESSAGE( "Stopping transport" );
    if ( _record_button->value() )
        toggle_record();

    jack_transport_stop( client );
}

void
Transport::toggle ( void )
{
    if ( rolling )
        stop();
    else
        start();
}


/*******/
/* GUI */
/*******/

Transport::Transport ( int X, int Y, int W, int H, const char *L )
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

void
Transport::cb_button ( Fl_Widget *w, void *v )
{
    ((Transport*)v)->cb_button( w );
}

void
Transport::cb_button ( Fl_Widget *w )
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
        {
            if ( _record_button->value() )
                timeline->record();
            else
                timeline->stop();
        }
    }
}

void
Transport::toggle_record ( void )
{
    if ( _record_button->value() )
        _record_button->value( 0 );
    else
        _record_button->value( 1 );

    _record_button->do_callback();
}

int
Transport::handle ( int m )
{
    /* FIXME: hack to avoid stealing focus */

    if ( m == FL_FOCUS  )
        return 0;
    else
        return Fl_Pack::handle( m );

}
