
/*******************************************************************************/
/* Copyright (C) 2008-2020 Jonathan Moore Liles (as "Non-Session-Manager")     */
/* Copyright (C) 2020- Nils Hilbricht                                          */
/*                                                                             */
/* This file is part of New-Session-Manager                                    */
/*                                                                             */
/* New-Session-Manager is free software: you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License as published by        */
/* the Free Software Foundation, either version 3 of the License, or           */
/* (at your option) any later version.                                         */
/*                                                                             */
/* New-Session-Manager is distributed in the hope that it will be useful,      */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of              */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               */
/* GNU General Public License for more details.                                */
/*                                                                             */
/* You should have received a copy of the GNU General Public License           */
/* along with New-Session-Manager. If not, see <https://www.gnu.org/licenses/>.*/
/*******************************************************************************/



#include "Endpoint.hpp"


#include <FL/Fl.H>

#include <FL/Fl_Window.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Progress.H>
#include "debug.h"
#include <FL/Fl_Browser.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/Fl_Tree.H>
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Tile.H>
#include <FL/Fl_Shared_Image.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Text_Display.H>
#include "FL/Fl_Packscroller.H"
#include "FL/Fl_Scalepack.H"

#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>

#define APP_NAME "NSM Legacy GUI"
#define APP_TITLE "NSM Legacy GUI"

#pragma GCC diagnostic ignored "-Wunused-result"

// static lo_address nsm_addr = NULL;
static time_t last_ping_response;

static OSC::Endpoint *osc;

struct Daemon
{
    const char *url;
    lo_address addr;
    bool is_child;

    Daemon ( )
        {
            url = NULL;
            addr = NULL;
            is_child = false;
        }
};

static std::list<Daemon*> daemon_list;                                  /* list of all connected daemons */

#define foreach_daemon( _it ) for ( std::list<Daemon*>::iterator _it = daemon_list.begin(); _it != daemon_list.end(); ++ _it )

static
Fl_Image *
get_program_icon ( const char *name )
{
    const char *tries[] =
    {
        "/usr/local/share/icons/hicolor/32x32/apps/%s.png",
        "/usr/local/share/pixmaps/%s.png",
        "/usr/local/share/pixmaps/%s.xpm",
        "/usr/share/icons/hicolor/32x32/apps/%s.png",
        "/usr/share/icons/hicolor/128x128/apps/%s.png",
        "/usr/share/pixmaps/%s.png",
        "/usr/share/pixmaps/%s.xpm",
    };

    for ( unsigned int i = 0; i < 6; i++ )
    {
        char *icon_p;

        asprintf( &icon_p, tries[i], name );

        Fl_Image *img = Fl_Shared_Image::get( icon_p, 32, 32 );

        free( icon_p );

        if ( img )
            return img;
    }

    return NULL;
}

class NSM_Client : public Fl_Group
{
    char *_client_id;
    char *_client_label;
    char *_client_name;

    Fl_Box *client_name;
    Fl_Box *icon_box;
    Fl_Progress *_progress;
    Fl_Light_Button *_dirty;
    Fl_Light_Button *_gui;
    Fl_Button *_remove_button;
    Fl_Button *_restart_button;
    Fl_Button *_kill_button;

    void
    set_label ( void )
        {
            char *l;

            if ( _client_label && _client_label[0] != '\0' )
                asprintf( &l, "%s (%s)", _client_name, _client_label );
            else
                l = strdup( _client_name );

            if ( ! icon_box->image() )
            {
                Fl_Image *img = get_program_icon( _client_name );

                if ( img )
                {
                    icon_box->image( img );
                }
            }

            client_name->copy_label( l );

            free(l);

            redraw();
        }

public:

    void
    name ( const char *v )
        {
            if ( _client_name )
                free( _client_name );

            _client_name = strdup( v );

            set_label();
        }

    void
    client_label ( const char *s )
        {
            if ( _client_label )
                free( _client_label );

            _client_label = strdup( s );

            set_label();
        }

    void
    client_id ( const char *v )
        {
            if ( _client_id )
                free( _client_id );

            _client_id = strdup( v );
        }

    void
    progress ( float f )
        {
            _progress->value( f );
            _progress->redraw();
        }

    void
    dirty ( bool b )
        {
            _dirty->value( b );
            _dirty->redraw();
        }

    void
    gui_visible ( bool b )
        {
            _gui->value( b );
            _gui->redraw();
        }


    void
    has_optional_gui ( void )
        {
            _gui->show();
            _gui->redraw();
        }

    void
    stopped ( bool b )
        {
            if ( b )
            {
                _remove_button->show();
                _restart_button->show();
                _kill_button->hide();
                _gui->deactivate();
                _dirty->deactivate();
                redraw();
            }
            else
            {
                _gui->activate();
                _dirty->activate();
                _kill_button->show();
                _restart_button->hide();
                _remove_button->hide();
            }

            /* _restart_button->redraw(); */
            /* _remove_button->redraw(); */
        }

