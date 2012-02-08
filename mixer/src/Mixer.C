
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
#include "const.h"

#include "Mixer.H"
#include "Mixer_Strip.H"

#include <FL/Fl_Pack.H>
#include <FL/Fl_Scroll.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/fl_ask.H>
#include <FL/Fl.H>
#include <FL/Fl_File_Chooser.H>
#include "New_Project_Dialog.H"
#include "Engine/Engine.H"
#include "FL/Fl_Flowpack.H"
#include "Project.H"
#include "FL/Fl_Menu_Settings.H"
#include "About_Dialog.H"

#include "file.h"

#include <string.h>
#include "debug.h"

#include "FL/color_scheme.H"
#include "OSC/Endpoint.H"
#include <lo/lo.h>

const double STATUS_UPDATE_FREQ = 0.2f;

const double OSC_INTERVAL = 0.1f;

extern char *user_config_dir;

#include "debug.h"

/* static void update_cb( void *v ) { */
/*     Fl::repeat_timeout( STATUS_UPDATE_FREQ, update_cb, v ); */

/*     ((Mixer*)v)->update(); */
/* } */



/************************/
/* OSC Message Handlers */
/************************/

OSC_HANDLER( generic )
{
    OSC_DMSG();

    return 0;
}

OSC_HANDLER( quit )
{
    OSC_DMSG();

    ((Mixer*)user_data)->command_quit();

    OSC_REPLY_OK();

    return 0;
}

OSC_HANDLER( save )
{
    OSC_DMSG();

    if ( ((Mixer*)user_data)->command_save() )
        OSC_REPLY_OK();
    else
        OSC_REPLY_ERR();

    return 0;
}

OSC_HANDLER( load )
{
   OSC_DMSG();

   const char *project_path = &argv[0]->s;
   const char *project_display_name = &argv[1]->s;

   if ( ((Mixer*)user_data)->command_load( project_path, project_display_name ) )
       OSC_REPLY_OK();
   else
       OSC_REPLY_ERR();

    return 0;
}

OSC_HANDLER( new )
{
   OSC_DMSG();

   const char *project_path = &argv[0]->s;
   const char *project_display_name = &argv[1]->s;

   if ( ((Mixer*)user_data)->command_new( project_path, project_display_name ) )
       OSC_REPLY_OK();
   else
       OSC_REPLY_ERR();

    return 0;
}

OSC_HANDLER( root )
{
   OSC_DMSG();

   OSC_REPLY( "load\nsave\nquit\nnew\n");

   return 0;
}

OSC_HANDLER( add_strip )
{
   OSC_DMSG();

   ((Mixer*)user_data)->command_add_strip();

   OSC_REPLY_OK();

   return 0;
}

