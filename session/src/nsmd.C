
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

#define _GNU_SOURCE

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
#include <uuid/uuid.h>
#include <unistd.h>
#include <uuid/uuid.h>
#include <time.h>
#include <libgen.h>
#include <dirent.h>
#include <ftw.h>
#include <list>
#include <getopt.h>

#include <OSC/Endpoint.H>
/* for locking */
#include "file.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"

static OSC::Endpoint *osc_server;
static lo_address gui_addr;
static bool gui_is_active = false;
static int signal_fd;

static int session_lock_fd = 0;
static char *session_root;

#define NSM_API_VERSION_MAJOR 0
#define NSM_API_VERSION_MINOR 3

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

#define APP_TITLE "Non Session Manager"

enum {
    COMMAND_NONE = 0,
    COMMAND_QUIT,
    COMMAND_KILL,
    COMMAND_SAVE,
    COMMAND_OPEN
};

struct Client
{
private:

    int _reply_errcode;
    char *_reply_message;

public:

    lo_address addr;                   /*  */
    char *name;                        /* client application name */
    char *executable_path;             /* path to client executable */
    int pending_command;                /*  */
    int pid;                           /* PID of client process */
    float progress;                    /*  */
    bool active;              /* client has registered via announce */
    bool dead_because_we_said;
//    bool stopped; /* the client quit, but not because we told it to--user still has to decide to remove it from the session */
    char *client_id;                                            /* short part of client ID */
    char *capabilities;                                         /* client capabilities... will be null for dumb clients */
    bool dirty;                                                 /* flag for client self-reported dirtiness */
    bool pre_existing;
    const char *status;

    bool
    has_error ( void )
        {
            return _reply_errcode != 0;
        }

    int
    error_code ( void )
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
            return pending_command != COMMAND_NONE;
        }

    bool is_dumb_client ( void )
        {
            return capabilities == NULL;
        }

    Client ( )
        {
            addr = 0;
            _reply_errcode = 0;
            _reply_message = 0;
            dead_because_we_said = false;
            pid = 0;
            progress = -0;
            pending_command = 0;
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
        MESSAGE( "Client %s died.", c->name );

        if ( c->pending_command == COMMAND_KILL ||
             c->pending_command == COMMAND_QUIT )
        {
            c->dead_because_we_said = true;
        }

        c->pending_command = COMMAND_NONE;
            
        c->active = false;
        c->pid = 0;

        if ( c->dead_because_we_said )
        {
            if ( gui_is_active )
                osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "removed" );

            client.remove( c );

            delete c;
        }
        else
        {
            if ( gui_is_active )
                osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "stopped" );
        }
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

    while ( i = index( i, '/' ) )
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

bool
replies_still_pending ( )
{
    for ( std::list<Client*>::const_iterator i = client.begin();
          i != client.end();
          ++i )
        /* if ( (*i)->active && (*i)->reply_pending() ) */
        /*     return true; */
        if ( (*i)->reply_pending() )
            return true;

    return false;
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

void
wait_for_replies ( )
{
    fprintf( stdout, "Waiting..." );
    fflush(stdout);

    int n = 7;

    while ( n-- )
    {
        printf( "." );
        fflush(stdout);

        osc_server->wait( 1000 );

        if ( ! replies_still_pending() )
            break;
    }

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
        MESSAGE( "Launching %s\n", executable );

        char *args[] = { strdup( executable ), NULL };

        setenv( "NSM_URL", url, 1 );
        
        if ( -1 == execvp( executable, args ) )
        {
            WARNING( "Error starting process: %s", strerror( errno ) );

            exit(-1);
        }
    }

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
        
        c->pending_command = COMMAND_SAVE;

        if ( gui_is_active )
            osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "save" );
    }
    else if ( c->is_dumb_client() )
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
    
    c->pending_command = COMMAND_OPEN;

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
        osc_server->send( lo_message_get_source( msg ), path,
                          ERR_NO_SESSION_OPEN,
                          "Cannot add to session because no session is loaded." );
                          

        return 0;
    }

    if ( ! launch( &argv[0]->s, NULL ) )
    {
        osc_server->send( lo_message_get_source( msg ), path,
                          ERR_LAUNCH_FAILED,
                          "Failed to launch process!" );
    }
    else
    {
        osc_server->send( lo_message_get_source( msg ), path,
                          ERR_OK,
                          "Launched." );
    }
 
    return 0;
}

