
/*******************************************************************************/
/* Copyright (C) 2008 Jonathan Moore Liles                                     */
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

/* jackpatch.c

  This program is just like ASSPatch, except that it works with Jack ports (audio and MIDI).

 */

#pragma GCC diagnostic ignored "-Wunused-parameter"

#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>

#include <jack/jack.h>

#include <lo/lo.h>

#include <pthread.h>

jack_client_t *client;

pthread_mutex_t port_lock;

lo_server losrv;
lo_address nsm_addr;
int nsm_is_active;

char *project_file;

#undef VERSION
#define APP_TITLE "JACKPatch"
#define VERSION	"0.2"

struct patch_record {
	struct {
		char *client;
		char *port;
	} src , dst;
    int active;                                                 /* true if patch has already been activate (by us) */
    struct patch_record *next;
};


struct port_record {
    char *port;
    int reg;                                                    /* true if registered, false if unregistered */
    struct port_record *next;
};

static struct port_record *new_ports = NULL;
static struct port_record *known_ports = NULL;

static struct patch_record *patch_list = NULL;

/**
 * Pretty-print patch relationship of /pr/
 */
void
print_patch ( struct patch_record *pr, int mode )
{
	printf( "%s from '%s:%s' to '%s:%s'\n", mode ? ">>" : "::",
			pr->src.client, pr->src.port, pr->dst.client, pr->dst.port );

}

void
enqueue ( struct patch_record *p )
{
	p->next = patch_list;
	patch_list = p;
}

void
dequeue ( struct patch_record *pr )
{
	if ( !pr )
		return;

	free( pr->src.port );
	free( pr->dst.port );
	free( pr->src.client );
	free( pr->dst.client );

	free( pr );
}

void
enqueue_port ( struct port_record **q, const char *port, int reg )
{
    struct port_record *p = malloc( sizeof( struct port_record ));

    p->port = strdup( port );
    p->reg = reg;
    p->next = *q;
    *q = p;
}

struct port_record *
dequeue_port ( struct port_record **q )
{
    if ( *q )
    {
        struct port_record *p = *q;

        *q = p->next;

        return p;
    }

    return NULL;
}

void enqueue_known_port ( const char *port )
{
    enqueue_port( &known_ports, port, 1 );
}

const char * find_known_port ( const char *port )
{
    struct port_record *pr;

    for ( pr = known_ports; pr; pr = pr->next )
        if ( !strcmp( port, pr->port ) )
            return pr->port;

    return NULL;
}


void
enqueue_new_port ( const char *port, int reg )
{
    pthread_mutex_lock( &port_lock );

    enqueue_port( &new_ports, port, reg );

    pthread_mutex_unlock( &port_lock );
}

struct port_record *
dequeue_new_port ( void )
{
    pthread_mutex_lock( &port_lock );

    struct port_record *p = dequeue_port( &new_ports );

    pthread_mutex_unlock( &port_lock );
    return p;
}


int
process_patch ( const char *patch )
{
    struct patch_record *pr;
    char *leftc, *rightc, *leftp, *rightp;
    char dir[3];

    int retval;

    retval = sscanf( patch, " %a[^:]:%a[^|] |%1[<>|] %a[^:]:%a[^\n]",
                     &leftc, &leftp, dir, &rightc, &rightp );

    if ( retval == EOF )
        return -1;

    if ( retval != 5 )
        return 0;

    /* trim space */
    int j;
    for ( j = strlen( leftp ) - 1; j > 0; j-- )
    {
        if ( leftp[j] == ' ' || leftp[j] == '\t' )
            leftp[j] = 0;
        else
            break;
    }

    dir[2] = 0;

    pr = malloc( sizeof( struct patch_record ) );

    switch ( *dir )
    {
        case '<':
            pr->src.client = rightc;
            pr->src.port = rightp;

            pr->dst.client = leftc;
            pr->dst.port = leftp;

            enqueue( pr );
            break;
        case '>':
            pr->src.client = leftc;
            pr->src.port = leftp;

            pr->dst.client = rightc;
            pr->dst.port = rightp;

            enqueue( pr );
            break;
        case '|':
            pr->src.client = rightc;
            pr->src.port = rightp;

            pr->dst.client = leftc;
            pr->dst.port = leftp;

            enqueue( pr );

            pr = malloc( sizeof( struct patch_record ) );

            pr->src.client = strdup( leftc );
            pr->src.port = strdup( leftp );

            pr->dst.client = strdup( rightc );
            pr->dst.port = strdup( rightp );

            enqueue( pr );
            break;
        default:
//            fprintf( stderr, "Invalid token '|%s' at line %i of %s!",  dir, i, file );
            free( pr );
            return 0;
    }

    pr->active = 0;

    print_patch( pr, 1 );

    return 1;
}

