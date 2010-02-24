
/*******************************************************************************/
/* Copyright (C) 2010 Jonathan Moore Liles                                     */
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
#include "DPM.H"
#include "Panner.H"
#include <FL/Fl_Scalepack.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Flip_Button.H>
#include <FL/Fl_Arc_Dial.H>
#include <FL/Boxtypes.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Value_Slider.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Progress.H>
//#include "Fader.H"
#include <JACK/Port.H>


#include "../FL/Fl_Sometimes_Input.H"

#include "Loggable.H"
class Chain;
class Fl_Flowpack;
class Controller_Module;
class Meter_Indicator_Module;
class Module;
class Fl_Flip_Button;
class Fl_Input;
class Fl_Menu_;
class Fl_Menu_Button;

class Mixer_Strip : public Fl_Group, public Loggable {

public:

    static int min_h ( void ) { return 336; }

    Mixer_Strip( const char *strip_name );
    Mixer_Strip();                                              /* for log create */
    virtual ~Mixer_Strip();

    void chain ( Chain *c );

    void log_children ( void );

    virtual void color ( Fl_Color c );
    virtual Fl_Color color ( void ) const;

    LOG_CREATE_FUNC( Mixer_Strip );

    static void configure_outputs ( Fl_Widget *o, void *v );
    void configure_outputs ( void );

    bool configure_ports ( int n );

    void handle_module_added ( Module *m );
    void handle_module_removed ( Module *m );

    void update ( void );

    void name ( const char *name );
    const char *name ( void ) const { return label(); }

private:

    /* used to defer setting the mode of the gain controller until the
     chain has been added and the controller connected to a default
     module */
    int _gain_controller_mode;

    Fl_Flip_Button *width_button;
    Fl_Flip_Button *tab_button;
    Fl_Button *close_button;
    Fl_Button *left_button;
    Fl_Button *right_button;
    Fl_Input *name_field;

    Fl_Flowpack *controls_pack;
    Fl_Group *tab_group;
    Fl_Group *signal_tab;
    Fl_Group *fader_tab;
    Fl_Pack *panner_pack;

    Chain *_chain;

    Controller_Module *gain_controller;
    Controller_Module *jack_input_controller;
    Controller_Module *spatialization_controller;
    Meter_Indicator_Module *meter_indicator;

    nframes_t nframes;

    Fl_Color _color;

    void init ( );
    void cb_handle(Fl_Widget*);
    static void cb_handle(Fl_Widget*, void*);

    void set_tab ( void );

    void update_port_names ( void );

    void menu_cb ( const Fl_Menu_ *m );
    static void menu_cb ( Fl_Widget *w, void *v );
    Fl_Menu_Button & menu ( void ) const;

protected:

    void get ( Log_Entry &e ) const;
    void set ( Log_Entry &e );

    virtual int handle ( int m );
    virtual void draw ( void );

public:

    void command_move_left ( void );
    void command_move_right ( void );
    void command_close ( void );
    void command_rename ( const char * s );
    void command_width ( bool b );
    void command_view ( bool b );

};
