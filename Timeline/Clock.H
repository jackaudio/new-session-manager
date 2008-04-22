
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


/* Digital clock widget to show points on the timeline. May be
switched between Bar Beat Tick and Wallclock displays  */

#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>

#include "Timeline.H"
#include "types.h"


class Clock : public Fl_Widget
{
    enum { BBT, HMS };

    nframes_t _when;

public:

    static void
    frame_to_HMS ( char *dst, int n, nframes_t frame )
        {
            float S = (double)frame / timeline->sample_rate();

            int M = S / 60; S -= M * 60;
            int H = M / 60; M -= H * 60;

            snprintf( dst, n, "%d:%d:%.1f", H, M, S );
        }

    Clock ( int X, int Y, int W, int H, const char *L=0 )
        : Fl_Widget( X, Y, W, H, L )
        {
            _when = 0;
            box( FL_BORDER_BOX );
            type( HMS );

            /* force size */
            size( 170, 40 );
        }

    void set ( nframes_t frame )
        {
            if ( _when != frame )
            {
                _when = frame;
                redraw();
            }
        }

    void draw ( void )
        {
//            fl_rectf( x(), y(), w(), h(), color() );

            draw_box();

            char buf[15];

            switch ( type() )
            {
                case HMS:
                    frame_to_HMS( buf, sizeof( buf ), _when );
                    break;
                case BBT:
//                    frame_to_BBT( _buf, sizeof( buf ), frame );
                    break;
                default:
                    printf( "error: invalid clock type\n" );
            }

            fl_font( FL_COURIER, 24 );

            Fl_Color c = FL_GREEN;

            fl_color( c );

            const int dx = x() + Fl::box_dx( box() );
            const int dy = y() + Fl::box_dy( box() );
            const int dw = w() - Fl::box_dw( box() );
            const int dh = h() - Fl::box_dh( box() );

            fl_draw( buf, dx, dy, dw, dh, (Fl_Align)(FL_ALIGN_CENTER | FL_ALIGN_TOP) );

            for ( int i = strlen( buf ); i--; )
                if ( isdigit( buf[ i ] ) )
                    buf[ i ] = ' ';

            fl_color( fl_darker( c ) );
            fl_draw( buf, dx, dy, dw, dh, (Fl_Align)(FL_ALIGN_CENTER | FL_ALIGN_TOP) );

            fl_font( FL_HELVETICA, 9 );
            fl_color( FL_RED );

            const char *s = type() == HMS ? "HMS" : "BBT";

            fl_draw( s, dx + 4, dy, dw, dh, (Fl_Align)( FL_ALIGN_LEFT | FL_ALIGN_BOTTOM ) );

            if ( label() )
                fl_draw( label(), dx, dy, dw, dh, (Fl_Align)( FL_ALIGN_RIGHT | FL_ALIGN_BOTTOM ) );

        }
};
