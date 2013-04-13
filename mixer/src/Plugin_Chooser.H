
/*******************************************************************************/
/* Copyright (C) 2013 Jonathan Moore Liles                                     */
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

#include <FL/Fl_Double_Window.H>
#include "Plugin_Module.H"
#include <vector>

class Plugin_Chooser_UI;
class Plugin_Chooser : public Fl_Double_Window
{
    Plugin_Chooser_UI *ui;
    
    std::list <Plugin_Module::Plugin_Info> _plugins;
    
    static void cb_handle ( Fl_Widget *w, void *v );
    void cb_handle ( Fl_Widget *w );
    static void cb_table ( Fl_Widget *w, void *v );
    void cb_table ( Fl_Widget *w );

    void search ( const char *name, const char *author, int ninputs, int noutputs, bool favorites );

    unsigned long _value;
    
    int load_favorites ( void );

    void save_favorites ( void );

public:

    unsigned long value ( void ) const { return _value; }
   
    Plugin_Chooser ( int X,int Y,int W,int H, const char *L=0 );

    virtual ~Plugin_Chooser( );

    static unsigned long plugin_chooser ( int ninputs );
};


