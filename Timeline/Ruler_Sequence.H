
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
#include "Ruler_Point.H"
#include "Timeline.H"

class Ruler_Sequence : public Sequence
{

public:

    Ruler_Sequence ( int X, int Y, int W, int H ) : Sequence ( X, Y, W, H )
        {
//            box( FL_UP_BOX );
        }

/*     void */
/*     draw ( void ) */
/*         { */
/* //            timeline->draw_measure_BBT( x(), y(), w(), h(), FL_WHITE ); */
/*             Sequence::draw(); */
/*         } */

    int handle ( int m )
        {

            if ( Sequence::handle( m ) )
                return 1;

            switch ( m )
            {
                case FL_PUSH:
                {
                    if ( Fl::event_button1() )
                    {
                        add( new Ruler_Point( this, x_to_offset( Fl::event_x() ), "mark" ) );
                        redraw();
                    }
                    break;
                }
                default:
                    break;

            }

            return 0;
        }
};
