
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

/* This is the main mixer group. It contains and manages Mixer_Strips. */

#include "Mixer.H"
#include "Mixer_Strip.H"

#include <FL/Fl_Pack.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/fl_ask.H>
#include <FL/Fl_File_Chooser.H>
#include "New_Project_Dialog.H"
#include "Engine/Engine.H"
#include "FL/Fl_Flowpack.H"
#include "Project.H"

#include <string.h>
#include "debug.h"

const double STATUS_UPDATE_FREQ = 0.2f;


#include "util/debug.h"

static void update_cb( void *v ) {
    Fl::repeat_timeout( STATUS_UPDATE_FREQ, update_cb, v );

    ((Mixer*)v)->update();
}


void Mixer::cb_menu(Fl_Widget* o) {
    Fl_Menu_Bar *menu = (Fl_Menu_Bar*)o;

/*     const Fl_Menu_Item *mi = &menu->menu()[menu->value()]; */

     char picked[256];
     // const char *picked = menu->text();

    menu->item_pathname( picked, sizeof( picked ) );

    if (! strcmp( picked, "&Project/&New") )
    {
        DMESSAGE( "New project" );
        const char *templates[] = { "Default", NULL };

        char *default_path;
        char *selected_template;

//        read_line( user_config_dir, "default_path", &default_path );

        char *path = new_project_chooser( templates, &default_path, &selected_template );

        if ( ! Project::create( path, selected_template ) )
            fl_alert( "Error creating project!" );

        free( path );
        free( selected_template );
        free( default_path );

//        write_line( user_config_dir, "default_path", default_path );

    }
    else if (! strcmp( picked, "&Project/&Open" ) )
    {
        char *path = NULL;

//        read_line( user_config_dir, "default_path", &path );

        const char *name = fl_dir_chooser( "Open Project", path, NULL );

        free( path );

        mixer->hide();

        if ( int err = Project::open( name ) )
        {
            fl_alert( "Error opening project: %s", Project::errstr( err ) );
            mixer->show();
        }

        mixer->show();
    }
    else if (! strcmp( picked, "&Project/&Save" ) )
    {
        Project::save();
    }
    else if (! strcmp( picked, "&Project/&Quit") )
    {
        quit();
    }
    else if ( !strcmp( picked, "&Mixer/&Add Strip" ) )
    {
        new_strip();
    }
    else if ( !strcmp( picked, "&Mixer/Add &N Strips" ) )
    {
        const char *s = fl_input( "Enter number of strips to add" );

        if ( s )
        {
            for ( int i = atoi( s ); i > 0; i-- )
                new_strip();
        }
    }
    else if (! strcmp( picked, "&Mixer/&Rows/One") )
    {
        rows( 1 );
    }
    else if (! strcmp( picked, "&Mixer/&Rows/Two") )
    {
        rows( 2 );
    }
    else if (! strcmp( picked, "&Mixer/&Rows/Three") )
    {
        rows( 3 );
    }

}

void Mixer::cb_menu(Fl_Widget* o, void* v) {
    ((Mixer*)(v))->cb_menu(o);
}

Mixer::Mixer ( int X, int Y, int W, int H, const char *L ) :
    Fl_Group( X, Y, W, H, L )
{
    _rows = 1;
    box( FL_NO_BOX );
    labelsize( 96 );
    { Fl_Menu_Bar *o = new Fl_Menu_Bar( X, Y, W, 24 );
        o->add( "&Project/&New" );
        o->add( "&Project/&Open" );
        o->add( "&Project/&Save", FL_CTRL + 's', 0, 0 );
        o->add( "&Project/&Quit", FL_CTRL + 'q', 0, 0 );
        o->add( "&Mixer/&Add Strip", 'a', 0, 0 );
        o->add( "&Mixer/Add &N Strips" );
        o->add( "&Mixer/&Rows/One", '1', 0, 0 );
        o->add( "&Mixer/&Rows/Two", '2', 0, 0 );
        o->add( "&Mixer/&Rows/Three", '3', 0, 0 );
        o->add( "_&Options" );
        o->add( "&Help/&Manual" );
        o->add( "&Help/&About" );
        o->callback( cb_menu, this );
    }
    { Fl_Scroll *o = scroll = new Fl_Scroll( X, Y + 24, W, H - 24 );
        o->box( FL_NO_BOX );
//        o->type( Fl_Scroll::HORIZONTAL_ALWAYS );
//        o->box( Fl_Scroll::BOTH );
        {
            Fl_Flowpack *o = mixer_strips = new Fl_Flowpack( X, Y + 24, W, H - 18 - 24 );
            label( "Non-Mixer" );
            align( (Fl_Align)(FL_ALIGN_CENTER | FL_ALIGN_INSIDE) );
            o->box( FL_NO_BOX );
            o->type( Fl_Pack::HORIZONTAL );
            o->hspacing( 2 );
            o->vspacing( 2 );
            o->end();
            Fl_Group::current()->resizable( o );
        }
        o->end();
        Fl_Group::current()->resizable( o );
    }

    end();

//    Fl::add_timeout( STATUS_UPDATE_FREQ, update_cb, this );

    MESSAGE( "Scanning for plugins..." );

}

