
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
#include "Loggable.H"

class Fl_Menu_Button;
class Thread;

class Plugin_Module : public Module {

    static Thread *plugin_discover_thread;

public:

    struct Plugin_Info
    {
        const char *path;
        unsigned long id;
        const char *name;
        const char *author;
        const char *category;
        int audio_inputs;
        int audio_outputs;
        const char *type;
        bool favorite;

        Plugin_Info ( )
            {
                path = 0;
                id = 0;
                name = 0;
                author = 0;
                category = 0;
                audio_inputs = 0;
                audio_outputs = 0;
                type = "LADSPA";
                favorite = 0;
            }

        bool operator< ( const Plugin_Info &rhs ) {
            return strcmp( name, rhs.name ) < 1;
        }
    };

    bool load ( unsigned long id );

private:

    volatile nframes_t _latency;
    nframes_t _last_latency;
    
    void init ( void );

    void bbox ( int &X, int &Y, int &W, int &H )
        {
            X = x();
            Y = y() + 5;
            W = w();
            H = h() - 10;
        }

    void cb_handle(Fl_Widget*);
    static void cb_handle(Fl_Widget*, void*);

    Fl_Button *close_button;

    struct ImplementationData;

    ImplementationData *_idata;

    int _plugin_ins;
    int _plugin_outs;
    bool _crosswire;

    static void *discover_thread ( void * );
 

    void set_input_buffer ( int n, void *buf );
    void set_output_buffer ( int n, void *buf );
    void set_control_buffer ( int n, void *buf );
    void activate ( void );
    void deactivate ( void );
    void process ( unsigned long nframes );

    bool plugin_instances ( unsigned int );

    void connect_ports ( void );

    bool loaded ( void ) const;

public:

    virtual nframes_t get_module_latency ( void ) const;

    virtual void update ( void );

    static std::list<Plugin_Info> get_all_plugins ( void );

    static void spawn_discover_thread ( void );
    static void join_discover_thread ( void );

    Plugin_Module ( );
    virtual ~Plugin_Module();

    int plugin_ins ( void ) const { return _plugin_ins; }
    int plugin_outs ( void ) const { return _plugin_outs; }

    void select_plugin ( unsigned long id );

    const char *name ( void ) const { return "Plugin"; }

    int can_support_inputs ( int );
    bool configure_inputs ( int );

    virtual bool bypass ( void ) const { return _bypass; }
    virtual void bypass ( bool v );

    virtual void process ( nframes_t );

    void handle_port_connection_change ( void );

    LOG_CREATE_FUNC( Plugin_Module );

    MODULE_CLONE_FUNC( Plugin_Module );

protected:

    void get ( Log_Entry &e ) const;
    void set ( Log_Entry &e );

};
