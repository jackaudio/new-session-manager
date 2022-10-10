
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

#define __MODULE__ "nsmd"

//debug.c has only one function that gets used multiple times by debug.h and for logging and printing
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
#include <time.h>
#include <libgen.h>
#include <list>
#include <getopt.h>
#include <sys/time.h>
#include <fts.h>

#include "Endpoint.hpp"
/* for locking */
#include "file.h"

#include <map>
#include <string>
#include <algorithm>

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-result"

static OSC::Endpoint *osc_server;
static lo_address gui_addr;
static bool gui_is_active = false;
static int signal_fd;

static char *session_root;
static char *lockfile_directory;
static char *daemon_file;

#define NSM_API_VERSION_MAJOR 1
#define NSM_API_VERSION_MINOR 1
#define NSM_API_VERSION_PATCH 2
#define VERSION_STRING "1.6.1"

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

#define APP_TITLE "New Session Manager"

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

void handle_signal_clean_exit ( int );


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

    int _pending_command;
    struct timeval _command_sent_time;

    bool _gui_visible;

    char *_label;

public:

    lo_address addr=0;                 /*  */
    char *name;                        /* First this is the basename of client executable, later it becomes the client-reported name which must be treated as if unrelated. */
    char *executable_path;             /* Contrary to the name this is basename(executable) */
    int pid;                           /* PID of client process */
    float progress;                    /*  */
    bool active;                       /* NSM capable: client has registered via announce */
    //bool stopped;                    /* the client quit, but not because we told it to--user still has to decide to remove it from the session */
    char *client_id;                   /* short part of client ID */
    char *capabilities;                /* client capabilities... will be null for dumb clients */
    bool dirty;                        /* flag for client self-reported dirtiness */
    bool pre_existing;
    const char *status;
    int launch_error;                   /* v1.4, leads to status for executable not found, permission denied etc. */
    char *name_with_id;                  /* v1.4, client.nABC */

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
            launch_error = 0;
            dirty = 0;
            status = 0;
            name_with_id = 0;

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
            if (name_with_id)
                free(name_with_id);

            name = executable_path = client_id = capabilities = name_with_id = NULL;
        }
};


static std::list< Client* > client;

/* helper macros for defining OSC handlers */
#define OSC_NAME( name ) osc_ ## name
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
        //There is a difference if a client quit on its own, e.g. via a menu or window manager,
        //or if the server send SIGTERM as quit signal. Both cases are equally valid.
        //We only check the case to print a different log message
        bool dead_because_we_said = ( c->pending_command() == COMMAND_KILL ||
                                      c->pending_command() == COMMAND_QUIT );

        if ( dead_because_we_said )
        {
            GUIMSG( "Client %s terminated by server instruction.", c->name_with_id );
        }
        else
        {
            GUIMSG( "Client %s terminated itself.", c->name_with_id );
        }


        //Decide if the client terminated or if removed from the session
        if ( c->pending_command() == COMMAND_QUIT )
        {
            c->status = "removed";
            if ( gui_is_active )
                osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status );

            client.remove(c); //This will not remove the clients save data
            delete c;
        }
        else
        {
            if ( c->launch_error )
               /* NSM API treats the stopped status as switch. You can only remove stopped.
                * Furthermore the GUI will change its client-buttons.
                * In consequence we cannot add an arbitrary "launch-error" status.
                * Compatible compromise is to use the label field to relay info the user,
                * which was the goal. There is nothing we can do about a failed launch anyway.
                */
                c->label( "launch error!" );
            else
                c->label( "" );

            c->status = "stopped";

            if ( gui_is_active )
            {
                osc_server->send( gui_addr, "/nsm/gui/client/label", c->client_id, c->label() );
                osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status );
            }
        }

        c->pending_command( COMMAND_NONE );

        c->active = false;
        c->pid = 0;
     }
}

