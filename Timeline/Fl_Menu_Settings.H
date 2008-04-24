
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


#include <FL/Fl_Menu_.H>
#include <stdio.h>

class Fl_Menu_Settings : public Fl_Menu_
{

    void remove_ampersands ( char *str, int n );
    void indent ( FILE *fp, int n );
    Fl_Menu_Item * dump ( Fl_Menu_ *bar, Fl_Menu_Item *menu, FILE *fp, int depth );
    void load ( Fl_Menu_ *bar, Fl_Menu_Item *item, FILE *fp, int depth, char *path, int pmax );

public:

    int item_pathname_x ( char *path, int n, Fl_Menu_Item *item );
    int dump ( Fl_Menu_Item *item, const char *name );


    int load ( Fl_Menu_Item *item, const char *name );


};