    void
    pending_command ( const char *command )
        {
            _progress->copy_label( command );

            stopped( 0 );

            if ( ! strcmp( command, "ready" ) )
            {
                _progress->value( 0.0f );
            }
            else if ( ! strcmp( command, "quit" ) ||
                      ! strcmp( command, "kill" ) ||
                      ! strcmp( command, "error" ) )
            {
                //Set a border color to indicate warning
                color( fl_color_average( FL_BLACK, FL_RED, 0.50 ) );
            }
            else if ( ! strcmp( command, "stopped" ) )
            {
                stopped( 1 );
            }

            redraw();
        }


    static void
    cb_button ( Fl_Widget *o, void * v )
        {
            ((NSM_Client*)v)->cb_button( o );
        }

    void
    cb_button ( Fl_Widget *o )
        {
            if ( o == _dirty )
            {
                MESSAGE( "Sending save.");
                foreach_daemon ( d )
                {
                    osc->send( (*d)->addr, "/nsm/gui/client/save", _client_id );
                }
            }
            else if ( o == _gui )
            {
                MESSAGE( "Sending hide/show GUI.");
                foreach_daemon ( d )
                {
                    if ( !_gui->value() )
                        osc->send( (*d)->addr, "/nsm/gui/client/show_optional_gui", _client_id );
                    else
                        osc->send( (*d)->addr, "/nsm/gui/client/hide_optional_gui", _client_id );
                }
            }
            else if ( o == _remove_button )
            {
                MESSAGE( "Sending remove.");
                foreach_daemon ( d )
                {
                    osc->send( (*d)->addr, "/nsm/gui/client/remove", _client_id );
                }
            }
            else if ( o == _restart_button )
            {
                MESSAGE( "Sending resume" );
                foreach_daemon ( d )
                {
                    osc->send( (*d)->addr, "/nsm/gui/client/resume", _client_id );
                }
            }
            else if ( o == _kill_button )
            {
                MESSAGE( "Sending stop" );
                foreach_daemon ( d )
                {
                    osc->send( (*d)->addr, "/nsm/gui/client/stop", _client_id );
                }
            }
        }


    const char *
    client_id ( void )
        { return _client_id; }

    NSM_Client ( int X, int Y, int W, int H, const char *L ) :
        Fl_Group( X, Y, W, H, L )
        {

            _client_id = NULL;
            _client_name = NULL;
            _client_label = NULL;

            align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE );
            box( FL_UP_FRAME );

            int yy = Y + H * 0.25;
            int hh = H * 0.50;
            int xx = X + W - ( 75 + Fl::box_dw( box() ) );
            int ss = 2;

            /* /\* dummy group *\/ */
            /* { Fl_Group *o = new Fl_Group( X, Y, W, H ); */
            /*     o->end(); */
            /*     resizable( o ); */
            /* } */

            { Fl_Pack *o = new Fl_Pack( X + 15, Y, 300 - 5, H );
                o->type( FL_HORIZONTAL );
                o->spacing( 10 );
                {  icon_box = new Fl_Box( 0, 0, 32, 32 );
                }

                { Fl_Box *o = client_name = new Fl_Box( 0, 0, 300, 48 );
                    o->align( FL_ALIGN_INSIDE | FL_ALIGN_LEFT );
                    o->labeltype( FL_NORMAL_LABEL );
                }
                o->end();
            }

            { Fl_Box *o = new Fl_Box( X + 300, Y, 100, h() );
                Fl_Group::current()->resizable(o);
                }

            { Fl_Progress *o = _progress = new Fl_Progress( xx, Y + H * 0.25, 75, H * 0.50, NULL );
                o->box( FL_FLAT_BOX );
                o->copy_label( "launch" );
                o->labelsize( 12 );
                o->minimum( 0.0f );
                o->maximum( 1.0f );
            }

            { Fl_Group *o = new Fl_Group( X + W - 400, Y, 400, H );

                xx -= 50 + ss;

                { Fl_Light_Button *o = _dirty = new Fl_Light_Button( xx, yy, 50, hh, "SAVE" );

                    o->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE );
                    o->labelsize( 9 );
                    o->box( FL_UP_BOX );
                    o->type(0);
                    o->value( 0 );
                    o->callback( cb_button, this );
                }

                xx -= 40 + ss;

