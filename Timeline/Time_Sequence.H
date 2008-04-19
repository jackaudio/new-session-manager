
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
#include "Time_Point.H"

#include <list>
using std::list;

class Time_Sequence : public Sequence
{

public:

    Time_Sequence ( int X, int Y, int W, int H ) : Sequence ( X, Y, W, H )
        {
            box( FL_UP_BOX );
        }

    time_sig
    time ( nframes_t when )
        {
            for ( list <Sequence_Widget *>::const_reverse_iterator i = _widgets.rbegin();
                  i != _widgets.rend(); i++ )
            {
                if ( (*i)->offset() < when )
                    return ((Time_Point*)(*i))->time();
            }

            return time_sig( 4, 4 );
        }

    void
    time ( nframes_t when, int bpb, int note )
        {
            add( new Time_Point( when, bpb, note ) );
        }

};
