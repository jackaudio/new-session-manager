
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

#include <FL/Fl.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/Fl_Slider.H>

#include <stdio.h>

class Scalebar : public Fl_Scrollbar
{
    double _zoom;

public:

    Scalebar ( int X, int Y, int W, int H ) : Fl_Scrollbar ( X, Y, W, H )
        {
            _zoom = 1.0f;

            step( 1 );
        }

    double zoom ( void ) const { return _zoom; }
    double value ( void ) const { return Fl_Slider::value(); }

    int
    handle ( int m )
        {
            switch ( m )
            {
                case FL_MOUSEWHEEL:
                {
                    int d = Fl::event_dy();

                    if ( d < 0 )
                        while ( d++ )
                            _zoom /= 2;
                    else
                        while ( d-- )
                            _zoom *= 2;

                    do_callback();

                    slider_size( w() / maximum() );

                    return 1;
                }
                default:
                    return Fl_Scrollbar::handle( m );
            }
        }
};
