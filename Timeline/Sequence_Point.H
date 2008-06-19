
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

#include "Sequence_Widget.H"

class Sequence_Point : public Sequence_Widget
{
    /* not permitted  */
    Sequence_Point & operator= ( const Sequence_Point &rhs );

protected:

    char *_label;

    void get ( Log_Entry &e ) const;
    void set ( Log_Entry &e );


    virtual void draw_box ( void );
    virtual void draw ( void );

    Sequence_Point ( const Sequence_Point &rhs );
    Sequence_Point ( );
    ~Sequence_Point ( );

public:

    const char *name ( void ) const { return _label; }
    void name ( const char *s )
        {
            if ( _label )
                free( _label );
            _label = strdup( s );
            redraw();
        }

    Fl_Align align ( void ) const { return FL_ALIGN_RIGHT; }
    virtual int abs_w ( void ) const { return 8; }

//    virtual int abs_x ( void ) const { return Sequence_Widget::abs_x() - ( abs_w() >> 1 ); }
//    virtual int x ( void ) const { return Sequence_Widget::line_x() - ( abs_w() >> 1 ); }

    virtual int x ( void ) const
        {
            const int x = Sequence_Widget::x();

            if ( x >= sequence()->x() + sequence()->w() )
                return sequence()->x() + sequence()->w() + abs_w();
            else
                return x;
        }

    nframes_t length ( void ) const { return timeline->x_to_ts( abs_w() ); }

};
