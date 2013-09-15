
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
#include <FL/Fl_Pack.H>

class Fl_Flowpack : public Fl_Group
{
    int _hspacing;
    int _vspacing;
    bool _flow;
    bool _flowdown;
    int _initial_height;
    int _initial_width;

public:

    Fl_Flowpack ( int X, int Y, int W, int H, const char *L = 0 )
        : Fl_Group( X, Y, W, H, L )
        {
            resizable( 0 );
            _hspacing = _vspacing = 0;
            _initial_width = W;
            _initial_height = H;
            _flow = true;
            _flowdown = false;
        }

    virtual ~Fl_Flowpack ( )
        {
        }


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
            _initial_width = W;
            _initial_height = H;

            layout();

            Fl_Group::resize( X, Y, w(), h() );
        }

    void
    draw ( void )
        {
            layout();
            Fl_Group::draw();
        }

    void dolayout ( void )
        {
            layout();
        }

    void
    layout ( void  )
        {
            resizable( 0 );

            int W;
            int H;
            int X = 0;
            int Y = 0;
            int LW = 0;
            int LH = 0;
            int LX = 0;
            int LY = 0;
            int RH = 0;
//            int RY = 0;
            int CW = 0;

            if ( _flow )
            {
                H = 0;
                W = 0;
            }
            else
            {
                H = _initial_height;
                W = _initial_width;
            }

            for ( int i = 0; i < children(); ++i )
            {
                Fl_Widget *o = child( i );

                if ( ! o->visible() )
                    continue;
                
                if ( _flow )
                {
                    if ( _flowdown )
                    {
                        if ( Y + o->h() <= _initial_height )
                        {
                            /* if it'll fit in this column, put it below the previous widget */
                            X = LX;
                        }
                        else
                        {
                            Y = H;
                            CW = 0;
                        }

                        CW = o->w() > CW ? o->w() : CW;
                        RH = Y + o->h() > RH ? Y + o->h() : RH;
                    }
                    else
                    {
                        if ( X + o->w() >= _initial_width )
                        {
                            /* maybe wrap to the next row */
                            H += RH + _vspacing;
//                        RY = Y;
                            RH = 0;
                            if ( X > W )
                                W = X;

                            X = 0;
                        }
                        else
                        {
                            /* otherwise, put it in the next column */
                            Y = H;

                        }                   

                        RH = o->h() > RH ? o->h() : RH;

                    }
                }

                LW = o->w();
                LH = o->h();

                LX = X;
                LY = Y;

                if ( _flow )
                {
                    if ( _flowdown )
                    {
                        Y += LH + _vspacing;
                        X += CW + _hspacing;
                    }
                    else
                    {
                        Y += RH + _vspacing;
                        X += LW + _hspacing;
                    }
                }
                else
                {
                    if ( type() == Fl_Pack::HORIZONTAL )
                    {
                        X += LW + _hspacing;
                        LH = _initial_height;
                        W = X;
                    }
                    else
                    {
                        Y += LH + _vspacing;
                        LW = _initial_width;
                        H = Y;
                    }
                }


                if ( ! ( o->x() == x() + LX &&
                         o->y() == y() + LY && 
                         o->w() == LW &&
                         o->h() == LH ) )
                    o->resize( x() + LX, 
                               y() + LY,
                               LW,
                               LH);
            }

            if ( _flow )
            {
                H += RH;
                if ( X > W )
                    W = X;

                /* if ( _flowdown ) */
                /*     H = _initial_height; */
            }
            
            Fl_Group::resize( x(), y(), W, H );
        }
};
