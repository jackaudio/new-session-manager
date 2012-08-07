
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

#pragma GCC diagnostic ignored "-Wunused-parameter"

#define _MODULE_ "nsm-proxy"
#define APP_NAME "NSM Proxy"
#define APP_TITLE "NSM Proxy"

#include "debug.h"

#include <lo/lo.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/wait.h>

static lo_server losrv;
static lo_address nsm_addr;
static lo_address gui_addr;
static int nsm_is_active;
static char *project_file;
static int die_now = 0;
static int signal_fd;

static char *nsm_client_id;
static char *nsm_display_name;

#define CONFIG_FILE_NAME "nsm-proxy.config"

class NSM_Proxy {

    char *_label;
    char *_executable;
    char *_arguments;
    int _save_signal;
    int _pid;

public:

    NSM_Proxy ( )
        {
            _label = _executable = _arguments = 0;
            _save_signal = 0;
            _pid = 0;
        }

    ~NSM_Proxy ( )
        {
        }
    
    void kill ( void )
        {
            if ( _pid )
                ::kill( _pid, SIGTERM );
        }

    bool start ( const char *executable, const char *arguments )
        {
            if ( _executable )
                free( _executable );
            if ( _arguments )
                free( _arguments );

            _executable = strdup( executable );

            if ( arguments )
                _arguments = strdup( arguments );
            else
                _arguments = NULL;

            return start();
        }

    bool start ( void )
        {
            if ( _pid )
                /* already running */
                return true;

            int pid;
            if ( ! (pid = fork()) )
            {
                MESSAGE( "Launching %s\n", _executable );
                
//                char *args[] = { strdup( executable ), NULL };

                char *cmd;

                if ( _arguments )
                    asprintf( &cmd, "exec %s %s", _executable, _arguments );
                else
                    asprintf( &cmd, "exec %s", _executable );

                char *args[] = { _executable, strdup( "-c" ), cmd, NULL };
                
                setenv( "NSM_CLIENT_ID", nsm_client_id, 1 );
                setenv( "NSM_SESSION_NAME", nsm_display_name, 1 );
                
                if ( -1 == execvp( "/bin/sh", args ) )
                {
                    WARNING( "Error starting process: %s", strerror( errno ) );
                    
                    exit(-1);
                }
            }

            _pid = pid;

            return _pid > 0;
        }

    void save_signal ( int s )
        {
            _save_signal = s;
        }
    
    void label ( const char *s )
        {
            if ( _label )
                free( _label );

            _label = strdup( s );

            lo_send_from( nsm_addr, losrv, LO_TT_IMMEDIATE, "/nsm/client/label", "s", _label );
        }

    void save ( void )
        {
            DMESSAGE( "Sending process save signal" );
            if ( _pid )
                ::kill( _pid, _save_signal );
        }


    bool dump ( const char *path )
        {
            FILE *fp = fopen( path, "w" );

            if ( !fp )
            {
                WARNING( "Error opening file for saving: %s", strerror( errno ) );
                return false;
            }

            if ( _executable && strlen(_executable) )
                fprintf( fp, "executable\n\t%s\n", _executable );

            if ( _arguments && strlen(_arguments) )
                fprintf( fp, "arguments\n\t%s\n", _arguments );

            fprintf( fp, "save signal\n\t%i\n", _save_signal );
            
            if ( _label && strlen(_label) )
                fprintf( fp, "label\n\t%s\n", _label );

            fclose( fp );

            return true;
        }

    bool restore ( const char *path )
        {
            FILE *fp = fopen( path, "r" );

            if ( ! fp )
            {
                WARNING( "Error opening file for restore: %s", strerror( errno ) );
                return false;
            }

            char *name;
            char *value;

            MESSAGE( "Loading file config \"%s\"", path ); 

            while ( 2 == fscanf( fp, "%a[^\n]\n\t%a[^\n]\n", &name, &value ) )
            {

                DMESSAGE( "%s=%s", name, value );
                
                if ( !strcmp( name, "executable" ) )
                    _executable = value;
                else if (!strcmp( name, "arguments" ) )
                    _arguments = value;
                else if ( !strcmp( name, "save signal" ) )
                {
                    _save_signal = atoi( value );
                    free( value );
                }
                else if ( !strcmp( name, "label" ) )
                {
                    label( value );
                    free( value );
                }
                else
                {
                    WARNING( "Unknown option \"%s\" in config file", name );
                }

                free( name );
            }

            fclose( fp );

            start();

            return true;
        }

