
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

#include "Track_Widget.H"

class Track_Point : public Track_Widget
{

protected:

    char *_label;

    void set ( char **a )
        {
        }

    char ** log_dump ( void )
        {
            return NULL;
        }
    void log_change ( void )
        {
        }

public:

    Fl_Align align ( void ) const { return FL_ALIGN_RIGHT; }
    int abs_w ( void ) const { return 10; }
    nframes_t length ( void ) const { return timeline->x_to_ts( abs_w() ); }

    Track_Point ( )
        {
            _label = NULL;
        }

    void
    draw ( int X, int Y, int W, int H )
        {
            Track_Widget::draw( x(), Y, w(), H );

            draw_label( _label, align() );
        }
};
