
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


#include "Panner.H"

/* 2D Panner widget. Supports various multichannel configurations. */

enum {
    NONE = -1,
    R    = 90,
    L    = 270,
    C   = 0,
    FL   = 315,
    FR   = 45,
    RL   = 225,
    RR   = 135,
};

/* multichannel layouts, in degrees */
int Panner::_configs[][12] =
{
    /* none, error condition? */
    { NONE },
    /* mono, panner disabled */
    { NONE },
    /* stereo */
    { L, R },
    /* stereo + mono */
    { L, R, C },
    /* quad */
    { FL, FR, RL, RR },
    /* 5.1 */
    { FL, FR, RL, RR, C },
    /* no such config */
    { NONE },
    /* 7.1 */
    { FL, FR, RL, RR, C, L, R },
};


Panner::Point *
Panner::event_point ( void )
{

    int X, Y, W, H;

    bbox( X, Y, W, H );

    W -= pw();
    H -= ph();

    for ( int i = _ins; i--; )
    {
        Point *p = &_points[ i ];

        if ( Fl::event_inside( X + ((p->x * (W / 2)) + (W / 2)),
                               Y + ((p->y * (H / 2)) + (H / 2)), pw(), ph() ) )
            return p;
    }

    return NULL;
}

void
Panner::draw ( void )
{
    draw_box();
//            draw_box( FL_FLAT_BOX, x(), y(), w(), h(), FL_BLACK );
    draw_label();

    int tw, th, tx, ty;

    bbox( tx, ty, tw, th );

    fl_color( FL_WHITE  );
    if ( _configs[ _outs ][0] >= 0 )
    {
        for ( int i = _outs; i--; )
        {
            int a = _configs[ _outs ][ i ];

            // FIXME: this is a hack... Why does fl_arc seem to put 0 degrees at 3 o'clock and inverted?
            a -= 90;
            a = 360 - a;

            fl_arc( tx, ty, tw, th, a - 3, a + 3 );
        }
    }

    tw -= pw();
    th -= ph();

    for ( int i = _ins; i--; )
    {
        Point *p = &_points[ i ];

        fl_color( (Fl_Color) 10 + i );

        const int bx = tx + ((tw / 2) * p->x + (tw / 2));
        const int by = ty + ((th / 2) * p->y + (th / 2));

        fl_rectf( bx, by, pw(), ph() );

        char pat[4];
        snprintf( pat, 4, "%d", i + 1 );

        fl_color( FL_BLACK );
        fl_font( FL_HELVETICA, ph() + 2 );
        fl_draw( pat, bx, by + ph() );
    }
}


int
Panner::handle ( int m )
{
    static Point *drag;

    switch ( m )
    {

        case FL_PUSH:
            if ( ( drag = event_point() ) )
            {
                printf( "bing\n" );
                return 1;
            }
            else
                return 0;
        case FL_RELEASE:
            drag = NULL;
            return 1;
        case FL_DRAG:
        {
            float X = Fl::event_x() - x();
            float Y = Fl::event_y() - y();

            drag->x = (float)(X / (w() / 2)) - 1.0f;
            drag->y = (float)(Y / (h() / 2)) - 1.0f;

            redraw();

            return 1;
        }

    }

    return 0;
}