                { Fl_Light_Button *o = _gui = new Fl_Light_Button( xx, yy, 40, hh, "GUI" );

                    o->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE );
                    o->labelsize( 9 );
                    o->box( FL_UP_BOX );
                    o->type(0);
                    o->value( 0 );
                    o->hide();
                    o->callback( cb_button, this );
                }

                xx -= 25 + ss;

                { Fl_Button *o = _kill_button = new Fl_Button( xx, yy, 25, hh, "@square" );
                    o->labelsize( 9 );
                    o->box( FL_UP_BOX );
                    o->type(0);
                    o->value( 0 );
                    o->tooltip( "Stop" );
                    o->callback( cb_button, this );
                }

                xx -= 25 + ss;

                { Fl_Button *o = _restart_button = new Fl_Button( xx, yy, 25, hh );


                    o->box( FL_UP_BOX );
                    o->type(0);
                    o->value( 0 );
                    o->label( "@>" );
                    o->tooltip( "Resume" );
                    o->hide();
                    o->callback( cb_button, this );
                }

                xx -= 25 + ss;

                { Fl_Button *o = _remove_button = new Fl_Button( xx, yy, 25, hh );


                    o->box( FL_UP_BOX );
                    o->type(0);
                    o->value( 0 );
                    o->label( "X" );
                    o->tooltip( "Remove" );
                    o->hide();
                    o->callback( cb_button, this );
                }


                o->end();
            }
            end();
        }

    ~NSM_Client ( )
        {
            if ( _client_id )
            {
                free( _client_id );
                _client_id = NULL;
            }

            if ( _client_name )
            {
                free( _client_name );
                _client_name = NULL;
            }

            if ( _client_label )
            {
                free( _client_label );
                _client_label = NULL;
            }

            if ( label() )
            {
                free( (char*)label() );
                label( NULL );
            }
        }
};

static
void
fl_awake_alert( void *v )
{
    if ( v )
    {
        fl_alert( "%s", (char*)v );
        free( v );
    }
}

void
browser_callback ( Fl_Widget *w, void * )
{
    w->window()->hide();
}

class NSM_Controller : public Fl_Group
{

    Fl_Text_Display *status_display;

public:

    Fl_Pack *clients_pack;
    Fl_Pack *buttons_pack;
    Fl_Button *close_button;
    Fl_Button *abort_button;
    Fl_Button *save_button;
    Fl_Button *open_button;
    Fl_Button *new_button;
    Fl_Button *add_button;
    Fl_Button *duplicate_button;
    Fl_Button *quit_button;
    Fl_Button *refresh_button;
    Fl_Box *session_name_box;

    Fl_Tree *session_browser;

    int status_lines;

    static void cb_handle ( Fl_Widget *w, void *v )
        {
            ((NSM_Controller*)v)->cb_handle( w );

        }

    void log_status ( const char *s )
        {
            time_t now;

            now = time( NULL );

            struct tm * tm =  localtime( &now );

            char *ts;
            asprintf( &ts, "%02i:%02i:%02i ", tm->tm_hour, tm->tm_min, tm->tm_sec );

            status_display->buffer()->append( ts );
            free( ts );

            status_display->buffer()->append( s );
            status_display->scroll( ++status_lines, 0 );
            status_display->buffer()->append( "\n" );
        }

