
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

#include "FL/Fl_Theme.H"
#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Preferences.H>

Fl_Theme *Fl_Theme::first;
Fl_Theme *Fl_Theme::_current;
Fl_Color_Scheme *Fl_Color_Scheme::first;

int Fl_Theme::total;
int Fl_Color_Scheme::total;

void
Fl_Theme::add ( Fl_Theme *t )
{
    t->next = first;
    first = t;
    total++;
}

Fl_Theme **
Fl_Theme::get ( void )
{
    Fl_Theme **r = (Fl_Theme**) malloc( sizeof( Fl_Theme* ) * ( total + 1 ) );

    int i = 0;
    for ( Fl_Theme *t = first; t; t = t->next, i++ )
        r[i] = t;

    r[i] = 0;

    return r;
}


static 
Fl_Preferences *prefs ( void )
{
    char *path;

    asprintf( &path, "%s/.config/ntk/", getenv("HOME" )  );
    
    Fl_Preferences *p = new Fl_Preferences( path, "ntk", "theme" );

    free( path );

    return p;
}

static void conf_set ( const char *key, const char *value )
{
    Fl_Preferences *p = prefs();

    p->set( key, value );

    delete p;
}

static void conf_set ( const char *key, Fl_Color value )
{
    Fl_Preferences *p = prefs();

    p->set( key, (int)value );

    delete p;
}

static const char *conf_get ( const char *key, const char *def )
{
    static char buf[256];

    Fl_Preferences *p = prefs();

    p->get( key, buf, def, sizeof( buf ) );

    delete p;

    return buf;
}

static 
Fl_Color
conf_get_color ( const char *key, Fl_Color def )
{
    Fl_Preferences *p = prefs();

    int c;

    p->get( key, c, def );

    delete p;

    return (Fl_Color)c;
}

static bool dont_save = false;

/* sets the configured default */
int
Fl_Theme::set ( void )
{
    const char *name = conf_get( "theme", "clean" );

    int rv = set( name );

    dont_save = true;

    Fl_Color_Scheme::set( "System" );

    dont_save = false;

    uchar r, g, b;
    
    Fl::get_color( conf_get_color( "background", FL_BACKGROUND_COLOR ), r, g, b );
    Fl::background( r, g, b );
    Fl::get_color( conf_get_color( "background2", FL_BACKGROUND2_COLOR ), r, g, b );            
    Fl::background2( r, g, b );
    Fl::get_color( conf_get_color( "foreground", FL_FOREGROUND_COLOR ), r, g, b );            
    Fl::foreground( r, g, b );
    
    return rv;
}

int
Fl_Theme::set ( const char *name )
{
    for ( Fl_Theme *t = first; t; t = t->next )
        if ( !strcasecmp( t->name(), name ) )
        {
            /* reset boxtypes */
            Fl::reload_scheme();

            printf( "Theme set to %s\n", t->name() );
            t->_init_func();
            Fl_Theme::_current = t;

            conf_set( "theme", t->name() );

            for ( Fl_Window *w = Fl::first_window(); w; w = Fl::next_window( w ) )
                w->redraw();

            return 1;
        }

    return 0;
}

void
Fl_Color_Scheme::add ( Fl_Color_Scheme *t )
{
    t->next = first;
    first = t;
    total++;
}

Fl_Color_Scheme **
Fl_Color_Scheme::get ( void )
{
    Fl_Color_Scheme **r = (Fl_Color_Scheme**) malloc( sizeof( Fl_Color_Scheme* ) * ( total + 1 ) );

    int i = 0;
    for ( Fl_Color_Scheme *t = first; t; t = t->next, i++ )
        r[i] = t;

    r[i] = 0;

    return r;
}

void
Fl_Color_Scheme::save ( void )
{
    if ( ! dont_save )
    {
        conf_set( "background", Fl::get_color( FL_BACKGROUND_COLOR ) );
        conf_set( "foreground", Fl::get_color( FL_FOREGROUND_COLOR ) );
        conf_set( "background2", Fl::get_color( FL_BACKGROUND2_COLOR ) );
    }

    for ( Fl_Window *w = Fl::first_window(); w; w = Fl::next_window( w ) )
        w->redraw();
}

int
Fl_Color_Scheme::set ( const char *name )
{
    for ( Fl_Color_Scheme *t = first; t; t = t->next )
        if ( !strcasecmp( t->name(), name ) )
        {
            uchar r, g, b;

            Fl::get_color( t->_bg, r, g, b );            
            Fl::background( r, g, b );
            Fl::get_color( t->_bg2, r, g, b );            
            Fl::background2( r, g, b );
            Fl::get_color( t->_fg, r, g, b );            
            Fl::foreground( r, g, b );
            /* Fl::get_color( t->_sel, r, g, b );             */
            /* Fl::selection( r, g, b ); */

            conf_set( "color_scheme", t->name() );

            save();

            return 1;
        }

    return 0;
}





