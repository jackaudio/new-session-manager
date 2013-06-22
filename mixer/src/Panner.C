
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
#include <FL/x.H>
#include <FL/Fl.H>
#include <stdio.h>
#include <math.h>
// #include <FL/fl_draw.H>

#include <FL/Fl_Shared_Image.H>

/* 2D Panner widget. Supports various multichannel configurations. */

Panner::Point *Panner::drag;

/** set X, Y, W, and H to the bounding box of point /p/ in screen coords */
void
Panner::point_bbox ( const Point *p, int *X, int *Y, int *W, int *H ) const
{
    int tx, ty, tw, th;

    bbox( tx, ty, tw, th );

    const float PW = pw(); 
    const float PH = ph();

    tw -= PW;
    th -= PH;

    float px, py;

    p->axes( &px, &py );

    *X = tx + ((tw / 2) * px + (tw / 2));
    *Y = ty + ((th / 2) * py + (th / 2));

    *W = PW;
    *H = PH;
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
Panner::draw_the_box ( int tx, int ty, int tw, int th )
{
    draw_box();

     if ( tw == 92 )
     {
         Fl_Image *i = Fl_Shared_Image::get( PIXMAP_PATH "/non-mixer/panner-92x92.png" );
        
         i->draw( tx, ty );
     }
   else if ( tw > 400 )
   {
         Fl_Image *i = Fl_Shared_Image::get( PIXMAP_PATH "/non-mixer/panner-512x512.png" );
        
         i->draw( tx, ty );
   }    
}

void
Panner::draw ( void )
{
    int tw, th, tx, ty;

    bbox( tx, ty, tw, th );

    fl_push_clip( x(),y(),w(),h() );

    draw_the_box( tx, ty, tw, th );

    const int b = 10;

//    draw_box();
    draw_label();

    if ( _bypassed )
    {
        draw_box();
        fl_color( 0 );
        fl_font( FL_HELVETICA, 12 );
        fl_draw( "(bypass)", x(), y(), w(), h(), FL_ALIGN_CENTER );
        goto done;
    }
   

    /* tx += b; */
    /* ty += b; */
    /* tw -= b * 2; */
    /* th -= b * 2; */


    fl_line_style( FL_SOLID, 1 );

    fl_color( FL_WHITE );

    {
        Point *p = &_points[0];

//        Fl_Color c = (Fl_Color)(10 + i);

        Fl_Color c = fl_color_add_alpha( fl_rgb_color( 192, 192, 206 ), 127 );
    
        int px, py, pw, ph;
        point_bbox( p, &px, &py, &pw, &ph );

        {

            const float S = ( 0.5 + ( 1.0f - p->d ) );

            float po = 5 * S;

            fl_push_clip( px - ( po * 12 ), 
                          py - ( po * 12 ),
                          pw + ( po * 24 ), ph + (po * 24 ));

            fl_color( fl_color_add_alpha( fl_rgb_color( 254,254,254 ), 254  ) );

            fl_pie( px + 5, py + 5, pw - 10, ph - 10, 0, 360 );

            fl_color(c);

            fl_pie( px, py, pw, ph, 0, 360 );

            if ( Fl::belowmouse() == this )
            {
                /* draw echo */
                fl_color( c = fl_darker( c ) );

                fl_arc( px - po, py - po, pw + ( po * 2 ), ph + ( po * 2 ), 0, 360 );
                
                fl_color( c = fl_darker( c ) );
            
                fl_arc( px - ( po * 2 ), py - ( po * 2 ), pw + ( po * 4 ), ph + ( po * 4 ), 0, 360 );
            }

            fl_pop_clip();
        }
    
        const char *s = p->label;

        fl_color( fl_rgb_color( 125,125,130 ) );
        fl_font( FL_HELVETICA, ph + 2 );
        fl_draw( s, px + 20, py + 1, 50, ph - 1, FL_ALIGN_LEFT );
    }

done:

    fl_line_style( FL_SOLID, 0 );

    fl_pop_clip();
}

/* return the current gain setting for the path in/out  */
Panner::Point *
Panner::point( int i )
{
    return &_points[ i ];
}

int
Panner::handle ( int m )
{
    int r = Fl_Widget::handle( m );

    switch ( m )
    {
        case FL_ENTER:
        case FL_LEAVE:
            redraw();
            return 1;
        case FL_PUSH:
        {
            if ( Fl::event_button2() )
            {
                _bypassed = ! _bypassed;
                redraw();
                return 1;
            }

            if ( Fl::event_button1() )
                drag = event_point();

            return 1;
        }
        case FL_RELEASE:
            if ( Fl::event_button1() && drag )
            {
                drag = NULL;
                do_callback();
                redraw();
                return 1;
            }
            else
                return 0;
        case FL_MOUSEWHEEL:
        {
            /* TODO: place point on opposite face of sphere */
        }
        case FL_DRAG:
        {
            if ( ! drag )
                return 0;

            /* else if ( Fl::event_button1() && ( drag = event_point() ) ) */
            /*     return 1; */
            /* else */


            int tx, ty, tw, th;
            bbox( tx, ty, tw, th );

            float X = Fl::event_x() - tx;
            float Y = Fl::event_y() - ty;

/*             if ( _outs < 3 ) */
/*                 drag->angle( (float)(X / (tw / 2)) - 1.0f, 0.0f ); */
/*             else */
            drag->angle( (float)(X / (tw / 2)) - 1.0f, (float)(Y / (th / 2)) - 1.0f );

            if ( when() & FL_WHEN_CHANGED )
                do_callback();

            damage(FL_DAMAGE_EXPOSE);

            return 1;
        }

    }

    return r;

//    return 0;
}
