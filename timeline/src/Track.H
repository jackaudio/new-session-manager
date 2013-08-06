
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

#include "JACK/Port.H"

#include "Timeline.H"

class Control_Sequence;
class Annotation_Sequence;
class Playback_DS;
class Record_DS;
// class JACK::Port;
class Audio_Region;
class Audio_File;
class Fl_Scalepack;
class Fl_Sometimes_Pack;
class Fl_Blink_Button;

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

    static bool colored_tracks;

    struct Capture
    {
        Audio_File *audio_file;
        Audio_Region *region;

        Capture ( )
            {
                region = 0;
                audio_file = 0;
            }
    };

    virtual Fl_Color color ( void ) const { return child(0)->color(); }
    virtual void color ( Fl_Color c ) { child(0)->color( c ); }

    bool operator< ( const Track &rhs ) const
        {
            return _row < rhs._row;
        }

private:

    static int _soloing;

    char *_name;

    bool _selected;

    int _size;

    int _row;

    enum { AUDIO } _type;

    Audio_Sequence *_sequence;

    bool configure_outputs ( int n );
    bool configure_inputs ( int n );
    void command_configure_channels ( int n );

    void update_port_names ( void );

    Track ( );
    void init ( void );

protected:

    void get ( Log_Entry &e ) const;
    void get_unjournaled ( Log_Entry &e ) const;
    void set ( Log_Entry &e );

    Fl_Scalepack       *controls;
    Fl_Sometimes_Pack  *control;
    Fl_Blink_Button    *connected_indicator;

    Fl_Group           *controls_heading;
    Fl_Group           *takes_heading;

public:

    virtual void log_children ( void ) const;

    Fl_Input       *name_field;
    Fl_Button      *record_button;
    Fl_Button      *mute_button;
    Fl_Button      *solo_button;
    Fl_Button      *show_all_takes_button;
    Fl_Button      *overlay_controls_button;
    Fl_Button      *menu_button;

    Fl_Pack        *pack;
    Fl_Pack        *annotation;
    Fl_Pack        *takes;


    vector<JACK::Port>   input;                        /* input ports... */
    vector<JACK::Port>   output;                       /* output ports... */

    Playback_DS    *playback_ds;
    Record_DS      *record_ds;

    /* for loggable */
    LOG_CREATE_FUNC( Track );

    Control_Sequence * control_by_name ( const char *name );
    char * get_unique_control_name ( const char *name );

    void add ( Annotation_Sequence *t );
    void remove ( Annotation_Sequence *t );

    void add ( Control_Sequence *t );
    void add ( Audio_Sequence *t );
    void remove ( Audio_Sequence *t );
    void remove ( Control_Sequence *t );

    void select ( int X, int Y, int W, int H, bool include_control, bool merge_control );

    int size ( void ) const { return _size; }

    int ncontrols ( void ) { return ((Fl_Group*)control)->children(); }

    void adjust_size ( void );
    void size ( int v );

    int height ( void ) const
        {
            static int table[] = { 25, 54, 80, 150, 300 };

            return table[ _size ];
        }

    void show_all_takes ( bool b )
        {
            show_all_takes_button->value( b );
            adjust_size();
        }

    bool show_all_takes ( void ) const
        {
            return show_all_takes_button->value();
        }

    void overlay_controls ( bool b )
        {
            overlay_controls_button->value( b );
            adjust_size();
        }
    bool overlay_controls ( void ) const
        {
            return overlay_controls_button->value();
        }

    void name ( const char *name );
    const char * name ( void ) const;

    bool mute ( void ) const { return mute_button->value(); }
    void mute ( bool b ) { mute_button->value( b ); }
    bool solo ( void ) const { return solo_button->value(); }
    void solo ( bool b );

    bool armed ( void ) const { return record_button->value(); }
    void armed ( bool b ) { record_button->value( b ); }

    bool selected ( void ) const { return _selected; }

    int row ( void ) const;
    void row ( int );

    static void cb_button ( Fl_Widget *w, void *v );
    void cb_button ( Fl_Widget *w );


    static int width ( void ) { return 200; }

    void sequence ( Audio_Sequence * t );
    Audio_Sequence * sequence ( void ) const { return _sequence; }


    Fl_Menu_Button & menu ( void ) const;

    static void menu_cb ( Fl_Widget *w, void *v );
    void menu_cb ( const Fl_Menu_ *m );

    void draw ( void );
    int handle ( int m );

    void process_osc ( void );
    void connect_osc ( void );
    void update_osc_connection_state ( void );

    /* Engine */
    const Audio_Region *capture_region ( void ) const;
    Capture *capture ( void );

    void resize_buffers ( nframes_t nframes );
    nframes_t process_input ( nframes_t nframes );
    nframes_t process_output ( nframes_t nframes );
    void seek ( nframes_t frame );
    void delay ( nframes_t frames );

    void record ( Capture *c, nframes_t frame );
    void write ( Capture *c, sample_t *buf, nframes_t nframes );
    void finalize ( Capture *c, nframes_t frame );

};
