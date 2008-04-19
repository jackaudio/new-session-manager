
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

#include "Track_Point.H"
#include "Loggable.H"

struct time_sig
{
    int beats_per_bar;
    int beat_type;

    time_sig ( )
        {
            beats_per_bar = 0;
            beat_type = 0;
        }

    time_sig ( int bpb, int note )
        {
            beats_per_bar = bpb;
            beat_type = note;
        }
};


#define __CLASS__ "Time_Point"

class Time_Point : public Track_Point
{
    time_sig _time;

    void
    _make_label ( void )
        {
            if ( ! _label )
                _label = new char[40];

            snprintf( _label, 40, "%d/%d", _time.beats_per_bar, _time.beat_type );
        }

    Time_Point ( ) : _time( 4, 4 )
        {

        }

protected:

    const char *class_name ( void ) { return "Time_Point"; }

    char ** get ( void );
    void set ( char **sa );


public:

    /* for loggable */
    static Loggable *
    create ( char **sa )
        {
            Time_Point *r = new Time_Point;

            r->set( sa );

            return (Loggable *)r;
        }

    Time_Point ( nframes_t when, int bpb, int note ) : _time( bpb, note )
        {
            _r->offset = when;
            _make_label();

            log_create();
        }

    Time_Point ( const Time_Point &rhs )
        {
            _r->offset = rhs._r->offset;
            _time = rhs._time;
        }

    Track_Widget *clone ( const Track_Widget *r )
        {
            return new Time_Point( *(Time_Point*)r );
        }

    ~Time_Point ( )
        {
            if ( _label ) delete[] _label;
            log_destroy();
        }

/*     beats_per_bar ( void ) const { return _time.beats_per_bar; } */
/*     beat_type ( void ) const { return _beat_type; } */

    void time ( int bpb, int note ) { _time.beats_per_bar = bpb; _time.beat_type = note; }
    time_sig time ( void ) const { return _time; }

    int
    handle ( int m )
        {
            int r = Track_Widget::handle( m );

            if ( m == FL_RELEASE )
            {
                _track->sort();
                timeline->redraw();
            }
            return r;
        }

};

#undef __CLASS__