    void
    cb_handle ( Fl_Widget *w )
        {
            if ( w == abort_button )
            {
                if ( 0 == fl_choice( "Are you sure you want to close this session? Unsaved changes will be lost.", "Close anyway", "Cancel", NULL ) )
                {
                    MESSAGE( "Sending abort." );

                    foreach_daemon ( d )
                    {
                        osc->send( (*d)->addr, "/nsm/server/abort" );
                    }
                }
            }
            if ( w == close_button )
            {
                MESSAGE( "Sending close." );
                foreach_daemon ( d )
                {
                    osc->send( (*d)->addr, "/nsm/server/close" );
                }
            }
            else if ( w == save_button )
            {
                MESSAGE( "Sending save." );
                foreach_daemon ( d )
                {
                    osc->send( (*d)->addr, "/nsm/server/save" );
                }
            }
            else if ( w == open_button )
            {
                const char *name = fl_input( "Open Session", NULL );

                if ( ! name || name[0] == '\0' )
                    return;

                Fl_Tree_Item *item = session_browser->find_item( name );

                if ( item )
                    session_browser->select_only( item, 1 );
            }
            else if ( w == duplicate_button )
            {
                const char *name = fl_input( "New Session", NULL );

                if ( ! name || name[0] == '\0' )
                    return;

                MESSAGE( "Sending duplicate for: %s", name );
                foreach_daemon ( d )
                {
                    osc->send( (*d)->addr, "/nsm/server/duplicate", name );
                }
            }
            else if ( w == quit_button )
            {
                window()->do_callback( window(), this );
            }
            else if ( w == refresh_button )
            {
                session_browser->clear();
                session_browser->redraw();
                MESSAGE( "Refreshing session list." );
                foreach_daemon ( d )
                {
                    osc->send( (*d)->addr, "/nsm/server/list" );
                }
            }
            else if ( w == session_browser )
            {
                if ( session_browser->callback_reason() != FL_TREE_REASON_SELECTED )
                    return;

                Fl_Tree_Item *item = session_browser->callback_item();

                if ( item )
                    session_browser->deselect( item, 0 ); //Deselect on program start, otherwise it looks like the first session is already loaded.

                if ( item->children() )
                    return;

                char name[1024];

                session_browser->item_pathname( name, sizeof(name), item );

                foreach_daemon ( d )
                {
                    osc->send( (*d)->addr, "/nsm/server/open", name );
                }
            }
            else if ( w == new_button )
            {
                const char *name = fl_input( "New Session", NULL );

                if ( ! name || name[0] == '\0' )
                    return;

                MESSAGE( "Sending new for: %s", name );
                foreach_daemon ( d )
                {
                    osc->send( (*d)->addr, "/nsm/server/new", name );
                }
            }
            else if ( w == add_button )
            {
                Fl_Select_Browser *browser;

                if ( daemon_list.size() > 1 )
                {
                    Fl_Window* win = new Fl_Window( window()->x(), window()->y(), 300, 400, "Choose Server" );
                    {
                        {
                            Fl_Box *o = new Fl_Box( 0,0, 300, 100 );

                            o->label( "Connected to multiple NSM servers, please select which one to add a client to." );
                            o->align( FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_WRAP );
                        }
                        {
                            Fl_Select_Browser *o = browser = new Fl_Select_Browser( 0, 100, 300, 300 );
                            o->box( FL_ROUNDED_BOX );
                            o->callback( browser_callback, win );
                            foreach_daemon( d )
                            {
                                o->add( (*d)->url );
                            }
                        }
                    }

                    win->end();

                    win->show();

                    while ( win->visible() )
                    {
                        Fl::wait();
                    }

                    if ( ! browser->value() )
                        return;

                    const char *n = fl_input( "Enter executable name" );

                    if ( !n || !*n || n[0] == '\0' )
                        return;

                    char *name = strdup( n );

                    lo_address nsm_addr = lo_address_new_from_url( browser->text( browser->value() ) );

                    osc->send( nsm_addr, "/nsm/server/add", name );

                    free( name );

                    delete win;
                }
                else
                {
                    const char *n = fl_input( "Enter executable name" );

                    if ( !n || !*n || n[0] == '\0' )
                        return;

                    char *name = strdup( n );

                    MESSAGE( "Sending add for: %s", name );
                    /* FIXME: user should get to choose which system to do the add on */
                    foreach_daemon ( d )
                    {
                        osc->send( (*d)->addr, "/nsm/server/add", name );
                    }

                    free( name );
                }

            }
        }


    NSM_Client *
    client_by_id ( const char *id )
        {
            for ( int i = clients_pack->children(); i--; )
            {
                NSM_Client *c = (NSM_Client*)clients_pack->child( i );

                if ( ! strcmp( c->client_id(), id ) )
                {
                    return c;
                }
            }
            return NULL;
        }


    const char *session_name ( void ) const
        {
            return session_name_box->label();
        }

    void
    session_name ( const char *name )
        {
            session_name_box->copy_label( name );

            if ( strlen( name ) )
            {
                save_button->activate();
                add_button->activate();
                duplicate_button->activate();
                abort_button->activate();
                close_button->activate();
            }
            else
            {
                save_button->deactivate();
                add_button->deactivate();
                duplicate_button->deactivate();
                abort_button->deactivate();
                close_button->deactivate();
            }

            redraw();
        }

    void
    client_stopped ( const char *client_id )
        {
            NSM_Client *c = client_by_id( client_id );

            if ( c )
            {
                c->stopped( 1 );
            }
        }

    void
    client_quit ( const char *client_id )
        {
            NSM_Client *c = client_by_id( client_id );

            if ( c )
            {
                clients_pack->remove( c );
                delete c;
            }

            if ( clients_pack->children() == 0 )
            {
                ((Fl_Packscroller*)clients_pack->parent())->yposition( 0 );
            }

            parent()->redraw();
        }

    void
    client_new ( const char *client_id, const char *client_name )
        {

            NSM_Client *c;

            c = client_by_id( client_id );

            if ( c )
            {
                c->name( client_name );
                return;
            }

            c = new NSM_Client( 0, 0, w(), 40, NULL );

            c->name( client_name );
            c->client_id( client_id );
            c->stopped( 0 );

            clients_pack->add( c );

            redraw();
        }

    void client_pending_command ( NSM_Client *c, const char *command )
        {
            if ( c )
            {
                if ( ! strcmp( command, "removed" ) )
                {
                    clients_pack->remove( c );
                    delete c;

                    parent()->redraw();
                }
                else
                    c->pending_command( command );
            }
        }


    void add_session_to_list ( const char *name )
        {
            session_browser->add( name );
            session_browser->redraw();
        }


