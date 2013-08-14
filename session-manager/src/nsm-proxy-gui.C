
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


#define _MODULE_ "nsm-proxy-gui"

#define APP_NAME "NSM Proxy"
#define APP_TITLE "NSM Proxy"

#include <FL/Fl_File_Chooser.H>
#include "NSM_Proxy_UI.H"
#include <lo/lo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

lo_server losrv;
lo_address nsmp_addr;

static NSM_Proxy_UI *ui;

int
osc_update ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    printf( "Got update for %s\n", path );

    Fl::lock();

    if (!strcmp( path, "/nsm/proxy/label" ))
        ui->label_input->value( &argv[0]->s );
    else if (!strcmp( path, "/nsm/proxy/arguments" ))
        ui->arguments_input->value( &argv[0]->s );
    else if (!strcmp( path, "/nsm/proxy/executable" ))
        ui->executable_input->value( &argv[0]->s );
    else if (!strcmp( path, "/nsm/proxy/config_file" ))
        ui->config_file_input->value( &argv[0]->s );
    else if (!strcmp( path, "/nsm/proxy/save_signal" ))
    {
        if ( argv[0]->i == SIGUSR1 )
            ui->save_signal_choice->value( 1 );
        else if ( argv[0]->i == SIGUSR2 )
            ui->save_signal_choice->value( 2 );
        else if ( argv[0]->i == SIGINT )
            ui->save_signal_choice->value( 3 );
        else
            ui->save_signal_choice->value( 0 );
    }
    else if (!strcmp( path, "/nsm/proxy/stop_signal" ))
    {
        if ( argv[0]->i == SIGTERM )
            ui->stop_signal_choice->value( 0 );
        else if ( argv[0]->i == SIGINT )
            ui->stop_signal_choice->value( 1 );
        else if ( argv[0]->i == SIGHUP )
            ui->stop_signal_choice->value( 2 );
    }

    Fl::unlock();

    return 0;
}


void
init_osc ( const char *osc_port )
{

    lo_server_thread loth = lo_server_thread_new( osc_port, NULL );
    losrv = lo_server_thread_get_server( loth );

//error_handler );

    char *url = lo_server_get_url(losrv);
    printf("OSC: %s\n",url);
    free(url);

    /* GUI */

    lo_server_thread_add_method( loth, "/nsm/proxy/executable", "s", osc_update, NULL );
    lo_server_thread_add_method( loth, "/nsm/proxy/arguments", "s", osc_update, NULL );
    lo_server_thread_add_method( loth, "/nsm/proxy/config_file", "s", osc_update, NULL );
    lo_server_thread_add_method( loth, "/nsm/proxy/label", "s", osc_update, NULL );
    lo_server_thread_add_method( loth, "/nsm/proxy/save_signal", "i", osc_update, NULL );
    lo_server_thread_add_method( loth, "/nsm/proxy/stop_signal", "i", osc_update, NULL );

    lo_server_thread_start( loth );
}

/*****************/
/* GUI Callbacks */
/*****************/

void
handle_kill ( Fl_Widget *o, void *v )
{
    lo_send_from( nsmp_addr, losrv, LO_TT_IMMEDIATE, "/nsm/proxy/kill", "" );
}

void
handle_start ( Fl_Widget *o, void *v )
{
    lo_send_from( nsmp_addr, losrv,  LO_TT_IMMEDIATE, "/nsm/proxy/start", "sss",
                  ui->executable_input->value(),
                  ui->arguments_input->value(),
                  ui->config_file_input->value() );
}

void
handle_label ( Fl_Widget *o, void *v )
{
    lo_send_from( nsmp_addr, losrv, LO_TT_IMMEDIATE, "/nsm/proxy/label", "s",
                  ui->label_input->value() );
}

void
handle_executable ( Fl_Widget *o, void *v )
{
    ui->label_input->value( ui->executable_input->value() );
}


void
handle_config_file ( Fl_Widget *o, void *v )
{
}

void
handle_config_file_browse ( Fl_Widget *o, void *v )
{
    const char * file = fl_file_chooser( "Pick file", "*", NULL, 1 );

    ui->config_file_input->value( file );
}

void
handle_save_signal ( Fl_Widget *o, void *v )
{
    int sig = 0;
    
    const char* picked = ui->save_signal_choice->mvalue()->label();
    
    if ( !strcmp( picked, "SIGUSR1" ) )
        sig = SIGUSR1;
    else if ( !strcmp( picked, "SIGUSR2" ) )
        sig = SIGUSR2;
    else if ( !strcmp( picked, "SIGINT" ) )
        sig = SIGINT;

    lo_send_from( nsmp_addr, losrv,  LO_TT_IMMEDIATE,"/nsm/proxy/save_signal", "i",
                  sig );
}

void
handle_stop_signal ( Fl_Widget *o, void *v )
{
    int sig = SIGTERM;
    
    const char* picked = ui->stop_signal_choice->mvalue()->label();
    
    if ( !strcmp( picked, "SIGTERM" ) )
        sig = SIGTERM;
    else if ( !strcmp( picked, "SIGINT" ) )
        sig = SIGINT;
    else if ( !strcmp( picked, "SIGHUP" ) )
        sig = SIGHUP;

    lo_send_from( nsmp_addr, losrv,  LO_TT_IMMEDIATE,"/nsm/proxy/stop_signal", "i",
                  sig );
}

void
connect_ui ( void )
{
    ui->executable_input->callback( handle_executable, NULL );
    ui->config_file_input->callback( handle_config_file, NULL );
    ui->kill_button->callback( handle_kill, NULL );
    ui->start_button->callback( handle_start, NULL );
    ui->save_signal_choice->callback( handle_save_signal, NULL );
    ui->stop_signal_choice->callback( handle_stop_signal, NULL );
    ui->label_input->callback( handle_label, NULL );
    ui->config_file_browse_button->callback( handle_config_file_browse, NULL );
}



int
main ( int argc, char **argv )
{
    if ( argc != 3 )
    {
        fprintf( stderr, "Usage: %s --connect-to url\n", argv[0] );
        return 1;
    }

    init_osc( NULL );

    nsmp_addr = lo_address_new_from_url( argv[2] );

    printf( "Connecting to nsm-proxy at: %s\n", argv[2] );

    ui = new NSM_Proxy_UI;

    Fl_Double_Window *w = ui->make_window();

    connect_ui();

    lo_send_from( nsmp_addr, losrv,  LO_TT_IMMEDIATE, "/nsm/proxy/update", "" );
    
    w->show();

    Fl::lock();

    Fl::run();

    return 0;
}
