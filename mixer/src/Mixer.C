
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
#include <FL/New_Project_Dialog.H>
#include "Engine/Engine.H"
#include <FL/Fl_Flowpack.H>
#include "Project.H"
#include <FL/Fl_Menu_Settings.H>
#include <FL/About_Dialog.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Theme_Chooser.H>
#include <FL/Fl_Value_SliderX.H>

#include "file.h"

#include <string.h>
#include "debug.h"
#include <unistd.h>
#include <sys/types.h>

#include "OSC/Endpoint.H"
#include <lo/lo.h>

const double STATUS_UPDATE_FREQ = 0.2f;

extern char *user_config_dir;
extern char *instance_name;

#include "debug.h"

#include "NSM.H"

extern NSM_Client *nsm;

/* static void update_cb( void *v ) { */
/*     Fl::repeat_timeout( STATUS_UPDATE_FREQ, update_cb, v ); */

/*     ((Mixer*)v)->update(); */
/* } */



/************************/
/* OSC Message Handlers */
/************************/

#undef OSC_REPLY_OK
#undef OSC_REPLY_ERR
#undef OSC_REPLY

#define OSC_REPLY_OK() ((OSC::Endpoint*)user_data)->send( lo_message_get_source( msg ), path, 0, "OK" )
#define OSC_REPLY( value ) ((OSC::Endpoint*)user_data)->send( lo_message_get_source( msg ), path, value )
#define OSC_REPLY_ERR(errcode, value) ((OSC::Endpoint*)user_data)->send( lo_message_get_source( msg ), path,errcode, value )
#define OSC_ENDPOINT() ((OSC::Endpoint*)user_data)

static int osc_add_strip ( const char *path, const char *, lo_arg **, int , lo_message msg, void *user_data )
{
   OSC_DMSG();

   Fl::lock();
   ((Mixer*)(OSC_ENDPOINT())->owner)->command_add_strip();

   Fl::unlock();

   OSC_REPLY_OK();

   return 0;
}

 int
Mixer::osc_non_hello ( const char *, const char *, lo_arg **, int , lo_message msg, void * )
{
    mixer->handle_hello( msg );
    return 0;
}


void
Mixer::handle_hello ( lo_message msg )
{    
    int argc = lo_message_get_argc( msg );
    lo_arg **argv = lo_message_get_argv( msg );
    
    if ( argc >= 4 )
    {
        const char *url = &argv[0]->s;
        const char *name = &argv[1]->s;
        const char *version = &argv[2]->s;
        const char *id = &argv[3]->s;
        
        MESSAGE( "Got hello from NON peer %s (%s) @ %s with ID \"%s\"", name, version, url, id );
                        
        mixer->osc_endpoint->handle_hello( id, url );
    }
}

void
Mixer::say_hello ( void )
{
    lo_message m = lo_message_new();

    lo_message_add( m, "sssss",
                    "/non/hello",
                    osc_endpoint->url(),
                    APP_NAME,
                    VERSION,
                    instance_name );

    nsm->broadcast( m );
    
    lo_message_free( m );
}




static 
Fl_Menu_Item *
find_item( Fl_Menu_ *menu, const char *path )
 {
     return const_cast<Fl_Menu_Item*>(menu->find_item( path ));
 }

void
Mixer::sm_active ( bool b )
{
    sm_blinker->value( b );
    sm_blinker->tooltip( nsm->session_manager_name() );

    if ( b )
    {
        find_item( menubar, "&Project/&Open" )->deactivate();
        find_item( menubar, "&Project/&New" )->deactivate();
    }
}