    void update ( lo_address to )
        {
            DMESSAGE( "Sending update" );

            lo_send_from( to, losrv, LO_TT_IMMEDIATE, "/nsm/proxy/save_signal", "i",  _save_signal );
            lo_send_from( to, losrv, LO_TT_IMMEDIATE, "/nsm/proxy/label", "s", _label ? _label : "" );
            lo_send_from( to, losrv, LO_TT_IMMEDIATE, "/nsm/proxy/executable", "s", _executable ? _executable : "" );
            lo_send_from( to, losrv, LO_TT_IMMEDIATE, "/nsm/proxy/arguments", "s", _arguments ? _arguments : "" );
        }
};

NSM_Proxy *nsm_proxy;

void
announce ( const char *nsm_url, const char *client_name, const char *process_name )
{
    printf( "Announcing to NSM\n" );

    lo_address to = lo_address_new_from_url( nsm_url );

    int pid = (int)getpid();

    lo_send_from( to, losrv, LO_TT_IMMEDIATE, "/nsm/server/announce", "sssiii",
                  client_name,
                  ":optional-gui:",
                  process_name,
                  1, /* api_major_version */
                  0, /* api_minor_version */
                  pid );

    lo_address_free( to );
}

bool
snapshot ( const char *file )
{
    /* mkdir( file, 0777 ); */
    
    char *path;
    asprintf( &path, "%s/%s", file, CONFIG_FILE_NAME );

    bool r = nsm_proxy->dump( path );

    free( path );

    return r;
}

bool
open ( const char *file )
{
    char *path;
    asprintf( &path, "%s/%s", file, CONFIG_FILE_NAME );

    bool r = nsm_proxy->restore( path );

    free( path );

    return r;
}

/****************/
/* OSC HANDLERS */
/****************/

/* NSM */

int
osc_announce_error ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    if ( strcmp( types, "sis" ) )
        return -1;

    if ( strcmp( "/nsm/server/announce", &argv[0]->s ) )
         return -1;

    printf( "Failed to register with NSM: %s\n", &argv[2]->s );
    nsm_is_active = 0;

    return 0;
}


int
osc_announce_reply ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    if ( strcmp( "/nsm/server/announce", &argv[0]->s ) )
         return -1;

    printf( "Successfully registered. NSM says: %s", &argv[1]->s );
    
    nsm_is_active = 1;
    nsm_addr = lo_address_new_from_url( lo_address_get_url( lo_message_get_source( msg ) ) );
    
    return 0;
}

int
osc_save ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    bool r = snapshot( project_file );

    nsm_proxy->save();

    if ( r )
        lo_send_from( nsm_addr, losrv, LO_TT_IMMEDIATE, "/reply", "ss", path, "OK" );
    else
        lo_send_from( nsm_addr, losrv, LO_TT_IMMEDIATE, "/error", "sis", path, -1, "Error saving project file" );

    return 0;
}

static int gui_pid;

void
show_gui ( void )
{

    int pid;
    if ( ! (pid = fork()) )
    {
        char executable[] = "nsm-proxy-gui";

        MESSAGE( "Launching %s\n", executable );
        
        char *url = lo_server_get_url( losrv );

        char *args[] = { executable, strdup( "--connect-to" ), url, NULL };
        
        if ( -1 == execvp( executable, args ) )
        {
            WARNING( "Error starting process: %s", strerror( errno ) );
            
            exit(-1);
        }
    }
    
    gui_pid = pid;

    lo_send_from( nsm_addr, losrv, LO_TT_IMMEDIATE, "/nsm/client/gui_is_shown", "" );
}

int
osc_show_gui ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    show_gui();

    /* FIXME: detect errors */

    lo_send_from( nsm_addr, losrv, LO_TT_IMMEDIATE, "/reply", "ss", path, "OK" );

    return 0;
}

void
hide_gui ( void )
{
    if ( gui_pid )
    {
        kill( gui_pid, SIGTERM );
    }
}

int
osc_hide_gui ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    hide_gui();

    lo_send_from( nsm_addr, losrv, LO_TT_IMMEDIATE, "/nsm/client/gui_is_hidden", "" );

    /* FIXME: detect errors */

    lo_send_from( nsm_addr, losrv, LO_TT_IMMEDIATE, "/reply", "ss", path, "OK" );

    return 0;
}

int
osc_open ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    const char *new_path = &argv[0]->s;
    const char *display_name = &argv[1]->s;
    const char *client_id = &argv[2]->s;

    if ( nsm_client_id )
        free(nsm_client_id);

    nsm_client_id = strdup( client_id );

    if ( nsm_display_name )
        free( nsm_display_name );

    nsm_display_name = strdup( display_name );

    char *new_filename;

    mkdir( new_path, 0777 );

    chdir( new_path );
    
    asprintf( &new_filename, "%s/%s", new_path, CONFIG_FILE_NAME );

    struct stat st;

    if ( 0 == stat( new_filename, &st ) )
    {
        if ( open( new_path ) )
        {
        }
        else
        {
            lo_send_from( nsm_addr, losrv, LO_TT_IMMEDIATE, "/error", "sis", path, -1, "Could not open file" );
            return 0;
        }

        lo_send_from( nsm_addr, losrv, LO_TT_IMMEDIATE, "/nsm/client/gui_is_hidden", "" );
    }
    else
    {
        show_gui();
    }

    if ( project_file )
        free( project_file );
    
    project_file = strdup( new_path );

