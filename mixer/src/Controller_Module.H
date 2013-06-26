
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

#include "Module.H"
#include <vector>
#include "JACK/Port.H"
#include "OSC/Endpoint.H"

class Fl_Menu_Button;
class Fl_Menu_;

class Fl_Valuator;

class Controller_Module : public Module
{

    bool _pad;

    volatile float control_value;

    Fl_Menu_Button & menu ( void );
    static void menu_cb ( Fl_Widget *w, void *v );
    void menu_cb ( const Fl_Menu_ *m );


    static void peer_callback(  OSC::Signal *sig,  OSC::Signal::State state, void *v );
    void peer_callback( OSC::Signal *sig, OSC::Signal::State state );
    void add_osc_peers_to_menu ( Fl_Menu_Button *m, const char *prefix );
    void add_osc_connections_to_menu ( Fl_Menu_Button *m, const char *prefix );

public:

    static bool _learn_mode;

    static bool learn_by_number;
    static bool learn_mode ( void ) { return _learn_mode; }
    static void learn_mode ( bool b ) { _learn_mode = b; }

    enum Mode { GUI, CV, OSC, MIDI };

    enum Type { KNOB,
                SLIDER,
                SPINNER,
                TOGGLE,
                SPATIALIZATION };

    Mode mode ( void ) const { return _mode; }
    void mode ( Mode v );

    Type type ( void ) const { return _type; }

    Controller_Module ( bool is_default = false );
    virtual ~Controller_Module ( );

    const char *name ( void ) const { return "Controller"; }

    int can_support_inputs ( int ) { return 0; }
    bool configure_inputs ( int ) { return false; }

    void pad ( bool v ) { _pad = v; }

    static void cb_handle ( Fl_Widget *w, void *v );
    void cb_handle ( Fl_Widget *w );
    static void cb_spatializer_handle ( Fl_Widget *w, void *v );
    void cb_spatializer_handle ( Fl_Widget *w );

    void connect_to ( Port *p );
    bool connect_spatializer_to ( Module *m );
    void disconnect ( void );

    void handle_control_changed ( Port *p );
    void handle_chain_name_changed ( void );
    void handle_control_disconnect ( Port *p );

    virtual void command_remove ( void );

    LOG_CREATE_FUNC( Controller_Module );

    virtual void update ( void );

    void process ( nframes_t nframes );

    void draw ( void );

    int handle ( int m );

//    void set_control_value ( float f ) { control_value = f; }

protected:

    void get ( Log_Entry &e ) const;
    void set ( Log_Entry &e );

private:

    char *generate_osc_path ( void );
    void change_osc_path ( char *path );
    
    std::vector<JACK::Port> jack_input;
    Mode _mode;
    Type _type;

    Fl_Valuator *control;

};
