
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

#include "Sequence.H"
#include "Control_Point.H"

#include "JACK/Port.H"

// class JACK::Port;
#include "OSC/Endpoint.H"

class Control_Sequence_Header;
class Fl_Menu_Button;

class Control_Sequence : public Sequence
{
    /* not permitted */
    Control_Sequence ( const Control_Sequence &rhs );
    Control_Sequence & operator = ( const Control_Sequence &rhs );

public:

    enum Curve_Type { None, Linear, Quadratic };

    enum Mode { 
        CV,
        OSC,
        MIDI
    };

private:

    static void cb_button ( Fl_Widget *w, void *v );
    void cb_button ( Fl_Widget *w );
    
    JACK::Port *_output;

    /* these are used to cache the saved osc connection until the
     * session is loaded, at which time we will reconnect */
    std::list<char*> _persistent_osc_connections;
    /* osc output port */
    volatile void *__osc_output;
    
    OSC::Signal *_osc_output ( void ) const
        {
            return (OSC::Signal *)__osc_output;
        }

    void _osc_output ( OSC::Signal * s)
        {
            __osc_output = s;
        }


    static void peer_callback(  OSC::Signal *sig,  OSC::Signal::State state, void *v );
    void peer_callback( OSC::Signal *sig, OSC::Signal::State state );
    void add_osc_peers_to_menu ( Fl_Menu_Button *m, const char *prefix );

    static Fl_Widget *_highlighted;

    Curve_Type _interpolation;

    void init ( void );

    void draw_curve ( bool filled );

    static void menu_cb ( Fl_Widget *w, void *v );
    void menu_cb ( const Fl_Menu_ *m );

    Mode _mode;
    
    float _rate; 

protected:
    
    Control_Sequence ( );

    virtual void get ( Log_Entry &e ) const;
    virtual void get_unjournaled ( Log_Entry &e ) const;
    void set ( Log_Entry &e );

    void draw_box ( void );
    void draw ( void );
    int handle ( int m );

    void update_osc_path ( void );
    void update_port_name ( void );


    Fl_Menu_Button & menu ( void );

public:

    Control_Sequence_Header * header ( void ) { return (Control_Sequence_Header*)child(0); }

    virtual void name ( const char *s );
    virtual const char *name ( void ) const;

    void process_osc ( void );
    void connect_osc ( void );
    void update_osc_connection_state ( void );

    static bool draw_with_polygon;
    static bool draw_with_grid;

    LOG_CREATE_FUNC( Control_Sequence );

    Control_Sequence ( Track *, const char *name = 0 );
    ~Control_Sequence ( );

    Fl_Cursor cursor ( void ) const { return FL_CURSOR_CROSS; }

    Curve_Type interpolation ( void ) const { return _interpolation; }
    void interpolation ( Curve_Type v ) 
        { 
            _interpolation = v; 
            damage( FL_DAMAGE_USER1 );
        }
    
    Mode mode ( void ) const { return _mode; }
    void mode ( Mode v );

    /* Engine */
    void output ( JACK::Port *p ) { _output = p; }
    nframes_t play ( sample_t *buf, nframes_t frame, nframes_t nframes );
    nframes_t process ( nframes_t nframes );

};
