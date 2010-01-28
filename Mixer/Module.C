
/*******************************************************************************/
/* Copyright (C) 2009 Jonathan Moore Liles                                     */
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

#include "Module.H"
#include <FL/fl_draw.H>
#include <FL/fl_ask.H>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Module_Parameter_Editor.H"
#include "Chain.H"

#include "JACK_Module.H"
#include "Gain_Module.H"
#include "Mono_Pan_Module.H"
#include "Meter_Module.H"
#include "Plugin_Module.H"

#include <FL/Fl_Menu_Button.H>
#include "FL/test_press.H"
#include "FL/menu_popup.H"



Module::Module ( int W, int H, const char *L ) : Fl_Group( 0, 0, W, H, L )
{
    init();

    log_create();
}

Module::Module ( bool is_default, int W, int H, const char *L ) : Fl_Group( 0, 0, W, H, L ), Loggable( !is_default )
{
    this->is_default( is_default );

    init();

    log_create();
}

Module::Module ( ) : Fl_Group( 0, 0, 0, 50, "Unnamed" )
{
    init();

    log_create();
}

Module::~Module ( )
{
    for ( unsigned int i = 0; i < audio_input.size(); ++i )
        audio_input[i].disconnect();
    for ( unsigned int i = 0; i < audio_output.size(); ++i )
        audio_output[i].disconnect();
    for ( unsigned int i = 0; i < control_input.size(); ++i )
        control_input[i].disconnect();
    for ( unsigned int i = 0; i < control_output.size(); ++i )
        control_output[i].disconnect();

    audio_input.clear();
    audio_output.clear();
    control_input.clear();
    control_output.clear();
}



void
Module::init ( void )
{
    _is_default = false;
    _editor = 0;
    _chain = 0;
    _instances = 1;
    _bypass = 0;
    box( FL_UP_BOX );
    labeltype( FL_NO_LABEL );
    clip_children( 1 );
}


void
Module::get ( Log_Entry &e ) const
{
//    e.add( ":name",            label()           );
//    e.add( ":color",           (unsigned long)color());
    {
        char *s = get_parameters();
        if ( strlen( s ) )
            e.add( ":parameter_values", s );
        delete[] s;
    }
    e.add( ":is_default", is_default() );
    e.add( ":chain", chain() );
    e.add( ":active", bypass() );
}

void
Module::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":chain" ) )
        {
            /* This trickiness is because we may need to know the name of
               our chain before we actually get added to it. */
            int i;
            sscanf( v, "%X", &i );
            Chain *t = (Chain*)Loggable::find( i );

            assert( t );

            chain( t );
        }
    }

    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

/*         if ( ! strcmp( s, ":name" ) ) */
/*             label( v ); */
        if ( ! strcmp( s, ":parameter_values" ) )
        {
            set_parameters( v );
        }
        else if ( ! ( strcmp( s, ":is_default" ) ) )
        {
            is_default( atoi( v ) );
        }
        else if ( ! ( strcmp( s, ":active" ) ) )
        {
            bypass( atoi( v ) );
        }
        else if ( ! strcmp( s, ":chain" ) )
        {
            int i;
            sscanf( v, "%X", &i );
            Chain *t = (Chain*)Loggable::find( i );

            assert( t );

            t->add( this );
        }
    }
}




/* return a string serializing this module's parameter settings.  The
   format is 1.0:2.0:... Where 1.0 is the value of the first control
   input, 2.0 is the value of the second control input etc.
*/
char *
Module::get_parameters ( void ) const
{
    char *s = new char[1024];
    s[0] = 0;
    char *sp = s;

    if ( control_input.size() )
    {
        for ( unsigned int i = 0; i < control_input.size(); ++i )
            sp += snprintf( sp, 1024 - (sp - s),"%f:", control_input[i].control_value() );

        *(sp - 1) = '\0';
    }

    return s;
}

