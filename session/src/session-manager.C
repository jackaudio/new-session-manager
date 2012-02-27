
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



#include "OSC/Endpoint.H"

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
#include <FL/Fl_Hold_Browser.H>
#include <FL/Fl_Tile.H>

#include "FL/Fl_Packscroller.H"

#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>

#define APP_NAME "Non Session Manager"

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

class NSM_Client : public Fl_Group
{
    char *_client_id;

//    Fl_Box *client_name;
    Fl_Progress *_progress;
    Fl_Light_Button *_dirty;
    Fl_Button *_remove_button;
    Fl_Button *_restart_button;

public:

    void
    name ( const char *v )
        {
            label( strdup( v ) );
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
    stopped ( bool b )
        {
            if ( b )
            {
                _remove_button->show();
                _restart_button->show();
                color( FL_RED );
                redraw();
            }
            else
            {
                _restart_button->hide();
                _remove_button->hide();
            }

            /* _restart_button->redraw(); */
            /* _remove_button->redraw(); */
        }

    void
    pending_command ( const char *command )
        {
            char *cmd = strdup( command );
            
            free( (void*)_progress->label() );

            _progress->label( cmd );

            stopped( 0 );
                
            if ( ! strcmp( command, "ready" ) )
            {
                color( FL_GREEN );
//                _progress->value( 0.0f );
            }
            else if ( ! strcmp( command, "quit" ) ||
                      ! strcmp( command, "kill" ) ||
                      ! strcmp( command, "error" ) )
            {
                color( FL_RED );
            }
            else if ( ! strcmp( command, "stopped" ) )
            {
                stopped( 1 );
            }
            else
            {
                color( FL_YELLOW );
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
            if ( o == _remove_button )
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
        }
             

    const char *
    client_id ( void )
        { return _client_id; }

    NSM_Client ( int X, int Y, int W, int H, const char *L ) :
        Fl_Group( X, Y, W, H, L )
        {

            _client_id = NULL;
            
            align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE );
            color( FL_RED );
            box( FL_UP_BOX );

            { Fl_Progress *o = _progress = new Fl_Progress( ( X + W ) - ( W / 4) - 20, Y + 5, ( W / 4 ), H - 10, NULL );
                o->label( strdup( "launch" ) );
                o->minimum( 0.0f );
                o->maximum( 1.0f );
            }
            { Fl_Light_Button *o = _dirty = new Fl_Light_Button( _progress->x() - 30, Y + 7, 25, 25 );
                o->box( FL_UP_BOX );
                o->type(0);
                o->color();
                o->selection_color( FL_YELLOW );
                o->value( 0 );
                o->callback( cb_button, this );
            }
            { Fl_Button *o = _remove_button = new Fl_Button( _progress->x() - 60, Y + 7, 25, 25 );
                o->box( FL_UP_BOX );
                o->type(0);
                o->color( FL_RED );
                o->value( 0 );
                o->label( "X" );
                o->tooltip( "Remove" );
                o->hide();
                o->callback( cb_button, this );
            }
            { Fl_Button *o = _restart_button = new Fl_Button( _progress->x() - 90, Y + 7, 25, 25 );
                o->box( FL_UP_BOX );
                o->type(0);
                o->color( FL_GREEN );
                o->value( 0 );
                o->label( "@>" );
                o->tooltip( "Resume" );
                o->hide();
                o->callback( cb_button, this );
            }

            end();
        }
};

void
browser_callback ( Fl_Widget *w, void * )
{
    w->window()->hide();
}

class NSM_Controller : public Fl_Group
{
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

    Fl_Hold_Browser *session_browser;
    
    static void cb_handle ( Fl_Widget *w, void *v )
        {
            ((NSM_Controller*)v)->cb_handle( w );

        }

    void
    cb_handle ( Fl_Widget *w )
        {
            if ( w == abort_button )
            {
                if ( 0 == fl_choice( "Are you sure you want to abort this session? Unsaved changes will be lost.", "Abort", "Cancel", NULL ) )
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
                
                if ( ! name )
                    return;

                MESSAGE( "Sending open for: %s", name );

                foreach_daemon ( d )
                {
                    osc->send( (*d)->addr, "/nsm/server/open", name );
                }
            }
            else if ( w == duplicate_button )
            {
                const char *name = fl_input( "New Session", NULL );
                
                if ( ! name )
                    return;

                MESSAGE( "Sending duplicate for: %s", name );
                foreach_daemon ( d )
                {
                    osc->send( (*d)->addr, "/nsm/server/duplicate", name );
                }
            }
            else if ( w == session_browser )
            {
                const char *name = session_browser->text( session_browser->value());

                /* strip out formatting codes */

                foreach_daemon ( d )
                {
                    osc->send( (*d)->addr, "/nsm/server/open", index( name, ' ' ) + 1 );
                }
            }
            else if ( w == new_button )
            {
                const char *name = fl_input( "New Session", NULL );

                if ( !name )
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
                            o->color( FL_BLACK );
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

                    const char *name = fl_input( "Add Client" );
                    
                    if ( !name )
                        return;

                    lo_address nsm_addr = lo_address_new_from_url( browser->text( browser->value() ) );

                    osc->send( nsm_addr, "/nsm/server/add", name );
                    
                    delete win;
                }
                else
                {
                    const char *name = fl_input( "Add Client" );
                    
                    if ( !name )
                        return;

                    MESSAGE( "Sending add for: %s", name );
                    /* FIXME: user should get to choose which system to do the add on */
                    foreach_daemon ( d )
                    {
                        osc->send( (*d)->addr, "/nsm/server/add", name );
                    }
                }

            }
        }