OSC_HANDLER( finger )
{
    OSC_DMSG();

    lo_address src = lo_message_get_source( msg );

    const char *s = "APP_TITLE\n";

/*     if ( 1 >= lo_send_from( src, ((Mixer*)user_data)->osc_endpoint, LO_TT_IMMEDIATE, "/finger-reply", "s", s ) ) */
/*     { */
/*         DMESSAGE( "Failed to send reply" ); */
/*     } */

    return 0;
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

        read_line( user_config_dir, "default_path", &default_path );

        char *path = new_project_chooser( templates, &default_path, &selected_template );

        if ( path )
        {
            if ( ! Project::create( path, selected_template ) )
                fl_alert( "Error creating project!" );
            free( path );
            free( selected_template );
        }

        update_menu();

        if ( default_path )
        {
            write_line( user_config_dir, "default_path", default_path );
            free( default_path );
        }

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
        }

        update_menu();

        mixer->show();
    }
    else if (! strcmp( picked, "&Project/&Save" ) )
    {
        command_save();
    }
    else if (! strcmp( picked, "&Project/&Quit") )
    {
        command_quit();
    }
    else if ( !strcmp( picked, "&Mixer/&Add Strip" ) )
    {
        command_add_strip();
    }
    else if ( !strcmp( picked, "&Mixer/Add &N Strips" ) )
    {
        const char *s = fl_input( "Enter number of strips to add" );

        if ( s )
        {
            for ( int i = atoi( s ); i > 0; i-- )
                command_add_strip();
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
    else if (! strcmp( picked, "&Options/&Display/&Style/&Default") )
    {
        Fl::scheme( "plastic" );
    }
    else if (! strcmp( picked, "&Options/&Display/&Style/&Flat") )
    {
        Fl::scheme( "gtk+" );
    }
    else if (! strcmp( picked, "&Options/&Display/&Colors/&System") )
    {
        color_scheme( "system" );
    }
    else if (! strcmp( picked, "&Options/&Display/&Colors/&Dark") )
    {
        color_scheme( "dark" );
    }
    else if (! strcmp( picked, "&Options/&Display/&Colors/&Very Dark") )
    {
        color_scheme( "very dark" );
    }
    else if (! strcmp( picked, "&Options/&Display/&Colors/&Light") )
    {
        color_scheme( "light" );
    }
    else if ( ! strcmp( picked, "&Help/&About" ) )
    {
        About_Dialog ab( PIXMAP_PATH "/non-mixer/logo.png" );

        ab.logo_box->label( VERSION );

        ab.title->label( "The Non Mixer" );

        ab.copyright->label( "Copyright (C) 2008-2010 Jonathan Moore Liles" );
        ab.credits->label(
            "Non-Mixer was written from scratch by\n"
            "Jonathan Moore Liles for his own use\n"
            "(see the manual).\n"
            "\n"
            "Nobody planned. Nobody helped.\n"
            "You can help now by donating time, money,\n"
            "and/or replacing the rest of Linux Audio\n"
            "with fast, light, reliable alternatives.\n" );

        ab.website_url->label( "http://non-mixer.tuxfamily.org" );

        ab.run();
    }
    else if ( !strcmp( picked, "&Help/&Manual" ))
    {
        char *pat;

        asprintf( &pat, "file://%s.html", DOCUMENT_PATH "/non-mixer/MANUAL" );

        open_url( pat );

        free( pat );
    }
}

void Mixer::cb_menu(Fl_Widget* o, void* v) {
    ((Mixer*)(v))->cb_menu(o);
}

Mixer::Mixer ( int X, int Y, int W, int H, const char *L ) :
    Fl_Group( X, Y, W, H, L )
{
    get_system_colors();

    Fl::scheme( "plastic" );
    color_scheme( "dark" );

    _rows = 1;
    box( FL_NO_BOX );
    labelsize( 96 );
    { Fl_Menu_Bar *o = menubar = new Fl_Menu_Bar( X, Y, W, 24 );
        o->add( "&Project/&New" );
        o->add( "&Project/&Open" );
        o->add( "&Project/&Save", FL_CTRL + 's', 0, 0 );
        o->add( "&Project/&Quit", FL_CTRL + 'q', 0, 0 );
        o->add( "&Mixer/&Add Strip", 'a', 0, 0 );
        o->add( "&Mixer/Add &N Strips" );
        o->add( "&Mixer/&Rows/One", '1', 0, 0 );
        o->add( "&Mixer/&Rows/Two", '2', 0, 0 );
        o->add( "&Mixer/&Rows/Three", '3', 0, 0 );
        o->add( "_&Options/&Display/&Style/&Default", 0, 0, 0, FL_MENU_RADIO | FL_MENU_VALUE );
        o->add( "_&Options/&Display/&Style/&Flat", 0, 0, 0, FL_MENU_RADIO );
        o->add( "_&Options/&Display/&Colors/&Dark", 0, 0, 0, FL_MENU_RADIO | FL_MENU_VALUE );
        o->add( "_&Options/&Display/&Colors/&Very Dark", 0, 0, 0, FL_MENU_RADIO  );
        o->add( "_&Options/&Display/&Colors/&Light", 0, 0, 0, FL_MENU_RADIO  );
        o->add( "_&Options/&Display/&Colors/&System", 0, 0, 0, FL_MENU_RADIO );
        o->add( "&Help/&Manual" );
        o->add( "&Help/&About" );
        o->callback( cb_menu, this );
    }
    { Fl_Box *o = project_name = new Fl_Box( X + 150, Y, W, 24 );
        o->labelfont( FL_HELVETICA_ITALIC );
        o->label( 0 );
        o->align( FL_ALIGN_INSIDE | FL_ALIGN_CENTER );
        o->labeltype( FL_SHADOW_LABEL );
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

    update_menu();

    load_options();

    osc_endpoint = new OSC::Endpoint();

    osc_endpoint->url();

    // if ( 1 >= lo_send_from( src, ((Mixer*)user_data)->osc_endpoint, LO_TT_IMMEDIATE, "/finger-reply", "s", s ) )

    osc_endpoint->add_method( "/nsm/quit", "", OSC_NAME( quit ), this );
    osc_endpoint->add_method( "/nsm/load", "ss", OSC_NAME( load ), this );
    osc_endpoint->add_method( "/nsm/save", "", OSC_NAME( save ), this );
    osc_endpoint->add_method( "/nsm/new", "ss", OSC_NAME( new ), this );
    osc_endpoint->add_method( "/nsm/", "", OSC_NAME( root ), this );
    osc_endpoint->add_method( "/finger", "", OSC_NAME( finger ), this );
    osc_endpoint->add_method( "/mixer/add_strip", "", OSC_NAME( add_strip ), this );
//    osc_endpoint->add_method( NULL, "", osc_generic, this );

//    osc_endpoint->start();

    /* poll so we can keep OSC handlers running in the GUI thread and avoid extra sync */
    Fl::add_timeout( OSC_INTERVAL, check_osc, this );
}

void
Mixer::check_osc ( void * v )
{
    ((Mixer*)v)->osc_endpoint->check();
    Fl::repeat_timeout( OSC_INTERVAL, check_osc, v );

}

Mixer::~Mixer ( )
{
    DMESSAGE( "Destroying mixer" );

    save_options();

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

    ms->take_focus();

    rows( _rows );

//    scroll->redraw();
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

    if ( parent() )
        parent()->redraw();
}

bool
Mixer::contains ( Mixer_Strip *ms )
{
    return ms->parent() == mixer_strips;
}

/* set the ideal number of rows... All may not actually fit. */
void
Mixer::rows ( int ideal_rows )
{
    int sh;

    int actual_rows = 1;

    if ( ideal_rows > 1 )
    {
        sh = (scroll->h() / ideal_rows ) - (mixer_strips->vspacing() * (ideal_rows - 1));
        mixer_strips->flow( true );

	if ( sh < Mixer_Strip::min_h() )
	  {
	    int can_fit = ( scroll->h() - 18 ) / Mixer_Strip::min_h();

            actual_rows = can_fit > 0 ? can_fit : 1;
	  }
        else
            actual_rows = ideal_rows;
    }
    else
        actual_rows = 1;

    if ( 1 == actual_rows )
    {
      sh = (scroll->h() - 18);
      mixer_strips->flow( false );

      actual_rows = 1;
    }

    int tw = 0;

    for ( int i = 0; i < mixer_strips->children(); ++i )
    {
        Mixer_Strip *t = (Mixer_Strip*)mixer_strips->child( i );

        t->size( t->w(), sh );

        tw += t->w() + mixer_strips->hspacing();
    }

    if ( actual_rows > 1 )
        mixer_strips->size( scroll->w() - 18, mixer_strips->h() );
    else
        mixer_strips->size( tw, mixer_strips->h() );

    _rows = ideal_rows;

    scroll->redraw();
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
    add( new Mixer_Strip( get_unique_track_name( "Unnamed" ) ) );
}

bool
Mixer::save ( void )
{
    MESSAGE( "Saving state" );
    Loggable::snapshot_callback( &Mixer::snapshot, this );
    Loggable::snapshot( "snapshot" );
    return true;
}

static const char options_filename[] = "options";

void
Mixer::load_options ( void )
{
// save options

    char *path;
    asprintf( &path, "%s/options", user_config_dir );
    ((Fl_Menu_Settings*)menubar)->load( menubar->find_item( "&Options" ), path );
    free( path );
}

void
Mixer::save_options ( void )
{
    char *path;
    asprintf( &path, "%s/%s", user_config_dir, options_filename );
    ((Fl_Menu_Settings*)menubar)->dump( menubar->find_item( "&Options" ), path );
    free( path );
}

void
Mixer::update_menu ( void )
{
    bool b = Project::open();

    if ( b )
    {
        ((Fl_Menu_Item*)menubar->find_item( "&Mixer" ))->flags &= ~FL_MENU_INACTIVE;
        ((Fl_Menu_Item*)menubar->find_item( "&Project/&Save" ))->flags &= ~FL_MENU_INACTIVE;
         mixer_strips->activate();
    }
    else
    {
        ((Fl_Menu_Item*)menubar->find_item( "&Mixer" ))->flags |= FL_MENU_INACTIVE;
        ((Fl_Menu_Item*)menubar->find_item( "&Project/&Save" ))->flags |= FL_MENU_INACTIVE;
        mixer_strips->deactivate();
    }

    project_name->label( Project::name() );
}

int
Mixer::handle ( int m )
{
    if ( Fl_Group::handle( m ) )
        return 1;

    switch ( m )
    {
        case FL_ENTER:
        case FL_LEAVE:
            return 1;
    }

    return 0;
}



/************/
/* Commands */
/************/

bool
Mixer::command_save ( void )
{
    return Project::save();
}

bool
Mixer::command_load ( const char *path, const char *display_name )
{
    if ( int err = Project::open( path ) )
    {
        // fl_alert( "Error opening project specified on commandline: %s", Project::errstr( err ) );
        return false;
    }

    if ( display_name )
        Project::name( display_name );

    update_menu();

    return true;
}

bool
Mixer::command_new ( const char *path, const char *display_name )
{
    if ( ! Project::create( path, "" ) )
        return false;

    if ( display_name )
        Project::name( display_name );

    update_menu();

    return true;
//        fl_alert( "Error creating project!" );
}

void
Mixer::command_quit ( void )
{
    if ( Loggable::dirty() )
    {
        int i = fl_choice( "There have been changes since the last save. Quitting now will discard them", "Discard", "Cancel", NULL );

        if ( i != 0 )
            return;
    }

    quit();
}

/*  */

void
Mixer::command_add_strip ( void )
{
    new_strip();
}
