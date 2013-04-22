
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

    Fl_Flip_Button *tab_button;
    Fl_Flowpack *controls_pack;
    Fl_Group *chain_tab;
    Fl_Group *control_tab;
    Fl_Pack *modules_pack;

    Mixer_Strip *_strip;
    const char *_name;

    std::list<Module*> process_queue;

    std::vector <Module::Port> scratch_port;

    Engine *_engine;

    Fl_Callback *_configure_outputs_callback;
    void *_configure_outputs_userdata;

private:

    static void snapshot ( void *v );
    void snapshot ( void );

    void cb_handle(Fl_Widget*);
    static void cb_handle(Fl_Widget*, void*);

    void draw_connections ( Module *m );
    void build_process_queue ( void );
    void add_to_process_queue ( Module *m );

    static void process ( nframes_t, void * );
    void process ( nframes_t );

    static void buffer_size ( nframes_t nframes, void *v );
    void buffer_size ( nframes_t nframes );

    
    static void port_connect ( jack_port_id_t a, jack_port_id_t b, int connect, void *v );
    void port_connect ( jack_port_id_t a, jack_port_id_t b, int connect );

    static void update_connection_status ( void *v );
    void update_connection_status ( void );

protected:

    void get ( Log_Entry &e ) const;
    void set ( Log_Entry &e );

public:

    Chain ( int X, int Y, int W, int H, const char *L = 0 );
    Chain ( );
    virtual ~Chain ( );

    void draw ( void );
    void resize ( int X, int Y, int W, int H );

    Mixer_Strip *strip ( void ) const { return _strip; }
    void strip ( Mixer_Strip *v );
    const char *name ( void ) const { return _name; }
    void name ( const char *name );

    int get_module_instance_number ( Module *m );

    void configure_ports ( void );
    int required_buffers ( void );

    bool can_support_input_channels ( int n );

    int modules ( void ) const { return modules_pack->children(); }
    Module *module ( int n ) const { return (Module*)modules_pack->child( n ); }
    void remove ( Controller_Module *m );
    void remove ( Module *m );
    bool add ( Module *m );
    bool add ( Controller_Module *m );
    bool insert ( Module *m, Module *n );
    void add_control ( Controller_Module *m );

    bool do_export ( const char *filename );

    void initialize_with_default ( void );

    bool can_configure_outputs ( Module *m, int n ) const;

    void configure_outputs_callback ( Fl_Callback *cb, void *v )
        {
            _configure_outputs_callback = cb;
            _configure_outputs_userdata = v;
        }

    Fl_Callback * configure_outputs_callback ( void ) const { return _configure_outputs_callback; }

    virtual void log_children ( void ) const;

    static unsigned int maximum_name_length ( void );

    Engine *engine ( void ) const { return _engine; }

    LOG_CREATE_FUNC( Chain );
};