    NSM_Controller ( int X, int Y, int W, int H, const char *L ) :
        Fl_Group( X, Y, W, H, L )
        {
            status_lines = 0;

            align( FL_ALIGN_RIGHT | FL_ALIGN_CENTER | FL_ALIGN_INSIDE );

            { Fl_Pack *o = buttons_pack = new Fl_Pack( X, Y, W, 30 );
                o->type( Fl_Pack::HORIZONTAL );
                o->box( FL_NO_BOX );
                { Fl_Button *o = quit_button = new Fl_Button( 0, 0, 50, 50, "&Quit" );
                    o->shortcut( FL_CTRL | 'q' );
                    o->box( FL_UP_BOX );
                    o->callback( cb_handle, (void*)this );
                }
                { Fl_Button *o = refresh_button = new Fl_Button( 0, 0, 70, 50, "&Refresh" );
                    o->shortcut( FL_CTRL | 'r' );
                    o->box( FL_UP_BOX );
                    o->callback( cb_handle, (void*)this );
                }
                { Fl_Button *o = new_button = new Fl_Button( 0, 0, 100, 50, "&New Session" );
                    o->shortcut( FL_CTRL | 'n' );
                    o->box( FL_UP_BOX );
                    o->callback( cb_handle, (void*)this );
                }
                { Fl_Button *o = save_button = new Fl_Button( 0, 0, 105, 50, "&Save" );
                    o->shortcut( FL_CTRL | 's' );
                    o->box( FL_UP_BOX );
                    o->callback( cb_handle, (void*)this );
                }
                { Fl_Button *o = close_button = new Fl_Button( 0, 0, 105, 50, "Save && Close" );
                    o->shortcut( FL_CTRL | 'e' ); // is this a good key?
                    o->box( FL_UP_BOX );
                    o->callback( cb_handle, (void*)this );
                }
                { Fl_Button *o = duplicate_button = new Fl_Button( 0, 0, 105, 50, "Save && &Dupl." );
                    o->shortcut( FL_CTRL | 'd' );
                    o->box( FL_UP_BOX );
                    o->callback( cb_handle, (void*)this );
                }
                { Fl_Button *o = open_button = new Fl_Button( 0, 0, 105, 50, "Save && &Open" );
                    o->shortcut( FL_CTRL | 'o' );
                    o->box( FL_UP_BOX );
                    o->callback( cb_handle, (void*)this );
                }
                { Fl_Button *o = abort_button = new Fl_Button( 0, 0, 160, 50, "Close &without Saving" );
                    o->shortcut( FL_CTRL | 'w' );
                    o->box( FL_UP_BOX );
                    o->callback( cb_handle, (void*)this );
                }

                o->end();
            }

            int SH = 14;

            { Fl_Tile *o = new Fl_Tile( X, Y + 30, W, H - 30 );
                { Fl_Scalepack *o = new Fl_Scalepack( X, Y + 30, 300, H - ( 30 +  SH ) );
                    o->type( FL_VERTICAL );
                    o->spacing( 2 );

                    { new Fl_Box( 0,0,100, 24, "Sessions" );
                    }

                    {
                        Fl_Tree *o = session_browser = new Fl_Tree( X, Y + 50, W / 3, H - ( 50 + SH ) );
                        o->callback( cb_handle, (void *)this );
                        o->sortorder( FL_TREE_SORT_ASCENDING );
                        o->showroot( 0 );
                        o->selectbox( FL_UP_FRAME );
                        o->box( FL_FLAT_BOX );
                        /* o->label( "Sessions" ); */
                        o->end();
                        Fl_Group::current()->resizable( o );
                    } // Fl_Tree
                    o->end();
                }

                Fl_Scalepack *scalepack;
                { Fl_Scalepack *o = scalepack = new Fl_Scalepack( X + 300, Y + 30, W - 300, H - ( 30 + SH ) );
                    o->type( FL_VERTICAL );
                    o->spacing( 2 );

                    {
                        session_name_box = new Fl_Box( 0, 0, 100, 25, "" );
                        session_name_box->labelsize( 20 );
                    }

                    { Fl_Button *o = add_button = new Fl_Button( 0, 0, 100, 25, "&Add Client to Session" );
                        o->shortcut( FL_CTRL | 'a' );
                        o->box( FL_UP_BOX );
                        o->align( FL_ALIGN_CLIP );
                        o->callback( cb_handle, (void*)this );
                    }

                    {
                        Fl_Packscroller *o = new Fl_Packscroller( 0, 0, 100, H - ( 30 + SH ) );
                        o->align( FL_ALIGN_TOP );
                        o->labeltype( FL_SHADOW_LABEL );
                        {
                            Fl_Pack *o = clients_pack = new Fl_Pack( 0, 0, 100, 100 );
                            o->align( FL_ALIGN_TOP );
                            o->spacing( 4 );
                            o->type( Fl_Pack::VERTICAL );
                            o->end();
                        }
                        o->end();
                        Fl_Group::current()->resizable( o );
                    } // Fl_Packscroller
                    o->end();
                    /* Fl_Group::current()->resizable( o ); */
                } // Fl_Scalepack

                { new Fl_Box( X + 1000, Y + 30, 100, H - ( 30 + SH ));
                }

                { Fl_Text_Display *o = status_display = new Fl_Text_Display( X, Y + H - SH, W, SH );
                    o->box( FL_UP_BOX );
                    Fl_Text_Buffer *b = new Fl_Text_Buffer();
                    o->textfont( FL_COURIER ); // Create the "technical log" look&feel
                    o->textsize( 12 );
                    o->buffer(b);
                }

                o->end();
                resizable( o );

            } // Fl_tile

            end();

            deactivate();
        }