void
Mixer::redraw_windows ( void )
{
    window()->redraw();

    if ( Fl::first_window() )
        for ( Fl_Window *w = Fl::first_window(); ( w = Fl::next_window( w ) ); )
            w->redraw();
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

        char *default_path;

        read_line( user_config_dir, "default_path", &default_path );

        char *path = new_project_chooser( &default_path  );

        if ( path )
        {
            if ( ! Project::create( path, NULL ) )
                fl_alert( "Error creating project!" );
            free( path );
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

        const char *name = fl_dir_chooser( "Open Project", path );

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
    else if ( !strcmp( picked, "&Mixer/&Import Strip" ) )
    {
        const char *s = fl_file_chooser( "Export strip to filename:", "*.strip", NULL, 0 );

        if ( s )
        {
            if (! Mixer_Strip::import_strip( s ) )
                fl_alert( "%s", "Failed to import strip!" );
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
    else if (! strcmp( picked, "&View/&Theme") )
    {
        fl_theme_chooser();
    }
    else if (! strcmp( picked, "&Options/&Display/&Knobs/&Burnished") )
    {
        Fl_Dial::default_style( Fl_Dial::BURNISHED_DIAL );
        redraw_windows();
    }
    else if (! strcmp( picked, "&Options/&Display/&Knobs/&Arc") )
    {
        Fl_Dial::default_style( Fl_Dial::ARC_DIAL );
        redraw_windows();
    }
    else if (! strcmp( picked, "&Options/&Display/&Knobs/&Plastic") )
    {
        Fl_Dial::default_style( Fl_Dial::PLASTIC_DIAL );
        redraw_windows();
    }
    else if (! strcmp( picked, "&Options/&Display/&Sliders/&Nice") )
    {
        Fl_Value_SliderX::default_style( Fl_Value_SliderX::NICE_SLIDER );
        redraw_windows();
    }
    else if (! strcmp( picked, "&Options/&Display/&Sliders/&Fill") )
    {
        Fl_Value_SliderX::default_style( Fl_Value_SliderX::FILL_SLIDER );
        redraw_windows();
    }
    else if (! strcmp( picked, "&Options/&Display/&Sliders/&Simple") )
    {
        Fl_Value_SliderX::default_style( Fl_Value_SliderX::SIMPLE_SLIDER );
        redraw_windows();
    }
    else if ( ! strcmp( picked, "&Help/&About" ) )
    {
        About_Dialog ab( PIXMAP_PATH "/non-mixer/icon-256x256.png" );

        ab.logo_box->label( VERSION );

        ab.title->label( "Non Mixer" );

        ab.copyright->label( "Copyright (C) 2008-2013 Jonathan Moore Liles" );
        ab.credits->label(
            "Non Mixer was written from scratch by\n"
            "Jonathan Moore Liles for his own use\n"
            "(see the manual).\n"
            "\n"
            "Nobody planned. Nobody helped.\n"
            "You can help now by donating time, money,\n"
            "and/or replacing the rest of Linux Audio\n"
            "with fast, light, reliable alternatives.\n" );

        ab.website_url->label( "http://non.tuxfamily.org" );

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

static void
progress_cb ( int p, void *v )
{
    static int oldp = 0;

    if ( p != oldp )
    {
        oldp = p;
        if ( nsm )
        {
            nsm->progress( p / 100.0f );
        }
        Fl::check();
    }
}

Mixer::Mixer ( int X, int Y, int W, int H, const char *L ) :
    Fl_Group( X, Y, W, H, L )
{
    Loggable::dirty_callback( &Mixer::handle_dirty, this );
    Loggable::progress_callback( progress_cb, NULL );

    _rows = 1;
    box( FL_FLAT_BOX );
    labelsize( 96 );
    { Fl_Group *o = new Fl_Group( X, Y, W, 24 );

        { Fl_Menu_Bar *o = menubar = new Fl_Menu_Bar( X, Y, W, 24 );
            o->add( "&Project/&New" );
            o->add( "&Project/&Open" );
            o->add( "&Project/&Save", FL_CTRL + 's', 0, 0 );
            o->add( "&Project/&Quit", FL_CTRL + 'q', 0, 0 );
            o->add( "&Mixer/&Add Strip", 'a', 0, 0 );
            o->add( "&Mixer/Add &N Strips" );
            o->add( "&Mixer/&Import Strip" );
            o->add( "&Mixer/&Rows/One", '1', 0, 0 );
            o->add( "&Mixer/&Rows/Two", '2', 0, 0 );
            o->add( "&Mixer/&Rows/Three", '3', 0, 0 );
            o->add( "&View/&Theme", 0, 0, 0 );
            o->add( "_&Options/&Display/&Knobs/&Arc", 0, 0, 0, FL_MENU_RADIO   );
            o->add( "_&Options/&Display/&Knobs/&Burnished", 0, 0, 0, FL_MENU_RADIO );
            o->add( "_&Options/&Display/&Knobs/&Plastic", 0, 0, 0, FL_MENU_RADIO | FL_MENU_VALUE );
            o->add( "_&Options/&Display/&Sliders/&Nice", 0, 0, 0, FL_MENU_RADIO | FL_MENU_VALUE );
            o->add( "_&Options/&Display/&Sliders/&Fill", 0, 0, 0, FL_MENU_RADIO );
            o->add( "_&Options/&Display/&Sliders/&Simple", 0, 0, 0, FL_MENU_RADIO );
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
            Fl_Group::current()->resizable( o );
        }
        { sm_blinker = new Fl_Button( ( X + W) - 37, Y + 4, 35, 15, "SM");
            sm_blinker->box(FL_ROUNDED_BOX);
            sm_blinker->down_box(FL_ROUNDED_BOX);
            sm_blinker->color(FL_DARK2);
            sm_blinker->selection_color((Fl_Color)93);
            sm_blinker->labeltype(FL_NORMAL_LABEL);
            sm_blinker->labelfont(3);
            sm_blinker->labelsize(14);
            sm_blinker->labelcolor(FL_DARK3);
            sm_blinker->align(Fl_Align(FL_ALIGN_CENTER));
            sm_blinker->when(FL_WHEN_RELEASE);
            sm_blinker->deactivate();

        } // Fl_Blink_Button* sm_blinker
        o->end();
    }
    { Fl_Scroll *o = scroll = new Fl_Scroll( X, Y + 24, W, H - 24 );
        o->box( FL_FLAT_BOX );
//        o->type( Fl_Scroll::HORIZONTAL_ALWAYS );
//        o->box( Fl_Scroll::BOTH );
        {
            Fl_Flowpack *o = mixer_strips = new Fl_Flowpack( X, Y + 24, W, H - 18 - 24 );
//            label( "Non-Mixer" );
            align( (Fl_Align)(FL_ALIGN_CENTER | FL_ALIGN_INSIDE) );
            o->box( FL_FLAT_BOX );
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
}

int
Mixer::init_osc ( const char *osc_port )
{
    osc_endpoint = new OSC::Endpoint();

    if ( int r = osc_endpoint->init( LO_UDP, osc_port ) )
        return r;

    osc_endpoint->owner = this;
    
    printf( "OSC=%s\n", osc_endpoint->url() );

    osc_endpoint->add_method( "/non/hello", "ssss", &Mixer::osc_non_hello, osc_endpoint, "" );
    
//  
    osc_endpoint->add_method( "/non/mixer/add_strip", "", osc_add_strip, osc_endpoint, "" );
  
    osc_endpoint->start();

    return 0;
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
Mixer::handle_dirty ( int d, void *v )
{
    //Mixer *m = (Mixer*)v;
    if ( !nsm )
        return;
    
    if ( d == 1 )
        nsm->is_dirty();
    else if ( d == 0 )
        nsm->is_clean();
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
    mixer->deactivate();

    if ( Project::open( path ) )
    {
        // fl_alert( "Error opening project specified on commandline: %s", Project::errstr( err ) );
        return false;
    }

    if ( display_name )
        Project::name( display_name );

    update_menu();

    mixer->activate();

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
