
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

#include <FL/Fl_Double_Window.H>
class Fl_Pack;
class Fl_Flowpack;
class Fl_Choice;
class Module;

class Module_Parameter_Editor : public Fl_Double_Window
{
    Module *_module;

    struct callback_data
    {
        Module_Parameter_Editor *base_widget;
        int port_number;

        callback_data ( Module_Parameter_Editor *base_widget, int port_number )
            {
                this->base_widget = base_widget;
                this->port_number = port_number;
            }
    };

    static void cb_value_handle ( Fl_Widget *w, void *v );
    static void cb_mode_handle ( Fl_Widget *w, void *v );
    static void cb_bound_handle ( Fl_Widget *w, void *v );
    void set_value (int i, float value );
    void bind_control ( int i );
    void make_controls ( void );

    Fl_Pack *main_pack;
    Fl_Flowpack *control_pack;
    Fl_Choice *mode_choice;
    bool _resized;



public:

    Module_Parameter_Editor ( Module *module );
    virtual ~Module_Parameter_Editor ( );
};