    int min_h ( void )
        {
            return 500;
        }

    void
    ping ( void )
        {
            if ( daemon_list.size() )
            {
                foreach_daemon( d )
                {
                    osc->send( (*d)->addr, "/osc/ping" );
                }
            }
            if ( last_ping_response )
            {
                if ( time(NULL) - last_ping_response > 10 )
                {
                    if ( active() )
                    {
                        deactivate();
                        log_status( "Server is not responding..." );
                    }
                }
                else
                {
                    if ( !active() )
                    {
                        log_status( "Server is back." );
                        activate();
                    }
                }
            }
        }


    int init_osc ( void )
        {
            osc = new OSC::Endpoint();

            if ( int r = osc->init( LO_UDP ) )
                return r;

            osc->owner = this;

            osc->add_method( "/error", "sis", osc_handler, osc, "msg" );
            osc->add_method( "/reply", "ss", osc_handler, osc, "msg" );
            osc->add_method( "/reply", "s", osc_handler, osc, "" );

            osc->add_method( "/nsm/server/broadcast", NULL, osc_broadcast_handler, osc, "msg" );
            osc->add_method( "/nsm/gui/server_announce", "s", osc_handler, osc, "msg" );
            osc->add_method( "/nsm/gui/server/message", "s", osc_handler, osc, "msg" );
            osc->add_method( "/nsm/gui/gui_announce", "s", osc_handler, osc, "msg" );
            osc->add_method( "/nsm/gui/session/session", "s", osc_handler, osc, "path,display_name" );
            osc->add_method( "/nsm/gui/session/name", "ss", osc_handler, osc, "path,display_name" );
            osc->add_method( "/nsm/gui/client/new", "ss", osc_handler, osc, "path,display_name" );
            osc->add_method( "/nsm/gui/client/status", "ss", osc_handler, osc, "path,display_name" );
            osc->add_method( "/nsm/gui/client/switch", "ss", osc_handler, osc, "path,display_name" );
            osc->add_method( "/nsm/gui/client/progress", "sf", osc_handler, osc, "path,display_name" );
            osc->add_method( "/nsm/gui/client/dirty", "si", osc_handler, osc, "path,display_name" );
            osc->add_method( "/nsm/gui/client/has_optional_gui", "s", osc_handler, osc, "path,display_name" );
            osc->add_method( "/nsm/gui/client/gui_visible", "si", osc_handler, osc, "path,display_name" );
            osc->add_method( "/nsm/gui/client/label", "ss", osc_handler, osc, "path,display_name" );

            osc->start();

            return 0;
        }


    void announce ( const char *nsm_url )
        {
            /* Daemon *d = new Daemon; */

            /* d->url = nsm_url; */
            lo_address nsm_addr = lo_address_new_from_url( nsm_url );
//            d->is_child = true;

            /* daemon_list.push_back( d ); */

            osc->send( nsm_addr, "/nsm/gui/gui_announce" );
        }

private:

    static int osc_broadcast_handler ( const char *path, const char *, lo_arg **, int argc, lo_message msg, void * )
        {
            if ( ! argc )
                /* need at least one argument... */
                return 0;

            MESSAGE( "Relaying broadcast" );

            foreach_daemon( d )
            {
                char *u1 = lo_address_get_url( (*d)->addr );
                char *u2 = lo_address_get_url( lo_message_get_source( msg ) );

                if ( strcmp( u1, u2 ) )
                {
                    osc->send( (*d)->addr, path, msg );
                }

                free( u1 );
                free( u2 );
            }

            return 0;
        }

