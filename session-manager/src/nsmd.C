
/*******************************************************************************/
/* Copyright (C) 2010 Jonathan Moore Liles                                     */
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

#define __MODULE__ "nsmd" 

#include "debug.h"

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <errno.h>
#include <string.h>
#include <list>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>
#include <libgen.h>
#include <dirent.h>
#include <ftw.h>
#include <list>
#include <getopt.h>
#include <sys/time.h>

#include <OSC/Endpoint.H>
/* for locking */
#include "file.h"

#include <map>
#include <string>
#include <algorithm>

#pragma GCC diagnostic ignored "-Wunused-parameter"

static OSC::Endpoint *osc_server;
static lo_address gui_addr;
static bool gui_is_active = false;
static int signal_fd;

static int session_lock_fd = 0;
static char *session_root;

#define NSM_API_VERSION_MAJOR 1
#define NSM_API_VERSION_MINOR 0

#define ERR_OK 0
#define ERR_GENERAL_ERROR    -1
#define ERR_INCOMPATIBLE_API -2
#define ERR_BLACKLISTED      -3
#define ERR_LAUNCH_FAILED    -4
#define ERR_NO_SUCH_FILE     -5
#define ERR_NO_SESSION_OPEN  -6
#define ERR_UNSAVED_CHANGES  -7
#define ERR_NOT_NOW          -8
#define ERR_BAD_PROJECT      -9
#define ERR_CREATE_FAILED    -10
#define ERR_SESSION_LOCKED   -11
#define ERR_OPERATION_PENDING -12

#define APP_TITLE "Non Session Manager"

enum {
    COMMAND_NONE = 0,
    COMMAND_QUIT,
    COMMAND_KILL,
    COMMAND_SAVE,
    COMMAND_OPEN,
    COMMAND_START,

    COMMAND_CLOSE,
    COMMAND_DUPLICATE,
    COMMAND_NEW
};

static int pending_operation = COMMAND_NONE;

static void wait ( long );


#define GUIMSG( fmt, args... ) \
{ \
    MESSAGE( fmt, ## args ); \
    if ( gui_is_active ) \
    { \
        char *s;\
        asprintf( &s, fmt, ## args );\
        osc_server->send( gui_addr, "/nsm/gui/server/message", s);\
        free(s);\
    }\
}

struct Client
{
private:

    int _reply_errcode;
    char *_reply_message;

    int _pending_command;                /*  */
    struct timeval _command_sent_time;

    bool _gui_visible;

    char *_label;

public:

    lo_address addr;                   /*  */
    char *name;                        /* client application name */
    char *executable_path;             /* path to client executable */
    int pid;                           /* PID of client process */
    float progress;                    /*  */
    bool active;              /* client has registered via announce */
//    bool stopped; /* the client quit, but not because we told it to--user still has to decide to remove it from the session */
    char *client_id;                                            /* short part of client ID */
    char *capabilities;                                         /* client capabilities... will be null for dumb clients */
    bool dirty;                                                 /* flag for client self-reported dirtiness */
    bool pre_existing;
    const char *status;

    const char *label ( void ) const { return _label; }
    void label ( const char *l )
        {
            if ( _label ) 
                free( _label );
            if ( l )
                _label = strdup( l ); 
            else
                _label = NULL;
        }
        
    bool gui_visible ( void ) const
        {
            return _gui_visible;
        }

    void gui_visible ( bool b ) 
        {
            _gui_visible = b;
        }

    bool
    has_error ( void ) const
        {
            return _reply_errcode != 0;
        }

    int
    error_code ( void ) const
        {
            return _reply_errcode;
        }

    const char * message ( void )
        {
            return _reply_message;
        }

    void
    set_reply ( int errcode, const char *message )
        {
            if ( _reply_message )
                free( _reply_message );

            _reply_message = strdup( message );
            _reply_errcode = errcode;
        }

    bool reply_pending ( void )
        {
            return _pending_command != COMMAND_NONE;
        }

    bool is_dumb_client ( void )
        {
            return capabilities == NULL;
        }

    void pending_command ( int command )
        {
            gettimeofday( &_command_sent_time, NULL );
            _pending_command = command;
        }
    
    double milliseconds_since_last_command ( void ) const
        {
            struct timeval now;

            gettimeofday( &now, NULL );

            double elapsedms = ( now.tv_sec - _command_sent_time.tv_sec ) * 1000.0;
            elapsedms += ( now.tv_usec - _command_sent_time.tv_usec ) / 1000.0;

            return elapsedms;
        }

    int pending_command ( void )
        {
            return _pending_command;
        }

// capability should be enclosed in colons. I.e. ":switch:"
    bool 
    is_capable_of ( const char *capability ) const
        {
            return capabilities &&
                strstr( capabilities, capability );
        }

    Client ( )
        {
            _label = 0;
            _gui_visible = true;
            addr = 0;
            _reply_errcode = 0;
            _reply_message = 0;
            pid = 0;
            progress = -0;
            _pending_command = 0;
            active = false;
            client_id = 0;
            capabilities = 0;
            name = 0;
            executable_path = 0;
            pre_existing = false;
        }

    ~Client ( ) 
        {
            if ( name )
                free(name);
            if (executable_path)
                free(executable_path);
            if (client_id)
                free(client_id);
            if (capabilities)
                free(capabilities);

            name = executable_path = client_id = capabilities = NULL;
        }
};


static std::list< Client* > client;

/* helper macros for defining OSC handlers */
#define OSC_NAME( name ) osc_ ## name
// #define OSCDMSG() DMESSAGE( "Got OSC message: %s", path );
#define OSC_HANDLER( name ) static int OSC_NAME( name ) ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )

static char *session_path = NULL;
static char *session_name = NULL;

bool
clients_have_errors ( )
{
    for ( std::list<Client*>::const_iterator i = client.begin();
          i != client.end();
          ++i )
        if ( (*i)->active && (*i)->has_error() )
            return true;

    return false;
}

Client *
get_client_by_pid ( int pid )
{
    std::list<Client*> *cl = &client;

    for ( std::list<Client*>::const_iterator i = cl->begin();
          i != cl->end();
          ++i )
        if ( (*i)->pid == pid )
            return *i;
    
    return NULL;
}

void clear_clients ( void )
{
    std::list<Client*> *cl = &client;

    for ( std::list<Client*>::iterator i = cl->begin();
          i != cl->end();
          ++i )
    {
        delete *i;
        i = cl->erase( i );
    }
}


void
handle_client_process_death ( int pid )
{
    Client *c = get_client_by_pid( (int)pid );
        
    if ( c )
    {
        bool dead_because_we_said = ( c->pending_command() == COMMAND_KILL ||
                                      c->pending_command() == COMMAND_QUIT );

        if ( dead_because_we_said )
        {
            GUIMSG( "Client %s terminated because we told it to.", c->name );
        }
        else
        {
            GUIMSG( "Client %s died unexpectedly.", c->name );
        }

        if ( c->pending_command() == COMMAND_QUIT )
        {
            if ( gui_is_active )
                osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "removed" );

            client.remove(c);
            delete c;
        }
        else
        {
            if ( gui_is_active )
                osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "stopped" );
        }
      
        c->pending_command( COMMAND_NONE );
            
        c->active = false;
        c->pid = 0;
     }
}

