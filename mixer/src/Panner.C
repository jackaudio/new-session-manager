
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
#include <FL/Fl_Tiled_Image.H>

#include "debug.h"

/* 2D Panner widget. Supports various multichannel configurations. */

Panner::Point *Panner::drag;
int Panner::_range_mode = 1;
int Panner::_projection_mode = 0;

Panner::Panner ( int X, int Y, int W, int H, const char *L ) :
    Fl_Group( X, Y, W, H, L )
{
    _bg_image = 0;
    _bg_image_scaled = 0;
    _bg_image_projection = 0;
//    _projection = POLAR;
    _points.push_back( Point( 1, 0 ) );
    
    static int ranges[] = { 1,5,10,15 };
    { Fl_Choice *o = _range_choice = new Fl_Choice(X + 40,Y + H - 18,75,18,"Range:");
        o->box(FL_UP_FRAME);
        o->down_box(FL_DOWN_FRAME);
        o->textsize(9);
        o->labelsize(9);
        o->align(FL_ALIGN_LEFT);
        o->add("1 Meter",0,0,&ranges[0]);
        o->add("5 Meters",0,0,&ranges[1]);
        o->add("10 Meters",0,0,&ranges[2]);
        o->add("15 Meters",0,0,&ranges[3]);
        o->value(_range_mode);
        o->callback( cb_mode, this );
    }

    { Fl_Choice *o = _projection_choice = new Fl_Choice(X + W - 75,Y + H - 18,75,18,"Projection:");
        o->box(FL_UP_FRAME);
        o->down_box(FL_DOWN_FRAME);
        o->textsize(9);
        o->labelsize(9);
        o->align(FL_ALIGN_LEFT);
        o->add("Spherical");
        o->add("Planar");
        o->value(_projection_mode);
        o->callback( cb_mode, this );
    }
                  
    end();
}

Panner::~Panner (  )
{
    if ( _bg_image )
    {
        if ( _bg_image_scaled )
            delete _bg_image;
        else
            ((Fl_Shared_Image*)_bg_image)->release();
    }
}

static int find_numeric_menu_item( const Fl_Menu_Item *menu, int n )
{
    for ( unsigned int i = 0; menu[i].text; i++ )
    {
	if ( atoi( menu[i].text ) == n )
            return i;
    }

    return -1;
}

void
Panner::cb_mode ( Fl_Widget *w, void *v )
{
    ((Panner*)v)->cb_mode( w );
}

void
Panner::cb_mode ( Fl_Widget *w )
{
    if ( w == _range_choice )
        _range_mode = _range_choice->value();
    else if ( w == _projection_choice )
        _projection_mode = _projection_choice->value();
}

void
Panner::range ( float v )
{
    int i = find_numeric_menu_item( _range_choice->menu(), v );
    
    _range_choice->value( i );

    _range_mode = i;
}

/** set X, Y, W, and H to the bounding box of point /p/ in screen coords */
void
Panner::point_bbox ( const Point *p, int *X, int *Y, int *W, int *H ) const
{
    int tx, ty, tw, th;

    bbox( tx, ty, tw, th );

    float px, py;
    float s = 1.0f;
       
    if ( projection() == POLAR )
    {
        project_polar( p, &px, &py, &s );
    }
    else
    {
        project_ortho( p, &px, &py, &s );
    }

    const float htw = float(tw)*0.5f;
    const float hth = float(th)*0.5f;


    *W = *H = tw * s;
     
    if ( *W < 8 )
        *W = 8;
    if ( *H < 8 )
        *H = 8;

    *X = tx + (htw * px + htw) - *W/2;
    *Y = ty + (hth * py + hth) - *H/2;

}

