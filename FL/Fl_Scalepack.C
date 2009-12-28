
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


/* Fl_Scalepack

   This is similar to an Fl_Pack, but instead of the pack resizing
   itself to enclose its children, this pack resizes its children to
   fit itself. Of course, this only works well with highly flexible
   widgets, but the task comes up often enough to warrent this class.

   If any child happens to be the resizable() widget, it is given half
   of the available area, with the other widgets packed around it in
   the remaining space.

*/

#include "Fl_Scalepack.H"

#include <FL/Fl.H>

#include <stdio.h>

Fl_Scalepack::Fl_Scalepack ( int X, int Y, int W, int H, const char *L ) :
    Fl_Group( X, Y, W, H, L )
{
    resizable( 0 );
    _spacing = 0;
}

void
Fl_Scalepack::draw ( void )
{
    int tx = x() + Fl::box_dx( box() );
    int ty = y() + Fl::box_dy( box() );
    int tw = w() - Fl::box_dw( box() );
    int th = h() - Fl::box_dh( box() );

    if ( damage() & FL_DAMAGE_ALL )
    {
        draw_box();
        draw_label();
    }

    int v = 0;

    for ( int i = children(); i--; )
        if ( child( i )->visible() )
            ++v;

    if ( 0 == v )
        return;

    int sz, pos;

    if ( type() == HORIZONTAL )
    {
        sz = tw / v;
        pos = tw - ( tw % sz );
    }
    else
    {
        sz = th / v;
        pos = th - ( th % sz );
    }

    for ( int i = children(); i--; )
        if ( child( i )->visible() )
        {
            pos -= sz;

            if ( type() == HORIZONTAL )
                child( i )->resize( tx + pos, ty, sz, th );
            else
                child( i )->resize( tx, ty + pos, tw, sz );

            if ( damage() & FL_DAMAGE_CHILD )
                update_child( *child( i ) );
            else
                draw_child( *child( i ) );
        }
}