void handle_sigchld ( )
{
    for ( ;; )
    {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);

        if (pid <= 0) 
            break;
        
        handle_client_process_death( pid );
    }
}


int
path_is_valid ( const char *path )
{
    char *s;

    asprintf( &s, "/%s/", path );

    int r = strstr( s, "/../" ) == NULL;

    free( s );

    return r;
}

int
mkpath ( const char *path, bool create_final_directory )
{
    char *p = strdup( path );

    char *i = p + 1;

    while ( ( i = index( i, '/' ) ) )
    {
        *i = 0;

        struct stat st;

        if ( stat( p, &st ) )
        {
            if ( mkdir( p, 0711 ) )
            {
                free( p );
                return -1;
            }
        }

        *i = '/';
        i++;
    } 

    if ( create_final_directory )
    {
        if ( mkdir( p, 0711 ) )
        {
            free( p );
            return -1;
        }
    }
    
    free( p );

    return 0;
}

void
set_name ( const char *name )
{
    if ( session_name )
        free( session_name );

    char *s = strdup( name );
    
    session_name = strdup( basename( s ) );

    free( s );
}

bool
address_matches ( lo_address addr1, lo_address addr2 )
{
    /* char *url1 = lo_address_get_url( addr1 ); */
    /* char *url2 = lo_address_get_url( addr2 ); */

    char *url1 = strdup( lo_address_get_port( addr1 ) );
    char *url2 = strdup(lo_address_get_port( addr2 ) );

    bool r = !strcmp( url1, url2 );

    free( url1 );
    free( url2 );

    return r;
}
 
Client *
get_client_by_id ( std::list<Client*> *cl, const char *id )
{
    for ( std::list<Client*>::const_iterator i = cl->begin();
          i != cl->end();
          ++i )
        if ( !strcmp( (*i)->client_id, id ) )
            return *i;

    return NULL;
}


Client *
get_client_by_name_and_id ( std::list<Client*> *cl, const char *name, const char *id )
{
    for ( std::list<Client*>::const_iterator i = cl->begin();
          i != cl->end();
          ++i )
        if ( !strcmp( (*i)->client_id, id ) && 
             ! strcmp( (*i)->name, name ) )
            return *i;

    return NULL;
}

Client *
get_client_by_address ( lo_address addr )
{
    for ( std::list<Client*>::iterator i = client.begin();
          i != client.end();
          ++i )
        if ( (*i)->addr && address_matches( (*i)->addr, addr ) )
            return *i;

    return NULL;
}


char *
generate_client_id ( Client *c ) 
{
    char id_str[6];

    id_str[0] = 'n';
    id_str[5] = 0;

    for ( int i = 1; i < 5; i++)
        id_str[i] = 'A' + (rand() % 25);

    return strdup(id_str);
}


bool
replies_still_pending ( void )
{
    for ( std::list<Client*>::const_iterator i = client.begin();
          i != client.end();
          ++i )
        if ( (*i)->active && (*i)->reply_pending() )
            return true;

    return false;
}

int
number_of_active_clients ( void )
{
    int active = 0;
    for ( std::list<Client*>::const_iterator i = client.begin(); i != client.end(); i++ )
    {
        if ( (*i)->active )
            active++;
    }

    return active;
}

void
wait_for_announce ( void )
{
    GUIMSG( "Waiting for announce messages from clients" );

    int n = 5 * 1000;
    
    long unsigned int active;

    while ( n > 0 )
    {
        n -= 100;

        wait(100);
        
        active = number_of_active_clients();

        if ( client.size() == active )
            break;
    }

    GUIMSG( "Done. %lu out of %lu clients announced within the initialization grace period", active, client.size() );
}

void
wait_for_replies ( void )
{    

    GUIMSG( "Waiting for clients to reply to commands" );

    int n = 60 * 1000;                                          /* 60 seconds */

    while ( n )
    {
        n -= 100;

        wait(100);
        
        if ( ! replies_still_pending() )
            break;
    }

    GUIMSG( "Done waiting" );
    /* FIXME: do something about unresponsive clients */
}


char * 
get_client_project_path  ( const char *session_path, Client *c )
{
    char *client_project_path;

    asprintf( &client_project_path, "%s/%s.%s", session_path, c->name, c->client_id );

    return client_project_path;
}

bool
launch ( const char *executable, const char *client_id )
{
    Client *c;

    if ( !client_id || !( c = get_client_by_id( &client, client_id ) ) )
    {
        c = new Client();

        c->executable_path = strdup( executable );

        {
            char *s = strdup( c->executable_path );
        
            c->name = strdup( basename( s ) );
        
            free( s );
        }
    
        if ( client_id )
            c->client_id = strdup( client_id );
        else
            c->client_id = generate_client_id( c );

        client.push_back( c );
    }

    char * url = osc_server->url();

    int pid;
    if ( ! (pid = fork()) )
    {
        GUIMSG( "Launching %s", executable );

        char *args[] = { strdup( executable ), NULL };

        setenv( "NSM_URL", url, 1 );

        /* Ensure the launched process can receive SIGCHLD */
        /* Unblocking SIGCHLD here does NOT unblock it for nsmd itself */
        sigset_t mask;
        sigemptyset( &mask );
        sigaddset( &mask, SIGCHLD );
        sigprocmask(SIG_UNBLOCK, &mask, NULL );
        
        if ( -1 == execvp( executable, args ) )
        {
            WARNING( "Error starting process: %s", strerror( errno ) );

            exit(-1);
        }
    }

    c->pending_command( COMMAND_START );
    c->pid = pid;

    MESSAGE( "Process has pid: %i", pid );

    if ( gui_is_active )
    {
        osc_server->send( gui_addr, "/nsm/gui/client/new", c->client_id, c->name );
        osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "launch" );
    }

    return true;
}

void
command_client_to_save ( Client *c )
{
    if ( c->active )
    {
        MESSAGE( "Telling %s to save", c->name );
        osc_server->send( c->addr, "/nsm/client/save" );
        
        c->pending_command( COMMAND_SAVE );

        if ( gui_is_active )
            osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "save" );
    }
    else if ( c->is_dumb_client() && c->pid )
    {
        // this is a dumb client... 
        if ( gui_is_active )
            osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "noop" );
    }
}

