
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

#ifndef Track_Header_H
#define Track_Header_H
#include <FL/Fl.H>
#include "Track.H"
#include <FL/Fl_Group.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Pack.H>

#include "Loggable.H"

class Track_Header : public Fl_Group, public Loggable
{

public:

    Track_Header( int X, int Y, int W, int H, const char *L = 0 );

private:
    Track * _track;

public:

    Fl_Input * name_field;
    Fl_Button *record_button;
    Fl_Button *mute_button;
    Fl_Button *solo_button;
    Fl_Menu_Button *take_menu;

    Fl_Pack *takes;

    const char *class_name ( void ) { return "Track_Header"; }

    void set ( char **sa )
        {
            return;
        }

    char ** get ( void )
        {
            return NULL;
        }

    static int width();

    void track( Track * t );

};
#endif
