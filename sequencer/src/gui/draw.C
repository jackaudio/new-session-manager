
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
    { EMPTY,     18,  18,  18  },
    { FULL,      255, 69,  0   },
    { PARTIAL,   0,   0,   0   },
    { CONTINUED, 80,  80,  80  },
    { LINE,      10,  10,  10  },
    { HIT,       255, 255, 255 },
    { PLAYHEAD,  10,  69,  10  },
    { SELECTED,  255, 10,  255 },
};

Fl_Color *state_colors;

Fl_Color velocity_colors[128];
Fl_Color velocity2_colors[128];

bool draw_borders = 1;

void
init_colors ( void )
{
    unsigned int i;
    /* velocity colors */

    for ( i = 128; i--; )
    {
//        velocity_colors[i] = fl_color_average( FL_GRAY, fl_rgb_color( i * 2, 255 - i * 2, 32 ), 0.4 );
//        velocity_colors[i] = fl_rgb_color( i * 2, 0, 0 );
        velocity_colors[i] = fl_rgb_color( i, 0, 0 );
        velocity2_colors[i] = fl_color_average( FL_WHITE, velocity_colors[i], 0.5 );
    }

    state_colors = (Fl_Color*)malloc(sizeof( Fl_Color ) * MAX_STATE );

    for ( i = elementsof( color_defs ); i--; )
    {
        state_colors[ color_defs[i].state ] = fl_rgb_color( color_defs[i].r,
                                                            color_defs[i].g,
                                                            color_defs[i].b );
    }
}

extern UI *ui;

static
void
clear_status ( void * )
{
    ui->status->label( NULL );
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

    ui->status->label( pat );

    Fl::add_timeout( 5.0f, clear_status );
}