void handle_sigchld ( )
{
    // compare waitpid(2)
    for ( ;; )
    {
        int status = 1; // make it not NULL to enable information storage in status
        pid_t pid = waitpid(-1, &status, WNOHANG); //-1 meaning wait for any child process. pid_t is signed integer

        if (pid <= 0)
        {
            break; // no child process has ended this loop. Check again.
        }
        else
        {
            //One child process has stopped. Find which and figure out the stop-conditions
            Client *c;
            c = get_client_by_pid( pid );
            if ( c )
            {
                //The following will not trigger with normal crashes, e.g. segfaults or python tracebacks
                if ( WIFEXITED( status ) ) // returns true if the child terminated normally
                    if ( WEXITSTATUS( status ) == 255 ) // as given by exit(-1) in launch()
                        c->launch_error = true;
            }
            // Call even if Client was already null. This will check itself again and was expected
            // to be called for the majority of nsmds development
            handle_client_process_death( pid );
        }
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
session_already_exists ( const char * relative_session_path)
{
    //A session is defined as a path with the file session.nsm
    //We receive the relative path with sub-directories like album/song as relative_session_path, without leading and trailing /
    struct stat st_session_exists_check;
    char * path;
    asprintf( &path, "%s/%s/session.nsm", session_root, relative_session_path );
    if ( stat (path, &st_session_exists_check) == 0) {
        free( path );
        return 0; // Already exists
    }
    else {
        free( path );
        return -1; // All good
    }
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
generate_client_id ( void )
{
    /* Before v1.4 this returned "n" + 4 random upper-case letters, which could lead to collisions.
    We changed behaviour to still generate 4 letters, but check for collision with existing IDs.

    Loaded client IDs are not checked, just copied from session.nsm because loading happens before
    any generation of new clients. Loaded clients are part of further checks of course.

    There is a theoretical limit when all 26^4 IDs are in use which will lead to an infinite loop
    of generation. We risk to leave this unhandled. */

    char id_str[6];

    id_str[0] = 'n';
    id_str[5] = 0;

    while ( true )
    {
        for ( int i = 1; i < 5; i++ )
            id_str[i] = 'A' + (rand() % 25);

        if ( get_client_by_id(&client, id_str )==NULL ) // found a free id
            break;
    }

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
number_of_reponsive_clients ( void )
{
    /* This was renamed from number_of_active_clients in version 1.4 to reflect
     * that not only active==true clients are in a state where waiting has ended, but also clients
     * that never started. It is used in wait_for_announce only, which added a 5000ms delay to startup
     *
     * We are sadly unable to distinguish between a client that has a slow announce and a client
     * without NSM-support. However, this is mitigated by nsm-proxy which is a reliable indicator
     * that this program will never announce (or rather nsm-proxy announces normally).
     */

    int responsive = 0;
    for ( std::list<Client*>::const_iterator i = client.begin(); i != client.end(); ++i )
    {
        //Optimisation: Clients that never launched (e.g. file not found) will be checked many times/seconds here. We skip them by counting them
        if ( (*i)->active || (*i)->launch_error )
            responsive++;
    }

    return responsive;
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

        active = number_of_reponsive_clients();

        if ( client.size() == active )
            break;
    }

    GUIMSG( "Done. %lu out of %lu clients announced (or failed to launch) within the initialization grace period",
            active, (long unsigned)client.size() );
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
            c->client_id = generate_client_id();

        asprintf( &c->name_with_id, "%s.%s", c->name, c->client_id );

        client.push_back( c );
    }

    char * url = osc_server->url();

    int pid;
    if ( ! (pid = fork()) )
    {
        //This is code of the child process. It will be executed after launch() has finished
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
            /* The program was not started. Causes: not installed on the current system, and the
             * session was transferred from another system, or permission denied (no executable flag)
             * Since we are running in a forked child process Client c does exist, but points to
             * a memory copy, not the real client. So we can't set any error code or status in the
             * client object. Instead we check the exit return code in handle_sigchld() and set the
            *  bool client->launch_error to true.
            */

            WARNING( "Error starting process %s: %s", executable, strerror( errno ) );
            exit(-1); //-1 later parsed as 255
        }
    }

    //This is code of the parent process. It is executed right at this point, before the child.
    c->pending_command( COMMAND_START );
    c->pid = pid;

    MESSAGE( "Process %s has pid: %i", executable, pid ); //We do not have a name yet, use executable

    //Normal launch. Setting launch_error to false is not redundant:
    //A previous launch-error fixed by the user, and then resume, needs this reset.
    c->launch_error = false;
    c->status = "launch";
    if ( gui_is_active )
    {
        //At this point we do not know if launched program will start or fail
        //And we do not know if it has nsm-support or not. This will be decided if it announces.
        osc_server->send( gui_addr, "/nsm/gui/client/new", c->client_id, c->executable_path ); // a second message may get send with c->name, if the client sends announce()
        osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status );
        osc_server->send( gui_addr, "/nsm/gui/client/label", c->client_id, "" );
    }

    return true;
}

void
command_client_to_save ( Client *c )
{
    if ( c->active )
    {
        MESSAGE( "Telling %s to save", c->name_with_id );
        osc_server->send( c->addr, "/nsm/client/save" );

        c->pending_command( COMMAND_SAVE );

        c->status = "save";
        if ( gui_is_active )
            osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id,  c->status );
    }
    else if ( c->is_dumb_client() && c->pid )
    {
        // this is a dumb client...
        c->status = "noop";
        if ( gui_is_active )
            osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status );
    }
}

void command_client_to_switch ( Client *c, const char *new_client_id )
{
    char *old_client_id = c->client_id;

    c->client_id = strdup( new_client_id );

    char *client_project_path = get_client_project_path( session_path, c );

    MESSAGE( "Commanding %s to switch \"%s\"", c->name_with_id, client_project_path );

    char *full_client_id;
    asprintf( &full_client_id, "%s.%s", c->name, c->client_id );

    osc_server->send( c->addr, "/nsm/client/open", client_project_path, session_name, full_client_id );

    free( full_client_id );
    free( client_project_path );

    c->pending_command( COMMAND_OPEN );

    c->status = "switch";
    if ( gui_is_active )
    {
        osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status );
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
            (*i)->status = "removed";
            if ( gui_is_active )
                osc_server->send( gui_addr, "/nsm/gui/client/status", (*i)->client_id, (*i)->status );

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
        osc_server->send( lo_message_get_source( msg ), "/reply", path, "Launched." );
    }

    return 0;
}

