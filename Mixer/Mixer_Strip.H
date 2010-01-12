
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
#include <FL/Fl_Input.H>
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

class Mixer_Strip : public Fl_Group, public Loggable {

public:

    Mixer_Strip(int X, int Y, int W, int H, const char *L = 0);
    Mixer_Strip( const char *name, int channels );
    Mixer_Strip();                                              /* for log create */
    ~Mixer_Strip();

//    Fl_Value_Slider *gain_slider;
    Fl_Flip_Button *prepost_button;
    Fl_Flip_Button *tab_button;
    Fl_Button *close_button;
    Fl_Button *left_button;
    Fl_Button *right_button;
    Fl_Input *name_field;

    Fl_Flowpack *controls_pack;
    Fl_Group *signal_tab;
    Fl_Group *fader_tab;
    Fl_Pack *panner_pack;

    Chain *_chain;

    Controller_Module *gain_controller;
    Controller_Module *jack_input_controller;
    Meter_Indicator_Module *meter_indicator;

    sample_t **chain_buffer;
    int chain_buffers;

    nframes_t nframes;

private:

    unsigned _color;

    void init ( );
    void cb_handle(Fl_Widget*);
    static void cb_handle(Fl_Widget*, void*);

    void set_tab ( void );

    void update_port_names ( void );

protected:

    void get ( Log_Entry &e ) const;
    void set ( Log_Entry &e );

    virtual int handle ( int m );

public:

    void chain ( Chain *c );

    void log_children ( void );

    void color ( Fl_Color c )
        {
            _color = c;
            name_field->color( _color );
            name_field->redraw();
        }

    Fl_Color color ( void ) const
        {
            return name_field->color();
        }

    LOG_CREATE_FUNC( Mixer_Strip );

    void process ( unsigned int nframes );

    static void configure_outputs ( Fl_Widget *o, void *v );
    void configure_outputs ( void );

    bool configure_ports ( int n );

    void handle_module_added ( Module *m );

    void update ( void );

    // int channels ( void ) const { return _in.size(); }
    void name ( const char *name );
    const char *name ( void ) const { return label(); }
};