Panner::Point *
Panner::event_point ( void )
{
    for ( int i = _points.size(); i--; )
    {
       
        int px, py, pw, ph;

        Point *p = &_points[ i ];

        if ( ! p->visible )
            continue;

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

    Fl_Image *i = 0;

    if ( _bg_image && ( _bg_image->h() != th || projection() != _bg_image_projection ) )
    {
        if ( _bg_image_scaled )
            delete _bg_image;
        else
            ((Fl_Shared_Image*)_bg_image)->release();

        _bg_image = 0;
    }

    if ( ! _bg_image )
    {
        if ( projection() == POLAR )
        {
            if ( th <= 92 )
                i = Fl_Shared_Image::get( PIXMAP_PATH "/non-mixer/panner-sphere-92x92.png" );
            else if ( th <= 502 )
                i = Fl_Shared_Image::get( PIXMAP_PATH "/non-mixer/panner-sphere-502x502.png" );
            else if ( th <= 802 )
                i = Fl_Shared_Image::get( PIXMAP_PATH "/non-mixer/panner-sphere-802x802.png" );
        }
        else
        {
            if ( th <= 92 )
                i = Fl_Shared_Image::get( PIXMAP_PATH "/non-mixer/panner-plane-92x92.png" );
            else if ( th <= 502 )
                i = Fl_Shared_Image::get( PIXMAP_PATH "/non-mixer/panner-plane-502x502.png" );
            else if ( th <= 802 )
                i = Fl_Shared_Image::get( PIXMAP_PATH "/non-mixer/panner-plane-802x802.png" );
        }

        if ( i && i->h() != th )
        {
            Fl_Image *scaled = i->copy( th, th );

            _bg_image = scaled;
            _bg_image_scaled = true;
        }
        else
        {
            _bg_image = i;
            _bg_image_scaled = false;
        }
    }
    
    _bg_image_projection = projection();

    if ( _bg_image )
        _bg_image->draw( tx, ty );
}

/** translate angle /a/ into x/y coords and place the result in /X/ and /Y/ */
void
Panner::project_polar ( const Point *p, float *X, float *Y, float *S ) const
{
    float xp = 0.0f;
    float yp = 0.0f;
    float zp = 8.0f;

    float x = 0 - p->y;
    float y = 0 - p->x;
    float z = 0 - p->z;
    
    x /= range();
    y /= range();
    z /= range();
                
    *X = ((x-xp) / (z + zp)) * (zp);
    *Y = ((y-yp) / (z + zp)) * (zp);
    *S = (0.025f / (z + zp)) * (zp);
}

/** translate angle /a/ into x/y coords and place the result in /X/ and /Y/ */
void
Panner::project_ortho ( const Point *p, float *X, float *Y, float *S ) const
{
    const float x = ( 0 - p->y ) / range();
    const float y = ( 0 - p->x ) / range();
//    const float z = p->z;
    
//    float zp = 4.0f;
    
    *X = x;
    *Y = y;
                
    *S = 0.025f;
}


void
Panner::set_ortho ( Point *p, float x, float y )
{
    y = 0 - y;

    y *= 2;
    x *= 2;
    
    p->x = (y) * range();
    p->y = (0 - x) * range();
}

void
Panner::set_polar ( Point *p, float x, float y )
{
    /* FIXME: not quite the inverse of the projection... */
    x = 0 - x;
    y = 0 - y;
    x *= 2;
    y *= 2;

    float r = powf( x,2 ) + powf(y,2 );
    float X = ( 2 * x ) / ( 1 + r );
    float Y = ( 2 * y ) / ( 1 + r );
    float Z = ( -1 + r ) / ( 1 + r );

    float S = p->radius() / range();

    X *= S;
    Y *= S;
    Z *= S;

    p->azimuth( -atan2f( X,Y ) * ( 180 / M_PI ) );

    if ( p->elevation() > 0.0f )
        p->elevation( -atan2f( Z, sqrtf( powf(X,2) + powf( Y, 2 ))) * ( 180 / M_PI ) );
    else
        p->elevation( atan2f( Z, sqrtf( powf(X,2) + powf( Y, 2 ))) * ( 180 / M_PI ) );
}



void
Panner::set_polar_radius ( Point *p, float x, float y )
{
    y = 0 - y;
    
    float r = sqrtf( powf( y, 2 ) + powf( x, 2 ) );

    if ( r > 1.0f )
        r = 1.0f;
    
    r *= range() * 2;

    if ( r > range() )
        r = range();

    p->radius( r );
}

void
Panner::draw ( void )
{
    int tw, th, tx, ty;

    bbox( tx, ty, tw, th );

    fl_push_clip( x(),y(),w(),h() );

    draw_the_box( tx, ty, tw, th );

//    draw_box();
    draw_label();

    /* if ( _bypassed ) */
    /* { */
    /*     draw_box(); */
    /*     fl_color( 0 ); */
    /*     fl_font( FL_HELVETICA, 12 ); */
    /*     fl_draw( "(bypass)", x(), y(), w(), h(), FL_ALIGN_CENTER ); */
    /*     goto done; */
    /* } */
   
    /* tx += b; */
    /* ty += b; */
    /* tw -= b * 2; */
    /* th -= b * 2; */

    fl_line_style( FL_SOLID, 1 );

    fl_color( FL_WHITE );

    for ( unsigned int i = 0; i < _points.size(); i++ )
    {
        Point *p = &_points[i];

        if ( ! p->visible )
            continue;

        Fl_Color c = fl_color_add_alpha( p->color, 100 );

        fl_color(c);

        int px, py, pw, ph;
        point_bbox( p, &px, &py, &pw, &ph );
      
        {
            float po = 5;

            fl_push_clip( px - ( po * 12 ), 
                          py - ( po * 12 ),
                          pw + ( po * 24 ), ph + (po * 24 ));

            fl_pie( px + 5, py + 5, pw - 10, ph - 10, 0, 360 );


            fl_pie( px, py, pw, ph, 0, 360 );

            fl_pop_clip();

            if ( projection() == POLAR )
            {
           
                fl_color( fl_color_average( fl_rgb_color( 127,127,127 ), p->color, 0.50 ) );
                fl_begin_loop();
                fl_circle( tx + tw/2, ty + th/2, tw/2.0f * ( ( p->radius() / range() )));
                fl_end_loop();
            }

        }
    
        const char *s = p->label;

        fl_color( fl_color_add_alpha( fl_rgb_color( 220,255,255 ), 127 ) );
        fl_font( FL_HELVETICA_BOLD_ITALIC, 10 );
        fl_draw( s, px + 20, py + 1, 50, ph - 1, FL_ALIGN_LEFT );

        if ( tw > 100 )
        {
            char pat[50];
            snprintf( pat, sizeof(pat), "%.1f°:%.1f° %.1fm", p->azimuth(), p->elevation(), p->radius() );
            
//        fl_color( fl_color_add_alpha( fl_rgb_color( 220,255,255 ), 127 ) );
            fl_font( FL_COURIER, 9 );

            fl_draw( pat, px + 20, py + 15, 50, ph - 1, FL_ALIGN_LEFT | FL_ALIGN_WRAP );

            /* fl_font( FL_HELVETICA_ITALIC, 9 ); */
            /* snprintf(pat, sizeof(pat), "range: %.1f meters", range() ); */
            /* fl_draw( pat, tx, ty, tw, th, FL_ALIGN_LEFT | FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE ); */
                
            /* if ( _projection == POLAR ) */
            /* { */
            /*     fl_draw( "Polar perspective; azimuth, elevation and radius input. Right click controls radius.", tx, ty, tw, th, FL_ALIGN_BOTTOM | FL_ALIGN_RIGHT | FL_ALIGN_INSIDE ); */
            /* } */
            /* else */
            /* { */
            /*     fl_draw( "Polar orthographic; angle and distance input.", tx, ty, tw, th, FL_ALIGN_BOTTOM | FL_ALIGN_RIGHT | FL_ALIGN_INSIDE ); */
            /* } */
        }

    }
    
    if ( tw > 200 )
        draw_children();

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
    int r = Fl_Group::handle( m );

    switch ( m )
    {
        case FL_ENTER:
        case FL_LEAVE:
            _projection_choice->value(_projection_mode);
            _range_choice->value(_range_mode);
            redraw();
            return 1;
        case FL_PUSH:
        {
            if ( Fl::event_button1() || Fl::event_button3() )
                drag = event_point();

            if ( Fl::event_button2() )
            {
                /* if ( _projection == POLAR ) */
                /*     _projection = ORTHO; */
                /* else */
                /*     _projection = POLAR; */
            }
            return 1;
        }
        case FL_RELEASE:
            if ( drag )
            {
                do_callback();
                drag = NULL;
                redraw();
                return 1;
            }
            else
                return 0;
        case FL_MOUSEWHEEL:
        {
/*             Point *p = event_point(); */

/*             if ( p ) */
/*                 drag = p; */

/*             if ( drag ) */
/*             { */
/* //                drag->elevation( drag->elevation() + Fl::event_dy()); */
/*                 drag->elevation( 0 - drag->elevation() ); */
/*                 do_callback(); */
/*                 redraw(); */
/*                 return 1; */
/*             } */

            return 1;
        }
        case FL_DRAG:
        {
            if ( ! drag )
                return 0;

            int tx, ty, tw, th;
            bbox( tx, ty, tw, th );

            float X = (float(Fl::event_x() - tx) / tw ) - 0.5f;
            float Y = (float(Fl::event_y() - ty) / th) - 0.5f;
            
            if ( Fl::event_button1() )
            {
                if ( POLAR == projection() )
                    set_polar( drag,X,Y );
                else
                {
                    if ( fabsf( X ) < 0.5f &&
                         fabsf( Y ) < 0.5f ) 
                        set_ortho( drag, X,Y );
                }
            }
            else
                set_polar_radius( drag,X,Y );

            if ( when() & FL_WHEN_CHANGED )
                do_callback();

            damage(FL_DAMAGE_EXPOSE);

            return 1;
        }

    }

    return r;

//    return 0;
}
