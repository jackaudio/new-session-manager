
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

   If and child happens to be the resizable() widget, then it will be
   resized so the all the other children can fit around it, with their
   current sizes (and the size of the Fl_Scalepack) maintained.

   NOTES: An Fl_Pack as a direct child will not work, because Fl_Pack
   changes its size in draw(), which throws off our resize
   calculation. The whole idea of widgets being able to resize
   themselves within draw() is horribly broken...
*/

#include "Fl_Scalepack.H"

#include <FL/Fl.H>
#include <FL/fl_draw.H>
#include <stdio.h>

Fl_Scalepack::Fl_Scalepack ( int X, int Y, int W, int H, const char *L ) :
    Fl_Group( X, Y, W, H, L )
{
    resizable( 0 );
    _spacing = 0;
}

void
Fl_Scalepack::resize ( int X, int Y, int W, int H )
{
    /* Fl_Group's resize will change our child widget sizes, which
     interferes with our own resizing method. */
    long dx = X - x();
    long dy = Y - y();
    
    bool r = W != w() || H != h();

    Fl_Widget::resize( X, Y, W, H );

    Fl_Widget*const* a = array();

    for (int i=children(); i--;)
    {
        Fl_Widget* o = *a++;

        o->position( o->x() + dx, o->y() + dy );
    }

    if ( r )
        redraw();
}

void
Fl_Scalepack::draw ( void )
{

    if ( resizable() == this )
        /* this resizable( this ) is the default for Fl_Group and is
         * reset by Fl_Group::clear(), but it is not our default... */
        resizable( 0 );

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

    int cth = 0;
    int ctw = 0;

    Fl_Widget * const * a = array();

    for ( int i = children(); i--; )
    {
        Fl_Widget *o = *a++;

        if ( o->visible() )
        {
            ++v;

            if ( o != this->resizable() )
            {
                cth += o->h();
                ctw += o->w();
            }

            cth += _spacing;
            ctw += _spacing;
        }
    }

    ctw -= _spacing;
    cth -= _spacing;

    if ( 0 == v )
        return;

    if ( this->resizable() )
    {
        int pos = 0;

        Fl_Widget * const * a = array();

        for ( int i = children(); i--; )
        {
            Fl_Widget *o = *a++;

            if ( o->visible() )
            {
                int X, Y, W, H;

                if ( type() == HORIZONTAL )
                {
                    X = tx + pos;
                    Y = ty;
                    W = o->w();
                    H = th;
                }
                else
                {
                    X = tx;
                    Y = ty + pos;
                    W = tw;
                    H = o->h();
                }

                if ( this->resizable() == o )
                {
                    if ( type() == HORIZONTAL )
                        W = tw - ctw - 3;
                    else
                        H = th - cth;
                }

                if (X != o->x() || Y != o->y() || W != o->w() || H != o->h() )
                {
                    o->resize(X,Y,W,H);
                    o->clear_damage(FL_DAMAGE_ALL);
                }

                if ( damage() & FL_DAMAGE_ALL )
                {
                    draw_child( *o );
                    draw_outside_label( *o );
                }
                else
                    update_child( *o );

/*                 if ( this->resizable() == o ) */
/*                     fl_rect( o->x(), o->y(), o->w(), o->h(), type() == VERTICAL ? FL_GREEN : FL_BLUE ); */

                if ( type() == HORIZONTAL )
                    pos += o->w() + spacing();
                else
                    pos += o->h() + spacing();

            }
        }
    }
    else
    {
        int sz = 0;
        int pos = 0;

        if ( type() == HORIZONTAL )
            sz = (tw - (_spacing * (v - 1))) / v;
        else
            sz = (th - (_spacing * (v - 1))) / v;

        Fl_Widget * const * a = array();

        for ( int i = children(); i--; )
        {
            Fl_Widget *o = *a++;

            if ( o->visible() )
            {
                int X, Y, W, H;

                if ( type() == HORIZONTAL )
                {
                    X = tx + pos;
                    Y = ty;
                    W = sz;
                    H = th;
                }
                else
                {
                    X = tx;
                    Y = ty + pos;
                    W = tw;
                    H = sz;
                }

                if (X != o->x() || Y != o->y() || W != o->w() || H != o->h() )
                {
                    o->resize(X,Y,W,H);
                    o->clear_damage(FL_DAMAGE_ALL);
                }

                if ( damage() & FL_DAMAGE_ALL )
                {
                    draw_child( *o );
                    draw_outside_label( *o );
                }
                else
                    update_child( *o );

//                    fl_rect( o->x(), o->y(), o->w(), o->h(), type() == VERTICAL ? FL_RED : FL_YELLOW );

                if ( type() == HORIZONTAL )
                    pos += o->w() + spacing();
                else
                    pos += o->h() + spacing();

            }
        }
    }
}
