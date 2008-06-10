
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


#pragma once

#include <jack/transport.h>
#include "types.h"

#include "Timeline.H"

#include <FL/Fl_Pack.H>
#include <FL/Fl_Button.H>

#include <stdio.h>

struct Transport : public jack_position_t, public Fl_Pack
{

private:

    static void cb_button ( Fl_Widget *w, void *arg );
    void cb_button ( Fl_Widget *w );

    /* not permitted */
    Transport ( const Transport &rhs );
    Transport & operator = ( const Transport &rhs );

    Fl_Button *_home_button;
    Fl_Button *_end_button;
    Fl_Button *_play_button;
    Fl_Button *_record_button;

public:

    Transport ( int X, int Y, int W, int H, const char *L=0 );


    void toggle_record ( void );
    int handle ( int m );

    bool rolling;
    bool recording;

    void poll ( void );
    void locate ( nframes_t frame );
    void start ( void );
    void stop ( void );
    void toggle ( void );
};

extern Transport* transport;