void command_client_to_switch ( Client *c, const char *new_client_id )
{
    char *old_client_id = c->client_id;

    c->client_id = strdup( new_client_id );
 
    char *client_project_path = get_client_project_path( session_path, c ); 
    
    MESSAGE( "Commanding %s to switch \"%s\"", c->name, client_project_path );
                
    char *full_client_id;
    asprintf( &full_client_id, "%s.%s", c->name, c->client_id );

    osc_server->send( c->addr, "/nsm/client/open", client_project_path, session_name, full_client_id );
    
    free( full_client_id );
    free( client_project_path );
    
    c->pending_command( COMMAND_OPEN );

    if ( gui_is_active )
    {
        osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "switch" );
        osc_server->send( gui_addr, "/nsm/gui/client/switch", old_client_id, c->client_id );
    }

    free( old_client_id );
}

void
purge_inactive_clients ( )
{
    for ( std::list<Client*>::iterator i = client.begin();
          i != client.end();
          ++i )
    {
        if ( ! (*i)->active )
        {
            if ( gui_is_active )
                osc_server->send( gui_addr, "/nsm/gui/client/status", (*i)->client_id, (*i)->status = "removed" );

            delete *i;

            i = client.erase( i );
        }
    }
}

bool
process_is_running ( int pid )
{
    if ( 0 == kill( pid, 0 ) )
    {
        return true;
    }
    else if ( ESRCH == errno )
    {
        return false;
    }

    return false;
}

void
purge_dead_clients ( )
{
    std::list<Client*> tmp( client );

    for ( std::list<Client*>::const_iterator i = tmp.begin();
          i != tmp.end();
          ++i )
    {
        const Client *c = *i;
        if ( c->pid )
        {
            if ( ! process_is_running( c->pid ) )
                handle_client_process_death( c->pid );
        }
    }
}

/************************/
/* OSC Message Handlers */
/************************/

OSC_HANDLER( add )
{
    if ( ! session_path )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          ERR_NO_SESSION_OPEN,
                          "Cannot add to session because no session is loaded." );
                          

        return 0;
    }

    if ( strchr( &argv[0]->s, '/' ) )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          ERR_LAUNCH_FAILED,
                          "Absolute paths are not permitted. Clients must be in $PATH" );        
        return 0;
    }

    if ( ! launch( &argv[0]->s, NULL ) )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          ERR_LAUNCH_FAILED,
                          "Failed to launch process!" );
    }
    else
    {
        osc_server->send( lo_message_get_source( msg ), "/reply", path,
                          ERR_OK,
                          "Launched." );
    }
 
    return 0;
}

OSC_HANDLER( announce )
{
    const char *client_name = &argv[0]->s;
    const char *capabilities = &argv[1]->s;
    const char *executable_path = &argv[2]->s;
    int major = argv[3]->i;
    int minor = argv[4]->i;
    int pid = argv[5]->i;

    GUIMSG( "Got announce from %s", client_name );

    if ( ! session_path )
    {
        osc_server->send( lo_message_get_source( msg ), "/error",
                          path,
                          ERR_NO_SESSION_OPEN,
                          "Sorry, but there's no session open for this application to join." );
        return 0;
    }

    bool expected_client = false;

    Client *c = NULL;

    for ( std::list<Client*>::iterator i = client.begin();
          i != client.end();
          ++i )
    {
        if ( ! strcmp( (*i)->executable_path, executable_path ) &&
             ! (*i)->active &&
             (*i)->pending_command() == COMMAND_START )
        {
            // I think we've found the slot we were looking for.
            MESSAGE( "Client was expected." );
            c = *i;
            break;
        }
    } 
    
    if ( ! c )
    {
        c = new Client();
        c->executable_path = strdup( executable_path );
        c->client_id = generate_client_id( c );
    }
    else
        expected_client = true;
   
    if ( major > NSM_API_VERSION_MAJOR )
    {
        MESSAGE( "Client is using incompatible and more recent API version %i.%i", major, minor ); 

        osc_server->send( lo_message_get_source( msg ), "/error",
                          path,
                          ERR_INCOMPATIBLE_API,
                          "Server is using an incompatible API version." );

        return 0;
    }

    c->pid = pid;
    c->capabilities = strdup( capabilities );
    c->addr = lo_address_new_from_url( lo_address_get_url( lo_message_get_source( msg ) ));
    c->name = strdup( client_name );
    c->active = true;

    MESSAGE( "Process has pid: %i", pid );
    
    if ( ! expected_client )
        client.push_back( c );

    MESSAGE( "The client \"%s\" at \"%s\" informs us it's ready to receive commands.", &argv[0]->s, lo_address_get_url( c->addr ) );

    osc_server->send( lo_message_get_source( msg ), "/reply",
                      path,
                      expected_client ? 
                      "Howdy, what took you so long?" :
                      "Well hello, stranger. Welcome to the party.",
                      APP_TITLE,
                      ":server-control:broadcast:optional-gui:" );

    if ( gui_is_active )
    {
        osc_server->send( gui_addr, "/nsm/gui/client/new", c->client_id, c->name );
        osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "open" );

        if ( c->is_capable_of( ":optional-gui:" ) )
            osc_server->send( gui_addr, "/nsm/gui/client/has_optional_gui", c->client_id );
    }

    {
        char *full_client_id;
        asprintf( &full_client_id, "%s.%s", c->name, c->client_id );
        
        char *client_project_path = get_client_project_path( session_path, c );
        
        osc_server->send( lo_message_get_source( msg ), "/nsm/client/open", client_project_path, session_name, full_client_id );
        
        c->pending_command( COMMAND_OPEN );

        free( full_client_id );
        free( client_project_path );
    }

    return 0;
}

void
save_session_file ( )
{
    char *session_file = NULL;
    asprintf( &session_file, "%s/session.nsm", session_path );
  
    FILE *fp = fopen( session_file, "w+" );

    free( session_file );
    
    /* FIXME: handle errors. */

    for ( std::list<Client*>::iterator i = client.begin();
          i != client.end();
          ++i )
    {
        fprintf( fp, "%s:%s:%s\n", (*i)->name, (*i)->executable_path, (*i)->client_id );
    } 

    fclose( fp );
}

Client *
client_by_name ( const char *name,
                 std::list<Client*> *cl )
{
    for ( std::list<Client*>::iterator i = cl->begin();
          i != cl->end();
          ++i )
    {
        if ( !strcmp( name, (*i)->name ) )
            return *i;
    }

    return NULL;
}