    static int osc_handler ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
        {
//            OSC_DMSG();

            NSM_Controller *controller = (NSM_Controller*)((OSC::Endpoint*)user_data)->owner;

            Fl::lock();

            if ( !strcmp( path, "/nsm/gui/server/message" ) && !strcmp( types, "s" ) )
            {
                controller->log_status( &argv[0]->s );
            }
            else if ( !strcmp( path, "/nsm/gui/session/session" ) &&
                 ! strcmp( types, "s" ) )
            {
                controller->add_session_to_list( &argv[0]->s );
            }
            else if ( !strcmp( path, "/nsm/gui/gui_announce" ) )
            {
                /* pre-existing server is replying to our announce message */
                controller->activate();

                lo_address nsm_addr = lo_message_get_source( msg );

                osc->send( nsm_addr, "/nsm/server/list" );
            }
            else if ( !strcmp( path, "/nsm/gui/server_announce" ) )
            {
                /* must be a server we launched */

                controller->activate();

                Daemon *d = new Daemon;

                d->url = lo_address_get_url( lo_message_get_source( msg ) );
                d->addr = lo_address_new_from_url( d->url );
                d->is_child = true;

                daemon_list.push_back( d );

                osc->send( d->addr, "/nsm/server/list" );
            }
            else if ( !strcmp( path, "/nsm/gui/session/name" ) &&
                      !strcmp( types, "ss" ))
            {
                controller->session_name( &argv[0]->s );

                if ( !strcmp( &argv[0]->s, "" ) )
                {
                    controller->session_browser->deselect_all();
                }
                else
                {
                    Fl_Tree_Item *o = controller->session_browser->find_item( &argv[1]->s );
                    if ( o )
                    {
                        controller->session_browser->select_only( o, 0 );
                        controller->session_browser->show_item( o, 0 );
                    }
                }
            }
            else if (!strcmp( path, "/error" ) &&
                     !strcmp( types, "sis" ) )
            {
                int err = argv[1]->i;

                if ( err != 0 )
                {
                    char *s;
                    asprintf( &s, "Command %s failed with:\n\n%s", &argv[0]->s, &argv[2]->s );

                    Fl::awake(fl_awake_alert, s);
                }
            }
            else if (!strcmp( path, "/reply" ) && argc && 's' == *types )
            {
                if ( !strcmp( &argv[0]->s, "/nsm/server/list" ) )
                {
                    controller->add_session_to_list( &argv[1]->s );
                }
                else if ( !strcmp( &argv[0]->s, "/osc/ping" ) )
                {
                    last_ping_response = time( NULL );
                }
                else if ( ! strcmp( types, "ss" ) )
                {
                    MESSAGE( "%s says %s", &argv[0]->s, &argv[1]->s);
                    controller->log_status( &argv[1]->s );
                }
            }

            if ( !strncmp( path, "/nsm/gui/client/", strlen( "/nsm/gui/client/" ) ) )
            {
                if ( !strcmp( path, "/nsm/gui/client/new" ) &&
                     !strcmp( types, "ss" ) )
                {
                    controller->client_new( &argv[0]->s, &argv[1]->s );
                }
                else
                {
                    NSM_Client *c = controller->client_by_id( &argv[0]->s );

                    if ( c )
                    {
                        if ( !strcmp( path, "/nsm/gui/client/status" ) &&
                             !strcmp( types, "ss" ))
                        {
                            controller->client_pending_command( c, &argv[1]->s );
                        }
                        else if ( !strcmp( path, "/nsm/gui/client/progress" ) &&
                                  !strcmp( types, "sf" ))
                        {
                            c->progress( argv[1]->f );
                        }
                        else if ( !strcmp( path, "/nsm/gui/client/dirty" ) &&
                                  !strcmp( types, "si" ))
                        {
                            c->dirty(  argv[1]->i );
                        }
                        else if ( !strcmp( path, "/nsm/gui/client/gui_visible" ) &&
                                  !strcmp( types, "si" ))
                        {
                            c->gui_visible(  argv[1]->i );
                        }
                        else if ( !strcmp( path, "/nsm/gui/client/label" ) &&
                                  !strcmp( types, "ss" ))
                        {
                            c->client_label( &argv[1]->s );
                        }
                        else if ( !strcmp( path, "/nsm/gui/client/has_optional_gui" ) &&
                                  !strcmp( types, "s" ))
                        {
                            c->has_optional_gui();
                        }
                        else if ( !strcmp( path, "/nsm/gui/client/switch" ) &&
                                  !strcmp( types, "ss" ))
                        {
                            c->client_id( &argv[1]->s );
                        }
                    }
                    else
                        MESSAGE( "Got message %s from unknown client", path );
                }
            }

            Fl::unlock();
            Fl::awake();

            return 0;
        }
};


static NSM_Controller *controller;

void
ping ( void * )
{
    controller->ping();
    Fl::repeat_timeout( 1.0, ping, NULL );
}

void
cb_main ( Fl_Widget *, void * )
{
    if ( Fl::event_key() != FL_Escape )
    {
        int children = 0;
        foreach_daemon ( d )
        {
            if ( (*d)->is_child )
                ++children;
        }

        if ( children )
        {
            if ( strlen( controller->session_name() ) )
            {
                fl_message( "%s", "You have to close the session before you can quit." );
                return;
            }
        }

        while ( Fl::first_window() ) Fl::first_window()->hide();
    }
}