void
Module::set_parameters ( const char *parameters )
{
    char *s = strdup( parameters );
    char *sp = s;

    char *start = s;
    int i = 0;
    for ( char *sp = s; ; ++sp )
    {
        if ( ':' == *sp || '\0' == *sp )
        {
            char was = *sp;

            *sp = '\0';

            DMESSAGE( start );

            if ( i < control_input.size() )
                control_input[i].control_value( atof( start ) );
            else
            {
                WARNING( "Module has no parameter at index %i", i );
                break;
            }

            i++;

            if ( '\0' == was  )
                break;

            start = sp + 1;
        }
    }

    free( s );
}



void
Module::draw_box ( void )
{
    fl_color( FL_WHITE );

    int tw, th, tx, ty;

    tw = w();
    th = h();
    ty = y();
    tx = x();

//    bbox( tx, ty, tw, th );

    fl_push_clip( tx, ty, tw, th );


    Fl_Color c = is_default() ? FL_BLACK : color();

    c = active() && ! bypass() ? c : fl_inactive( c );

    int spacing = w() / instances();
    for ( int i = instances(); i--; )
    {
        fl_draw_box( box(), tx + (spacing * i), ty, tw / instances(), th, Fl::belowmouse() == this ? fl_lighter( c ) : c );
    }

    if ( audio_input.size() && audio_output.size() )
    {
        /* maybe draw control indicators */
        if ( control_input.size() )
            fl_draw_box( FL_ROUNDED_BOX, tx + 4, ty + 4, 5, 5, is_being_controlled() ? FL_YELLOW : fl_inactive( FL_YELLOW ) );
        if ( control_output.size() )
            fl_draw_box( FL_ROUNDED_BOX, tx + tw - 8, ty + 4, 5, 5, is_controlling() ? FL_YELLOW : fl_inactive( FL_YELLOW ) );
    }

    fl_pop_clip();
//    box( FL_NO_BOX );

    Fl_Group::draw_children();
}

void
Module::draw_label ( void )
{
    int tw, th, tx, ty;

    bbox( tx, ty, tw, th );

    const char *lp = label();

    int l = strlen( label() );

    Fl_Color c = FL_FOREGROUND_COLOR;

    if ( bypass() || ! active() )
        c = FL_BLACK;

    fl_color( c );

    char *s = NULL;

    if ( l > 10 )
    {
        s = new char[l];
        char *sp = s;

        for ( ; *lp; ++lp )
            switch ( *lp )
            {
                case 'i': case 'e': case 'o': case 'u': case 'a':
                    break;
                default:
                    *(sp++) = *lp;
            }
        *sp = '\0';

    }

    if ( l > 20 )
        fl_font( FL_HELVETICA, 10 );
    else
        fl_font( FL_HELVETICA, 14 );

    fl_draw( s ? s : lp, tx, ty, tw, th, (Fl_Align)(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP ) );

    if ( s )
        delete[] s;
}

void
Module::insert_menu_cb ( const Fl_Menu_ *m )
{
    void * v = m->menu()[ m->value() ].user_data();

    if ( v )
    {
        unsigned long id = *((unsigned long *)v);

        Module *mod = NULL;

        switch ( id )
        {
            case -1:
                mod = new JACK_Module();
                break;
            case -2:
                mod = new Gain_Module();
                break;
            case -3:
                mod = new Meter_Module();
                break;
            case -4:
                mod = new Mono_Pan_Module();
                break;
            default:
            {
                Plugin_Module *m = new Plugin_Module();

                m->load( id );

                mod = m;
            }
        }

        if ( mod )
        {
            if ( !strcmp( mod->name(), "JACK" ) )
            {
                DMESSAGE( "Special casing JACK module" );
                JACK_Module *jm = (JACK_Module*)mod;
                jm->chain( chain() );
                jm->configure_inputs( ninputs() );
                jm->configure_outputs( ninputs() );
            }

            if ( ! chain()->insert( this, mod ) )
            {
                fl_alert( "Cannot insert this module at this point in the chain" );
                delete mod;
                return;
            }

            redraw();
        }
    }
}

void
Module::insert_menu_cb ( Fl_Widget *w, void *v )
{
    ((Module*)v)->insert_menu_cb( (Fl_Menu_*) w );
}

