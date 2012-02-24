
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

class Control_Sequence : public Sequence
{
    /* not permitted */
    Control_Sequence ( const Control_Sequence &rhs );
    Control_Sequence & operator = ( const Control_Sequence &rhs );

public:

    enum curve_type_e { None, Linear, Quadratic };

private:

    JACK::Port *_output;

    /* these are used to cache the saved osc connection until the
     * session is loaded, at which time we will reconnect */
    char *_osc_connected_peer;
    char *_osc_connected_path;
    /* osc output port */
    OSC::Signal *_osc_output;

    bool _highlighted;

    curve_type_e _interpolation;
    int _frequency;
    float _interval;

    void init ( void );

    void draw_curve ( bool flip, bool filled );

    static void menu_cb ( Fl_Widget *w, void *v );
    void menu_cb ( const Fl_Menu_ *m );

    
    float _rate; 

protected:


    virtual void get ( Log_Entry &e ) const;
    virtual void get_unjournaled ( Log_Entry &e ) const;
    void set ( Log_Entry &e );

    Control_Sequence (  ) : Sequence( 0 )
        {
            init();
        }


    void draw ( void );
    int handle ( int m );

public:

    static void process_osc ( void *v );
    void process_osc ( void );

    void connect_osc ( void );

    static bool draw_with_gradient;
    static bool draw_with_polygon;
    static bool draw_with_grid;

    LOG_CREATE_FUNC( Control_Sequence );

    Control_Sequence ( Track * );
    ~Control_Sequence ( );

    Fl_Cursor cursor ( void ) const { return FL_CURSOR_CROSS; }

    curve_type_e interpolation ( void ) const { return _interpolation; }
    void interpolation ( curve_type_e v ) { _interpolation = v; }
    int frequency ( void ) const { return _frequency; }
    void frequency ( int v ) { _frequency = v; _interval = ( 1.0f / (float)v ) / 2; }

    /* Engine */
    void output ( JACK::Port *p ) { _output = p; }
    nframes_t play ( sample_t *buf, nframes_t frame, nframes_t nframes );
    nframes_t process ( nframes_t nframes );

};
