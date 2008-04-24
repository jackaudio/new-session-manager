
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

#include "Fl_Menu_Settings.H"

#include <string.h>

/* code to dump and restore (portions of) an Fl_Menu_ */

void
Fl_Menu_Settings::remove_ampersands ( char *str, int n )
{
    char *d = str;
    char *s = str;

    while ( n-- )
    {
        if ( *s == '&' )
        {
            ++s;
            continue;
        }

        *(d++) = *(s++);
    }

    *d = '\0';
}

void
Fl_Menu_Settings::indent ( FILE *fp, int n )
{
    while ( n-- )
        fprintf( fp, "\t" );
}

int
Fl_Menu_Settings::item_pathname_x ( char *path, int n, Fl_Menu_Item *item )
{
    Fl_Menu_::item_pathname( path, n, item );

    remove_ampersands( path, n );
}

/** dump options from submenu /menu/ of menubar /bar/ to file /fp/ */
Fl_Menu_Item *
Fl_Menu_Settings::dump ( Fl_Menu_ *bar, Fl_Menu_Item *menu, FILE *fp, int depth )
{
    static char path[256];
    Fl_Menu_Item *m = menu;

    for ( ; m->text; ++m )
    {
        bool is_radio = false;

        if ( m->flags & FL_SUBMENU )
        {
            strcpy( path, m->text );
            remove_ampersands( path, sizeof( path ) - 1 );

            indent( fp, depth );
            fprintf( fp, "%s\n", path );

            /* recurse */
            m = dump( bar, ++m, fp, depth + 1 );
            continue;
        }

        if ( m->flags & FL_MENU_RADIO )
            is_radio = true;

//        bar->item_pathname( path, sizeof( path ) - 1, m );
        item_pathname_x( path, sizeof( path ) - 1, m );


        if ( m->flags & FL_MENU_TOGGLE || m->flags & FL_MENU_RADIO )
        {
            if ( ! is_radio )
            {
                indent( fp, depth );

                fprintf( fp, "%s\n", rindex( path, '/' ) + 1 );

                indent( fp, depth + 1 );

                fprintf( fp, "%s\n", m->flags & FL_MENU_VALUE ? "true" : "false" );
            }
            else if ( m->flags & FL_MENU_VALUE )
            {
                indent( fp, depth );

                *rindex( path, '/' ) = '\0';

                fprintf( fp, "%s\n", rindex( path, '/' ) + 1 );

                indent( fp, depth + 1 );

                fprintf( fp, "%s\n", path + strlen( path ) + 1  );
            }
        }
    }

    return m;
}

int
Fl_Menu_Settings::dump ( Fl_Menu_Item *item, const char *name )
{
    FILE *fp = fopen( name, "w" );

    if ( ! fp )
        return false;

    dump( this, item, fp, 0 );

    fclose( fp );


    return true;
}
