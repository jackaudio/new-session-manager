
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

#pragma once

#include <FL/Fl.H>

/* class Fl_Theme  */
/* { */

/* public: */

/*     virtual const char *name ( void ) const = 0; */
/*     virtual const char *author ( void ) const = 0; */
/*     virtual const char *description ( void ) const = 0; */

/*     virtual void up_box ( int X, int Y, int W, int H, Fl_Color c ) = 0; */
/*     virtual void down_box ( int X, int Y, int W, int H, Fl_Color c ) = 0; */
/*     virtual void thin_up_box ( int X, int Y, int W, int H, Fl_Color c ) = 0; */
/*     virtual void thin_down_box ( int X, int Y, int W, int H, Fl_Color c ) = 0; */
/*     virtual void round_up_box ( int X, int Y, int W, int H, Fl_Color c ) = 0; */
/*     virtual void round_down_box ( int X, int Y, int W, int H, Fl_Color c ) = 0; */
/*     virtual void rounded_box ( int X, int Y, int W, int H, Fl_Color c ) = 0; */
/*     virtual void oval_box ( int X, int Y, int W, int H, Fl_Color c ) = 0; */
/*     virtual void shadow_box ( int X, int Y, int W, int H, Fl_Color c ) = 0; */
/*     virtual void rshadow_box ( int X, int Y, int W, int H, Fl_Color c ) = 0; */
/*     virtual void diamond_box ( int X, int Y, int W, int H, Fl_Color c ) = 0; */
/*     virtual void check_on ( int X, int Y, int W, int H, Fl_Color c ) = 0; */
/*     virtual void check_off ( int X, int Y, int W, int H, Fl_Color c ) = 0; */
/*     virtual void radio_on ( int X, int Y, int W, int H, Fl_Color c ) = 0; */
/*     virtual void radio_off ( int X, int Y, int W, int H, Fl_Color c ) = 0; */

/*     virtual void up_frame ( int X, int Y, int W, int H, Fl_Color c ) = 0; */
/*     virtual void down_frame ( int X, int Y, int W, int H, Fl_Color c ) = 0; */

/* }; */

class Fl_Color_Scheme
{
    Fl_Color_Scheme *next;
    static int total;
    static Fl_Color_Scheme *first;

    Fl_Color _bg;
    Fl_Color _bg2;
    Fl_Color _fg;
    Fl_Color _sel;

    const char *_name;

public:

    const char *name ( void ) const { return _name; }

    Fl_Color_Scheme ( const char *name, Fl_Color background, Fl_Color background2, Fl_Color foreground, Fl_Color selection )
        {
            _bg = background;
            _bg2 = background2;
            _fg = foreground;
            _sel = selection;
            _name = name;

        }

    static void add ( Fl_Color_Scheme *td );
    static Fl_Color_Scheme **get ( void );
    static int set ( const char *name );
    static void save ( void );
};


class Fl_Theme
{
    Fl_Theme *next;
    static int total;
    static Fl_Theme *first;
    static Fl_Theme *_current;
   
    const char *_name;
    const char *_description;
    const char *_author;

    void (*_init_func)(void);

public:

    const char *name ( void ) const { return _name; }
    const char *description ( void ) { return _description; }
    const char *author ( void ) { return _author; }

    Fl_Theme( const char *name, const char *description, const char *author, void (*init_func)(void) )
        {
            _name = name;
            _description = description;
            _author = author;
            _init_func = init_func;
        }

    static void add ( Fl_Theme *td );
    static Fl_Theme **get ( void );
    static int set ( void );
    static int set ( const char *name );
    static const Fl_Theme *current ( void ) { return _current; }
};