void
clear_all_patches ( )
{
    struct patch_record *pr;

    while ( patch_list )
    {
        pr = patch_list;
        patch_list = pr->next;
        dequeue( pr );
    }
}

/**
 * Crudely parse configuration file named by /file/ using fscanf
 */
int
read_config ( const char *file )
{
    FILE *fp;
    int i = 0;

    if ( NULL == ( fp = fopen( file, "r" ) ) )
         return 0;

    clear_all_patches();

    while ( !feof( fp ) && !ferror( fp ) )
    {
        int retval;
        unsigned int k;
        char buf[BUFSIZ];

        i++;

        for ( k = 0; k < sizeof( buf ) - 1; k++ )
        {
            retval = fread( buf + k, 1, 1, fp );
            if ( retval != 1 )
                break;
            if ( buf[k] == '\n' )
            {
                if ( k == 0 )
                    continue;
                else
                    break;
            }
        }

        if ( retval == 0 )
            break;

        retval = process_patch( buf );

        if ( retval < 0 )
            break;

        if ( retval == 0 )
        {
            printf( "bad line %i.\n", i );
            continue;
        }
    }

    fclose( fp );

    return 1;
}



/* returns 0 if connection failed, true if succeeded. Already connected
 * is not considered failure */
void
connect_path ( struct patch_record *pr )
{
    int r = 0;

    char srcport[512];
    char dstport[512];

    snprintf( srcport, 512, "%s:%s", pr->src.client, pr->src.port );
    snprintf( dstport, 512, "%s:%s", pr->dst.client, pr->dst.port );

    if ( pr->active )
    {
        /* patch is already active, don't bother JACK with it... */
        return;
    }

    if ( ! ( find_known_port( srcport ) && find_known_port( dstport )  ) )
    {
        /* one of the ports doesn't exist yet... don't attempt
         * connection, jack will just complain. */
        printf( "Not attempting connection because one of the ports is missing.\n" );
    }

    printf( "Connecting %s |> %s\n", srcport, dstport );

    r = jack_connect( client, srcport, dstport );

    print_patch( pr, r );

    if ( r == 0 || r == EEXIST )
    {
        pr->active = 1;
        return;
    }
    else
    {
        pr->active = 0;
        printf( "Error is %i\n", r );
        return;
    }
}


void
do_for_matching_patches ( const char *portname, void (*func)( struct patch_record * ) )
{
    struct patch_record *pr;

    char client[512];
    char port[512];

    sscanf( portname, "%[^:]:%[^\n]", client, port );

    for ( pr = patch_list; pr; pr = pr->next )
    {
        if ( ( !strcmp( client, pr->src.client ) && !strcmp( port, pr->src.port ) ) ||
             ( !strcmp( client, pr->dst.client ) && !strcmp( port, pr->dst.port ) ) )
        {
            func( pr );
        }
    }
}

void
inactivate_path ( struct patch_record *pr )
{
    pr->active = 0;
}

void
inactivate_patch ( const char *portname )
{
    do_for_matching_patches( portname, inactivate_path );
}

void
activate_patch ( const char *portname )
{
    do_for_matching_patches( portname, connect_path );
}

void remove_known_port ( const char *port )
{
    /* remove it from the list of known ports */
    {
        struct port_record *pr;
        struct port_record *lp = NULL;
        
        for ( pr = known_ports; pr; lp = pr, pr = pr->next )
            if ( !strcmp( port, pr->port ) )
            {
                if ( lp )
                    lp->next = pr->next;
                else
                    known_ports = pr->next;
                
                free( pr->port );
                free( pr );
                
                break;
            }
    }
    
    /* now mark all patches including this port as inactive */
    inactivate_patch ( port );
}


/**
 * Attempt to activate all connections in patch list
 */
void
activate_all_patches ( void )
{
    struct patch_record *pr;

    for ( pr = patch_list; pr; pr = pr->next )
        connect_path( pr );
}

/** called for every new port */
void
handle_new_port ( const char *portname )
{
    enqueue_known_port( portname );

    printf( "New endpoint '%s' registered.\n", portname );
    /* this is a new port */
    activate_patch( portname );
}

void
snapshot ( const char *file )
{
    FILE *fp;

    const char **port;
    const char **ports = jack_get_ports( client, NULL, NULL,  JackPortIsOutput );

    if ( ! ports )
        return;

    if ( NULL == ( fp = fopen( file, "w" ) ) )
    {
        fprintf( stderr, "Error opening snapshot file for writing" );
        return;
    }

    for ( port = ports; *port; port++ )
    {
        jack_port_t *p;

        p = jack_port_by_name( client, *port );

        const char **connections;
        const char **connection;

        connections = jack_port_get_all_connections( client, p );

        if ( ! connections )
            continue;

        for ( connection = connections; *connection; connection++ )
        {
            fprintf( fp, "%-40s |> %s\n", *port, *connection );
            printf( "++ %s |> %s\n", *port, *connection );
        }

        free( connections );
    }

    free( ports );

    fclose( fp );
}

