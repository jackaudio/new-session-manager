
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
class Spatialization_Console;
namespace OSC { class Endpoint; }
#include <lo/lo.h>
class Group;

class Mixer : public Fl_Group
{

public:

    OSC::Endpoint *osc_endpoint;
    Fl_Button *sm_blinker;

private:

    float _update_interval;

    static void show_tooltip ( const char *s );
    static void hide_tooltip ( void );

    int _rows;
    int _strip_height;

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
    Fl_Box *_status;

    Fl_Flowpack *mixer_strips;

    void load_options ( void );
    void save_options ( void );
    void save_translations ( void );
    void load_translations ( void );

    static void send_feedback_cb ( void *v );
    void send_feedback ( void );
    void redraw_windows ( void );

    static void handle_dirty ( int, void *v );

    static int osc_non_hello ( const char *, const char *, lo_arg **, int , lo_message msg, void * );
    static int osc_strip_by_number ( const char *, const char *, lo_arg **, int , lo_message msg, void * );

    static void update_cb ( void * );
    void update_cb ( void );


public:
    
    void auto_connect ( void );
    void maybe_auto_connect_output ( Module::Port *p );
    std::list<std::string> get_auto_connect_targets ( void );
    Group * group_by_name ( const char * name );
    char *get_unique_group_name ( const char *name );

    std::list <Group*> groups;
    Group *group ( int n );
    void add_group ( Group *g );
    void remove_group ( Group *g );
    
    void update_menu ( void );

    static Spatialization_Console *spatialization_console;

    int nstrips ( void ) const;
    Mixer_Strip* track_by_number ( int n );

    void update_frequency ( float f );
    
    void status ( const char *s ) { 
        if ( s ) _status->copy_label( s );
        else _status->label(0); 
        _status->redraw(); }

    virtual int handle ( int m );

    char * get_unique_track_name ( const char *name );

    int min_h ( void ) const { return Mixer_Strip::min_h() + (18 * 2); }

    void rows ( int n );
    virtual void resize ( int X, int Y, int W, int H );

    void new_strip ( void );
    void add ( Mixer_Strip *ms );
    void remove ( Mixer_Strip *ms );
    void move_left ( Mixer_Strip *ms );
    void move_right ( Mixer_Strip *ms );
    void insert ( Mixer_Strip *ms, Mixer_Strip *before );
    void insert ( Mixer_Strip *ms, int i );
    bool contains ( Mixer_Strip *ms );
    Mixer_Strip * event_inside ( void );
    int find_strip ( const Mixer_Strip *m ) const;

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
    
    void command_toggle_fader_view ( void );
    void command_clear_mappings ( void );
    void command_new ( void );
    bool command_save ( void );
    bool command_load ( const char *path, const char *display_name = 0 );
    bool command_new ( const char *path, const char *display_name = 0 );
    void command_quit ( void );

    void command_add_strip ( void );

};

extern Mixer* mixer;