bool
dumb_clients_are_alive ( )
{
    std::list<Client*> *cl = &client;
    
    for ( std::list<Client*>::iterator i = cl->begin();
          i != cl->end();
          ++i )
    {
        if ( (*i)->is_dumb_client() && (*i)->pid > 0 )
            return true;
    }

    return false;
}

void
wait_for_dumb_clients_to_die ( )
{
    struct signalfd_siginfo fdsi;
    
    GUIMSG( "Waiting for any dumb clients to die." );
    
    for ( int i = 0; i < 6; i++ )
    {
        MESSAGE( "Loop %i", i );

        if ( ! dumb_clients_are_alive() )
            break;
        
        ssize_t s = read(signal_fd, &fdsi, sizeof(struct signalfd_siginfo));
        
        if (s == sizeof(struct signalfd_siginfo))
        {
            if (fdsi.ssi_signo == SIGCHLD)
                handle_sigchld();
        }
        
        usleep( 50000 );
    }
    
    GUIMSG( "Done waiting" );

    /* FIXME: give up on remaining clients and purge them */
}


bool
killed_clients_are_alive ( )
{
    std::list<Client*> *cl = &client;
    
    for ( std::list<Client*>::iterator i = cl->begin();
          i != cl->end();
          ++i )
    {
        if ( ( (*i)->pending_command() == COMMAND_QUIT ||
               (*i)->pending_command() == COMMAND_KILL ) &&
             (*i)->pid > 0 )
            return true;
    }

    return false;
}

void
wait_for_killed_clients_to_die ( )
{
    struct signalfd_siginfo fdsi;
    
    MESSAGE( "Waiting for killed clients to die." );
    
    for ( int i = 0; i < 60; i++ )
    {
        MESSAGE( "Loop %i", i );

        if ( ! killed_clients_are_alive() )
            goto done;
        
        ssize_t s = read(signal_fd, &fdsi, sizeof(struct signalfd_siginfo));
        
        if (s == sizeof(struct signalfd_siginfo))
        {
            if (fdsi.ssi_signo == SIGCHLD)
                handle_sigchld();
        }
        
        purge_dead_clients();

        /* check OSC so we can get /progress messages. */
        osc_server->check();

        sleep(1);
    }

    WARNING( "Killed clients are still alive" );
    
    return;

done:

    MESSAGE( "All clients have died." );
}


void
command_all_clients_to_save ( )
{
    if ( session_path )
    {
        GUIMSG( "Commanding attached clients to save." );
        
        for ( std::list<Client*>::iterator i = client.begin();
              i != client.end();
              ++i )
        {
            command_client_to_save( *i );
        }
    
        wait_for_replies();
    
        save_session_file();
    }
}

void
command_client_to_stop ( Client *c )
{
    GUIMSG( "Stopping client %s", c->name );

    if ( c->pid > 0 )
    {
        c->pending_command( COMMAND_KILL );

        kill( c->pid, SIGTERM );

        if ( gui_is_active )
            osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "stopped" );
    }
}

void
command_client_to_quit ( Client *c )
{
    MESSAGE( "Commanding %s to quit", c->name );
    
    if ( c->active )
    {
        c->pending_command( COMMAND_QUIT );

        kill( c->pid, SIGTERM );

        if ( gui_is_active )
            osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "quit" );
    }
    else if ( c->is_dumb_client() )
    {
        if ( c->pid > 0 )
        {
            if ( gui_is_active )
                osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "quit" );
            
            /* should be kill? */
            c->pending_command( COMMAND_QUIT );
    
            // this is a dumb client... try and kill it
            kill( c->pid, SIGTERM );
        }
        else
        {
            if ( gui_is_active )
                osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "removed" );
        }
    }
}

void
close_session ( )
{
    if ( ! session_path )
        return;

    for ( std::list<Client*>::iterator i = client.begin();
          i != client.end();
          ++i )
    {
        command_client_to_quit( *i );
    }

    wait_for_killed_clients_to_die();

    purge_inactive_clients();
    
    clear_clients();

    if ( session_path )
    {
        char *session_lock;
        asprintf( &session_lock, "%s/.lock", session_path );

        release_lock( &session_lock_fd, session_lock );

        free(session_lock);

        free(session_path);
        session_path = NULL;
        free(session_name);
        session_name = NULL;
    }

    if ( gui_is_active )
    {
        osc_server->send( gui_addr, "/nsm/gui/session/name", "", "" );
    }
}

void
tell_client_session_is_loaded( Client *c )
{
    if ( c->active )
//!c->is_dumb_client() )
    {
        MESSAGE( "Telling client %s that session is loaded.", c->name );
        osc_server->send( c->addr, "/nsm/client/session_is_loaded" );
    }
}

void
tell_all_clients_session_is_loaded ( void )
{
    MESSAGE( "Telling all clients that session is loaded..." );

   for ( std::list<Client*>::iterator i = client.begin();
          i != client.end();
          ++i )
    {
        tell_client_session_is_loaded( *i );
    }
}