OSC_HANDLER( announce )
{
    /* A client announces itself which identifies it as real nsm-capable client, internally represented by the c->active bool.

    If nsmd started the client itself (e.g. through a GUI) at this point the program is already
    part of the session and registered with c->name=basename(executable). For these clients a
    second client/new message is sent, indicating an upgrade of the formerly dumb client. Through
    this c->name changes from executable to the self-reported client name from this announce
    message.

    Before v1.4 clients that announce themselves (started with NSM URL ENV present) never triggered
    the first client/new which sends an executable. This created a problem with attaching GUIs to a
    running nsmd never were able to infer any data from executables, like icons. Changed so that
    every new client scenario sends basename(executable) first.
    */

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
            MESSAGE( "Client %s was expected.", (*i)->name );
            c = *i;
            break;
        }
    }

    if ( ! c )
    {
        c = new Client();
        c->executable_path = strdup( executable_path );
        c->client_id = generate_client_id();
    }
    else
        expected_client = true;

    if ( major > NSM_API_VERSION_MAJOR )
    {
        MESSAGE( "Client %s is using incompatible and more recent API version %i.%i", c->name_with_id, major, minor );

        osc_server->send( lo_message_get_source( msg ), "/error",
                          path,
                          ERR_INCOMPATIBLE_API,
                          "Server is using an incompatible API version." );

        return 0;
    }

    c->pid = pid;
    c->capabilities = strdup( capabilities );
    c->addr = lo_address_new_from_url( lo_address_get_url( lo_message_get_source( msg ) ));
    c->name = strdup( client_name ); //replace executable with clients self-reported pretty name
    c->active = true;

    asprintf( &c->name_with_id, "%s.%s", c->name, c->client_id );


    MESSAGE( "Process %s has pid: %i", c->name_with_id, pid );

    if ( ! expected_client )
        client.push_back( c );

    MESSAGE( "The client \"%s\" at \"%s\" informs us it's ready to receive commands.", &argv[0]->s, lo_address_get_url( c->addr ) );

    osc_server->send( lo_message_get_source( msg ), "/reply",
                      path,
                      expected_client ?
                      "Acknowledged as full NSM client (started ourselves)." :
                      "Acknowledged as full NSM client (registered itself from the outside).",
                      APP_TITLE,
                      ":server-control:broadcast:optional-gui:" );

    c->status = "open";
    if ( gui_is_active )
    {
        osc_server->send( gui_addr, "/nsm/gui/client/new", c->client_id, c->name ); //pretty-name. not exectuable
        osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status );

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

int
save_session_file ( )
{
    char *session_file = NULL;
    asprintf( &session_file, "%s/session.nsm", session_path );

    FILE *fp = fopen( session_file, "w" );

    if ( fp == NULL ) {
        WARNING( "No write access to %s with error: %s.", session_file, strerror( errno ) );
        free( session_file );
        //No need to fclose because fp is null and was never opened.
        return 1; //error
    }

    free( session_file );

    for ( std::list<Client*>::iterator i = client.begin();
          i != client.end();
          ++i )
    {
        fprintf( fp, "%s:%s:%s\n", (*i)->name, (*i)->executable_path, (*i)->client_id );
    }

    fclose( fp );
    return 0; //all ok
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
             {
                MESSAGE( "Waiting for %s", (*i)->name_with_id ); //This replaced the Loop 1, Loop 2 ... 60 message from wait_for_dumb_clients_to_die where you couldn't see which client actually was hanging
                return true;
              }
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
             {
                MESSAGE( "Waiting for %s", (*i)->name_with_id ); //This replaced the Loop 1, Loop 2 ... 60 message from wait_for_killed_clients_to_die where you couldn't see which client actually was hanging
                return true;
              }
    }

    return false;
}

void
wait_for_killed_clients_to_die ( )
{
    struct signalfd_siginfo fdsi;

    MESSAGE( "Waiting 30 seconds for killed clients to die." );
    for ( int i = 0; i < 30; i++ )
    {
        if ( ! killed_clients_are_alive() )
        {
            MESSAGE( "All clients have died." );
            return;
        }

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


    /* The clients that are still alive are dangerous to the user.
    * Their GUI will be most likely hidden or non-responsive, their jack client still open.
    * And now the session will close, maybe even nsmd will quit. The hanging process is left
    * open and invisible to the user. As a last resort it must be killed until we lose control over
    * the process.
    */
    WARNING( "Killed clients are still alive" );
    std::list<Client*> *cl = &client;
    for ( std::list<Client*>::iterator i = cl->begin();
          i != cl->end();
          ++i )
    {
        if ( (*i)->pid > 0 )
             {
                WARNING( "SIGKILL to: %s", (*i)->name_with_id );
                kill( (*i)->pid, SIGKILL );
             }
    }
    return;
}


void
command_all_clients_to_save ( )
{
    if ( session_path )
    {
        GUIMSG( "Commanding attached clients to save." );

        int save_error = save_session_file();

        if ( save_error == 1 ) {
            GUIMSG( "...but the session file is write protected. Will not forward save command to clients." );
            WARNING( "Aborting client save commands because the session file is write protected" );
            return;
        }



        for ( std::list<Client*>::iterator i = client.begin();
              i != client.end();
              ++i )
        {
            command_client_to_save( *i );
        }

        wait_for_replies();
    }
}

void
command_client_to_stop ( Client *c )
{
    GUIMSG( "Stopping client %s", c->name_with_id );

    if ( c->pid > 0 )
    {
        c->pending_command( COMMAND_KILL );

        kill( c->pid, SIGTERM );

        c->status = "stopped";
        if ( gui_is_active )
            osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status );
    }
}

void
command_client_to_quit ( Client *c )
{
    MESSAGE( "Commanding %s to quit", c->name_with_id );

    if ( c->active )
    {
        c->pending_command( COMMAND_QUIT );

        kill( c->pid, SIGTERM );

        c->status = "quit";
        if ( gui_is_active )
            osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status );
    }
    else if ( c->is_dumb_client() )
    {
        if ( c->pid > 0 )
        {
            c->status = "quit";
            if ( gui_is_active )
                osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status );

            /* should be kill? */
            c->pending_command( COMMAND_QUIT );

            // this is a dumb client... try and kill it
            kill( c->pid, SIGTERM );
        }
        else
        {
            c->status = "removed";
            if ( gui_is_active )
                osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status );
        }
    }
}

char *
get_lock_file_name( const char * session_name, const char * full_absolute_session_path )
{
    // To avoid collisions of two simple session names under either different subdirs o even different session roots.
    char *session_hash = simple_hash( full_absolute_session_path );

    char *session_lock;
    asprintf( &session_lock, "%s/%s%s", lockfile_directory, session_name, session_hash ); //lockfile_directory and session_name are variables in the current context.

    free(session_hash);
    return session_lock;
}