OSC_HANDLER( announce )
{
    MESSAGE( "Got announce" );

    const char *client_name = &argv[0]->s;
    const char *capabilities = &argv[1]->s;
    const char *executable_path = &argv[2]->s;
    int major = argv[3]->i;
    int minor = argv[4]->i;
    int pid = argv[5]->i;

    bool expected_client = false;

    Client *c = NULL;

    for ( std::list<Client*>::iterator i = client.begin();
          i != client.end();
          ++i )
    {
        if ( ! strcmp( (*i)->executable_path, executable_path ) 
             && ! (*i)->active )
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
        DMESSAGE( "Client is using incompatible and more recent API version %i.%i", major, minor ); 

        osc_server->send( lo_message_get_source( msg ), "/error",
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
                      ":server-control:" );

    if ( gui_is_active )
    {
        osc_server->send( gui_addr, "/nsm/gui/client/new", c->client_id, c->name );
        osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "open" );
    }

    {
        char *full_client_id;
        asprintf( &full_client_id, "%s.%s", c->name, c->client_id );
        
        char *client_project_path = get_client_project_path( session_path, c );
        
        osc_server->send( lo_message_get_source( msg ), "/nsm/client/open", client_project_path, session_name, full_client_id );
        
        c->pending_command = COMMAND_OPEN;

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

// capability should be enclosed in colons. I.e. ":switch:"
bool 
client_is_capable_of ( Client *c, const char *capability )
{
    return c->capabilities &&
        strstr( c->capabilities, capability );
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
    
    MESSAGE( "Waiting for any dumb clients to die." );
    
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
        if ( ( (*i)->pending_command == COMMAND_QUIT ||
               (*i)->pending_command == COMMAND_KILL ) &&
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
    
    for ( int i = 0; i < 24; i++ )
    {
        MESSAGE( "Loop %i", i );

        if ( ! killed_clients_are_alive() )
            break;
        
        ssize_t s = read(signal_fd, &fdsi, sizeof(struct signalfd_siginfo));
        
        if (s == sizeof(struct signalfd_siginfo))
        {
            if (fdsi.ssi_signo == SIGCHLD)
                handle_sigchld();
        }
        
        purge_dead_clients();

        usleep( 200 * 1000 );
    }

    if ( killed_clients_are_alive() )
    {
        WARNING( "Killed clients are still alive" );
    /* FIXME: give up on remaining clients and purge them */
    }
    else
        MESSAGE( "All clients have died." );

}


void
command_all_clients_to_save ( )
{
    if ( session_path )
    {
        MESSAGE( "Commanding attached clients to save." );
        
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
command_client_to_quit ( Client *c )
{
    MESSAGE( "Commanding %s to quit", c->name );
    
    if ( c->active )
    {
        c->pending_command = COMMAND_QUIT;

        kill( c->pid, SIGTERM );

        if ( gui_is_active )
            osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "quit" );
    }
    else if ( c->is_dumb_client() )
    {
        if ( c->pid > 0 )
        {
            if ( gui_is_active )
                osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "kill" );
            
            /* should be kill? */
            c->pending_command = COMMAND_KILL;
            
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
        osc_server->send( gui_addr, "/nsm/gui/session/name", "" );
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

    session_path = strdup( path );

    set_name( path );

    std::list<Client*> new_clients;

    {
        char * client_name = NULL;
        char * client_executable = NULL;
        char * client_id = NULL;
        
        // load the client list
        while ( fscanf( fp, "%a[^:]:%a[^:]:%a[^:\n]\n", &client_name, &client_executable, &client_id ) > 0 )
        {
            Client *c = new Client();
            
            c->name = client_name;
            c->executable_path = client_executable;
            c->client_id =  client_id;
            
            new_clients.push_back( c );
        }
    }

    MESSAGE( "Commanding unneeded and dumb clients to quit" );

    for ( std::list<Client*>::iterator i = client.begin();
          i != client.end();
          ++i )
    {
        if ( ! client_is_capable_of( *i, ":switch:" )
             ||
             ! client_by_name( (*i)->name, &new_clients ) )
        {
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

//    osc_server->wait( 3000 );

    wait_for_replies();

    tell_all_clients_session_is_loaded();

    MESSAGE( "Loaded." );

    new_clients.clear();

    if ( gui_is_active )
    {
        osc_server->send( gui_addr, "/nsm/gui/session/name", session_name );
    }
    
    return ERR_OK;
}

OSC_HANDLER( save )
{
    if ( ! session_path )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path, 
                          ERR_NO_SESSION_OPEN,
                          "No session to save.");
                          
        return 0;
    }
    
    command_all_clients_to_save();

    MESSAGE( "Done." );

    osc_server->send( lo_message_get_source( msg ), "/reply", path, "Saved." );

    return 0;
}

OSC_HANDLER( duplicate )
{
    if ( ! session_path )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path, 
                          ERR_NO_SESSION_OPEN,
                          "No session to duplicate.");
                          
        return 0;
    }

    if ( ! path_is_valid( &argv[0]->s ) )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          ERR_CREATE_FAILED,
                          "Invalid session name." );

        return 0;
    }     

    command_all_clients_to_save();
    
    if ( clients_have_errors() )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          ERR_GENERAL_ERROR,
                          "Some clients could not save" );

        return 0;
    }

