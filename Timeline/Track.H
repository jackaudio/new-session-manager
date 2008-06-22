
/*******************************************************************************/
/* Copyright (C) 2008 Jonathan Moore Liles                                     */
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
#include "Sequence.H"
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Box.H>
#include <FL/fl_draw.H>

#include "Loggable.H"

/* TODO: rename this to Audio_Track or something since it's clearly audio specific. */

#include <vector>
using std::vector;

#include "Engine/Port.H"

#include "Timeline.H"

class Control_Sequence;
class Annotation_Sequence;
class Playback_DS;
class Record_DS;
class Port;
class Audio_Region;
class Audio_File;


//class Audio_Sequence;

#include "Audio_Sequence.H"

class Track : public Fl_Group, public Loggable
{

    /* not permitted  */
    Track ( const Track &rhs );
    Track & operator= ( const Track &rhs );

public:

    Track ( const char *L, int channels=1 );
    virtual ~Track ( );

    static bool soloing ( void ) { return _soloing; }

    static const char *capture_format;

    struct Capture
    {
        Audio_File *audio_file;
        Audio_Region *region;
    };

private:

    static int _soloing;

    char *_name;

    bool _selected;

    bool _show_all_takes;

    int _size;

    enum { AUDIO } _type;

    Audio_Sequence *_sequence;

    bool configure_outputs ( int n );
    bool configure_inputs ( int n );

    void update_port_names ( void );
    const char *name_for_port( Port::type_e type, int n );
    void update_take_menu ( void );

    Track ( );
    void init ( void );


protected:

    void get ( Log_Entry &e ) const;
    void set ( Log_Entry &e );

public:

    Fl_Input       *name_field;
    Fl_Button      *record_button;
    Fl_Button      *mute_button;
    Fl_Button      *solo_button;
    Fl_Menu_Button *take_menu;
    Fl_Group       *controls;

    Fl_Pack        *pack;
    Fl_Pack        *annotation;
    Fl_Pack        *control;
    Fl_Pack        *takes;


    vector<Port>   input;                        /* input ports... */
    vector<Port>   output;                       /* output ports... */

    Playback_DS    *playback_ds;
    Record_DS      *record_ds;

    /* for loggable */
    LOG_CREATE_FUNC( Track );

    void add ( Annotation_Sequence *t );
    void remove ( Annotation_Sequence *t );

    void add ( Control_Sequence *t );
    void add ( Audio_Sequence *t );
    void remove ( Audio_Sequence *t );
    void remove ( Control_Sequence *t );

    void select ( int X, int Y, int W, int H, bool include_control, bool merge_control );

    int size ( void ) const { return _size; }

    int ncontrols ( void ) { return control->children(); }

    void resize ( void );
    void size ( int v );

    int height ( void ) const
        {
            static int table[] = { 30, 80, 150, 300 };

            return table[ _size ];
        }

    void show_all_takes ( bool b )
        {
            _show_all_takes = b;
            resize();
        }

    void name ( const char *name )
        {
            if ( _name )
                free( _name );

            _name = strdup( name );

            if ( name_field )
                name_field->value( _name );

            update_port_names();
        }

    const char * name ( void ) const { return _name; }
    bool mute ( void ) const { return mute_button->value(); }
    bool solo ( void ) const { return solo_button->value(); }
    bool armed ( void ) const { return record_button->value(); }
    bool selected ( void ) const { return _selected; }

    static void cb_input_field ( Fl_Widget *w, void *v );
    void cb_input_field ( void );
    static void cb_button ( Fl_Widget *w, void *v );
    void cb_button ( Fl_Widget *w );


    static int width ( void ) { return 150; }

    void sequence ( Audio_Sequence * t );
    Audio_Sequence * sequence ( void ) const { return _sequence; }


    Fl_Menu_Button & menu ( void ) const;

    static void menu_cb ( Fl_Widget *w, void *v );
    void menu_cb ( const Fl_Menu_ *m );

    void draw ( void );
    int handle ( int m );

    /* Engine */
    const Audio_Region *capture_region ( void ) const;

    void resize_buffers ( nframes_t nframes );
    nframes_t process ( nframes_t nframes );
    void seek ( nframes_t frame );

    void record ( Capture *c, nframes_t frame );
    void write ( Capture *c, sample_t *buf, nframes_t nframes );
    void finalize ( Capture *c, nframes_t frame );

};
