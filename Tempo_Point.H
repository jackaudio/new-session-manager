
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

#define __CLASS__ "Tempo_Point"

class Tempo_Point : public Track_Point
{
    float _tempo;

    void
    _make_label ( void )
        {
            if ( ! _label )
                _label = new char[40];

            snprintf( _label, 40, "%.1f", _tempo );
        }

protected:

    const char *class_name ( void ) { return "Tempo_Point"; }

/*     void log_create ( void )  { log( "create", "%lu %f", _offset, _tempo ); } */
/*     void log_destroy ( void ) { log( "destroy", NULL ); } */
/*     void log_move ( void )    { log( "move", "%lu", _offset ); } */

public:

    Tempo_Point ( nframes_t when, float bpm )
        {
            _tempo = bpm;
            _offset = when;

            _make_label();

            log_create();
        }

    ~Tempo_Point ( )
        { if ( _label ) delete[] _label; }

    float tempo ( void ) const { return _tempo; }
    void tempo ( float v ) { _tempo = v; }


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
#undef __CLASS__