int
load_session_file ( const char * path )
{
    char *session_file = NULL;
    asprintf( &session_file, "%s/session.nsm", path );
    char *session_lock = NULL;
    asprintf( &session_lock, "%s/.lock", path );
    
    if ( ! acquire_lock( &session_lock_fd, session_lock ) )
    {
        free( session_file );
        free( session_lock );

        WARNING( "Session is locked by another process" );
        return ERR_SESSION_LOCKED;
    }


    FILE *fp;

    if ( ! ( fp = fopen( session_file, "r" ) ) )
    {
        free( session_file );
        return ERR_CREATE_FAILED;
    }

    free( session_file );
    
    session_path = strdup( path );

    set_name( path );

    std::list<Client*> new_clients;

    {
        char * client_name = NULL;
        char * client_executable = NULL;
        char * client_id = NULL;
        
        // load the client list
        while ( fscanf( fp, "%m[^:]:%m[^:]:%m[^:\n]\n", &client_name, &client_executable, &client_id ) > 0 )
        {
            Client *c = new Client();
            
            c->name = client_name;
            c->executable_path = client_executable;
            c->client_id =  client_id;
            
            new_clients.push_back( c );
        }
    }

    fclose(fp);
    
    MESSAGE( "Commanding unneeded and dumb clients to quit" );
    
    std::map<std::string,int> client_map;

    /* count how many instances of each client are needed in the new session */
    for ( std::list<Client*>::iterator i = new_clients.begin();
          i != new_clients.end();
          ++i )
    {
        if ( client_map.find( (*i)->name) != client_map.end() )
            client_map[(*i)->name]++;
        else
            client_map[(*i)->name] = 1;
    }

    for ( std::list<Client*>::iterator i = client.begin();
          i != client.end();
          ++i )
    {
        if ( ! (*i)->is_capable_of( ":switch:" ) || client_map.find((*i)->name ) == client_map.end()  )
        {
            /* client is not capable of switch, or is not wanted in the new session */
            command_client_to_quit( *i );
        }
        else
        {
            /* client is switch capable and may be wanted in the new session */
            if ( client_map[ (*i)->name ]-- <= 0 )
                /* nope,, we already have as many as we need, stop this one */
                command_client_to_quit( *i );
        }
        
    }

//    wait_for_replies();

    wait_for_killed_clients_to_die();

//    wait_for_dumb_clients_to_die();

    purge_inactive_clients();

    for ( std::list<Client*>::iterator i = client.begin();
          i != client.end();
          ++i )
    {
        (*i)->pre_existing = true;
    }    

    MESSAGE( "Commanding smart clients to switch" );

    for ( std::list<Client*>::iterator i = new_clients.begin();
          i != new_clients.end();
          ++i )
    {
        Client *c = NULL;
            
        /* in a duplicated session, clients will have the same
         * IDs, so be sure to pick the right one to avoid race
         * conditions in JACK name registration. */
        c = get_client_by_name_and_id( &client, (*i)->name, (*i)->client_id );
        
        if ( ! c )
            c = client_by_name( (*i)->name, &client );
        
        if ( c && c->pre_existing && !c->reply_pending() )
        {
            // since we already shutdown clients not capable of 'switch', we can assume that these are.
            command_client_to_switch( c, (*i)->client_id );
        }
        else
        {
            /* sleep a little bit because liblo derives its sequence
             * of port numbers from the system time (second
             * resolution) and if too many clients start at once they
             * won't be able to find a free port. */
            usleep( 100 * 1000 );

            launch( (*i)->executable_path, (*i)->client_id );
        }
    }

    /* this part is a little tricky... the clients need some time to
     * send their 'announce' messages before we can send them 'open'
     * and know that a reply is pending and we should continue waiting
     * until they finish. wait_for_replies() must check for OSC
     * messages immediately, even if no replies seem to be pending
     * yet. */

    /* dumb clients will never send an 'announce message', so we need
     * to give up waiting on them fairly soon. */

    wait_for_announce();

    wait_for_replies();

    tell_all_clients_session_is_loaded();

    MESSAGE( "Loaded." );

    new_clients.clear();

    if ( gui_is_active )
    {
        osc_server->send( gui_addr, "/nsm/gui/session/name", session_name, session_path + strlen( session_root ));
    }
    
    return ERR_OK;
}

OSC_HANDLER( save )
{
    if ( pending_operation != COMMAND_NONE )
    {
            osc_server->send( lo_message_get_source( msg ), "/error", path,
                              ERR_OPERATION_PENDING,
                              "An operation pending." );
            return 0;
    }

    if ( ! session_path )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path, 
                          ERR_NO_SESSION_OPEN,
                          "No session to save.");
                          
        goto done;
    }
    
    command_all_clients_to_save();

    MESSAGE( "Done." );

    osc_server->send( lo_message_get_source( msg ), "/reply", path, "Saved." );

done:

    pending_operation = COMMAND_NONE;

    return 0;
}

OSC_HANDLER( duplicate )
{
    if ( pending_operation != COMMAND_NONE )
    {
            osc_server->send( lo_message_get_source( msg ), "/error", path,
                              ERR_OPERATION_PENDING,
                              "An operation pending." );
            return 0;
    }

    pending_operation = COMMAND_DUPLICATE;

    if ( ! session_path )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path, 
                          ERR_NO_SESSION_OPEN,
                          "No session to duplicate.");
        goto done;
    }

    if ( ! path_is_valid( &argv[0]->s ) )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          ERR_CREATE_FAILED,
                          "Invalid session name." );

        goto done;
    }     

    command_all_clients_to_save();
    
    if ( clients_have_errors() )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          ERR_GENERAL_ERROR,
                          "Some clients could not save" );

        goto done;
    }

//    save_session_file();

    char *spath;
    asprintf( &spath, "%s/%s", session_root, &argv[0]->s );

    mkpath( spath, false );

    /* FIXME: code a recursive copy instead of calling the shell */

    char *cmd;
    asprintf( &cmd, "cp -R \"%s\" \"%s\"", session_path, spath);

    system( cmd );

    free( cmd );

    osc_server->send( gui_addr,  "/nsm/gui/session/session", &argv[0]->s  );

    MESSAGE( "Attempting to open %s", spath );

    if ( !load_session_file( spath ) )
    {
        MESSAGE( "Loaded" );
        osc_server->send( lo_message_get_source( msg ), "/reply", path,
                          "Loaded." );
    }
    else
    {
        MESSAGE( "Failed" );
        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          ERR_NO_SUCH_FILE,
                          "No such file." );
        free(spath);
        return -1;
    }

    free( spath );
 
    MESSAGE( "Done" );

    osc_server->send( lo_message_get_source( msg ), "/reply", path, "Duplicated." );

done:

    pending_operation = COMMAND_NONE;

    return 0;
}

OSC_HANDLER( new )
{
    if ( pending_operation != COMMAND_NONE )
    {
            osc_server->send( lo_message_get_source( msg ), "/error", path,
                              ERR_OPERATION_PENDING,
                              "An operation pending." );
            return 0;
    }

    pending_operation = COMMAND_NEW;

    if ( ! path_is_valid( &argv[0]->s ) )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          ERR_CREATE_FAILED,
                          "Invalid session name." );

        pending_operation = COMMAND_NONE;

        return 0;
    }     

    if ( session_path )
    {
        command_all_clients_to_save();
        
        close_session();
    }

    GUIMSG( "Creating new session \"%s\"", &argv[0]->s );

    char *spath;
    asprintf( &spath, "%s/%s", session_root, &argv[0]->s );
   
    if ( mkpath( spath, true ) )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          ERR_CREATE_FAILED,
                          "Could not create the session directory" );

        free(spath);

        pending_operation = COMMAND_NONE;

        return 0;
    }

    session_path = strdup( spath );
    
    set_name( session_path );

    osc_server->send( lo_message_get_source( msg ), "/reply", path, "Created." );

    if ( gui_is_active )
    {
        osc_server->send( gui_addr,  "/nsm/gui/session/session", &argv[0]->s  );

        osc_server->send( gui_addr, "/nsm/gui/session/name", &argv[0]->s, &argv[0]->s );
    }

    save_session_file();

    free( spath );

    osc_server->send( lo_message_get_source( msg ), "/reply", path,
                              "Session created" );

    pending_operation = COMMAND_NONE;
        
    return 0;
}

static lo_address list_response_address;

