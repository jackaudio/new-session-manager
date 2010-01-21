
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

#include <FL/Fl.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Button.H>

#include "Module.H"
#include "JACK/Port.H"
#include <vector>
#include <list>
#include "Loggable.H"

class Mixer_Strip;
class Fl_Flowpack;
class Fl_Flip_Button;
class Engine;
class Controller_Module;

class Chain : public Fl_Group, public Loggable {

    Fl_Pack *modules_pack;
    Fl_Flowpack *controls_pack;


    Fl_Flip_Button *tab_button;
    Fl_Group *chain_tab;
    Fl_Group *control_tab;

    void cb_handle(Fl_Widget*);
    static void cb_handle(Fl_Widget*, void*);

/*     int _ins; */
/*     int _outs; */

    Mixer_Strip *_strip;

//    sample_t **_buffer;
//    int _nbuffers;

    Fl_Callback *_configure_outputs_callback;
    void *_configure_outputs_userdata;

    const char *_name;

    void draw_connections ( Module *m );

    std::list<Module*> process_queue;

    void build_process_queue ( void );
    void add_to_process_queue ( Module *m );

    std::vector <Module::Port> scratch_port;

    Engine *_engine;

    static void process ( nframes_t, void * );
    void process ( nframes_t );

protected:

    void get ( Log_Entry &e ) const;
    void set ( Log_Entry &e );

    int handle ( int m );
    void draw ( void );

public:

    void resize ( int X, int Y, int W, int H );

    Mixer_Strip *strip ( void ) const { return _strip; }
    void strip ( Mixer_Strip *v );
    const char *name ( void ) const { return _name; }
    void name ( const char *name );

    void configure_ports ( void );
    int required_buffers ( void );

    Chain ( int X, int Y, int W, int H, const char *L = 0 );
    Chain ( );
    virtual ~Chain ( );

    bool can_support_input_channels ( int n );

/*     void ins ( int i ) { _ins = i; } */
/*     void outs ( int i ) { _outs = i; } */
/*     int ins ( void ) const { return _ins; } */
/*     int outs ( void ) const { return _outs; } */

    int modules ( void ) const { return modules_pack->children(); }
    Module *module ( int n ) const { return (Module*)modules_pack->child( n ); }
    void remove ( Module *m );
    bool add ( Module *m );
    bool add ( Controller_Module *m );
    bool insert ( Module *m, Module *n );
    void add_control ( Controller_Module *m );

    void initialize_with_default ( void );

    bool can_configure_outputs ( Module *m, int n ) const;

    void configure_outputs_callback ( Fl_Callback *cb, void *v )
        {
            _configure_outputs_callback = cb;
            _configure_outputs_userdata = v;
        }

    Fl_Callback * configure_outputs_callback ( void ) const { return _configure_outputs_callback; }

    void log_children ( void );

    Engine *engine ( void ) const { return _engine; }

    LOG_CREATE_FUNC( Chain );
};
