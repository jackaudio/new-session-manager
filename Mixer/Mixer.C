
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
#include "New_Project_Dialog.H"
#include "Engine/Engine.H"

#include "Project.H"

#include <string.h>
#include "debug.h"

const double STATUS_UPDATE_FREQ = 0.2f;

static Fl_Pack *mixer_strips;


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
    else if (! strcmp( picked, "&Project/&Save" ) )
    {
        Project::save();
    }
    if (! strcmp( picked, "&Project/&Quit") )
    {
        quit();
    }
    else if ( !strcmp( picked, "&Mixer/&Add Strip" ) )
    {
        new_strip();
    }
}

void Mixer::cb_menu(Fl_Widget* o, void* v) {
    ((Mixer*)(v))->cb_menu(o);
}

Mixer::Mixer ( int X, int Y, int W, int H, const char *L ) :
    Fl_Group( X, Y, W, H, L )
{
    box( FL_NO_BOX );
    labelsize( 96 );
    { Fl_Menu_Bar *o = new Fl_Menu_Bar( X, Y, W, 24 );
        o->add( "&Project/&New" );
        o->add( "&Project/&Open" );
        o->add( "&Project/&Save", FL_CTRL + 's', 0, 0 );
        o->add( "&Project/&Quit", FL_CTRL + 'q', 0, 0 );
        o->add( "&Mixer/&Add Strip", 'a', 0, 0 );
        o->add( "&Options" );
        o->callback( cb_menu, this );
    }
    { Fl_Scroll *o = scroll = new Fl_Scroll( X, Y + 24, W, H - 24 );
        o->box( FL_NO_BOX );
        o->type( Fl_Scroll::HORIZONTAL_ALWAYS );
        {
            Fl_Pack *o = mixer_strips = new Fl_Pack( X, Y + 24, W, H - 18 - 24 );
            label( "Non-Mixer" );
            align( (Fl_Align)(FL_ALIGN_CENTER | FL_ALIGN_INSIDE) );
            o->box( FL_NO_BOX );
            o->type( Fl_Pack::HORIZONTAL );
            o->spacing( 2 );
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
    /* FIXME: teardown */

}

void Mixer::resize ( int X, int Y, int W, int H )
{
    Fl_Group::resize( X, Y, W, H );

    mixer_strips->resize( X, Y + 24, W, H - 18 - 24 );

    scroll->resize( X, Y + 24, W, H - 24 );
}

void Mixer::add ( Mixer_Strip *ms )
{
    MESSAGE( "Add mixer strip \"%s\"", ms->name() );

    engine->lock();

    mixer_strips->add( ms );
//    mixer_strips->insert( *ms, 0 );

    engine->unlock();

    scroll->redraw();

//    redraw();
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
    engine->lock();

    mixer_strips->remove( ms );
    mixer_strips->insert( *ms, before );

    engine->unlock();

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

    engine->lock();

    mixer_strips->remove( ms );

    engine->unlock();

    delete ms;

    parent()->redraw();
}

bool
Mixer::contains ( Mixer_Strip *ms )
{
    return ms->parent() == mixer_strips;
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

void
Mixer::process ( unsigned int nframes )
{
    THREAD_ASSERT( RT );

    for ( int i = mixer_strips->children(); i--; )
    {
        ((Mixer_Strip*)mixer_strips->child( i ))->process( nframes );
    }
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
    engine->lock();

    add( new Mixer_Strip( get_unique_track_name( "Unnamed" ), 1 ) );

    engine->unlock();

//    scroll->size( mixer_strips->w(), scroll->h() );
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