int
list_file ( const char *fpath, const struct stat *sb, int tflag )
{
    char *s;

    if ( tflag == FTW_F )
    {
        s = strdup( fpath );
        if ( ! strcmp( "session.nsm", basename( s ) ) )
        {
            free( s );
            s = strdup( fpath );

            s = dirname( s );

            memmove( s, s + strlen( session_root ) + 1, (strlen( s ) - strlen( session_root )) + 1);

            osc_server->send( list_response_address, "/reply", "/nsm/server/list", s  );

            free( s );
        }
        else
            free( s );
    }

    return 0;
}

OSC_HANDLER( list )
{
    GUIMSG( "Listing sessions" );

    list_response_address = lo_message_get_source( msg );
    
    ftw( session_root, list_file, 20 );

    osc_server->send( lo_message_get_source( msg ), path,
                      ERR_OK,
                      "Done." );
    return 0;
}

OSC_HANDLER( open )
{
    GUIMSG( "Opening session %s", &argv[0]->s );
    
    if ( pending_operation != COMMAND_NONE )
    {
            osc_server->send( lo_message_get_source( msg ), "/error", path,
                              ERR_OPERATION_PENDING,
                              "An operation pending." );
            return 0;
    }

    pending_operation = COMMAND_OPEN;


    if ( session_path )
    {

        command_all_clients_to_save();
        
        if ( clients_have_errors() )
        {
            osc_server->send( lo_message_get_source( msg ), "/error", path,
                              ERR_GENERAL_ERROR,
                              "Some clients could not save" );

            pending_operation = COMMAND_NONE;
            return 0;
        }
        
//        save_session_file();
    }


    char *spath;
    asprintf( &spath, "%s/%s", session_root, &argv[0]->s );

    MESSAGE( "Attempting to open %s", spath );

    int err = load_session_file( spath );
        
    if ( ! err )
    {
        MESSAGE( "Loaded" );
        osc_server->send( lo_message_get_source( msg ), "/reply", path,
                          "Loaded." );
    }
    else
    {
        MESSAGE( "Failed" );
        const char *m = NULL;
        switch ( err )
        {
            case ERR_CREATE_FAILED:
                m = "Could not create session file!";
                break;
            case ERR_SESSION_LOCKED:
                m = "Session is locked by another process!";
                break;
            case ERR_NO_SUCH_FILE:
                m = "The named session does not exist.";
                break;
            default:
                m = "Unknown error";
        }


        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          err,
                          m );
    }

    free( spath );
 
    MESSAGE( "Done" );

    pending_operation = COMMAND_NONE;

    return 0;
}


OSC_HANDLER( quit )
{
    close_session();

    exit(0);

    return 0;
}

OSC_HANDLER( abort )
{
  if ( pending_operation != COMMAND_NONE )
    {
            osc_server->send( lo_message_get_source( msg ), "/error", path,
                              ERR_OPERATION_PENDING,
                              "An operation pending." );
            return 0;
    }
  
  pending_operation = COMMAND_CLOSE;


    if ( ! session_path )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          ERR_NO_SESSION_OPEN,
                          "No session to abort." );

        goto done;
    }

    GUIMSG( "Commanding attached clients to quit." );

    close_session();

    osc_server->send( lo_message_get_source( msg ), "/reply", path,
                      "Aborted." );
                      
    MESSAGE( "Done" );

done:

    pending_operation = COMMAND_NONE;

    return 0;
}

OSC_HANDLER( close )
{
    if ( pending_operation != COMMAND_NONE )
    {
            osc_server->send( lo_message_get_source( msg ), "/error", path,
                              ERR_OPERATION_PENDING,
                              "An operation pending." );
            return 0;
    }

    pending_operation = COMMAND_CLOSE;

    if ( ! session_path )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          ERR_NO_SESSION_OPEN,
                          "No session to close." );

        goto done;
    }
    command_all_clients_to_save();

    GUIMSG( "Commanding attached clients to quit." );

    close_session();

    osc_server->send( lo_message_get_source( msg ), "/reply", path,
                      "Closed." );
                      
    MESSAGE( "Done" );

done:

    pending_operation = COMMAND_NONE;

    return 0;
}


OSC_HANDLER( broadcast )
{
    const char *to_path = &argv[0]->s;

    /* don't allow clients to broadcast NSM commands */
    if ( ! strncmp( to_path, "/nsm/", strlen( "/nsm/" ) ) )
        return 0;

    std::list<OSC::OSC_Value> new_args;

    for ( int i = 1; i < argc; ++i )
    {
        switch ( types[i] )
        {
            case 's':
                new_args.push_back( OSC::OSC_String( &argv[i]->s ) );
                break;
            case 'i':
                new_args.push_back( OSC::OSC_Int( argv[i]->i ) );
                break;
            case 'f':
                new_args.push_back( OSC::OSC_Float( argv[i]->f ) );
                break;
        }
    }

    char *sender_url = lo_address_get_url( lo_message_get_source( msg ) );

    for ( std::list<Client*>::iterator i = client.begin();
          i != client.end();
          ++i )
    {
        if ( ! (*i)->addr )
            continue;

        char *url = lo_address_get_url( (*i)->addr );

        if ( strcmp( sender_url, url ) )
        {
            osc_server->send( (*i)->addr, to_path, new_args );
        }

        free( url );
    }

    /* also relay to attached GUI so that the broadcast can be
     * propagated to another NSMD instance */
    if ( gui_is_active )
    {
        
        char *u1 = lo_address_get_url( gui_addr );
     
        if ( strcmp( u1, sender_url ) )
        {
            new_args.push_front( OSC::OSC_String( to_path ) );
            
            osc_server->send( gui_addr, path, new_args );
        }
        
        free(u1);
    }
    
    free( sender_url );

    return 0;
}



/*********************************/
/* Client Informational Messages */
/*********************************/

OSC_HANDLER( progress )
{
    Client *c = get_client_by_address( lo_message_get_source( msg ) );

    if ( c )
    {
        c->progress = argv[0]->f;

        /* MESSAGE( "%s progress: %i%%", c->name, (int)(c->progress * 100.0f) ); */

        if ( gui_is_active )
        {
            osc_server->send( gui_addr, "/nsm/gui/client/progress", c->client_id, (float)c->progress );
        }
    }

    return 0;
}

OSC_HANDLER( is_dirty )
{
    MESSAGE( "Client sends dirty" );

    Client *c = get_client_by_address( lo_message_get_source( msg ) );
    
    if ( ! c )
        return 0;

    c->dirty = 1;

    if ( gui_is_active )
        osc_server->send( gui_addr, "/nsm/gui/client/dirty", c->client_id, c->dirty );

    return 0;
}

OSC_HANDLER( is_clean )
{
    MESSAGE( "Client sends clean" );

    Client *c = get_client_by_address( lo_message_get_source( msg ) );

    if ( ! c )
        return 0;

    c->dirty = 0;

    if ( gui_is_active )
        osc_server->send( gui_addr, "/nsm/gui/client/dirty", c->client_id, c->dirty );

    return 0;
}

