
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
#include <Module.H>
#include "Loggable.H"

class Fl_Pack;
class Fl_Flowpack;
class Module;
class Fl_Menu_Button;
class Panner;
class Controller_Module;
#include <vector>

class Spatialization_Console : public Loggable, public Fl_Double_Window
{
    Panner *panner;

    static void cb_panner_value_handle ( Fl_Widget *w, void *v );
    void make_controls ( void );

    static void cb_window ( Fl_Widget *w, void *v );
    static void cb_window ( Fl_Widget *w );

    bool _resized;
    int _min_width;

    int azimuth_port_number;
    int elevation_port_number;
  
    std::vector<Fl_Widget*> controls_by_port;  

protected:

    virtual void get ( Log_Entry &e ) const;
    virtual void set ( Log_Entry &e );

public:

    LOG_CREATE_FUNC( Spatialization_Console );

    void handle_control_changed ( Controller_Module *m );

    void update ( void );

    Spatialization_Console ( );
    virtual ~Spatialization_Console ( );
};
