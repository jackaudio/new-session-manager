
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

#include <math.h>

/* 2D Panner widget. Supports various multichannel configurations. */


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


/* speaker symbol */
#define BP fl_begin_polygon()
#define EP fl_end_polygon()
#define BCP fl_begin_complex_polygon()
#define ECP fl_end_complex_polygon()
#define BL fl_begin_line()
#define EL fl_end_line()
#define BC fl_begin_loop()
#define EC fl_end_loop()
#define vv(x,y) fl_vertex(x,y)

static void draw_speaker ( Fl_Color col )
{
    fl_color(col);

    BP; vv(0.2,0.4); vv(0.6,0.4); vv(0.6,-0.4); vv(0.2,-0.4); EP;
    BP; vv(-0.6,0.8); vv(0.2,0.0); vv(-0.6,-0.8); EP;

    fl_color( fl_darker( col ) );

    BC; vv(0.2,0.4); vv(0.6,0.4); vv(0.6,-0.4); vv(0.2,-0.4); EC;
    BC; vv(-0.6,0.8); vv(0.2,0.0); vv(-0.6,-0.8); EC;
}


/** set X, Y, W, and H to the bounding box of point /p/ in screen coords */
void
Panner::point_bbox ( const Point *p, int *X, int *Y, int *W, int *H ) const
{
    int tx, ty, tw, th;

    bbox( tx, ty, tw, th );

    tw -= pw();
    th -= ph();

    float px, py;

    p->axes( &px, &py );

    *X = tx + ((tw / 2) * px + (tw / 2));
    *Y = ty + ((th / 2) * py + (th / 2));

    *W = pw();
    *H = ph();
}

Panner::Point *
Panner::event_point ( void )
{
    for ( int i = _ins; i--; )
    {
        int px, py, pw, ph;

        Point *p = &_points[ i ];

        point_bbox( p, &px, &py, &pw, &ph );

//        printf( "%d, %d -- %d %d %d %d\n", Fl::event_x(), Fl::event_y(),  px, py, pw, ph );

        if ( Fl::event_inside( px, py, pw, ph ) )
            return p;
    }

    return NULL;
}

void
Panner::draw ( void )
{
    draw_box();
//    draw_box( FL_FLAT_BOX, x(), y(), w(), h(), FL_BLACK );
    draw_label();


    if ( _bypassed )
    {
        fl_color( 0 );
        fl_font( FL_HELVETICA, 12 );
        fl_draw( "(bypass)", x(), y(), w(), h(), FL_ALIGN_CENTER );
        return;
    }

    int tw, th, tx, ty;

    bbox( tx, ty, tw, th );

    fl_push_clip( tx, ty, tw, th );

    fl_color( FL_WHITE );

    const int b = 10;

    tx += b;
    ty += b;
    tw -= b * 2;
    th -= b * 2;

    fl_arc( tx, ty, tw, th, 0, 360 );

    if ( _configs[ _outs ][0] >= 0 )
    {
        for ( int i = _outs; i--; )
        {
            int a = _configs[ _outs ][ i ];

            Point p( 1.2f, (float)a );

            float px, py;

            p.axes( &px, &py );

            fl_push_matrix();

            const int bx = tx + ((tw / 2) * px + (tw / 2));
            const int by = ty + ((th / 2) * py + (th / 2));

            fl_translate( bx, by );

            fl_scale( 5, 5 );

            a = 90 - a;

            fl_rotate( a );

            draw_speaker( FL_WHITE );

            fl_rotate( -a );

            fl_pop_matrix();

        }
    }

    /* ensure that points are drawn *inside* the circle */

    for ( int i = _ins; i--; )
    {
        Point *p = &_points[ i ];

        Fl_Color c = (Fl_Color)(10 + i);

        int px, py, pw, ph;
        point_bbox( p, &px, &py, &pw, &ph );

        /* draw point */
        fl_color( c );
        fl_pie( px, py, pw, ph, 0, 360 );

        /* draw echo */
        fl_color( c = fl_darker( c ) );
        fl_arc( px - 5, py - 5, pw + 10, ph + 10, 0, 360 );
        fl_color( c = fl_darker( c ) );
        fl_arc( px - 10, py - 10, pw + 20, ph + 20, 0, 360 );
        fl_color( c = fl_darker( c ) );
        fl_arc( px - 30, py - 30, pw + 60, ph + 60, 0, 360 );

        /* draw number */
        char pat[4];
        snprintf( pat, 4, "%d", i + 1 );

        fl_color( FL_BLACK );
        fl_font( FL_HELVETICA, ph + 2 );
        fl_draw( pat, px + 1, py + 1, pw - 1, ph - 1, FL_ALIGN_CENTER );

        /* draw line */

/*         fl_color( FL_WHITE ); */
/*         fl_line( bx + pw() / 2, by + ph() / 2, tx + (tw / 2), ty + (th / 2) ); */

    }

    fl_pop_clip();
}

/* return the current gain setting for the path in/out  */
Panner::Point
Panner::point( int i )
{
    return _points[ i ];
}

int
Panner::handle ( int m )
{
    static Point *drag;

    switch ( m )
    {

        case FL_PUSH:

            if ( Fl::event_button2() )
            {
                _bypassed = ! _bypassed;
                redraw();
                return 0;
            }
            else if ( Fl::event_button1() && ( drag = event_point() ) )
                return 1;
            else
                return 0;
        case FL_RELEASE:
            drag = NULL;
            return 1;
        case FL_DRAG:
        {
            float X = Fl::event_x() - x();
            float Y = Fl::event_y() - y();

            int tx, ty, tw, th;
            bbox( tx, ty, tw, th );

/*             if ( _outs < 3 ) */
/*                 drag->angle( (float)(X / (tw / 2)) - 1.0f, 0.0f ); */
/*             else */
                drag->angle( (float)(X / (tw / 2)) - 1.0f, (float)(Y / (th / 2)) - 1.0f );


            printf( "%f %f\n", drag->a, drag->d );

            redraw();

            return 1;
        }

    }

    return 0;
}