// new_filename;

    lo_send_from( nsm_addr, losrv, LO_TT_IMMEDIATE, "/reply", "ss", path, "OK" );

    if ( gui_addr )
        nsm_proxy->update( gui_addr );

    return 0;
}



/* GUI */

int
osc_label ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    nsm_proxy->label( &argv[0]->s );

    return 0;
}

int
osc_save_signal ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    nsm_proxy->save_signal( argv[0]->i );
    
    return 0;
}

int
osc_start ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    snapshot( project_file );

    if ( nsm_proxy->start( &argv[0]->s, &argv[1]->s ) )
    {
        hide_gui();
    }
    
    return 0;
}

int
osc_kill ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    nsm_proxy->kill();
    
    return 0;
}

int
osc_update ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    lo_address to = lo_address_new_from_url( lo_address_get_url( lo_message_get_source( msg ) ));

    nsm_proxy->update( to );

    gui_addr = to;

    return 0;
}



void
signal_handler ( int x )
{
    die_now = 1;
}

void
set_traps ( void )
{
	signal( SIGHUP, signal_handler );
	signal( SIGINT, signal_handler );
//	signal( SIGQUIT, signal_handler );
//	signal( SIGSEGV, signal_handler );
//	signal( SIGPIPE, signal_handler );
	signal( SIGTERM, signal_handler );
}


void
init_osc ( const char *osc_port )
{
    losrv = lo_server_new( osc_port, NULL );
//error_handler );

    char *url = lo_server_get_url(losrv);
    printf("OSC: %s\n",url);
    free(url);

    /* NSM */
    lo_server_add_method( losrv, "/nsm/client/save", "", osc_save, NULL );
    lo_server_add_method( losrv, "/nsm/client/open", "sss", osc_open, NULL );
    lo_server_add_method( losrv, "/nsm/client/show_optional_gui", "", osc_show_gui, NULL );
    lo_server_add_method( losrv, "/nsm/client/hide_optional_gui", "", osc_hide_gui, NULL );
    lo_server_add_method( losrv, "/error", "sis", osc_announce_error, NULL );
    lo_server_add_method( losrv, "/reply", "ssss", osc_announce_reply, NULL );

    /* GUI */
    lo_server_add_method( losrv, "/nsm/proxy/label", "s", osc_label, NULL );
    lo_server_add_method( losrv, "/nsm/proxy/save_signal", "i", osc_save_signal, NULL );
    lo_server_add_method( losrv, "/nsm/proxy/kill", "", osc_kill, NULL );
    lo_server_add_method( losrv, "/nsm/proxy/start", "ss", osc_start, NULL );
    lo_server_add_method( losrv, "/nsm/proxy/update", "", osc_update, NULL );

}

void
die ( void )
{
    if ( gui_pid )
    {
        DMESSAGE( "Killing GUI" );

        kill( gui_pid, SIGTERM );
    }

    nsm_proxy->kill();

    exit(0);
}


void handle_sigchld ( )
{
    for ( ;; )
    {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);

        if (pid <= 0) 
            break;
        
        if ( pid == gui_pid )
        {
            lo_send_from( nsm_addr, losrv, LO_TT_IMMEDIATE, "/nsm/client/gui_is_hidden", "" );

            gui_pid = 0;

            /* don't care... */
            continue;
        }

        /* otherwise, it was our proxied process that died, so we should die too */
        printf( "proxied process died... nsm-proxy dying too\n" );

        die_now = 1;
    }
}

int
main ( int argc, char **argv )
{
    set_traps();

    sigset_t mask;

    sigemptyset( &mask );
    sigaddset( &mask, SIGCHLD );

    sigprocmask(SIG_BLOCK, &mask, NULL );

    signal_fd = signalfd( -1, &mask, SFD_NONBLOCK );

    nsm_proxy = new NSM_Proxy();

    init_osc( NULL );

    const char *nsm_url = getenv( "NSM_URL" );

    if ( nsm_url )
    {
        announce( nsm_url, APP_TITLE, argv[0] );
    }
    else
    {
        fprintf( stderr, "Could not register as NSM client.\n" );
        exit(1);
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
        
        lo_server_recv_noblock( losrv, 500 );

        if ( die_now )
            die();
    }
} 
