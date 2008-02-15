
/*******************************************************************************/
/* Copyright (C) 2007-2008 Jonathan Moore Liles                                */
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

/* This file contains ALL platform specific drawing code required by the canvas */

#include "ui.H"
#include "draw.H"

#include "../common.h"
#include <stdlib.h>
#include <math.h>

#include "../canvas.H"

struct color_table {
    int state;
    unsigned char r, g, b;
};

struct color_table color_defs[] = {
 { EMPTY,     38,  38,  38  },
 { FULL,      255, 69,  0   },
 { PARTIAL,   0,   0,   0   },
 { CONTINUED, 80,  80,  80  },
 { LINE,      26,  26,  26  },
 { HIT,       255, 255, 255 },
 { PLAYHEAD,  10,  69,  10  },
 { SELECTED,  255, 10,  255 },
 };

Fl_Color *state_colors;

Fl_Color velocity_colors[128];

void
init_colors ( void )
{
    unsigned int i;
    /* velocity colors */

    for ( i = 128; i--; )
        velocity_colors[i] = fl_rgb_color( i * 2, 255 - i * 2, 32 );

    state_colors = (Fl_Color*)malloc(sizeof( Fl_Color ) * MAX_STATE );

    for ( i = elementsof( color_defs ); i--; )
    {
        state_colors[ color_defs[i].state ] = fl_rgb_color( color_defs[i].r,
                                                            color_defs[i].g,
                                                            color_defs[i].b );
    }
}

int
gui_draw_ruler ( int x, int y, int w, int div_w, int div, int ofs, int p1, int p2 )
{
    /* Across the top */

    fl_font( FL_TIMES, ruler_height );

    int h = ruler_height;

    fl_color( canvas_background_color );

    //  fl_rectf( x, y, x + (div_w * w), y + h );
    fl_rectf( x, y, (div_w * w), h );

    fl_color( FL_RED );

    fl_line( x + div_w / 2, y, x + div_w * w, y );

    char pat[40];
    int z = div;
    int i;
    for ( i = 0; i < w; i++ )
        if ( 0 == i % z )
        {
            int nx = x + (i * div_w) + (div_w / 2);

            fl_color( FL_RED );

            fl_line( nx, y, nx, y + h );

            int k = ofs + i;
            sprintf( pat, "%i", 1 + (k / z ));

            fl_color( FL_WHITE );
            fl_draw( pat, nx + div_w / 2, y + h + 1 / 2 );
        }

    if ( p1 != p2 )
    {
        if ( p1 >= 0 )
        {
            if ( p1 < p2 )
                fl_color( FL_GREEN );
            else
                fl_color( FL_RED );

            fl_rectf( x + (div_w * p1), y + h / 2, div_w, h / 2 );

        }
        if ( p2 >= 0 )
        {
            if ( p2 < p1 )
                fl_color( FL_GREEN );
            else
                fl_color( FL_RED );
            fl_rectf( x + (div_w * p2), y + h / 2, div_w, h / 2 );

        }
    }

    return h;
}

void
gui_clear_area ( int x, int y, int w, int h )
{
    fl_color( canvas_background_color );

    fl_rectf( x, y, w, h );
}

int
gui_draw_string ( int x, int y, int w, int h, int color, const char *s, bool draw )
{
    int rw;

    if ( ! s )
        return 0;

    fl_font( FL_COURIER, min( h, 18 ) );

    rw = fl_width( s );

    if ( fl_not_clipped( x, y, rw, h ) && draw )
    {
        gui_clear_area( x, y, w, h );

        if ( color )
            fl_color( velocity_colors[ color ] );
        else
            fl_color( FL_DARK_CYAN );

        fl_draw( s, x, y + h / 2 + fl_descent() );
    }

    return rw;
}

void
gui_draw_shape ( int x, int y, int w, int h, int bw, int shape, int state, int flags, int color  )
{
    /* take advantage of FLTK's clipping */
    if ( ! fl_not_clipped( x, y, w, h ) )
        return;

    if ( flags & F_PLAYHEAD )
    {
        state = state == FULL ? HIT : PLAYHEAD;
        flags &= ~ F_SELECTION;
    }

    if ( state == FULL && color  )
        fl_color( velocity_colors[ color ] );
    else
        fl_color( state_colors[ state ] );

    if ( flags & F_SELECTION )
        fl_color( fl_darker( fl_color() ) );

    switch ( shape )
    {
        case CIRCLE:
            fl_pie( x + bw / 2, y + bw / 2, w - bw, h - bw, 0, 360 );
            break;
        case SQUARE:
            fl_rectf( x + bw, y + bw, w - bw * 2, h - bw * 2 );
            break;
        case HALF_CIRCLE:
            fl_pie( x + bw / 2, y + bw / 2, w - bw, h - bw, 0, 360 / 2);
            break;
        case DIAMOND:
            fl_polygon( x + w / 2, y + bw / 2, x + w - bw / 2, y + h / 2, x + w / 2, y + h - bw / 2, x + bw / 2, y + h / 2 );
            break;
        default:
            ASSERTION( "unknown shape" );
            break;
    }

    if ( flags & F_P1 || flags & F_P2 )
    {
        if ( flags & F_P1 )
            fl_color( FL_GREEN );
        else
            fl_color( FL_RED );

        int rw = w / 4;
        int rh = h / 4;

        fl_rectf( x + (w / 2) - (rw / 2), y + (h / 2) - (rh / 2), rw, rh );
    }
}

extern UI *ui;

static
void
clear_status ( void *arg )
{
    ui->status->value( "" );
}

/** inform the user of something via a status bar */
void
gui_status ( const char *fmt, ... )
{
    va_list args;

    static char pat[256];

	if ( fmt )
	{
		va_start( args, fmt );
		vsnprintf( pat, 256, fmt, args );
		va_end( args );
	}

    ui->status->value( pat );

    Fl::add_timeout( 5.0f, clear_status );
}
