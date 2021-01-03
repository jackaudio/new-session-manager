
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
#include <getopt.h>


lo_server losrv;
lo_address nsmp_addr;

static NSM_Proxy_UI *ui;

static char *client_error;

int
osc_update ( const char *path, const char *types, lo_arg **argv, int argc, lo_message msg, void *user_data )
{
    //Updates are arriving one OSC message at a time.

    printf( "Got update for %s\n", path );

    Fl::lock();

    if (!strcmp( path, "/nsm/proxy/label" ))
        ui->label_input->value( &argv[0]->s );
    else if (!strcmp( path, "/nsm/proxy/arguments" ))
        ui->arguments_input->value( &argv[0]->s );
    else if (!strcmp( path, "/nsm/proxy/executable" )) {
        ui->executable_input->value( &argv[0]->s );
        if ( strcmp( &argv[0]->s , "") ) {
            //We want to avoid that the button is always labeled 'Start', creating the
            //false impression that a new sub-client instance is started each time you press it.
            //If the string is empty there is no program running at the moment.
            //This does not cover all cases but gets us there 90%, which is good enough for something cosmetic.
            ui->start_button->label("Ok");
        }
    }
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
    //Executed when Start Button is clicked.
    //"/nsm/proxy/start" for the sub-client is always sent, no matter if the software already runs.
    //nsm-proxy.cpp handles the redundancy.
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
            //Create a new floating Window that shows an error message.
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
    //The NSM-Proxy GUI is a client that communicates with another binary, nsm-proxy via OSC.
    //This GUI executable is actually closed and restarted each time you show/hide the GUI.
    //In other words: it is not a persistent GUI state.


    //Command line parameters
    const char * gui_url = NULL;
    static struct option long_options[] =
    {
        { "connect-to", required_argument, 0, 'u' },
        { "help", no_argument, 0, 'h' },
        { 0, 0, 0, 0 }
    };
    int option_index = 0;
    int c = 0;
    while ( ( c = getopt_long_only( argc, argv, "", long_options, &option_index  ) ) != -1 )
    {
        switch ( c )
        {
            case 'u':
            {
                gui_url = optarg;
                break;
            }

            case 'h':
            {
                const char *usage =
                "nsm-proxy-gui - GUI for nsm-proxy, a wrapper for executables without direct NSM-Support.\n\n"
                "Usage:\n"
                "  nsm-proxy-gui --help\n"
                "  nsm-proxy-gui --connect-to\n"
                "\n"
                "Options:\n"
                "  --help                Show this screen\n"
                "  --connect-to          Connect to running nsm-proxy\n"
                "\n\n"
                "nsmd-proxy-gui is usually not called by the user directly,\n"
                "but autostarted when nsm-proxy is added to a session (through a GUI).\n"
                "";
                puts ( usage );
                exit(0);
                break;
            }
        }
    }

    if ( gui_url == NULL )
        exit(1);

    init_osc( NULL );

    nsmp_addr = lo_address_new_from_url( gui_url );

    if  ( ! nsmp_addr )
        exit(1);

    printf( "Connecting to nsm-proxy at: %s\n", gui_url );

    ui = new NSM_Proxy_UI;

    Fl::scheme( "gtk+" );
    Fl::visual(FL_DOUBLE|FL_INDEX); // FLKT Double_Window: "higly recommended […] put before the first show() of any window in your program"

    Fl_Double_Window *w = ui->make_window();

    connect_ui();

    lo_send_from( nsmp_addr, losrv,  LO_TT_IMMEDIATE, "/nsm/proxy/update", "" );



    w->show();

    Fl::lock();

    //Setting colors only after main window creation.
    //We keep them all in once place instead of setting them in the widgets
    //Colors are the same as nsm-legacy-gui.cpp . If one changes you need to change the other by hand.


    Fl::set_color( 55, 223, 237, 255 ); //Override FLUID palette with RGB Value. 55 is label text. Same as FL_FOREGROUND_COLOR
    Fl::set_color( 41, 55, 61, 69 ); //Override FLUID palette with RGB Value. 41 is label background

    Fl::set_color( FL_DARK1, 37, 40, 45 ); //Main window background
    Fl::set_color( FL_BACKGROUND_COLOR, 37, 40, 45 ); //These are the colors used as backgrounds by almost all widgets and used to draw the edges of all the boxtypes.
    Fl::set_color( FL_BACKGROUND2_COLOR, 55, 61, 69 ); //This color is used as a background by Fl_Input and other text widgets.
    Fl::set_color( FL_FOREGROUND_COLOR, 223, 237, 255 );
    Fl::set_color( FL_INACTIVE_COLOR, 255, 0, 0 ); // Not used
    Fl::set_color( FL_SELECTION_COLOR, 80, 84, 92 ); // e.g. the currently selected session
    Fl::reload_scheme();


    Fl::add_timeout( 0.5f, check_error, NULL );

    Fl::run();

    return 0;
}
