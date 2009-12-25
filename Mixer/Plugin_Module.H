
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

class Plugin_Module : Module {

    void init ( void );

    int _ins;
    int _outs;
    int _instances;

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

    static Plugin_Info* discover ( void );

    bool load ( Plugin_Info * );

    void set_input_buffer ( int n, void *buf );
    void set_output_buffer ( int n, void *buf );
    void set_control_buffer ( int n, void *buf );
    void activate ( void );
    void deactivate ( void );
    void process ( unsigned long nframes );
    bool active ( void ) const { return _active; }

public:

    Plugin_Module( int W, int H, const char *L = 0 );
    Plugin_Module ( );
    virtual ~Plugin_Module();

    static Plugin_Module * pick_plugin ( void );

    int plugin_ins ( void ) const { return _plugin_ins; }
    int plugin_outs ( void ) const { return _plugin_outs; }

    bool ins ( int i );
    int ins ( void ) const { return _ins; }
    int outs ( void ) const { return plugin_outs() * _instances; }

    bool controllable ( void ) const { return false; }

    int instances ( void ) const { return _instances; }
    void instances ( int i ) { _instances = i; }

    void select_plugin ( unsigned long id );

    const char *name ( void ) const { return "Plugin"; }

    int can_support_inputs ( int );
    bool configure_inputs ( int );

    void process ( void );

protected:

//    virtual void draw ( void );
    virtual int handle ( int );

};
