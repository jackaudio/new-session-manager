
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

#define MAX_PATH 1024

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
Fl_Menu_Settings::item_pathname_x ( char *path, int n, const Fl_Menu_Item *item )
{
    Fl_Menu_::item_pathname( path, n, item );

    remove_ampersands( path, n );
}

/** dump options from submenu /menu/ of menubar /bar/ to file /fp/ */
const Fl_Menu_Item *
Fl_Menu_Settings::dump ( Fl_Menu_ *bar, const Fl_Menu_Item *menu, FILE *fp, int depth )
{
    static char path[256];
    const Fl_Menu_Item *m = menu;

    for ( ; m->text; ++m )
    {
        bool is_radio = false;

        if ( m->flags & FL_SUBMENU )
//      if ( m->submenu() )
        {
            strcpy( path, m->text );
            remove_ampersands( path, strlen( path ) );

            indent( fp, depth );
            fprintf( fp, "%s\n", path );

            /* recurse */
            m = dump( bar, ++m, fp, depth + 1 );

//            ++m;

//            m = dump( bar, m->flags & FL_SUBMENU_POINTER ? (Fl_Menu_Item*) m->user_data() : m, fp, depth + 1 );

            if ( ! depth )
                break;
            else
                continue;
        }

        if ( m->radio() )
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
                *rindex( path, '/' ) = '\0';

                indent( fp, depth );

                fprintf( fp, "%s\n", path + strlen( path ) + 1  );
            }
        }

    }

    return m;
}

/** dump menu to file /name/ starting at /item. */
int
Fl_Menu_Settings::dump ( const Fl_Menu_Item *item, const char *name )
{
    FILE *fp = fopen( name, "w" );

    if ( ! fp )
        return false;

    dump( this, item, fp, 0 );

    fclose( fp );


    return true;
}

#define strlcat strncat

/* taken from Fl_Menu_.cxx and modified to ignore hotkeys and case */
const Fl_Menu_Item *
Fl_Menu_Settings::find_item_x ( const char *name, const Fl_Menu_Item *item )
{
    char menupath [ MAX_PATH ] = "";			// File/Export

    const Fl_Menu_Item *m = item ? item : menu();

    int depth = 0;

    while ( depth >= 0 )
        for ( ;m ; ++m )
        {
            if ( m->flags & FL_SUBMENU )
            {
                // IT'S A SUBMENU
                // we do not support searches through FL_SUBMENU_POINTER links
                if ( menupath[0] )
                    strlcat( menupath, "/", sizeof( menupath ) );

                strlcat( menupath, m->label(), sizeof( menupath ) );

                remove_ampersands( menupath, strlen( menupath ) );

                if ( ! strcasecmp( menupath, name ) )
                    return m;
                else
                {
                    ++depth;
                    continue;
                }
            }
            else
            {
                if ( ! m->label() )
                {
                    // END OF SUBMENU? Pop back one level.
                    char *ss = strrchr( menupath, '/' );
                    if ( ss )
                        *ss = 0;
                    else
                        menupath[0] = '\0';

                    --depth;
                    ++m;

                    break;
                }

                // IT'S A MENU ITEM
                char itempath[ MAX_PATH ];                // eg. Edit/Copy
                strcpy( itempath, menupath );

                if ( itempath[0] )
                    strlcat( itempath, "/", sizeof( itempath ) );

                strlcat( itempath, m->label(), sizeof( itempath ) );

                remove_ampersands( itempath, strlen( itempath ) );

                if ( !strcasecmp( itempath, name ) )
                    return m;
            }
        }

    return ( Fl_Menu_Item * )0;
}


static void
path_push ( char *path, const char *s )
{
    strcat( path, s );
    strcat( path, "/" );
}

static void
path_pop ( char *path )
{
    char *s;

    int l = strlen( path );

    if ( path[ l - 1 ] == '/' )
        path[ l - 1 ] = '\0';

    s = rindex( path, '/' );

    s = s ? s : path;

    *(s + 1) = '\0';
}

void
Fl_Menu_Settings::load ( Fl_Menu_ *bar, const Fl_Menu_Item *item, FILE *fp, int depth, char *path, int pmax )
{
    char line[256];

    while ( ! feof( fp ) )
    {
        *line = '\0';

        fgets( line, sizeof( line ), fp );

        if ( *line == '#' )
            continue;

        line[ strlen( line ) - 1 ] = '\0';

        int ld = strspn( line, "\t" );

        if ( ld > depth )
        {
            path_push( path, line + ld );

            ++depth;

//            load( bar, item, fp, depth + 1, path, pmax );
            /*  */;
        }
        else if ( ld < depth )
        {
            /* we should know the path and the value now */

            // path_pop( path );
            *rindex( path, '/' ) = '\0';

//            printf( "%s = %s\n", path, path + strlen( path ) + 1 );

            const Fl_Menu_Item *it = find_item_x( path, item + 1 );

            if ( it && it->radio() )                            /* radio button */
            {
                bar->picked( it );

                path_pop( path );
            }
            else                                                /* toggle */
            {
                *rindex( path, '/' ) = '\0';

                if ( ( it = find_item_x( path, item + 1 ) ) && it->checkbox() )
                {
                    int v = 0 == strcasecmp( "true", (path + strlen( path ) + 1 ) );

                    if ( v != ( it->value() != 0 ) /* grr, FLTK */ )
                        bar->picked( it );
                }

            }

            while ( ld < depth )
            {
                path_pop( path );
                depth--;
            }

            path_push( path, line + ld );
        }
        else                                          /* d == depth */
        {
            /* doesn't apply? */
        }
    }
}

/** load settings from file /name/ into menu starting at /item */
int
Fl_Menu_Settings::load ( const Fl_Menu_Item *item, const char *name )
{
    FILE *fp = fopen( name, "r" );

    if ( ! fp )
        return false;

    char path[ MAX_PATH ];
    path[0] = '\0';

    load( this, item, fp, 0, path, sizeof( path ) );

    fclose( fp );
}
