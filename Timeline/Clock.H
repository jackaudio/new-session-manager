
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
#include <FL/Fl.H>

#include "Timeline.H"
#include "types.h"

const float CLOCK_UPDATE_FREQ = 0.06f;

/* TODO: frames per second? */


class Clock : public Fl_Widget
{
    nframes_t _when;
    nframes_t *_v;

    static void
    update_cb ( void *v )
        {
            ((Clock*)v)->update_cb();
        }

    void
    update_cb ( void )
        {
            Fl::repeat_timeout( CLOCK_UPDATE_FREQ, update_cb, this );

            set( *_v );
        }


public:

    enum { HMS = 0, BBT, Timecode, Sample, TYPE_MAX };

    static void
    frame_to_Timecode ( char *dst, int n, nframes_t frame )
        {
            float S = (double)frame / timeline->sample_rate();

            int M = S / 60; S -= M * 60;
            int H = M / 60; M -= H * 60;
            int HS = ((int)(S * 100)) - (((int)S) * 100);

            snprintf( dst, n, "%02d:%02d:%02.0f:%02d", H, M, S, HS );
        }

    static void
    frame_to_HMS ( char *dst, int n, nframes_t frame )
        {
            float S = (double)frame / timeline->sample_rate();

            int M = S / 60; S -= M * 60;
            int H = M / 60; M -= H * 60;

            snprintf( dst, n, "%02d:%02d:%05.3f", H, M, S );
        }

    static void
    frame_to_Sample ( char *dst, int n, nframes_t frame )
        {
            snprintf( dst, n, "%lu", (unsigned long)frame );
        }

    static void
    frame_to_BBT ( char *dst, int n, nframes_t frame )
        {
            snprintf( dst, n, "unimplemented" );
        }



    Clock ( int X, int Y, int W, int H, const char *L=0 )
        : Fl_Widget( X, Y, W, H, L )
        {
            _when = 0;
            _v = 0;
            box( FL_BORDER_BOX );
            type( HMS );

            /* force size */
            size( 170, 40 );
        }

    ~Clock ( )
        {
            Fl::remove_timeout( update_cb );
        }

    void run ( nframes_t *v )
        {
            _v = v;

            Fl::add_timeout( CLOCK_UPDATE_FREQ, update_cb, this );
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
            draw_box();

            fl_push_clip( x(), y(), w(), h() );

            char buf[15];
            *buf = '\0';

            switch ( type() )
            {
                case HMS:
                    frame_to_HMS( buf, sizeof( buf ), _when );
                    break;
                case BBT:
                    frame_to_BBT( buf, sizeof( buf ), _when );
                    break;
                case Timecode:
                    frame_to_Timecode( buf, sizeof( buf ), _when );
                    break;
                case Sample:
                    frame_to_Sample( buf, sizeof( buf ), _when );
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

            fl_draw( buf, dx, dy, dw, dh - 9, FL_ALIGN_CENTER );

            for ( int i = strlen( buf ); i--; )
                if ( isdigit( buf[ i ] ) )
                    buf[ i ] = ' ';

            fl_color( fl_darker( c ) );
            fl_draw( buf, dx, dy, dw, dh - 9, FL_ALIGN_CENTER );

            fl_font( FL_HELVETICA, 9 );

            const char *types[] = { "HMS", "BBT", "Timecode", "Sample" };

            fl_color( FL_CYAN );

            switch ( type() )
            {
                case Timecode:
                    snprintf( buf, sizeof( buf ), "%.1f", 30.0 );
                    fl_draw( buf, dx, dy, dw, dh, FL_ALIGN_BOTTOM );
                    break;
                case Sample:
                    snprintf( buf, sizeof( buf ), "%lu", (unsigned long)timeline->sample_rate() );
                    fl_draw( buf, dx, dy, dw, dh, FL_ALIGN_BOTTOM );
                    break;
                default:
                    break;
            }

            const char *s = types[ type() ];

            fl_color( FL_RED );

            fl_draw( s, dx + 4, dy, dw, dh, (Fl_Align)( FL_ALIGN_LEFT | FL_ALIGN_BOTTOM ) );

            if ( label() )
                fl_draw( label(), dx, dy, dw, dh, (Fl_Align)( FL_ALIGN_RIGHT | FL_ALIGN_BOTTOM ) );

            fl_pop_clip();
        }


    int handle ( int m )
        {
            if ( m == FL_PUSH )
            {
                int t = type() + 1;

                if ( t >= TYPE_MAX )
                    t = 0;

                type( t );

                redraw();

                return 0;
            }

            return 0;
        }
};