OSC_HANDLER( gui_is_hidden )
{
    MESSAGE( "Client sends gui hidden" );

    Client *c = get_client_by_address( lo_message_get_source( msg ) );

    if ( ! c )
        return 0;

    c->gui_visible( false );

    if ( gui_is_active )
        osc_server->send( gui_addr, "/nsm/gui/client/gui_visible", c->client_id, c->gui_visible() );

    return 0;
}

OSC_HANDLER( gui_is_shown )
{
    MESSAGE( "Client sends gui shown" );

    Client *c = get_client_by_address( lo_message_get_source( msg ) );

    if ( ! c )
        return 0;

    c->gui_visible( true );

    if ( gui_is_active )
        osc_server->send( gui_addr, "/nsm/gui/client/gui_visible", c->client_id, c->gui_visible() );

    return 0;
}

OSC_HANDLER( message )
{
    Client *c = get_client_by_address( lo_message_get_source( msg ) );

    if ( ! c )
        return 0;

    if ( gui_is_active )
        osc_server->send( gui_addr, "/nsm/gui/client/message", c->client_id, argv[0]->i, &argv[1]->s );

    return 0;
}

OSC_HANDLER( label )
{
    Client *c = get_client_by_address( lo_message_get_source( msg ) );

    if ( ! c )
        return 0;

    if ( strcmp( types, "s" ) )
        return -1;
    
    c->label( &argv[0]->s );

    if ( gui_is_active )
        osc_server->send( gui_addr, "/nsm/gui/client/label", c->client_id, &argv[0]->s );

    return 0;
}

/**********************/
/* Response Handlers  */
/**********************/


OSC_HANDLER( error )
{
    Client *c = get_client_by_address( lo_message_get_source( msg ) );

    if ( ! c )
    {
        WARNING( "Error from unknown client" );
        return 0;
    }

//    const char *rpath = &argv[0]->s;

    int err_code = argv[1]->i;
    
    const char *message = &argv[2]->s;

    c->set_reply( err_code, message );

    MESSAGE( "Client \"%s\" replied with error: %s (%i) in %fms", c->name, message, err_code, c->milliseconds_since_last_command() ); 
    c->pending_command( COMMAND_NONE );
    
    if ( gui_is_active )
        osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "error" );

    return 0;
}


OSC_HANDLER( reply )
{
    Client *c = get_client_by_address( lo_message_get_source( msg ) );

//    const char *rpath = &argv[0]->s;
    const char *message = &argv[1]->s;
    
    if ( c )
    {
        c->set_reply( ERR_OK, message );

        MESSAGE( "Client \"%s\" replied with: %s in %fms", c->name, message, c->milliseconds_since_last_command() ); 
        
        c->pending_command( COMMAND_NONE );
        
        if ( gui_is_active )
            osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "ready" );
    }
    else
        MESSAGE( "Reply from unknown client" );

    return 0;
}



/******************/
/* GUI operations */
/******************/


OSC_HANDLER( stop )
{
    Client *c = get_client_by_id( &client, &argv[0]->s );

    if ( c )
    {
        command_client_to_stop( c );
        
        if ( gui_is_active )
            osc_server->send( gui_addr, "/reply", "Client stopped." );
    }
    else
    {
        if ( gui_is_active )
            osc_server->send( gui_addr, "/error", -10, "No such client." );
    }

   
    return 0;
}

OSC_HANDLER( remove )
{
    Client *c = get_client_by_id( &client, &argv[0]->s );

    if ( c )
    {
        if ( c->pid == 0 &&
             ! c->active )
        {
            osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "removed" );
            
            client.remove( c );
            
            delete c;

            if ( gui_is_active )
                osc_server->send( gui_addr, "/reply", "Client removed." );
        }
    }
    else
    {
        if ( gui_is_active )
            osc_server->send( gui_addr, "/error", -10, "No such client." );
    }

   
    return 0;
}

OSC_HANDLER( resume )
{
    Client *c = get_client_by_id( &client, &argv[0]->s );
    
    /* FIXME: return error if no such client? */
    if ( c )
    {
        if ( c->pid == 0 &&
             ! c->active )
        {
            if ( ! launch( c->executable_path, c->client_id ) )
            {
                
            }
        }
    }
    
    return 0;
}


OSC_HANDLER( client_save )
{
    Client *c = get_client_by_id( &client, &argv[0]->s );

    /* FIXME: return error if no such client? */
    if ( c )
    {
        if ( c->active )
        {
            command_client_to_save( c );
        }
    }

    return 0;
}

OSC_HANDLER( client_show_optional_gui )
{
    Client *c = get_client_by_id( &client, &argv[0]->s );

    /* FIXME: return error if no such client? */
    if ( c )
    {
        if ( c->active )
        {
            osc_server->send( c->addr, "/nsm/client/show_optional_gui" );
        }
    }

    return 0;
}

OSC_HANDLER( client_hide_optional_gui )
{
    Client *c = get_client_by_id( &client, &argv[0]->s );

    /* FIXME: return error if no such client? */
    if ( c )
    {
        if ( c->active )
        {
            osc_server->send( c->addr, "/nsm/client/hide_optional_gui" );
        }
    }

    return 0;
}

void
announce_gui( const char *url, bool is_reply )
{
    gui_addr = lo_address_new_from_url( url );
    gui_is_active = true;

    if ( is_reply )
        osc_server->send( gui_addr, "/nsm/gui/gui_announce", "hi" );
    else
        osc_server->send( gui_addr, "/nsm/gui/server_announce", "hi" );

    for ( std::list<Client*>::iterator i = client.begin();
          i != client.end();
          ++i )
    {
        Client *c = *i;

        osc_server->send( gui_addr, "/nsm/gui/client/new", c->client_id, c->name );        
        osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status );

    }

    osc_server->send( gui_addr, "/nsm/gui/session/name", session_name ? session_name : "", session_path ? session_path : "" );

    DMESSAGE( "Registered with GUI" );
}


OSC_HANDLER( gui_announce )
{
    announce_gui( lo_address_get_url( lo_message_get_source( msg ) ), true );

    return 0;
}

OSC_HANDLER( ping )
{
    osc_server->send( lo_message_get_source( msg ), "/reply", path );

    return 0;
}

OSC_HANDLER( null )
{
    WARNING( "Unrecognized message with type signature \"%s\" at path \"%s\"", types, path );

    return 0;
}



