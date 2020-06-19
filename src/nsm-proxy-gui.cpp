
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

#pragma GCC diagnostic ignored "-Wunused-parameter"


#define _MODULE_ "nsm-proxy-gui"

#define APP_NAME "NSM Proxy"
#define APP_TITLE "NSM Proxy"

#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Text_Display.H>
#include "NSM_Proxy_UI.H"
#include <lo/lo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

lo_server losrv;
lo_address nsmp_addr;

static NSM_Proxy_UI *ui;

static char *client_error;

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
    if (!strcmp( path, "/nsm/proxy/client_error" ))
    {
        if ( client_error != NULL )
            free(client_error);

        client_error = NULL;

        if ( strlen(&argv[0]->s) > 0 )
            client_error = strdup(&argv[0]->s);
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
    lo_server_thread_add_method( loth, "/nsm/proxy/client_error", "s", osc_update, NULL );

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



void cb_dismiss_button ( Fl_Widget *w, void *v )
{
    w->window()->hide();
}

void
check_error ( void *v )
{
    if ( client_error )
    {
        {
            Fl_Double_Window *o = new Fl_Double_Window(600,300+15,"Abnormal Termination");
            {
                Fl_Box *o = new Fl_Box(0+15,0+15,600-30,50);
                o->box(FL_BORDER_BOX);
                o->color(FL_RED);
                o->labelcolor(FL_WHITE);
                o->align(FL_ALIGN_CENTER|FL_ALIGN_WRAP);
                o->copy_label( client_error );
            }
            {
                Fl_Text_Display *o = new Fl_Text_Display(0+15,50+15,600-30,300-75-30);
                o->buffer(new Fl_Text_Buffer());
                o->buffer()->loadfile( "error.log" );
            }
            {
                Fl_Button *o = new Fl_Button(600-75-15,300-25,75,25,"Dismiss");
                o->callback(cb_dismiss_button,0);
            }

            o->show();
        }
        
        free(client_error);
        client_error = NULL;
    }
    
    Fl::repeat_timeout( 0.5f, check_error, v );
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

    Fl::add_timeout( 0.5f, check_error, NULL );

    Fl::run();

    return 0;
}