    void
    ForwardSort( Fl_Browser *b ) {
        for ( int t=1; t<=b->size(); t++ ) {
            for ( int r=t+1; r<=b->size(); r++ ) {
                if ( strcmp(b->text(t), b->text(r)) > 0 ) {
                    b->swap(t,r);
                }
            }
        }
    }
    
    void
    sort_sessions ( void )
        {
            ForwardSort( session_browser );
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
    
    void
    session_name ( const char *name )
        {
            if ( clients_pack->label() )
                free( (char*)clients_pack->label() );
                      
            clients_pack->parent()->label( strdup( name ) );
            
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
            char *s;
            asprintf( &s, "@S18@C3 %s", name );

            for ( int i = 1; i <= session_browser->size(); i++ )
            {
                if ( !strcmp( session_browser->text( i ), s ) )
                {
                    free( s );
                    return;
                }
            }
            
            session_browser->add( s );
            free(s);
        }


    NSM_Controller ( int X, int Y, int W, int H, const char *L ) :
        Fl_Group( X, Y, W, H, L )
        {
            align( FL_ALIGN_RIGHT | FL_ALIGN_CENTER | FL_ALIGN_INSIDE );
            
            { Fl_Pack *o = buttons_pack = new Fl_Pack( X, Y, W, 30 );
                o->type( Fl_Pack::HORIZONTAL );
                o->box( FL_NO_BOX );
                { Fl_Button *o = open_button = new Fl_Button( 0, 0, 80, 50, "Open" );
                    o->box( FL_UP_BOX );
                    o->callback( cb_handle, (void*)this );
                }
                { Fl_Button *o = close_button = new Fl_Button( 0, 0, 80, 50, "Close" );
                    o->box( FL_UP_BOX );
                    o->callback( cb_handle, (void*)this );
                }
                { Fl_Button *o = abort_button = new Fl_Button( 0, 0, 80, 50, "Abort" );
                    o->box( FL_UP_BOX );
                    o->color( FL_RED );
                    o->callback( cb_handle, (void*)this );
                }
                { Fl_Button *o = save_button = new Fl_Button( 0, 0, 80, 50, "Save" );
                    o->box( FL_UP_BOX );
                    o->callback( cb_handle, (void*)this );
                }
                { Fl_Button *o = new_button = new Fl_Button( 0, 0, 80, 50, "New" );
                    o->box( FL_UP_BOX );
                    o->callback( cb_handle, (void*)this );
                }
                { Fl_Button *o = duplicate_button = new Fl_Button( 0, 0, 100, 50, "Duplicate" );
                    o->box( FL_UP_BOX );
                    o->callback( cb_handle, (void*)this );
                }
                { Fl_Button *o = add_button = new Fl_Button( 0, 0, 100, 100, "Add Client" );
                    o->box( FL_UP_BOX );
                    o->callback( cb_handle, (void*)this );
                }

                o->end();
                add(o);
            }
            { Fl_Tile *o = new Fl_Tile( X, Y + 50, W, H - 50 );
                { 
                    Fl_Hold_Browser *o = session_browser = new Fl_Hold_Browser( X, Y + 50, W / 3, H - 50 );
                    o->callback( cb_handle, (void *)this );
                    o->color( fl_darker( FL_GRAY ) );
                    o->selection_color( fl_darker( FL_GREEN ) );
                    o->box( FL_ROUNDED_BOX );
                    o->label( "Sessions" );
                }
                {
                    Fl_Packscroller *o = new Fl_Packscroller(  X + ( W / 3 ), Y + 50, ( W / 3 ) * 2, H - 50 );
                    o->align( FL_ALIGN_TOP );
                    o->labeltype( FL_SHADOW_LABEL );
                    {
                        Fl_Pack *o = clients_pack = new Fl_Pack( X + ( W / 3 ), Y + 50, ( W / 3 ) * 2, H - 50 );
                        o->align( FL_ALIGN_TOP );
                        o->type( Fl_Pack::VERTICAL );
                        o->end();
                    }
                    Fl_Group::current()->resizable( o );
                    o->end();
                }
                resizable( o );
                o->end();
            }

//            Fl_Group::current()->resizable( this );

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
                        fl_alert( "Server is not responding..." );
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
            
            osc->url();

            osc->add_method( "/error", "sis", osc_handler, osc, "msg" );
            osc->add_method( "/reply", "ss", osc_handler, osc, "msg" );
            osc->add_method( "/reply", "s", osc_handler, osc, "" );

            osc->add_method( "/nsm/server/broadcast", NULL, osc_broadcast_handler, osc, "msg" );
            osc->add_method( "/nsm/gui/server_announce", "s", osc_handler, osc, "msg" );
            osc->add_method( "/nsm/gui/gui_announce", "s", osc_handler, osc, "msg" );
            osc->add_method( "/nsm/gui/session/session", "s", osc_handler, osc, "path,display_name" );
            osc->add_method( "/nsm/gui/session/name", "s", osc_handler, osc, "path,display_name" );
            osc->add_method( "/nsm/gui/client/new", "ss", osc_handler, osc, "path,display_name" );
            osc->add_method( "/nsm/gui/client/status", "ss", osc_handler, osc, "path,display_name" );
            osc->add_method( "/nsm/gui/client/switch", "ss", osc_handler, osc, "path,display_name" );
            osc->add_method( "/nsm/gui/client/progress", "sf", osc_handler, osc, "path,display_name" );
            osc->add_method( "/nsm/gui/client/dirty", "si", osc_handler, osc, "path,display_name" );

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

            DMESSAGE( "Relaying broadcast" );

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

            if ( !strcmp( path, "/nsm/gui/session/session" ) &&
                 ! strcmp( types, "s" ) )
            {
                controller->add_session_to_list( &argv[0]->s );
                controller->sort_sessions();
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
                      !strcmp( types, "s" ))
            {
                controller->session_name( &argv[0]->s );
            }
            else if (!strcmp( path, "/error" ) &&
                     !strcmp( types, "sis" ) )
            {
                int err = argv[1]->i;

                if ( err != 0 )
                    fl_alert( "Command %s failed with:\n\n%s", &argv[0]->s, &argv[2]->s );
            }
            else if (!strcmp( path, "/reply" ) && argc && 's' == *types )
            {
                if ( !strcmp( &argv[0]->s, "/nsm/server/list" ) )
                {
                    controller->add_session_to_list( &argv[1]->s );
                    controller->sort_sessions();
                }
                else if ( !strcmp( &argv[0]->s, "/osc/ping" ) )
                {
                    last_ping_response = time( NULL );
                }
                else if ( ! strcmp( types, "ss" ) )
                    MESSAGE( "%s says %s", &argv[0]->s, &argv[1]->s);
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

int
main (int argc, char **argv )
{

    Fl::get_system_colors();
    Fl::scheme( "plastic" );
    Fl::lock();
    
    Fl_Double_Window *main_window;

    {
        Fl_Double_Window *o = main_window = new Fl_Double_Window( 600, 800, APP_NAME );
        {
            main_window->xclass( APP_NAME );

            Fl_Widget *o = controller = new NSM_Controller( 0, 0, main_window->w(), main_window->h(), NULL );
            controller->session_name( "" );

            Fl_Group::current()->resizable(o);
        }
        o->end();

        o->size_range( main_window->w(), controller->min_h(), 0, 0 );

//        o->callback( (Fl_Callback*)cb_main, main_window );
        o->show( argc, argv );
        // o->show();
    }


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
                DMESSAGE( "Adding %s to daemon list", optarg );
                Daemon *d = new Daemon;
                
                d->url = optarg;
                d->addr = lo_address_new_from_url( optarg );

                daemon_list.push_back( d );
                break;
            }
            case 'h':
                printf( "Usage: %s [--nsmd-url...] [-- server options ]\n\n", argv[0] );
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

            option_index += 2;

            char **args = (char **)malloc( 4 + argc - option_index );

            int i = 0;
            args[i++] = (char*)"nsmd";
            args[i++] = (char*)"--gui-url";
            args[i++] = url;
            

            for ( ; option_index < argc; i++, option_index++ )
            {
                DMESSAGE( "Passing argument: %s", argv[option_index] );
                args[i] = argv[option_index];
            }

            args[i] = 0;

            if ( -1 == execvp( "nsmd", args ) )
            {
                FATAL( "Error starting process: %s", strerror( errno ) );
            }
        }
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














