
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
#include "Tempo_Point.H"

class Tempo_Sequence : public Sequence
{

protected:

    /* not used */
    void get ( Log_Entry & ) const { }
    void set ( Log_Entry & ) { }

public:

    Fl_Cursor cursor ( void ) const { return FL_CURSOR_DEFAULT; }

    Tempo_Sequence ( int X, int Y, int W, int H ) : Sequence ( X, Y, W, H )
        {
//            box( FL_DOWN_BOX );
        }

    void handle_widget_change ( nframes_t start, nframes_t length );
    int handle ( int m );

};
