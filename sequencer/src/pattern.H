
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
#include "common.h"

#include <vector>
using std::vector;

class pattern : public Grid
{
    static MIDI::event_list _recorded_events;
    static vector <pattern *> _patterns;
    static int _solo;
    static int _pattern_recording;


    static int solo ( void );

    int _channel, _port;

    bool _recording;
    mutable volatile bool _cleared;

    mutable volatile int _queued;

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
    static void record_event ( const MIDI::midievent *e );

    pattern * create ( void );
    pattern * by_number ( int n ) const;

    pattern * clone ( void );
    void put ( int x, int y, tick_t l );
    const char * row_name ( int r ) const;
    void draw_row_names ( Canvas *c ) const;
    void trigger ( tick_t start, tick_t end );
    void trigger ( void );
    void stop ( void ) const;
    void play ( tick_t start, tick_t end ) const;

    void load ( smf *f );

    void record( int mode );
    void record_stop ( void );

    void queue ( int mode );
    int queue ( void ) const;

    void row_name_press ( int y );

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

    virtual tick_t default_length ( void ) const
        {
            return PPQN * 4 / _note;
        }

};
