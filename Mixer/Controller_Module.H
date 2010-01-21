
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

class Fl_Menu_Button;
class Fl_Menu_;

class Fl_Valuator;

class Controller_Module : public Module
{

    static void update_cb ( void *v );
    void update_cb ( void );

    bool _pad;

    volatile float control_value;

    Fl_Menu_Button & menu ( void );
    static void menu_cb ( Fl_Widget *w, void *v );
    void menu_cb ( const Fl_Menu_ *m );

public:

    enum Mode { GUI, CV, OSC, MIDI };

    Mode mode ( void ) const { return _mode; }
    void mode ( Mode v );

    Controller_Module ( bool is_default = false );
//    Controller_Module ( int W, int H, const char *L=0 );
    virtual ~Controller_Module ( );

    const char *name ( void ) const { return "Controller"; }

    int can_support_inputs ( int ) { return 0; }
    bool configure_inputs ( int ) { return false; }

    void pad ( bool v ) { _pad = v; }

    static void cb_handle ( Fl_Widget *w, void *v );
    void cb_handle ( Fl_Widget *w );

    void connect_to ( Port *p );

    LOG_CREATE_FUNC( Controller_Module );


protected:

    void get ( Log_Entry &e ) const;
    void set ( Log_Entry &e );

    //   virtual void draw ( void );
    virtual void process ( void );

    virtual void draw ( void )
        {
            draw_box();
            Fl_Group::draw();
        }

    virtual int handle ( int m );

    void resize ( int, int, int, int );

private:

    std::vector<JACK::Port> jack_input;
    Mode _mode;

    Fl_Valuator *control;

};