int
main (int argc, char **argv )
{
    Fl::scheme( "gtk+" );

    Fl::visual(FL_DOUBLE|FL_INDEX); // FLKT Double_Window: "higly recommended […] put before the first show() of any window in your program"

    fl_register_images();

    Fl::lock();

    Fl_Double_Window *main_window;

    {
        Fl_Double_Window *o = main_window = new Fl_Double_Window( 800, 600, APP_TITLE );
        {
            main_window->xclass( APP_NAME );

            Fl_Widget *o = controller = new NSM_Controller( 0, 0, main_window->w(), main_window->h(), NULL );
            controller->session_name( "" );

            Fl_Group::current()->resizable(o);
        }
        o->end();

        o->size_range( main_window->w(), controller->min_h(), 0, 0 );

        o->callback( (Fl_Callback*)cb_main, main_window );

        o->show( 0, NULL );
    }

    //Setting colors only after main window creation.
    //We keep them all in once place instead of setting them in the widgets
    Fl::set_color( FL_BACKGROUND_COLOR, 37, 40, 45 ); //These are the colors used as backgrounds by almost all widgets and used to draw the edges of all the boxtypes.
    Fl::set_color( FL_BACKGROUND2_COLOR, 55, 61, 69 ); //This color is used as a background by Fl_Input and other text widgets.
    Fl::set_color( FL_FOREGROUND_COLOR, 223, 237, 255 );
    Fl::set_color( FL_INACTIVE_COLOR, 255, 0, 0 ); // Not used
    Fl::set_color( FL_SELECTION_COLOR, 80, 84, 92 ); // e.g. the currently selected session
    Fl::reload_scheme();
    controller->session_browser->item_labelfgcolor( fl_rgb_color (213, 227, 245 ) ); // a bit darker than foreground



    static struct option long_options[] =
        {
            { "nsm-url", required_argument, 0, 'n' },
            { "help", no_argument, 0, 'h' },
            { 0, 0, 0, 0 }
        };

    int option_index = 0;
    int c = 0;

    while ( ( c = getopt_long_only( argc, argv, "", long_options, &option_index  ) ) != -1 )
    {
        switch ( c )
        {
            case 'n':
            {
                MESSAGE( "Adding %s to daemon list", optarg );
                Daemon *d = new Daemon;

                d->url = optarg;
                d->addr = lo_address_new_from_url( optarg );

                daemon_list.push_back( d );
                break;
            }
            case 'h':
                //Print usage message according to POSIX.1-2017
                const char *usage =
                "nsm-legacy-gui - FLTK GUI for the 'New Session Manager'\n\n"
                "Usage:\n"
                "  nsm-legacy-gui\n"
                "  nsm-legacy-gui --help\n"
                "\n"
                "Options:\n"
                "  --help                Show this screen\n"
                "  --nsm-url url         Connect to a running nsmd [Example: osc.udp://mycomputer.localdomain:38356/].\n"
                "  --                    Everything after -- will be given to nsmd as server options. See nsmd --help .\n"
                "\n"
                "For backwards compatibility this executable also exist as symlink 'non-session-manager'\n"
                "\n"
                "";
                puts ( usage );
                exit(0);
                break;
        }
    }

    const char *nsm_url = getenv( "NSM_URL" );

    if ( nsm_url )
    {
        MESSAGE( "Found NSM URL of \"%s\" in environment, attempting to connect.", nsm_url );

        Daemon *d = new Daemon;

        d->url = nsm_url;
        d->addr = lo_address_new_from_url( nsm_url );

        daemon_list.push_back( d );
    }

    if ( controller->init_osc() )
        FATAL( "Could not create OSC server" );

    if ( daemon_list.size() )
    {
        foreach_daemon ( d )
        {
            controller->announce( (*d)->url );
        }
    }
    else
    {
        /* start a new daemon... */
        MESSAGE( "Starting daemon..." );

        char *url = osc->url();

        if ( ! fork() )
        {
            /* pass non-option arguments on to daemon */

            char *args[4 + argc - optind];

            int i = 0;
            args[i++] = strdup("nsmd");
            args[i++] = strdup("--gui-url");
            args[i++] = url;

            for ( ; optind < argc; i++, optind++ )
            {
                MESSAGE( "Passing argument: %s", argv[optind] );
                args[i] = argv[optind];
            }

            args[i] = 0;

            if ( -1 == execvp( "nsmd", args ) )
            {
                FATAL( "Error starting process: %s", strerror( errno ) );
            }
        }

        free(url);
    }

    Fl::add_timeout( 1.0, ping, NULL );
    Fl::run();

    foreach_daemon ( d )
    {
        if ( (*d)->is_child )
        {
            MESSAGE( "Telling server to quit" );
            osc->send( (*d)->addr, "/nsm/server/quit" );
        }
    }

    return 0;
}

