
/*******************************************************************************/
/* Copyright (C) 2009 Jonathan Moore Liles                                     */
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

#pragma once

#include <FL/Fl_Group.H>

class Fl_Flowpack : public Fl_Group
{
    int _hspacing;
    int _vspacing;
    int _max_width;
    bool _flow;
    bool _flowdown;

public:

    Fl_Flowpack ( int X, int Y, int W, int H, const char *L = 0 )
        : Fl_Group( X, Y, W, H, L )
        {
            resizable( 0 );
            _max_width = _hspacing = _vspacing = 0;
            _flow = true;
            _flowdown = false;
        }

    virtual ~Fl_Flowpack ( )
        {
        }

    int max_width ( void ) const { return _max_width; }

    void vspacing ( int v ) { _vspacing = v; }
    int vspacing ( void ) const { return _vspacing; };

    void hspacing ( int h ) { _hspacing = h; }
    int hspacing ( void ) const { return _hspacing; };

    bool flow ( void ) const { return _flow; }
    void flow ( bool v ) { _flow = v; }

    bool flowdown ( void ) const { return _flowdown; }
    void flowdown ( bool v ) { _flowdown = v; }

    void
    add ( Fl_Widget *w )
        {
            Fl_Group::add( w );
            dolayout();
        }

    void
    remove ( Fl_Widget *w )
        {
            Fl_Group::remove( w );
            dolayout();
        }

    void
    resize ( int X, int Y, int W, int H )
        {
            int NW = W;
            int NH = H;

            layout( NW, NH );

            Fl_Group::resize( X, Y, NW, NH );
        }

    void
    draw ( void )
        {
            dolayout();
            Fl_Group::draw();
        }

    void dolayout ( void )
        {
            int H = h();
            int W = w();

            layout( W, H );

            if ( H != h() || W != w() )
                size( W, H );
        }

    void
    layout ( int &W, int &H )
        {
            resizable( 0 );

            int X = 0;
            int Y = 0;
            H = 0;
            /* int H = 0; */

            _max_width = 0;

            int LW = 0;
            int LH = 0;
            int LX = 0;
            int LY = 0;

            for ( int i = 0; i < children(); ++i )
            {
                Fl_Widget *o = child( i );

                if ( ! o->visible() )
                    continue;

                H = o->h() > H ? o->h() : H;

                if ( _flow )
                {
                    if ( _flowdown && Y + o->h() < H )
                    {
                        /* if it'll fit in this column, put it below the previous widget */
                        X = LX;
                    }
                    else if ( X + o->w() >= W )
                    {
                        /* maybe wrap to the next row */
                        H += o->h();
                        
                        X = 0;
                    }
                    else
                    {
                        /* otherwise, put it in the next column */
                        Y = LY;
                    }                   

                }

                LW = o->w();
                LH = o->h();

                /* avoid bothering the control with lots of resize() calls */

                LX = X;
                LY = Y;

                if ( ! ( o->x() == x() + LX &&
                         o->y() == y() + LY ) )
                    o->position( x() + LX, y() + LY );

                if ( _flow )
                {
                    Y += LH + _vspacing;
                    X += LW + _hspacing;
                }
                else
                {
                    if ( type() == Fl_Pack::HORIZONTAL )
                    {
                        X += LW + _hspacing;
                    }
                    else
                    {
                        Y += LH + _vspacing;
                    }
                }

                if ( X > _max_width )
                    _max_width = X;
            }

            if ( ! _flow )
                W = X;
        }
};
