
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

#include "Sequence_Point.H"
// #include "Tempo_Sequence.H"

class Tempo_Point : public Sequence_Point
{
    float _tempo;

    void
    _make_label ( void )
        {
            if ( ! _label )
                _label = (char*)malloc( 40 );

            snprintf( _label, 40, "%.1f", _tempo );
        }

protected:

//    const char *class_name ( void ) { return "Tempo_Point"; }

    virtual void get ( Log_Entry &e ) const;
    void set ( Log_Entry &e );

    Tempo_Point ( );

public:

    LOG_CREATE_FUNC( Tempo_Point );
    SEQUENCE_WIDGET_CLONE_FUNC( Tempo_Point );

    static bool edit ( float *tempo );

    Tempo_Point ( nframes_t when, float bpm );

    ~Tempo_Point ( );

    Tempo_Point ( const Tempo_Point &rhs ) : Sequence_Point( rhs )
        {
            _tempo = rhs._tempo;

            log_create();
        }


    float tempo ( void ) const
        { return _tempo; }
    void  tempo ( float v )
        { if ( v != _tempo ) { _tempo = v; _make_label(); timeline->redraw(); } }

    int handle ( int m );
};