//    save_session_file();

    const char *src = session_path;

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

    if ( load_session_file( spath ) )
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
    }

    free( spath );
 
    MESSAGE( "Done" );


    osc_server->send( lo_message_get_source( msg ), "/reply", path, "Duplicated." );

    return 0;
}

OSC_HANDLER( new )
{
    if ( ! path_is_valid( &argv[0]->s ) )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          ERR_CREATE_FAILED,
                          "Invalid session name." );

        return 0;
    }     

    if ( session_path )
    {
        command_all_clients_to_save();
        
        close_session();
    }

    MESSAGE( "Creating new session" );


    char *spath;
    asprintf( &spath, "%s/%s", session_root, &argv[0]->s );
   
    if ( mkpath( spath, true ) )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          ERR_CREATE_FAILED,
                          "Could not create the session directory" );

        free(spath);

        return 0;
    }

    session_path = strdup( spath );
    
    set_name( session_path );

    osc_server->send( lo_message_get_source( msg ), "/reply", path, "Created." );

    if ( gui_is_active )
    {
        osc_server->send( gui_addr,  "/nsm/gui/session/session", &argv[0]->s  );
        osc_server->send( gui_addr, "/nsm/gui/session/name", &argv[0]->s  );
    }

    save_session_file();

    free( spath );

    osc_server->send( lo_message_get_source( msg ), "/reply", path,
                              "Session created" );
        
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
    MESSAGE( "Listing sessions" );

    list_response_address = lo_message_get_source( msg );
    
    ftw( session_root, list_file, 20 );

    osc_server->send( lo_message_get_source( msg ), path,
                      ERR_OK,
                      "Done." );

    return 0;
}

OSC_HANDLER( open )
{
    DMESSAGE( "Got open" );

    if ( session_path )
    {
        command_all_clients_to_save();
        
        if ( clients_have_errors() )
        {
            osc_server->send( lo_message_get_source( msg ), "/error", path,
                              ERR_GENERAL_ERROR,
                              "Some clients could not save" );
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
    if ( ! session_path )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          ERR_NO_SESSION_OPEN,
                          "No session to abort." );

        return 0;
    }

    MESSAGE( "Commanding attached clients to quit." );

    close_session();

    osc_server->send( lo_message_get_source( msg ), "/reply", path,
                      "Aborted." );
                      
    MESSAGE( "Done" );

    return 0;
}

OSC_HANDLER( close )
{
    if ( ! session_path )
    {
        osc_server->send( lo_message_get_source( msg ), "/error", path,
                          ERR_NO_SESSION_OPEN,
                          "No session to close." );

        return 0;
    }

    command_all_clients_to_save();

    MESSAGE( "Commanding attached clients to quit." );

    close_session();

    osc_server->send( lo_message_get_source( msg ), "/reply", path,
                      "Closed." );
                      
    MESSAGE( "Done" );

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

        MESSAGE( "%s progress: %i%%", c->name, (int)(c->progress * 100.0f) );

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

    c->dirty = 1;

    if ( gui_is_active )
        osc_server->send( gui_addr, "/nsm/gui/client/dirty", c->client_id, c->dirty );

    return 0;
}

