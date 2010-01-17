
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

#include <FL/Fl.H>
#include <FL/Fl_Menu_.H>
#include <FL/Fl_Menu_Item.H>

/** popup menu and execute callback */
bool
menu_popup ( Fl_Menu_ *m )
{
    const Fl_Menu_Item *r = m->menu()->popup( Fl::event_x(), Fl::event_y(), m->label() );

    if ( r )
    {
        m->value( r );
        if ( r->callback() )
            r->do_callback( static_cast<Fl_Widget*>(m) );
        return true;
    }

    return false;
}

/** set a single callback for all items in menu.  */
void
menu_set_callback( Fl_Menu_Item *menu, void (*callback)( Fl_Widget *, void * ), void *user_data )
{
    for ( int i = menu->size(); i--; )
        if ( menu[i].label() && ! menu[i].submenu() )
        {
            menu[i].callback( callback );
            menu[i].user_data( user_data );
        }
}
