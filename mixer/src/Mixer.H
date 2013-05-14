
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
#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Pack.H>
#include "Mixer_Strip.H"

#include "Thread.H"

class Fl_Blink_Button;
class Fl_Flowpack;
class Fl_Menu_Bar;
namespace OSC { class Endpoint; }
#include <lo/lo.h>

class Mixer : public Fl_Group
{

public:

    OSC::Endpoint *osc_endpoint;
    Fl_Button *sm_blinker;

private:

    int _rows;

    Fl_Color system_colors[3];

    Mixer_Strip* track_by_name ( const char *name );

    void snapshot ( void );
    static void snapshot ( void *v ) { ((Mixer*)v)->snapshot(); }

    void cb_menu(Fl_Widget*);
    static void cb_menu(Fl_Widget*, void*);

    Fl_Menu_Bar *menubar;
    Fl_Scroll *scroll;
    Fl_Pack *pack;
    Fl_Box *project_name;

    Fl_Flowpack *mixer_strips;

    void load_options ( void );
    void save_options ( void );
    void update_menu ( void );
    
    void redraw_windows ( void );

    static void handle_dirty ( int, void *v );

    static int osc_non_hello ( const char *, const char *, lo_arg **, int , lo_message msg, void * );

public:

    char * get_unique_track_name ( const char *name );

    int min_h ( void ) const { return Mixer_Strip::min_h() + (18 * 2); }

    void rows ( int n );
    virtual void resize ( int X, int Y, int W, int H );

    void new_strip ( void );
    void process ( unsigned int nframes );
    void add ( Mixer_Strip *ms );
    void remove ( Mixer_Strip *ms );
    void move_left ( Mixer_Strip *ms );
    void move_right ( Mixer_Strip *ms );
    void insert ( Mixer_Strip *ms, Mixer_Strip *before );
    void insert ( Mixer_Strip *ms, int i );
    bool contains ( Mixer_Strip *ms );

    bool save ( void );
    void quit ( void );

    Mixer ( int X, int Y, int W, int H, const char *L );
    virtual ~Mixer();

    void handle_hello ( lo_message msg );

    void announce ( const char *nash_url, const char *process_name );

    int init_osc ( const char* osc_port );

    void sm_active ( bool b );

    void say_hello ( void );

    void save_default_project_settings ( void );
    void load_default_project_settings ( void );
    void reset_project_settings ( void );
    void save_project_settings ( void );
    void load_project_settings ( void );

public:

    void command_new ( void );
    bool command_save ( void );
    bool command_load ( const char *path, const char *display_name = 0 );
    bool command_new ( const char *path, const char *display_name = 0 );
    void command_quit ( void );

    void command_add_strip ( void );

};

extern Mixer* mixer;
