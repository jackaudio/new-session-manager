
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

struct time_sig
{
    int beats_per_bar;
    int note_type;

    time_sig ( int bpb, int note )
        {
            beats_per_bar = bpb;
            note_type = note;
        }
};

class Time_Point : public Track_Point
{
    time_sig _time;

    void
    _make_label ( void )
        {
            if ( ! _label )
                _label = new char[40];

            snprintf( _label, 40, "%d/%d", _time.beats_per_bar, _time.note_type );
        }

public:

    Time_Point ( nframes_t when, int bpb, int note ) : _time( bpb, note )
        {
            _offset = when;
            _make_label();
        }

    ~Time_Point ( )
        { if ( _label ) delete[] _label; }

/*     beats_per_bar ( void ) const { return _time.beats_per_bar; } */
/*     note_type ( void ) const { return _note_type; } */

    void time ( int bpb, int note ) { _time.beats_per_bar = bpb; _time.note_type = note; }
    time_sig time ( void ) const { return _time; }

    int
    handle ( int m )
        {
            int r = Track_Widget::handle( m );

            if ( m == FL_RELEASE )
            {
                _track->sort();
                timeline->tracks->redraw();
            }
            return r;
        }

};