static void
wait ( long timeout )
{
    struct signalfd_siginfo fdsi;
    
    ssize_t s = read(signal_fd, &fdsi, sizeof(struct signalfd_siginfo));
    
    if (s == sizeof(struct signalfd_siginfo))
    {
        if (fdsi.ssi_signo == SIGCHLD)
                handle_sigchld();
    }
    
    osc_server->wait( timeout );
    
    purge_dead_clients();
}

int main(int argc, char *argv[])
{
    sigset_t mask;
    sigemptyset( &mask );
    sigaddset( &mask, SIGCHLD );
    sigprocmask(SIG_BLOCK, &mask, NULL );
    signal_fd = signalfd( -1, &mask, SFD_NONBLOCK );

    /* generate random seed for client ids */
    {
        time_t seconds;
        time(&seconds);
        
        srand( (unsigned int) seconds );
    }

//    char *osc_port = "6666";
    char *osc_port = NULL;
    const char *gui_url = NULL;

    static struct option long_options[] = 
    {
        { "detach", no_argument, 0, 'd' },
        { "session-root", required_argument, 0, 's' },
        { "osc-port", required_argument, 0, 'p' },
        { "gui-url", required_argument, 0, 'g' },
        { "help", no_argument, 0, 'h' },
        { 0, 0, 0, 0 }
    };

    int option_index = 0;
    int c = 0;

    bool detach = false;

    while ( ( c = getopt_long_only( argc, argv, "", long_options, &option_index  ) ) != -1 )
    {
        switch ( c )
        {
            case 'd':
                detach = true;
                break;
            case 's':
            {
                session_root = optarg;

                /* get rid of trailing slash */
                char *s = rindex(session_root,'/');
                if ( s == &session_root[strlen(session_root) - 1] )
                    *s = '\0';
                break;
            }
            case 'p':
                DMESSAGE( "Using OSC port %s", optarg );
                osc_port = optarg;
                break;
            case 'g':                
                DMESSAGE( "Going to connect to GUI at: %s", optarg );
                gui_url = optarg;
                break;
            case 'h':
                printf( "Usage: %s [--osc-port portnum] [--session-root path]\n\n", argv[0] );
                exit(0);
                break;
        }
    }

    if ( !session_root )
        asprintf( &session_root, "%s/%s", getenv( "HOME" ), "NSM Sessions" );
  
    struct stat st;

    if ( stat( session_root, &st ) )
    {
        if ( mkdir( session_root, 0771 ) )
        {
            FATAL( "Failed to create session directory: %s", strerror( errno ) );
        }
    }

    MESSAGE( "Session root is: %s", session_root );

    osc_server = new OSC::Endpoint();

    if ( osc_server->init( LO_UDP, osc_port ) )
    {
        FATAL( "Failed to create OSC server." );
    }

    printf( "NSM_URL=%s\n", osc_server->url() );

    if ( gui_url )
    {
        announce_gui( gui_url, false );
    }

    /*  */
    osc_server->add_method( "/nsm/server/announce", "sssiii", OSC_NAME( announce ), NULL, "client_name,capabilities,executable,api_version_major,api_version_minor,client_pid" );

    /* response handlers */
    osc_server->add_method( "/reply", "ss", OSC_NAME( reply ), NULL, "err_code,msg" );
    osc_server->add_method( "/error", "sis", OSC_NAME( error ), NULL, "err_code,msg" );
  
    osc_server->add_method( "/nsm/client/progress", "f", OSC_NAME( progress ), NULL, "progress" );
    osc_server->add_method( "/nsm/client/is_dirty", "", OSC_NAME( is_dirty ), NULL, "dirtiness" );
    osc_server->add_method( "/nsm/client/is_clean", "", OSC_NAME( is_clean ), NULL, "dirtiness" );
    osc_server->add_method( "/nsm/client/message", "is", OSC_NAME( message ), NULL, "message" );
    osc_server->add_method( "/nsm/client/gui_is_hidden", "", OSC_NAME( gui_is_hidden ), NULL, "message" );
    osc_server->add_method( "/nsm/client/gui_is_shown", "", OSC_NAME( gui_is_shown ), NULL, "message" );
    osc_server->add_method( "/nsm/client/label", "s", OSC_NAME( label ), NULL, "message" );
    
    /*  */
    osc_server->add_method( "/nsm/gui/gui_announce", "", OSC_NAME( gui_announce ), NULL, "" );
    osc_server->add_method( "/nsm/gui/client/stop", "s", OSC_NAME( stop ), NULL, "client_id" );
    osc_server->add_method( "/nsm/gui/client/remove", "s", OSC_NAME( remove ), NULL, "client_id" );
    osc_server->add_method( "/nsm/gui/client/resume", "s", OSC_NAME( resume ), NULL, "client_id" );
    osc_server->add_method( "/nsm/gui/client/save", "s", OSC_NAME( client_save ), NULL, "client_id" );
    osc_server->add_method( "/nsm/gui/client/show_optional_gui", "s", OSC_NAME( client_show_optional_gui ), NULL, "client_id" );
    osc_server->add_method( "/nsm/gui/client/hide_optional_gui", "s", OSC_NAME( client_hide_optional_gui ), NULL, "client_id" );

    osc_server->add_method( "/osc/ping", "", OSC_NAME( ping ), NULL, "" );

    osc_server->add_method( "/nsm/server/broadcast", NULL, OSC_NAME( broadcast ), NULL, "" );
    osc_server->add_method( "/nsm/server/duplicate", "s", OSC_NAME( duplicate ), NULL, "" );
    osc_server->add_method( "/nsm/server/abort", "", OSC_NAME( abort ), NULL, "" );
    osc_server->add_method( "/nsm/server/list", "", OSC_NAME( list ), NULL, "" );
    osc_server->add_method( "/nsm/server/add", "s", OSC_NAME( add ), NULL, "executable_name" );
    osc_server->add_method( "/nsm/server/new", "s", OSC_NAME( new ), NULL, "name" );
    osc_server->add_method( "/nsm/server/save", "", OSC_NAME( save ), NULL, "" );
    osc_server->add_method( "/nsm/server/open", "s", OSC_NAME( open ), NULL, "name" );
    osc_server->add_method( "/nsm/server/close", "", OSC_NAME( close ), NULL, "" );
    osc_server->add_method( "/nsm/server/quit", "", OSC_NAME( quit ), NULL, "" );

    osc_server->add_method( NULL, NULL, OSC_NAME( null ),NULL, "" );

    if ( detach )
    {
        MESSAGE( "Detaching from console" );
        if ( fork() )
        {
            exit( 0 );            
        }
        else
        {
            fclose( stdin );
            fclose( stdout );
            fclose( stderr );
        }
    }

    /* listen for sigchld signals and process OSC messages forever */
    for ( ;; )
    {
        wait( 1000 );
    }
    
//    osc_server->run();

    return 0;
}
