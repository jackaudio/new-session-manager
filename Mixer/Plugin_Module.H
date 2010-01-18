
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

class Plugin_Module : public Module {

public:

    struct Plugin_Info
    {
        const char *path;
        unsigned long id;

        Plugin_Info ( )
            {
                path = 0;
                id = 0;
            }
    };

    bool load ( unsigned long id );

private:

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

    bool _active;
    int _plugin_ins;
    int _plugin_outs;
    bool _crosswire;

    static Plugin_Info* discover ( void );


    void set_input_buffer ( int n, void *buf );
    void set_output_buffer ( int n, void *buf );
    void set_control_buffer ( int n, void *buf );
    void activate ( void );
    void deactivate ( void );
    void process ( unsigned long nframes );
    bool active ( void ) const { return _active; }

    bool plugin_instances ( unsigned int );

    void connect_ports ( void );

public:

    Plugin_Module ( );
    virtual ~Plugin_Module();

    static Plugin_Module * pick_plugin ( void );
    static void add_plugins_to_menu ( Fl_Menu_Button *menu );

    int plugin_ins ( void ) const { return _plugin_ins; }
    int plugin_outs ( void ) const { return _plugin_outs; }

    void select_plugin ( unsigned long id );

    const char *name ( void ) const { return "Plugin"; }

    int can_support_inputs ( int );
    bool configure_inputs ( int );

    void process ( void );

    void handle_port_connection_change ( void );

    LOG_CREATE_FUNC( Plugin_Module );

protected:

    virtual int handle ( int );
    void get ( Log_Entry &e ) const;
    void set ( Log_Entry &e );

};
