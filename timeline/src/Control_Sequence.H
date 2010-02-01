
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

class Control_Sequence : public Sequence
{
    /* not permitted */
    Control_Sequence ( const Control_Sequence &rhs );
    Control_Sequence & operator = ( const Control_Sequence &rhs );

public:

    enum curve_type_e { Linear, Quadratic };

private:

    JACK::Port *_output;

    bool _highlighted;

    curve_type_e _type;

    void init ( void );

    void draw_curve ( bool flip, bool filled );

protected:


    virtual void get ( Log_Entry &e ) const;
    void set ( Log_Entry &e );

    Control_Sequence (  ) : Sequence( 0 )
        {
            init();
        }


    void draw ( void );
    int handle ( int m );

public:

    static bool draw_with_gradient;
    static bool draw_with_polygon;
    static bool draw_with_grid;

    LOG_CREATE_FUNC( Control_Sequence );

    Control_Sequence ( Track * );
    ~Control_Sequence ( );

    Fl_Cursor cursor ( void ) const { return FL_CURSOR_CROSS; }

    /* Engine */
    void output ( JACK::Port *p ) { _output = p; }
    nframes_t play ( sample_t *buf, nframes_t frame, nframes_t nframes );
    nframes_t process ( nframes_t nframes );

};
