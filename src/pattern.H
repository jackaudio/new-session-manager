
/*******************************************************************************/
/* Copyright (C) 2007-2008 Jonathan Moore Liles                                */
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

#include "grid.H"
#include "canvas.H"
#include "mapping.H"
// #include "event.H"

typedef unsigned long tick_t;

#include <vector>
using std::vector;

class pattern : public Grid
{
    static event_list _recorded_events;
    static vector <pattern *> _patterns;
    static int _solo;
    static int _pattern_recording;

    static int solo ( void );

    int _channel, _port;

    bool _recording;
    mutable volatile bool _cleared;

    volatile bool _triggered;

//    int _key;

    int _note;


    void _add ( void );


public:

    static signal <void> signal_create_destroy;

    Mapping mapping;

    pattern ( void );
    ~pattern ( void );
    pattern ( const pattern &rhs );

    static int patterns ( void );
    static pattern * pattern_by_number ( int n );
    static void reset ( void );
    static pattern * import ( smf *f, int track );

    static pattern * recording ( void );
    static void record_event ( const midievent *e );

    pattern * create ( void );
    pattern * by_number ( int n ) const;

    pattern * clone ( void );
    void put ( int x, int y, tick_t l );
    const char * row_name ( int r ) const;
    void draw_row_names ( Canvas *c ) const;
    void trigger ( tick_t start, tick_t end );
    void stop ( void ) const;
    void play ( tick_t start, tick_t end ) const;

    void load ( smf *f );

    void record( int mode );
    void record_stop ( void );

    void toggle_trigger ( void );
    bool triggered ( void ) const;

    void randomize_row ( int y, int feel, float probability );

    int port ( void ) const;
    void port ( int p );
    int channel ( void ) const;
    void channel ( int c );
    int note ( void ) const;
    void note ( int n );

    void mode ( int n );
    int mode ( void ) const;

    int key ( void ) const;
    void key ( int k );

    int load ( const char *name );
    void save ( const char *name ) const;
    void dump ( smf *f ) const;

    int ppqn ( void ) const;
    void ppqn ( int n );

};
