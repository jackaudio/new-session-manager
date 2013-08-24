
/*******************************************************************************/
/* Copyright (C) 2009 Jonathan Moore Liles                                     */
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

class Fl_Box;
class Fl_Browser;
#include "Module.H"
#include "JACK/Port.H"
#include <vector>

class JACK_Module : public Module
{
    char *_prefix;

protected:
    
    void prefix ( const char *s )
        {
            if ( _prefix )
                free( _prefix );
            
            _prefix = NULL;
            
            if ( s )
                _prefix = strdup( s );
        }

    Fl_Button * dec_button;
    Fl_Button * inc_button;
    Fl_Browser * connection_display;
    Fl_Box * input_connection_handle;
    Fl_Box * output_connection_handle;
    Fl_Box * output_connection2_handle;
    
    static void cb_button ( Fl_Widget *w, void *v );
    void cb_button ( Fl_Widget *w );

protected:

    unsigned int _connection_handle_outputs[2][2];
 
public:

    void update_connection_status ( void );

    JACK_Module ( bool log = true );
    virtual ~JACK_Module ( );

    virtual const char *basename ( void ) const { return "JACK"; }
    virtual const char *name ( void ) const { return "JACK"; }
    virtual bool initialize ( void );

    virtual void draw ( void );
    virtual int handle ( int m );

    virtual int can_support_inputs ( int );
    void remove_aux_audio_outputs ( void );
    virtual bool configure_inputs ( int n );
    virtual bool configure_outputs ( int n );

    virtual void handle_control_changed ( Port *p );

    LOG_CREATE_FUNC( JACK_Module );


protected:

    virtual void process ( nframes_t nframes );

};