Mixer::~Mixer ( )
{
    DMESSAGE( "Destroying mixer" );

    /* FIXME: teardown */
    mixer_strips->clear();
}

void Mixer::resize ( int X, int Y, int W, int H )
{
    Fl_Group::resize( X, Y, W, H );

    mixer_strips->resize( X, Y + 24, W, H - 18 - 24 );

    scroll->resize( X, Y + 24, W, H - 24 );

    rows( _rows );
}

void Mixer::add ( Mixer_Strip *ms )
{
    MESSAGE( "Add mixer strip \"%s\"", ms->name() );

    mixer_strips->add( ms );

    rows( _rows );

    scroll->redraw();
}

void
Mixer::quit ( void )
{
    /* TODO: save project? */

    while ( Fl::first_window() ) Fl::first_window()->hide();
}


void
Mixer::insert ( Mixer_Strip *ms, Mixer_Strip *before )
{
    mixer_strips->remove( ms );
    mixer_strips->insert( *ms, before );

    scroll->redraw();
}
void
Mixer::insert ( Mixer_Strip *ms, int i )
{
    Mixer_Strip *before = (Mixer_Strip*)mixer_strips->child( i );

    insert( ms, before);
}

void
Mixer::move_left ( Mixer_Strip *ms )
{
    int i = mixer_strips->find( ms );

    if ( i > 0 )
        insert( ms, i - 1 );
}

void
Mixer::move_right ( Mixer_Strip *ms )
{
    int i = mixer_strips->find( ms );

    if ( i < mixer_strips->children() - 1 )
        insert( ms, i + 2 );
}

void Mixer::remove ( Mixer_Strip *ms )
{
    MESSAGE( "Remove mixer strip \"%s\"", ms->name() );

    mixer_strips->remove( ms );

    parent()->redraw();
}

bool
Mixer::contains ( Mixer_Strip *ms )
{
    return ms->parent() == mixer_strips;
}

void
Mixer::rows ( int n )
{
    int sh;

    if ( n > 1 )
        sh = (scroll->h() / n) - (mixer_strips->vspacing() * (n - 1));
    else
        sh = (scroll->h() - 18) / n;

    if ( sh < Mixer_Strip::min_h() )
        return;

    int tw = 0;

    for ( int i = 0; i < mixer_strips->children(); ++i )
    {
        Mixer_Strip *t = (Mixer_Strip*)mixer_strips->child( i );

        t->size( t->w(), sh );

        tw += t->w() + mixer_strips->hspacing();
    }

    if ( n > 1 )
        mixer_strips->size( scroll->w() - 18, mixer_strips->h() );
    else
        mixer_strips->size( tw, mixer_strips->h() );

    _rows = n;

    scroll->redraw();
}

void Mixer::update ( void )
{
    THREAD_ASSERT( UI );

    for ( int i = mixer_strips->children(); i--; )
    {
        ((Mixer_Strip*)mixer_strips->child( i ))->update();
    }
    // redraw();
}

/** retrun a pointer to the track named /name/, or NULL if no track is named /name/ */
Mixer_Strip *
Mixer::track_by_name ( const char *name )
{
    for ( int i = mixer_strips->children(); i-- ; )
    {
        Mixer_Strip *t = (Mixer_Strip*)mixer_strips->child( i );

        if ( ! strcmp( name, t->name() ) )
            return t;
    }

    return NULL;
}

/** return a malloc'd string representing a unique name for a new track */
char *
Mixer::get_unique_track_name ( const char *name )
{
    char pat[256];

    strcpy( pat, name );

    for ( int i = 1; track_by_name( pat ); ++i )
        snprintf( pat, sizeof( pat ), "%s.%d", name, i );

    return strdup( pat );
}


void
Mixer::snapshot ( void )
{
    for ( int i = 0; i < mixer_strips->children(); ++i )
        ((Mixer_Strip*)mixer_strips->child( i ))->log_children();
}


void
Mixer::new_strip ( void )
{
    add( new Mixer_Strip( get_unique_track_name( "Unnamed" ), 1 ) );
}

bool
Mixer::save ( void )
{
    MESSAGE( "Saving state" );
    Loggable::snapshot_callback( &Mixer::snapshot, this );
    Loggable::snapshot( "snapshot" );
    return true;
}


int
Mixer::handle ( int m )
{
    int r = Fl_Group::handle( m );

    switch ( m )
    {
        case FL_ENTER:
        case FL_LEAVE:
            return 1;
        default:
            return r;
            break;
    }

    // return 0;
    return r;
}
