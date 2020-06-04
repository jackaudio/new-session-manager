
/*******************************************************************************/
/* Copyright (C) 2008-2020 Jonathan Moore Liles (as "Non-Session-Manager")     */
/*                                                                             */
/* This file is part of New-Session-Manager                                    */
/*                                                                             */
/* New-Session-Manager is free software: you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by        */
/* the Free Software Foundation, either version 3 of the License, or           */
/* (at your option) any later version.                                         */
/*                                                                             */
/* New-Session-Manager is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of              */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               */
/* GNU General Public License for more details.                                */
/*                                                                             */
/* You should have received a copy of the GNU General Public License           */
/* along with New-Session-Manager. If not, see <https://www.gnu.org/licenses/>.*/
/*******************************************************************************/

#pragma once

#include <FL/Fl_Group.H>

class Fl_Scalepack : public Fl_Group
{

    int _spacing;

public:

    enum { VERTICAL, HORIZONTAL };

    Fl_Scalepack ( int X, int Y, int W, int H, const char *L = 0 );
    virtual ~Fl_Scalepack ( ) { }

    int spacing ( void ) const { return _spacing; }
    void spacing ( int v ) { _spacing = v; redraw(); }

    virtual void resize ( int, int, int, int );

    virtual void draw ( void );

};
