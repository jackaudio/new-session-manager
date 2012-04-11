
/*******************************************************************************/
/* Copyright (C) 2012 Jonathan Moore Liles                                     */
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

#include "FL/Crystal_Theme.H"
#include "FL/Gleam_Theme.H"
#include "FL/Clean_Theme.H"

#include "FL/Fl_Theme.H"

void fl_register_themes ( void )
{
    init_clean_theme();
    init_crystal_theme();
    init_gleam_theme();

    {
        Fl_Color_Scheme *o = new Fl_Color_Scheme( "Dark",
                                                  fl_rgb_color( 50, 50, 50 ),
                                                  fl_rgb_color( 100, 100, 100 ),
                                                  fl_rgb_color( 255, 255, 255 ),
                                                  FL_YELLOW );
        Fl_Color_Scheme::add( o );
    }
    {
        Fl_Color_Scheme *o = new Fl_Color_Scheme( "Darker",
                                                  fl_rgb_color( 20, 20, 20 ),
                                                  fl_rgb_color( 100, 100, 100 ),
                                                  fl_rgb_color( 240, 240, 240 ),
                                                  FL_YELLOW );
        Fl_Color_Scheme::add( o );
    }
    {
        Fl_Color_Scheme *o = new Fl_Color_Scheme( "Gray",
                                                  fl_rgb_color( 100, 100, 100 ),
                                                  fl_rgb_color( 127, 127, 127 ),
                                                  fl_rgb_color( 255, 255, 255 ),
                                                  FL_YELLOW );
        Fl_Color_Scheme::add( o );
    }
    {
        Fl_Color_Scheme *o = new Fl_Color_Scheme( "Black",
                                                  fl_rgb_color( 0, 0, 0 ),
                                                  fl_rgb_color( 20, 20, 20 ),
                                                  fl_rgb_color( 240, 240, 240 ),
                                                  FL_YELLOW );
        Fl_Color_Scheme::add( o );
    }
    {
        Fl_Color_Scheme *o = new Fl_Color_Scheme( "Light",
                                                  fl_rgb_color( 220, 220, 220 ),
                                                  fl_rgb_color( 192, 192, 192 ),
                                                  fl_rgb_color( 0, 0, 0 ),
                                                  FL_BLUE );
        Fl_Color_Scheme::add( o );
    }
    {
        Fl::get_system_colors();

        Fl_Color_Scheme *o = new Fl_Color_Scheme( "System",
                                                  (Fl_Color)Fl::get_color( FL_BACKGROUND_COLOR ),
                                                  (Fl_Color)Fl::get_color( FL_BACKGROUND2_COLOR  ),
                                                  (Fl_Color)Fl::get_color( FL_FOREGROUND_COLOR  ),
                                                  FL_BLUE );
        Fl_Color_Scheme::add( o );
    }
}
