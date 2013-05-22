
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

#include <MIDI/event.H>
#include <MIDI/event_list.H>
#include "const.h"
#include "instrument.H"

class smf;

#include "debug.h"

const int MAX_UNDO = 50;

#include <list>

#include <sigc++/sigc++.h>
using namespace sigc;

using std::list;

/* patterns and phrases use this structure */

class Canvas;

/* everything that must be lock-free. This whole structure gets copied and swapped
   for each write method */
struct data {

    tick_t           length;
    int              state;
    MIDI::event_list       events;

    data( void )
        {
            length = 0;
            state = 0;
        }

    data( const data &rhs )
        {
            events = rhs.events;
            length = rhs.length;
            state = rhs.state;
        }
/*     data() { MESSAGE( "allocating new editable data" ); }       */
    //   ~data() { MESSAGE( "deleting old undo data, freeing at least %d bytes.", events.size() * sizeof ( event ) + sizeof ( list <event> )); }
};


struct Viewport {

    #define format "%d:%d:%d:%d"

    int x, y, w, h;

    Viewport ( void )
        {
            x = y = w = h = 0;
        }

    char *
    dump ( void ) const
        {
            char *s;

            asprintf( &s, format, x, y, w, h );

            return s;
        }

    void
    read( const char *s )
        {
            sscanf( s, format, &x, &y, &w, &h );
        }

    #undef format
};



class Grid : public trackable
{
    static MIDI::event_list _clipboard;

protected:

    unsigned int     _height;
    char     *_notes;
    char     *_name;
    int      _number;
    
    unsigned int     _bpb;                                  /* beats per bar */
    unsigned int     _ppqn;                                 /* pulses per quarter note (beat) */


    /* Used by playback thread. When a pattern or phrase is triggered,
       these fields are filled in appropriately */
    mutable tick_t _start;                                              /* absolute start tick of loop */
    mutable tick_t _end;                                                /* absolute end tick of loop  */
    volatile mutable tick_t   _index;                                   /* playhead, relative to start -- primarily used to draw the playhead */
    volatile mutable bool     _playing;                                 /* true if currently playing  */

    mutable volatile int    _mode;                                       /* mute, solo */

    // FIXME: shouldn't this be "volatile"?
//    const volatile data *_rd;                                   /* read only data used by RT thread */
    const data * volatile _rd;
    data *_rw;                                                  /* temporary writable copy used by UI thread */

    list <data *> _history;

    void _remove_marked ( void );
    MIDI::event * _event ( int x, int y, bool write ) const;
    bool _delete ( int x, int y  );
    void _link ( void );
    void _relink ( void );
    void _fix_length ( void );


private:

    volatile int _locked;

public:

    typedef void draw_note_func_t ( tick_t x, int y, tick_t l, int velocity, int selected, void *userdata );

    void draw_notes ( draw_note_func_t draw_note, void *userdata ) const;

    signal <void> signal_events_change;
    signal <void> signal_settings_change;

    Viewport viewport;                                          /* used by the Canvas */

    Grid ( void );
    virtual ~Grid ( void );
    Grid ( const Grid &rhs );

    virtual bool velocity_sensitive ( void ) const { return true; }

    int y_to_note ( int y ) const;
    int note_to_y ( int n ) const;
    tick_t x_to_ts ( unsigned long x ) const;
    double ts_to_x ( tick_t ts ) const;

    virtual Grid * create ( void ) = 0;
    virtual Grid * clone ( void ) = 0;
    virtual void draw_row_names ( Canvas *c ) const = 0;
    virtual const char * row_name ( int r ) const = 0;

    virtual Grid * by_number ( int n ) const = 0;

    virtual void put ( int x, int y, tick_t l, int velocity = 64 );
    virtual bool is_set ( int x, int y ) const;

    void lock ( void );
    void unlock ( void );
    void clear ( void );
    void del ( int x, int y );
    void adj_velocity ( int x, int y, int n );
    void adj_duration ( int x, int y, int l );
    void set_duration ( int x, int y, int l );
    void set_end ( int x, int y, int l );
    int get_start ( int *x, int *y ) const;
    void move ( int x, int y, int nx, int ny );
    void record_event ( MIDI::event *e );
    tick_t index ( void ) const;
    bool playing ( void ) const;

    int height ( void ) const;
    void height ( int h );
    tick_t length ( void ) const;
    void length ( tick_t l );

    void bars ( int n );
    int bars ( void ) const;
    int beats ( void ) const;
    void trim ( void );
    void expand ( void );
    int division ( void ) const;
    int subdivision ( void ) const;
    int ppqn ( void ) const;
    int number ( void ) const;
    void name ( char *s );
    const char * name ( void ) const;
    void notes ( char *s );
    char * notes ( void ) const;
    virtual void mode ( int m );
    virtual int mode ( void ) const;
    int next_note_x ( int x ) const;
    int prev_note_x ( int x ) const;

    void fit ( void );

    void delete_selected ( void );
    void move_selected ( int l );
    void nudge_selected ( int l );
    void selected_velocity ( int v );

    void crop ( int l, int r );
    void crop ( int l, int r, int t, int b );

    void toggle_select ( int x, int y );
    void insert_time ( int x, int r );
    void select ( int start, int end );
    void select ( int start, int end, int t, int b );
    void delete_time ( int start, int end );
    void select_none ( void );
    void select_all ( void );
    void invert_selection ( void );

    void resolution ( unsigned int n );
    int resolution ( void ) const;

    void dump ( smf *f, int channel ) const;
    void draw ( Canvas *c, int bx, int by, int bw, int bh );
    void print ( void ) const;

    MIDI::event_list * events ( void ) const;
    void events ( const MIDI::event_list * el );

    void get_note_properties ( int x, int y, MIDI::note_properties *p ) const;

    int min_selected ( void ) const;

    void cut ( void );
    void copy ( void );
    void paste ( int offset );

    void undo ( void );

    virtual tick_t default_length ( void ) const
        {
            return PPQN;
        }

};


inline int
Grid::y_to_note ( int y ) const
{
    return 127 - y;
}

inline int
Grid::note_to_y ( int n ) const
{
    return 127 - n;
}

inline tick_t
Grid::x_to_ts ( unsigned long x ) const
{
    return (x * PPQN) / _ppqn;
}

inline double
Grid::ts_to_x ( tick_t ts ) const
{
    return (ts * _ppqn) / PPQN;
}
