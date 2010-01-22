
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

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Pack.H>
#include "Mixer_Strip.H"

class Fl_Flowpack;

class Mixer : public Fl_Group
{

private:

    int _rows;

    Mixer_Strip* track_by_name ( const char *name );
    char * get_unique_track_name ( const char *name );

    void snapshot ( void );
    static void snapshot ( void *v ) { ((Mixer*)v)->snapshot(); }

    void cb_menu(Fl_Widget*);
    static void cb_menu(Fl_Widget*, void*);

    Fl_Scroll *scroll;
    Fl_Pack *pack;

    Fl_Flowpack *mixer_strips;

protected:

    virtual int handle ( int m );

public:

    void rows ( int n );
    virtual void resize ( int X, int Y, int W, int H );

    void update ( void );

    void new_strip ( void );
    void process ( unsigned int nframes );
    void add ( Mixer_Strip *ms );
    void remove ( Mixer_Strip *ms );
    void move_left ( Mixer_Strip *ms );
    void move_right ( Mixer_Strip *ms );
    void insert ( Mixer_Strip *ms, Mixer_Strip *before );
    void insert ( Mixer_Strip *ms, int i );
    bool contains ( Mixer_Strip *ms );

    bool save ( void );
    void quit ( void );

    Mixer ( int X, int Y, int W, int H, const char *L );
    virtual ~Mixer();
};

extern Mixer* mixer;