void
Module::menu_cb ( const Fl_Menu_ *m )
{
    char picked[256];

    strncpy( picked, m->mvalue()->label(), sizeof( picked ) );

//    m->item_pathname( picked, sizeof( picked ) );

    DMESSAGE( "%s", picked );

    Logger log( this );

    if ( ! strcmp( picked, "Edit Parameters" ) )
        command_open_parameter_editor();
    else if ( ! strcmp( picked, "Activate" ) )
        command_activate();
    else if ( ! strcmp( picked, "Deactivate" ) )
        command_deactivate();
    else if ( ! strcmp( picked, "Remove" ) )
        command_remove();
}

void
Module::menu_cb ( Fl_Widget *w, void *v )
{
    ((Module*)v)->menu_cb( (Fl_Menu_*) w );
}

/** build the context menu */
Fl_Menu_Button &
Module::menu ( void ) const
{
    static Fl_Menu_Button m( 0, 0, 0, 0, "Module" );
    static Fl_Menu_Button *insert_menu = NULL;

    if ( ! insert_menu )
    {
        insert_menu = new Fl_Menu_Button( 0, 0, 0, 0 );

        insert_menu->add( "Gain", 0, 0, new unsigned long(-2) );
        insert_menu->add( "Meter", 0, 0, new unsigned long(-3) );
        insert_menu->add( "Mono Pan", 0, 0, new unsigned long(-4) );

        Plugin_Module::add_plugins_to_menu( insert_menu );

//        menu_set_callback( insert_menu, &Module::insert_menu_cb, (void*)this );
        insert_menu->callback( &Module::insert_menu_cb, (void*)this );
    }

    m.clear();

    m.add( "Insert", 0, &Module::menu_cb, (void*)this, 0);
    m.add( "Insert", 0, &Module::menu_cb, const_cast< Fl_Menu_Item *>( insert_menu->menu() ), FL_SUBMENU_POINTER );
    m.add( "Edit Parameters", 0, &Module::menu_cb, (void*)this, 0 );
    m.add( "Activate",   0, &Module::menu_cb, (void*)this, ! bypass() ? FL_MENU_INACTIVE : 0 );
    m.add( "Deactivate", 0, &Module::menu_cb, (void*)this, bypass() ? FL_MENU_INACTIVE : 0 );
    m.add( "Remove",    0, &Module::menu_cb, (void*)this );

//    menu_set_callback( menu, &Module::menu_cb, (void*)this );
    m.callback( &Module::insert_menu_cb, (void*)this );

    return m;
}

int
Module::handle ( int m )
{
    switch ( m )
    {
        case FL_KEYBOARD:
        {
            if ( Fl_Group::handle( m ) )
                return 1;

            if ( test_press( FL_Menu ) )
            {
                menu_popup( &menu(), x(), y() );
                return 1;
            }
            else
                return menu().test_shortcut() != 0;
        }
        case FL_PUSH:
        {
            if ( Fl_Group::handle( m ) )
                return 1;
            else if ( test_press( FL_BUTTON3 ) )
            {
                menu_popup( &menu() );
                return 1;
            }
            else if ( test_press( FL_BUTTON1 ) )
            {
                command_open_parameter_editor();
                return 1;
            }
            else if ( test_press( FL_BUTTON3 | FL_CTRL ) )
            {
                command_remove();
                return 1;
            }

            return 0;
        }
    }

    return Fl_Group::handle( m );
}


/************/
/* Commands */
/************/

void
Module::command_open_parameter_editor ( void )
{
    if ( _editor )
    {
        _editor->show();
    }
    else if ( ncontrol_inputs() )
    {
        DMESSAGE( "Opening module parameters for \"%s\"", label() );
        _editor = new Module_Parameter_Editor( this );

        _editor->show();

        do { Fl::wait(); }
        while ( _editor->shown() );

        DMESSAGE( "Module parameters for \"%s\" closed",label() );

        delete _editor;

        _editor = NULL;
    }
}

void
Module::command_activate ( void )
{
    bypass( false );
}

void
Module::command_deactivate ( void )
{
    bypass( true );
}

void
Module::command_remove ( void )
{
    if ( is_default() )
        fl_alert( "Default modules may not be deleted." );
    else
    {
        chain()->remove( this );
        Fl::delete_widget( this );
    }
}