OSC_HANDLER( is_clean )
{
    MESSAGE( "Client sends clean" );

    Client *c = get_client_by_address( lo_message_get_source( msg ) );

    c->dirty = 0;

    if ( gui_is_active )
        osc_server->send( gui_addr, "/nsm/gui/client/dirty", c->client_id, c->dirty );

    return 0;
}


OSC_HANDLER( message )
{
    Client *c = get_client_by_address( lo_message_get_source( msg ) );

    if ( gui_is_active )
        osc_server->send( gui_addr, "/nsm/gui/client/message", c->client_id, argv[0]->i, &argv[1]->s );

    return 0;
}

/**********************/
/* Response Handlers  */
/**********************/


OSC_HANDLER( error )
{
    Client *c = get_client_by_address( lo_message_get_source( msg ) );

    const char *rpath = &argv[0]->s;

    int err_code = argv[1]->i;
    
    const char *message = &argv[2]->s;

    c->set_reply( err_code, message );

    MESSAGE( "Client \"%s\" replied with error: %s (%i)", c->name, message, err_code ); 
    c->pending_command = COMMAND_NONE;
    
    if ( gui_is_active )
        osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status = "error" );

    return 0;
}


OSC_HANDLER( reply )
{
    Client *c = get_client_by_address( lo_message_get_source( msg ) );

    const char *rpath = &argv[0]->s;
    const char *message = &argv[1]->s;
    
    if ( c )
    {
        c->set_reply( ERR_OK, message );

        MESSAGE( "Client \"%s\" replied with: %s", c->name, message ); 
        
        c->pending_command = COMMAND_NONE;
        
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

    osc_server->send( gui_addr, "/nsm/gui/session/name", session_name ? session_name : "" );

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
                session_root = optarg;
                break;
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
    
    /*  */
    osc_server->add_method( "/nsm/gui/gui_announce", "", OSC_NAME( gui_announce ), NULL, "" );
    osc_server->add_method( "/nsm/gui/client/remove", "s", OSC_NAME( remove ), NULL, "client_id" );
    osc_server->add_method( "/nsm/gui/client/resume", "s", OSC_NAME( resume ), NULL, "client_id" );
    osc_server->add_method( "/nsm/gui/client/save", "s", OSC_NAME( client_save ), NULL, "client_id" );

    osc_server->add_method( "/osc/ping", "", OSC_NAME( ping ), NULL, "" );

    osc_server->add_method( "/nsm/server/broadcast", NULL, OSC_NAME( broadcast ), NULL, "" );
    osc_server->add_method( "/nsm/server/duplicate", "s", OSC_NAME( duplicate ), NULL, "" );
    osc_server->add_method( "/nsm/server/abort", "", OSC_NAME( abort ), NULL, "" );
    osc_server->add_method( "/nsm/server/list", "", OSC_NAME( list ), NULL, "" );
    osc_server->add_method( "/nsm/server/add", "s", OSC_NAME( add ), NULL, "commandline" );
    osc_server->add_method( "/nsm/server/new", "s", OSC_NAME( new ), NULL, "name" );
    osc_server->add_method( "/nsm/server/save", "", OSC_NAME( save ), NULL, "" );
    osc_server->add_method( "/nsm/server/open", "s", OSC_NAME( open ), NULL, "name" );
    osc_server->add_method( "/nsm/server/close", "", OSC_NAME( close ), NULL, "" );
    osc_server->add_method( "/nsm/server/quit", "", OSC_NAME( quit ), NULL, "" );


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

    struct signalfd_siginfo fdsi;


    /* listen for sigchld signals and process OSC messages forever */
    for ( ;; )
    {
        ssize_t s = read(signal_fd, &fdsi, sizeof(struct signalfd_siginfo));
        
        if (s == sizeof(struct signalfd_siginfo))
        {
            if (fdsi.ssi_signo == SIGCHLD)
                handle_sigchld();
        }
        
        osc_server->wait( 1000 );

        purge_dead_clients();
    }
    
//    osc_server->run();

    return 0;
}