void
write_lock_file(  const char *filename, const char * session_path )
{
    //Not a GNU lockfile, which features were never used by nsmd anyway,
    //but simply a file with information about the NSM Server and the loaded session
    FILE *fp = fopen( filename, "w" );
    if ( !fp )  {
        FATAL( "Failed to write lock file to %s with error: %s", filename, strerror( errno ) );
    }

    fprintf( fp, "%s\n%s\n%d\n", session_path, osc_server->url(), getpid());
    MESSAGE( "Created lock file %s", filename );

    fclose( fp );
}

void
delete_lock_file( const char *filename )
{
    unlink( filename );
    MESSAGE( "Deleted lock file %s", filename );
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
        char * session_lock = get_lock_file_name( session_name, session_path);
        delete_lock_file( session_lock );
        MESSAGE( "Session %s was closed.", session_path  );

        free(session_lock);
        free(session_path);
        session_path = NULL;
        free(session_name);
        session_name = NULL;
    }

    if ( gui_is_active )
    {
        osc_server->send( gui_addr, "/nsm/gui/session/name", "", "" ); //Empty string = no current session
    }
}

void
tell_client_session_is_loaded( Client *c )
{
    if ( c->active )
//!c->is_dumb_client() )
    {
        MESSAGE( "Telling client %s that session is loaded.", c->name_with_id );
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
    //parameter "path" is the absolute path to the session including session root, without session.nsm

    //First check if the session file actually exists, before closing the current one
    const char * relative_session_path =  strdup( path ) + strlen( session_root ) + 1; //+1 for trailing /
    if ( session_already_exists( relative_session_path ) != 0) {
        WARNING ( "Instructed to load %s which does not exist. Doing nothing.", path );
        return ERR_NO_SUCH_FILE;
    }


    if ( session_path && session_name ) {
        //We are already in a session. This is switch, or load during duplicate etc.
        MESSAGE ( "Instructed to load %s while %s is still open. This is a normal operation. Attempting to switch clients intelligently, if they support it. Otherwise closing and re-opening.", path, session_path );
        char * session_lock = get_lock_file_name( session_name, session_path);
        delete_lock_file( session_lock );
    }

    set_name( path ); //Do this first so we have the simple name name for lockfiles and log messages

    char *session_file = NULL;
    asprintf( &session_file, "%s/session.nsm", path );

    //Check if the lockfile already exists, which means another nsmd currently has loaded the session we want to load.
    char * session_lock = get_lock_file_name( session_name, path );
    struct stat st_lockfile_exists_check;
    if ( stat (session_lock, &st_lockfile_exists_check) == 0)
    {
        WARNING( "Session %s is already loaded from another nsmd and locked by file %s", session_name, session_lock );

        free( session_file );
        free( session_lock );

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

            asprintf( &c->name_with_id, "%s.%s", c->name, c->client_id );

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

    //We already checked if the logfile exists above, and it didn't.
    //We also tested for write permissions to our XDG run-dir, which we confirmed to have.
    //We can create the lockfile now.
    write_lock_file( session_lock, session_path );
    MESSAGE( "Session %s was loaded.", session_path);

    new_clients.clear();

    if ( gui_is_active )
    {   //This is not the case when --load-session was used. GUI announce will come later.

        //Send two parameters to signal that the session was loaded: simple session-name, relative session path below session root
        MESSAGE( "Informing GUI about running session name: %s with relative path %s", session_name, session_path + strlen( session_root ) );
        osc_server->send( gui_addr, "/nsm/gui/session/name", session_name, session_path + strlen( session_root ));
    }

    return ERR_OK;
}

OSC_HANDLER( save )
{
    lo_address sender_addr;
    sender_addr = lo_address_new_from_url( lo_address_get_url( lo_message_get_source( msg ) ));

    if ( pending_operation != COMMAND_NONE )
    {
            osc_server->send( sender_addr, "/error", path,
                              ERR_OPERATION_PENDING,
                              "An operation pending." );
            return 0;
    }

    if ( ! session_path )
    {
        osc_server->send( sender_addr, "/error", path,
                          ERR_NO_SESSION_OPEN,
                          "No session to save.");

        goto done;
    }

    command_all_clients_to_save();

    MESSAGE( "Done." );

    osc_server->send( sender_addr, "/reply", path, "Saved." );

done:

    pending_operation = COMMAND_NONE;

    return 0;
}

OSC_HANDLER( duplicate )
{
    lo_address sender_addr;
    sender_addr = lo_address_new_from_url( lo_address_get_url( lo_message_get_source( msg ) ));

    if ( pending_operation != COMMAND_NONE )
    {
            osc_server->send( sender_addr, "/error", path,
                              ERR_OPERATION_PENDING,
                              "An operation pending." );
            return 0;
    }

    pending_operation = COMMAND_DUPLICATE;

    if ( ! session_path )
    {
        osc_server->send( sender_addr, "/error", path,
                          ERR_NO_SESSION_OPEN,
                          "No session to duplicate.");
        goto done;
    }

    if ( ! path_is_valid( &argv[0]->s ) )
    {
        osc_server->send( sender_addr, "/error", path,
                          ERR_CREATE_FAILED,
                          "Invalid session name." );

        goto done;
    }

    if ( session_already_exists(&argv[0]->s) == 0) {
        osc_server->send( sender_addr, "/error", path,
                          ERR_CREATE_FAILED,
                          "Session name already exists." );

        pending_operation = COMMAND_NONE;

        return 0;
    }

    command_all_clients_to_save();

    if ( clients_have_errors() )
    {
        osc_server->send( sender_addr, "/error", path,
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

    MESSAGE( "Attempting to open during DUPLICATE: %s", spath );

    //The original session is still open. load_session_file will close it, and possibly ::switch::

    if ( !load_session_file( spath ) )
    {
        MESSAGE( "Loaded" );
        osc_server->send( sender_addr, "/reply", path,
                          "Loaded." );
    }
    else
    {
        MESSAGE( "Failed" );
        osc_server->send( sender_addr, "/error", path,
                          ERR_NO_SUCH_FILE,
                          "No such file." );
        free(spath);
        pending_operation = COMMAND_NONE;
        return -1;
    }

    free( spath );

    MESSAGE( "Done" );

    osc_server->send( sender_addr, "/reply", path, "Duplicated." );

done:

    pending_operation = COMMAND_NONE;

    return 0;
}

OSC_HANDLER( new )
{
    lo_address sender_addr;
    sender_addr = lo_address_new_from_url( lo_address_get_url( lo_message_get_source( msg ) ));

    if ( pending_operation != COMMAND_NONE )
    {
            osc_server->send( sender_addr, "/error", path,
                              ERR_OPERATION_PENDING,
                              "An operation pending." );
            return 0;
    }

    pending_operation = COMMAND_NEW;

    if ( ! path_is_valid( &argv[0]->s ) )
    {
        osc_server->send( sender_addr, "/error", path,
                          ERR_CREATE_FAILED,
                          "Invalid session name." );

        pending_operation = COMMAND_NONE;

        return 0;
    }

    if ( session_already_exists(&argv[0]->s) == 0) {
        osc_server->send( sender_addr, "/error", path,
                          ERR_CREATE_FAILED,
                          "Session name already exists." );

        pending_operation = COMMAND_NONE;

        return 0;
    }


    if ( session_path ) //Already a session running?
    {
        command_all_clients_to_save();

        close_session();
    }

    GUIMSG( "Creating new session \"%s\"", &argv[0]->s );

    char *spath;
    asprintf( &spath, "%s/%s", session_root, &argv[0]->s );

    if ( mkpath( spath, true ) )
    {
        osc_server->send( sender_addr, "/error", path,
                          ERR_CREATE_FAILED,
                          "Could not create the session directory" );

        free(spath);

        pending_operation = COMMAND_NONE;

        return 0;
    }

    session_path = strdup( spath );

    set_name( session_path );

    char * session_lock = get_lock_file_name( session_name, session_path);
    write_lock_file( session_lock, session_path );
    free ( session_lock );


    osc_server->send( sender_addr, "/reply", path, "Created." );

    if ( gui_is_active )
    {
        osc_server->send( gui_addr,  "/nsm/gui/session/session", &argv[0]->s  );

        //Send two parameters to signal that the session was loaded: simple session-name, relative session path below session root
        MESSAGE( "Informing GUI about running session name: %s with relative path %s", session_name, session_path + strlen( session_root ) );
        osc_server->send( gui_addr, "/nsm/gui/session/name", session_name, session_path + strlen( session_root ));
    }

    save_session_file();

    free( spath );

    osc_server->send( sender_addr, "/reply", path, "Session created" );

    pending_operation = COMMAND_NONE;

    return 0;
}

int
fts_comparer_to_process_files_before_dirs( const FTSENT ** first, const FTSENT ** second )
{
    /*
       The argument compar() specifies a user-defined function which may
       be used to order the traversal of the hierarchy.  It takes two
       pointers to pointers to FTSENT structures as arguments and should
       return a negative value, zero, or a positive value to indicate if
       the file referenced by its first argument comes before, in any
       order with respect to, or after, the file referenced by its
       second argument.  The fts_accpath, fts_path, and fts_pathlen
       fields of the FTSENT structures may never be used in this
       comparison.  If the fts_info field is set to FTS_NS or FTS_NSOK,
       the fts_statp field may not either.  If the compar() argument is
       NULL, the directory traversal order is in the order listed in
       path_argv for the root paths, and in the order listed in the
       directory for everything else.
    */

    if ( (*first)->fts_info & FTS_F )
        return (-1); //first
    else if ( (*second)->fts_info & FTS_F )
        return (1); //last
    else
        return strcmp((*first)->fts_name, (*second)->fts_name);
        //return (0); //doesn't matter
}


static lo_address list_response_address;

OSC_HANDLER( list )
{
    //Parse the session_root recursively for session.nsm files and send names with /nsm/server/list
    //Sessions can be structured with sub-directories.
    //The file session.nsm marks a real session and is a 'leaf' of the session tree.
    //No other sessions are allowed below a dir containing session.nsm .

    GUIMSG( "Listing sessions" );
    list_response_address = lo_message_get_source( msg );

    //Use fts to walk the session_root
    /* An array of paths to traverse. Each path must be null
     * terminated and the list must end with a NULL pointer. */
    char *paths[] = { session_root, NULL };

    /* 2nd parameter: An options parameter. Must include either
       FTS_PHYSICAL or FTS_LOGICAL---they change how symbolic links
       are handled.

       Last parameter is a comparator which you can optionally provide
       to change the traversal of the filesystem hierarchy.

       Our comparator processes files before directories, so we can depend on that
       to remember if we are already in a session-dir.
    */

    FTS *ftsp = fts_open(paths, FTS_LOGICAL, fts_comparer_to_process_files_before_dirs);
    if(ftsp == NULL)
    {
        FATAL( "fts_open" );
        exit(EXIT_FAILURE);
    }

    FTSENT * currentSession = NULL;
    while( 1 ) // call fts_read() enough times to get each file
    {
        FTSENT *ent = fts_read(ftsp); // get next entry (could be file or directory).
        if( ent == NULL )
        {
            if( errno == 0 )
                break; // No more items, bail out of while loop
            else
            {
                // fts_read() had an error.
                FATAL( "fts_read" );
                exit(EXIT_FAILURE);
            }
        }

        // Handle Types of Files

        // Given a "entry", determine if it is a file or directory
        if( ent->fts_info & FTS_D )   // We are entering into a directory
            {
                //printf( "Enter dir: %s\n", ent->fts_path );
                if ( currentSession != NULL )
                {
                    //printf( "already found current session.nsm: %s  . Waiting to leave dir. Ignoring %s\n", currentSession->fts_name, ent->fts_path );

                    // Setup that no descendants of this file are visited.
                    int err = fts_set( ftsp, ent, FTS_SKIP );
                    if ( err != 0 )
                    {
                        FATAL( "fts_set" );
                        exit(EXIT_FAILURE);
                    }
                }
            }
        else if( ent->fts_info & FTS_DP ) // We are exiting a directory
            {
                //printf( "Exit dir: %s\n", ent->fts_path );
                if ( ent == currentSession )
                {
                    //printf( "Exit current session dir: %s\n", ent->fts_path );
                    currentSession = NULL;
                }
            }
        else if( ent->fts_info & FTS_F ) // The entry is a file.
        {
            //printf( "File: %s\n", ent->fts_path );
            if ( ! strcmp( "session.nsm", basename( ent->fts_path ) ) )
            {
                //Convert path to session name:
                char *s;
                s = strdup( ent->fts_path );
                s = dirname( s );
                memmove( s, s + strlen( session_root ) + 1, (strlen( s ) - strlen( session_root )) + 1);
                osc_server->send( list_response_address, "/reply", "/nsm/server/list", s  );
                free( s );
                currentSession = ent->fts_parent; //save the directory entry. not the session.nsm entry.
            }
        }
    }

    // close fts and check for error closing.
    if(fts_close(ftsp) == -1)
        FATAL( "fts_close" );

    // As marker that all sessions were sent reply with an empty string, which is impossible to conflict with a session name
    osc_server->send( list_response_address, "/reply", "/nsm/server/list", "" );
    return 0;
}


OSC_HANDLER( open )
{
    lo_address sender_addr;
    sender_addr = lo_address_new_from_url( lo_address_get_url( lo_message_get_source( msg ) ));

    GUIMSG( "Opening session %s", &argv[0]->s );

    if ( pending_operation != COMMAND_NONE )
    {
            osc_server->send( sender_addr, "/error", path,
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
            osc_server->send( sender_addr, "/error", path,
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
        osc_server->send( sender_addr, "/reply", path,
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


        osc_server->send( sender_addr, "/error", path,
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

    handle_signal_clean_exit(0);

    return 0;
}

OSC_HANDLER( abort )
{
    lo_address sender_addr;
    sender_addr = lo_address_new_from_url( lo_address_get_url( lo_message_get_source( msg ) ));

    if ( pending_operation != COMMAND_NONE )
        {
                osc_server->send( sender_addr, "/error", path,
                                ERR_OPERATION_PENDING,
                                "An operation pending." );
                return 0;
        }

    pending_operation = COMMAND_CLOSE;


    if ( ! session_path )
    {
        osc_server->send( sender_addr, "/error", path,
                          ERR_NO_SESSION_OPEN,
                          "No session to abort." );

        goto done;
    }

    GUIMSG( "Commanding attached clients to quit." );

    close_session();

    osc_server->send( sender_addr, "/reply", path,
                      "Aborted." );

    MESSAGE( "Done" );

done:

    pending_operation = COMMAND_NONE;

    return 0;
}

OSC_HANDLER( close )
{
    lo_address sender_addr;
    sender_addr = lo_address_new_from_url( lo_address_get_url( lo_message_get_source( msg ) ));

    if ( pending_operation != COMMAND_NONE )
    {
            osc_server->send( sender_addr, "/error", path,
                              ERR_OPERATION_PENDING,
                              "An operation pending." );
            return 0;
    }

    pending_operation = COMMAND_CLOSE;

    if ( ! session_path )
    {
        osc_server->send( sender_addr, "/error", path,
                          ERR_NO_SESSION_OPEN,
                          "No session to close." );

        goto done;
    }
    command_all_clients_to_save();

    GUIMSG( "Commanding attached clients to quit." );

    close_session();

    osc_server->send( sender_addr, "/reply", path, "Closed." );

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
        osc_server->send( gui_addr, "/nsm/gui/client/label", c->client_id, c->label() );

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

    MESSAGE( "Client \"%s\" replied with error: %s (%i) in %fms", c->name_with_id, message, err_code, c->milliseconds_since_last_command() );
    c->pending_command( COMMAND_NONE );

    c->status = "error";
    if ( gui_is_active )
        osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status );

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

        MESSAGE( "Client \"%s\" replied with: %s in %fms", c->name_with_id, message, c->milliseconds_since_last_command() );

        c->pending_command( COMMAND_NONE );

        c->status = "ready";
        if ( gui_is_active )
            osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status );
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

            c->status = "removed";
            if ( gui_is_active )
                osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status );

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
    // This is send for a new and empty nsmd as well as already running, headless, ones.
    // If a GUI connects to an existing server with a running session this will trigger a list of
    // clients send to the new GUI.

    MESSAGE ( "A GUI announced to us from the URL %s", url );

    gui_addr = lo_address_new_from_url( url );
    gui_is_active = true; //global state

    if ( is_reply )
        // the default case. A GUI starts its own nsmd or connects to a running one
        osc_server->send( gui_addr, "/nsm/gui/gui_announce", "hi" );
    else
        //The server was started directly and instructed to connect to a running GUI.
        osc_server->send( gui_addr, "/nsm/gui/server_announce", "hi" );

    //The session root is not inluced in /nsm/gui/session/name
    //For the general information we need to send this message:
    osc_server->send( gui_addr, "/nsm/gui/session/root", session_root );



    //Send session name and relative path. If both are empty it signals that no session is currently open,
    //which is the default state if a GUI started nsmd.
    //No session_path without session_name. We only need to test for session_name.
    if ( !session_name || session_name[0]  == '\0' )
    {
        MESSAGE( "Informing GUI that no session is running by sending two empty strings" );
        osc_server->send( gui_addr, "/nsm/gui/session/name", "", "" ); //Empty string = no current session
    }
    else
    {
        // Send a list of clients to the newly registered GUI in case there was already a session open
        // First clients, then session name was original nsmd order.
        // We keep it that way, the only change is that we made even the attempt dependent on a running session.
        MESSAGE ( "Informing GUI about %li already running clients", client.size() );
        for ( std::list<Client*>::iterator i = client.begin();
              i != client.end();
              ++i )
        {
            Client *c = *i;
            osc_server->send( gui_addr, "/nsm/gui/client/new", c->client_id, c->executable_path ); // we send new twice. see announce() comment
            if ( c->status )
                osc_server->send( gui_addr, "/nsm/gui/client/status", c->client_id, c->status );
            if ( c->is_capable_of( ":optional-gui:" ) )
                osc_server->send( gui_addr, "/nsm/gui/client/has_optional_gui", c->client_id );
            if ( c->label() ) // could be NULL
                osc_server->send( gui_addr, "/nsm/gui/client/label", c->client_id, c->label() );
            if ( c->active )
                osc_server->send( gui_addr, "/nsm/gui/client/new", c->client_id, c->name ); // upgrade to pretty-name
        }

        MESSAGE( "Informing GUI about running session name: %s with relative path %s", session_name, session_path + strlen( session_root ) );
        osc_server->send( gui_addr, "/nsm/gui/session/name", session_name, session_path + strlen( session_root ));
    }

    MESSAGE( "Registration with GUI complete" );
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


void
handle_signal_clean_exit ( int signal )
{
    WARNING( "Caught SIGNAL %i. Stopping nsmd.", signal);
    // We want a clean exit even when things go wrong.
    close_session();
    free( session_root );
    free( lockfile_directory );
    unlink( daemon_file );
    MESSAGE( "Deleted daemon file %s", daemon_file );
    free( daemon_file );

    exit(0);
}


int main(int argc, char *argv[])
{
     signal(SIGINT, handle_signal_clean_exit);
     signal(SIGTERM, handle_signal_clean_exit);
     signal(SIGSEGV, handle_signal_clean_exit);


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

    //Command line parameters
    char *osc_port = NULL;
    const char *gui_url = NULL;
    const char *load_session = NULL;

    static struct option long_options[] =
    {
        { "detach", no_argument, 0, 'd' },
        { "session-root", required_argument, 0, 's' },
        { "osc-port", required_argument, 0, 'p' },
        { "gui-url", required_argument, 0, 'g' },
        { "help", no_argument, 0, 'h' },
        { "version", no_argument, 0, 'v' },
        { "load-session", required_argument, 0, 'l'},
        { "quiet", no_argument, 0, 'q'},  //supresses all normal MESSAGE except WARNING and FATAL
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
                session_root = strdup(optarg);
                /* get rid of trailing slash */
                char *s = rindex(session_root,'/');
                if ( s == &session_root[strlen(session_root) - 1] )
                    *s = '\0';
                break;
            }
            case 'p':
                MESSAGE( "Using OSC port %s", optarg );
                osc_port = optarg;
                break;
            case 'g':
                MESSAGE( "Going to connect to GUI at: %s", optarg );
                gui_url = optarg;
                break;
            case 'l':
                MESSAGE( "Loading existing session file %s", optarg);
                load_session = optarg;
                break;
            case 'v':
                printf( "%s " VERSION_STRING "\n", argv[0] );
                exit(0);
                break;
            case 'q':
                quietMessages = true; //from debug.h
                break;
            case 'h':
                //Print usage message according to POSIX.1-2017
                const char *usage =
                "nsmd - Daemon and server for the 'New Session Manager'\n\n"
                "Usage:\n"
                "  nsmd\n"
                "  nsmd --help\n"
                "  nsmd --version\n"
                "\n"
                "Options:\n"
                "  --help                Show this screen\n"
                "  --version             Show version\n"
                "  --osc-port portnum    OSC port number [Default: provided by system].\n"
                "  --session-root path   Base path for sessions [Default: $XDG_DATA_HOME/nsm/].\n"
                "  --load-session name   Load existing session [Example: \"My Song\"].\n"
                "  --gui-url url         Connect to running legacy-gui [Example: osc.udp://mycomputer.localdomain:38356/].\n"
                "  --detach              Detach from console.\n"
                "  --quiet               Suppress messages except warnings and errors.\n"
                "\n\n"
                "nsmd can be run headless with existing sessions. To create new ones it is recommended to use a GUI\n"
                "such as nsm-legacy-gui (included) or Agordejo (separate package)\n"
                "";
                puts ( usage );
                exit(0);
                break;
        }
    }


    /*Get the XDG runtime directory for lockfiles
    * https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
    * /Unlike $XDG_DATA_HOME the runtime env var must be set, usually to /run/user/<id>/
    *
    * If the system is not XDG compliant (var not set) we fall back to /run/user/<id> directly
    * This is in accordance with https://en.wikipedia.org/wiki/Filesystem_Hierarchy_Standard 3.0
    */
    struct stat rundir_check;
    lockfile_directory = getenv( "XDG_RUNTIME_DIR" );

    //Env var is not set
    if ( !lockfile_directory || !lockfile_directory[0] ) {

        //manpage "These functions are always successful and never modify errno."
        //uid_t id = getuid();
        uid_t uid_for_rundir = geteuid(); // if the user was changed for this process. Usually just 1000, for the first user
        asprintf( &lockfile_directory, "/run/user/%d/", uid_for_rundir);
        WARNING( "$XDG_RUNTIME_DIR not set. You could set that manually. Now trying to fall back to %s", lockfile_directory);
    }
    //Now we have a directory path and need to test it.

    //Is set but no access
    if ( stat( lockfile_directory, &rundir_check ) != 0 ) {
        FATAL( "Failed to access FHS run-dir directory %s with error: %s", lockfile_directory, strerror( errno ) );
    }
    //Is set but not a directory
    else if ( !S_ISDIR(rundir_check.st_mode) ) {
        FATAL( "FHS run-dir is %s which is not a directory", lockfile_directory );
    }
    //A-Ok
    else {
        asprintf( &lockfile_directory, "%s/%s", lockfile_directory, "nsm");

        //Create the 'nsm' subdirectory. This may fail on it's own.
        struct stat st_lockfile_dir_mkdir;
        if ( stat( lockfile_directory, &st_lockfile_dir_mkdir ) )
        {
            if ( mkdir( lockfile_directory, 0771 ) )
            {
                FATAL( "Failed to create lock file directory %s with error: %s", lockfile_directory, strerror( errno ) );
            }
        }
        MESSAGE( "Using %s for lock-files.", lockfile_directory );

        //Now create another subdir for daemons .../nsm/d/ where each daemon has a port number file
        char * daemon_directory;
        asprintf( &daemon_directory, "%s/d", lockfile_directory);
        struct stat st_daemonfile_dir_mkdir;
        if ( stat( daemon_directory, &st_daemonfile_dir_mkdir ) )
        {
            if ( mkdir( daemon_directory, 0771 ) )
            {
                FATAL( "Failed to create daemon file directory %s with error: %s", daemon_directory, strerror( errno ) );
            }
        }
        //daemon_file is a global var
        asprintf( &daemon_file, "%s/%d", daemon_directory, getpid());
        free ( daemon_directory );
        MESSAGE( "Using %s as daemon file.", daemon_file );
        //The actual daemon file will be written below after announcing the session url.
    }

    if ( !session_root ) {
        /* The user gave no specific session directory. We use the default.
        * The default dir follows the XDG Basedir Specifications:
        * It is used by looking up environment variables.
        * https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html
        *    $XDG_DATA_HOME defines the base directory relative to which user-specific data files
        *    should be stored. If $XDG_DATA_HOME is either not set or empty, a default equal to
        *    $HOME/.local/share should be used.
        *
        * Up to version 1.5.3 the default dir was ~/NSM Sessions .
        * If this old directory exists we will use it but write mild warning to the log.
        * Moving old sessions is left to the user, or an external GUI.
        */
        struct stat st_session_root;

        //TODO: Valgrind shows a memory leak for the next line. Why?
        asprintf( &session_root, "%s/%s", getenv( "HOME" ), "NSM Sessions" );
        if ( stat( session_root, &st_session_root ) == 0 && S_ISDIR(st_session_root.st_mode)) {
            WARNING ( "An old session directory was detected in %s. You can continue to use it but it is recommended to move your sessions to $XDG_DATA_HOME/nsm/. If you don't know where that is simply rename your current session-directory and start nsmd, which will tell you the new directory.", session_root);
        }
        else {
            const char *xdg_data_home = getenv( "XDG_DATA_HOME" );
            if ( xdg_data_home ) {
                //If $XDG_DATA_HOME is explicitly set by the user we assume it to exist. We don't want to recursively create system directories.
                //If the xdg-dir does not exist yet we FATAL out just below.
                asprintf( &session_root, "%s/%s", xdg_data_home, "nsm" );
            }
            else {
                //If $XDG_DATA_HOME is either not set or empty, a default equal to $HOME/.local/share should be used.
                asprintf( &session_root, "%s/.local/share/nsm", getenv( "HOME" ));
            }
        }
    }

    struct stat st_session_root_mkdir;

    if ( stat( session_root, &st_session_root_mkdir ) )
    {
        if ( mkdir( session_root, 0771 ) )
        {
            FATAL( "Failed to create session directory %s with error: %s", session_root, strerror( errno ) );
        }
    }



    MESSAGE( "Session root is: %s", session_root );

    osc_server = new OSC::Endpoint();

    if ( osc_server->init( LO_UDP, osc_port ) )
    {
        WARNING( "Failed to create OSC server. Exiting." );
        exit( 1 );
    }

    char * url = osc_server->url();

    printf( "NSM_URL=%s\n", url );

    //Write the URL into the daemon_file that is named after our PID
    FILE *fpdaemon = fopen( daemon_file, "w" );
    if ( !fpdaemon )  {
        FATAL( "Failed to write daemon file to %s with error: %s", daemon_file, strerror( errno ) );
    }
    fprintf( fpdaemon, "%s\n", url );
    MESSAGE( "Created daemon file %s", daemon_file );
    fclose( fpdaemon );

    free( url );

    if ( gui_url )
    {
        //The server was started directly and instructed to connect to a running GUI.
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

   if ( load_session )
   {
        char *spath;
        asprintf( &spath, "%s/%s", session_root, load_session); // Build the session path. --load-session works with --session-root
        MESSAGE( "Loading session given by parameter %s", spath);
        load_session_file( spath );
        free ( spath );
    }

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
    int start_ppid = getppid(); //get parent pid
    for ( ;; )
    {
        wait( 1000 ); //1000 ms
        //This still has some corner cases, like a race condition on startup that never gets the real PID, but
        //we cover the majority of cases at least:
        if ( start_ppid != getppid() ) {
            WARNING ( "Our parent PID changed from %d to %d, which indicates a possible GUI crash. The user has no control over the session anymore. Trying to shut down cleanly.", start_ppid, getppid());
            handle_signal_clean_exit ( 0 );
        }
    }

    //Code after here will not be executed if nsmd is stopped with any abort-signal like SIGINT.
    //Without a signal handler clients will remain active ("zombies") without nsmd as parent.
    //Therefore exit is handled by handle_signal_clean_exit()
    return 0;
}
