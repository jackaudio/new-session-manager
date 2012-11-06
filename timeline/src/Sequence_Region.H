
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
#include "Sequence_Widget.H"

/* Base class representing a /region/ of time on a /sequence/, with
   controls for the offset and length points. */

class Sequence_Region : public Sequence_Widget
{

protected:

    virtual void get ( Log_Entry &e ) const;
    virtual void set ( Log_Entry &e );

    Sequence_Region ( );
    Sequence_Region ( const Sequence_Region &rhs );
    virtual ~Sequence_Region ( );

    virtual Fl_Align align ( void ) const { return FL_ALIGN_INSIDE | FL_ALIGN_CENTER | FL_ALIGN_WRAP; }
    virtual int handle ( int m );
    virtual void draw_box( void );
    virtual void draw ( void );
    virtual void draw_label ( void ) { Sequence_Widget::draw_label(); }
    virtual void draw_label ( const char *label, Fl_Align align, Fl_Color color=(Fl_Color)0, int xo=0, int yo=0 );

public:

    LOG_NAME_FUNC( Region );

    void trim_right ( nframes_t where );
    void trim_left ( nframes_t where );

    enum trim_e { NO, LEFT, RIGHT };
    void trim ( enum trim_e t, int X );

    void split ( Sequence_Region *copy, nframes_t where );
};
