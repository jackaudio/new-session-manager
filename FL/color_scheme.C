
/*******************************************************************************/
/* Copyright (C) 2010 Jonathan Moore Liles                                     */
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

#include "color_scheme.H"
#include <FL/Fl.H>
#include <string.h>

static Fl_Color system_colors[3];

void
get_system_colors ( void )
{
    Fl::get_system_colors();

    system_colors[ 0 ] = (Fl_Color)Fl::get_color( FL_BACKGROUND_COLOR );
    system_colors[ 1 ] = (Fl_Color)Fl::get_color( FL_FOREGROUND_COLOR  );
    system_colors[ 2 ] = (Fl_Color)Fl::get_color( FL_BACKGROUND2_COLOR  );
}

void
color_scheme ( const char *name )
{
    if ( !strcasecmp( name, "dark" ) )
    {
        Fl::background2( 100, 100, 100 );
        Fl::background( 50, 50, 50 );
        Fl::foreground( 255, 255, 255 );
    }
    else if ( !strcasecmp( name, "light" ))
    {
        Fl::background2( 192, 192, 192 );
        Fl::background( 220, 220, 220 );
        Fl::foreground( 0, 0, 0 );
    }
    if ( !strcasecmp( name, "very dark" ) )
    {
        Fl::background2( 100, 100, 100 );
        Fl::background( 20, 20, 20 );
        Fl::foreground( 240, 240, 240 );
    }
    else if ( !strcasecmp( name, "system" ) )
    {
        unsigned char r, g, b;

        Fl::get_color( system_colors[ 0 ], r, g, b );

        Fl::background( r, g, b );

        Fl::get_color( system_colors[ 1 ], r, g, b );

        Fl::foreground( r, g, b );

        Fl::get_color( system_colors[ 2 ], r, g, b );

        Fl::background2( r, g, b );
    }

    Fl::scheme( Fl::scheme() );
}
