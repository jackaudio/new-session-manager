
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

/* Controls the audio transport */

#include "Transport.H"
#include "Timeline.H"

#include "Engine/Engine.H"



Transport::Transport ( int X, int Y, int W, int H, const char *L )
    : Fl_Pack( X, Y, W, H, L )
{
    recording = false;
    rolling = false;
    _stop_disables_record = true;

    bar = 0;
    beat = 0;
    tick = 0;
    beats_per_minute = 120;
    ticks_per_beat = 1920;
    beat_type = 4;
    beats_per_bar = 4;
    next_time = 0;
    frame_time =0;
    frame_rate = 48000;
    frame = 0;

    const int bw = W / 5;

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

    _punch_button = o = new Fl_Button( 0, 0, bw, 0, "Punch" );
    o->type( FL_TOGGLE_BUTTON );
    o->labelsize( 9 );
    o->labeltype( FL_NORMAL_LABEL );
    o->shortcut( 'P' );
    o->callback( cb_button, this );
    o->when( FL_WHEN_CHANGED );
    o->color2( FL_RED );
    o->box( FL_UP_BOX );

    end();
}



void
Transport::cb_button ( Fl_Widget *w, void *v )
{
    ((Transport*)v)->cb_button( w );
}

void
Transport::update_record_state ( void )
{
    Fl_Button *w = _record_button;

    /* handle display */
    if ( w->value() )
        w->labelcolor( FL_RED );
    else
        w->labelcolor( fl_color_average( FL_RED, FL_WHITE, 0.25f ) );

    w->redraw();

    /* this covers the case where the record toggle button is
     * pressed while the transport is already rolling. Recording
     * should begin or end on the next frame */
    if ( rolling )
    {
        if ( ! recording && w->value() )
        {
            timeline->record();
            recording = true;
        }
        else if ( recording )
        {
            timeline->stop();
            recording = false;
        }
    }
}

/** cb_button
 * common handler for all transport buttons */
void
Transport::cb_button ( Fl_Widget *w )
{
    if ( w == _home_button )
        locate( timeline->playback_home() );
    else if ( w == _end_button )
        locate( timeline->playback_end() );
    else if ( w == _play_button )
        toggle();
    else if ( w == _record_button )
        update_record_state();
    else if ( w == _punch_button )
        timeline->redraw();
}

void
Transport::toggle_record ( void )
{
    _record_button->value( ! _record_button->value() );
     update_record_state();
}

bool
Transport::rec_enabled ( void ) const
{
    return _record_button->value();
}

bool
Transport::punch_enabled ( void ) const
{
    return _punch_button->value();
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

/***********/
/* Control */
/***********/

void
Transport::poll ( void )
{

    jack_transport_state_t ts;

    ts = engine->transport_query( this );

    rolling = ts == JackTransportRolling;
}

void
Transport::locate ( nframes_t frame )
{
    if ( ! engine )
        return;

    if ( ! recording )
    {
        // don't allow seeking while record is in progress
        engine->transport_locate( frame );

        /* so there isn't a race waiting for the transport to sync */
        this->frame = frame;
    }

    timeline->_created_new_takes = false;
}


void
Transport::start ( void )
{
//    MESSAGE( "Starting transport" );
    if ( _record_button->value() )
    {
        rolling = true;
        update_record_state();
    }

    if ( engine )
        engine->transport_start();
}

void
Transport::stop ( void )
{
//    MESSAGE( "Stopping transport" );
    if ( _record_button->value() )
    {
        if ( _stop_disables_record )
            _record_button->value( 0 );

        timeline->_created_new_takes = false;

        update_record_state();
    }

    if ( engine )
        engine->transport_stop();
}

void
Transport::toggle ( void )
{
    if ( rolling )
        stop();
    else
        start();
}
