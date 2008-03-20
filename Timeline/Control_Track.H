
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

#include "Track.H"
#include "Control_Point.H"

class Control_Track : public Track
{

public:


    Control_Track ( int X, int Y, int W, int H ) : Track( X, Y, W, H )
        {
            color( fl_darker( FL_GREEN ) );

            log_create();
        }

    ~Control_Track ( )
        {
            log_destroy();
        }

    const char *class_name ( void ) { return "Control_Track"; }

    void
    draw ( void )
        {

            if ( ! fl_not_clipped( x(), y(), w(), h() ) )
                return;

            fl_push_clip( x(), y(), w(), h() );

            draw_box();

            int X, Y, W, H;

            fl_clip_box( x(), y(), w(), h(), X, Y, W, H );

            fl_line_style( FL_SOLID, 4 );

            fl_color( fl_color_average( selection_color(), color(), 0.90f ) );

            fl_begin_complex_polygon();

            list <Track_Widget *>::const_iterator e = _widgets.end();
            e--;

            if ( _widgets.size() )
                for ( list <Track_Widget *>::const_iterator r = _widgets.begin(); ; r++ )
                {
                    if ( r == _widgets.begin() )
                    {
                        fl_vertex( x(), y() + h() );
                        fl_vertex( x(), (*r)->y() );
                    }

                    fl_vertex( (*r)->x(), (*r)->y() );

                    if ( r == e )
                    {
                        fl_vertex( x() + w(), (*r)->y() );
                        fl_vertex( x() + w(), y() + h() );
                        break;
                    }

                }

            fl_end_complex_polygon();

            fl_line_style( FL_SOLID, 0 );

            timeline->draw_measure_lines( x(), y(), w(), h(), color() );

            for ( list <Track_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); r++ )
                (*r)->draw_box( X, Y, W, H );

            fl_pop_clip();
        }

    int
    handle ( int m )
        {
            int r = Track::handle( m );

            if ( r )
                return r;

            switch ( m )
            {
                case FL_PUSH:
                {
                    if ( Fl::event_button1() )
                    {
                        Control_Point *r = new Control_Point( this, timeline->xoffset + timeline->x_to_ts( Fl::event_x() - x() ), (float)(Fl::event_y() - y()) / h() );

                        add( r );
                    }

                    return 1;
                }
                default:
                    return 0;
            }
        }
};