static int die_now = 0;

void
signal_handler ( int x )
{
    die_now = 1;
}

void
die ( void )
{
    jack_deactivate( client );
    jack_client_close( client );
    client = NULL;
    exit( 0 );
}

/** set_traps
 *
 * Handle signals
 */
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

/****************/
/* OSC HANDLERS */
/****************/

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
    snapshot( project_file );

    lo_send_from( nsm_addr, losrv, LO_TT_IMMEDIATE, "/reply", "ss", path, "OK" );

    return 0;
}

int
osc_open ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    const char *new_path = &argv[0]->s;
//    const char *display_name = &argv[1]->s;

    char *new_filename;
    
    asprintf( &new_filename, "%s.jackpatch", new_path );

    struct stat st;

    if ( 0 == stat( new_filename, &st ) )
    {
        if ( read_config( new_filename ) )
        {
            printf( "Reading patch definitions from: %s\n", new_filename );
            /* wipe_ports(); */
            /* check_for_new_ports(); */
            activate_all_patches();
        }
        else
        {
            lo_send_from( nsm_addr, losrv, LO_TT_IMMEDIATE, "/error", "sis", path, -1, "Could not open file" );
            return 0;
        }
    }
    else
    {
        clear_all_patches();
    }

    if ( project_file )
        free( project_file );
    
    project_file = new_filename;

    lo_send_from( nsm_addr, losrv, LO_TT_IMMEDIATE, "/reply", "ss", path, "OK" );

    return 0;
}

void
announce ( const char *nsm_url, const char *client_name, const char *process_name )
{
    printf( "Announcing to NSM\n" );

    lo_address to = lo_address_new_from_url( nsm_url );

    int pid = (int)getpid();

    lo_send_from( to, losrv, LO_TT_IMMEDIATE, "/nsm/server/announce", "sssiii",
                  client_name,
                  ":switch:",
                  process_name,
                  0, /* api_major_version */
                  8, /* api_minor_version */
                  pid );

    lo_address_free( to );
}

void
init_osc ( const char *osc_port )
{
    losrv = lo_server_new( osc_port, NULL );
//error_handler );

    char *url = lo_server_get_url(losrv);
    printf("OSC: %s\n",url);
    free(url);

    lo_server_add_method( losrv, "/nsm/client/save", "", osc_save, NULL );
    lo_server_add_method( losrv, "/nsm/client/open", "sss", osc_open, NULL );
    lo_server_add_method( losrv, "/error", "sis", osc_announce_error, NULL );
    lo_server_add_method( losrv, "/reply", "ssss", osc_announce_reply, NULL );
}


void
check_for_new_ports ( void )
{
    struct port_record *p = NULL;

    while ( ( p = dequeue_new_port() ) )
    {
        if ( p->reg )
            handle_new_port( p->port );
        else
            remove_known_port( p->port );
        
        free( p->port );
        free( p );
    }
}

void
port_registration_callback( jack_port_id_t id, int reg, void *arg )
{
    jack_port_t *p = jack_port_by_id( client, id );
    
    const char *port = jack_port_name( p );

    enqueue_new_port( port, reg );
}

/*  */

int
main ( int argc, char **argv )
{

    /* get_args( argc, argv ); */

    jack_status_t status;

    client = jack_client_open( APP_TITLE, JackNullOption, &status );

    jack_set_port_registration_callback( client, port_registration_callback, NULL );

    if ( ! client )
    {
        fprintf( stderr, "Could not register JACK client\n" );
        exit(1);
        
    }

    pthread_mutex_init( &port_lock, NULL  );
    
    jack_activate( client );

    set_traps();

    if ( argc > 1 )
    {
        if ( ! strcmp( argv[1], "--save" ) )
        {
            if ( argc > 2 )
            {
                printf( "Saving current graph to: %s\n", argv[2] );
                snapshot( argv[2] );
                exit(0);
            }
        }
        else
        {
            read_config( argv[1] );
            printf( "Monitoring...\n" );
            for ( ;; )
            {
                usleep( 50000 );
                check_for_new_ports();
            }
        }
    }

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

    for ( ;; )
    {
        lo_server_recv_noblock( losrv, 500 );

        check_for_new_ports();

        if ( die_now )
            die();
    }
}
