
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

#include "const.h"

#include "Controller_Module.H"

#include <stdio.h>

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Counter.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Menu_.H>
#include <FL/Fl_Light_Button.H>
#include "FL/Boxtypes.H"
#include <FL/fl_draw.H>
#include "FL/Fl_Arc_Dial.H"
#include "FL/Fl_Labelpad_Group.H"
#include "FL/Fl_Value_SliderX.H"
#include "FL/test_press.H"
#include "FL/menu_popup.H"

#include "Engine/Engine.H"
#include "Chain.H"



const float CONTROL_UPDATE_FREQ = 0.1f;



Controller_Module::Controller_Module ( bool is_default ) : Module( is_default, 50, 100, name() )
{
//    label( "" );
    box( FL_NO_BOX );

    _pad = true;
    control = 0;
    control_value =0.0f;

    add_port( Port( this, Port::OUTPUT, Port::CONTROL ) );

    _mode = GUI;

//    mode( GUI );
//    mode( CV );
//    configure_inputs( 1 );

    end();

    Fl::add_timeout( CONTROL_UPDATE_FREQ, update_cb, this );

    log_create();

}

Controller_Module::~Controller_Module ( )
{
    Fl::remove_timeout( update_cb, this );
    log_destroy();
}


void
Controller_Module::get ( Log_Entry &e ) const
{
    Module::get( e );

    Port *p = control_output[0].connected_port();
    Module *m = p->module();

    e.add( ":module", m );
    e.add( ":port", m->control_input_port_index( p ) );
    e.add( ":mode", mode() );
}

void
Controller_Module::set ( Log_Entry &e )
{
    Module::set( e );

    int port = -1;
    Module *module = NULL;

    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":port" ) )
        {
            port = atoi( v );
        }
        else if ( ! strcmp( s, ":module" ) )
        {
            int i;
            sscanf( v, "%X", &i );
            Module *t = (Module*)Loggable::find( i );

            assert( t );

            module = t;
        }
    }

    if ( port >= 0 && module )
    {
        connect_to( &module->control_input[port] );
        module->chain()->add_control( this );
        label( module->control_input[port].name() );
    }

    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":mode" ) )
        {
            mode( (Mode)atoi( v ) );
        }
    }

}



void
Controller_Module::mode ( Mode m )
{

    if( mode() != CV && m == CV )
    {
        if ( control_output[0].connected() )
        {
            chain()->engine()->lock();

            Port *p = control_output[0].connected_port();

            JACK::Port po( chain()->engine(), JACK::Port::Input, p->name(), 0, "CV" );

            if ( ! po.activate() )
            {
                fl_alert( "Could not activate JACK port \"%s\"", po.name() );
                return;
            }

            if ( po.valid() )
            {
                jack_input.push_back( po );
            }

            chain()->engine()->unlock();
        }
    }
    else if ( mode() == CV && m == GUI )
    {
        chain()->engine()->lock();

        jack_input.back().shutdown();
        jack_input.pop_back();

        chain()->engine()->unlock();
    }

    _mode = m ;
}

void
Controller_Module::connect_to ( Port *p )
{
    control_output[0].connect_to( p );

    Fl_Widget *w;

    if ( p->hints.type == Module::Port::Hints::BOOLEAN )
    {
        Fl_Light_Button *o = new Fl_Light_Button( 0, 0, 40, 40, p->name() );
        w = o;
        o->value( p->control_value() );

    }
    else if ( p->hints.type == Module::Port::Hints::INTEGER )
    {

        Fl_Counter *o = new Fl_Counter(0, 0, 58, 24, p->name() );
        control = o;
        w = o;

        o->type(1);
        o->step(1);

        if ( p->hints.ranged )
        {
            o->minimum( p->hints.minimum );
            o->maximum( p->hints.maximum );
        }

        o->value( p->control_value() );
    }
    else if ( p->hints.type == Module::Port::Hints::LOGARITHMIC )
    {
        Fl_Value_SliderX *o = new Fl_Value_SliderX(0, 0, 30, 250, p->name() );
        control = o;
        w = o;

        o->type(4);
        o->color(FL_GRAY0);
        o->selection_color((Fl_Color)1);
        o->minimum(1.5);
        o->maximum(0);
        o->value(1);
        o->textsize(14);

        if ( p->hints.ranged )
        {
            o->minimum( p->hints.maximum );
            o->maximum( p->hints.minimum );
        }

        o->value( p->control_value() );
    }
    else
    {
        { Fl_Arc_Dial *o = new Fl_Arc_Dial( 0, 0, 40, 40, p->name() );
            w = o;
            control = o;
            if ( p->hints.ranged )
            {
                o->minimum( p->hints.minimum );
                o->maximum( p->hints.maximum );
            }

            o->box( FL_BURNISHED_OVAL_BOX );
            o->color( fl_darker( fl_darker( FL_GRAY ) ) );
            o->selection_color( FL_WHITE );
            o->value( p->control_value() );
        }

    }

    control_value = p->control_value();

    w->set_visible_focus();
    w->align(FL_ALIGN_TOP);
    w->labelsize( 10 );
    w->callback( cb_handle, this );

    if ( _pad )
    {
        Fl_Labelpad_Group *flg = new Fl_Labelpad_Group( w );
        flg->set_visible_focus();
        size( flg->w(), flg->h() );
        add( flg );
    }
    else
    {
        w->resize( x(), y(), this->w(), h() );
        add( w );
        resizable( w );
    }
}



void
Controller_Module::resize ( int X, int Y, int W, int H )
{
    Module::resize( X, Y, W, H );

    if ( ! _pad && children() )
    {
        child( 0 )->resize( X, Y, W, H );
    }
}

void
Controller_Module::update_cb ( void *v )
{
    ((Controller_Module*)v)->update_cb();
}

void
Controller_Module::update_cb ( void )
{
    Fl::repeat_timeout( CONTROL_UPDATE_FREQ, update_cb, this );

    if ( control && control_output[0].connected() )
        control->value(control_value);
}

void
Controller_Module::cb_handle ( Fl_Widget *w, void *v )
{
    ((Controller_Module*)v)->cb_handle( w );
}

void
Controller_Module::cb_handle ( Fl_Widget *w )
{
    control_value = ((Fl_Valuator*)w)->value();
    if ( control_output[0].connected() )
    {
        control_output[0].control_value( control_value );
        Port *p = control_output[0].connected_port();
        Module *m = p->module();

        m->handle_control_changed( p );
    }
}

void
Controller_Module::menu_cb ( Fl_Widget *w, void *v )
{
    ((Controller_Module*)v)->menu_cb( (Fl_Menu_*) w );
}

void
Controller_Module::menu_cb ( const Fl_Menu_ *m )
{
    char picked[256];

    m->item_pathname( picked, sizeof( picked ) );

    Logger log( this );

    if ( ! strcmp( picked, "Mode/Manual" ) )
        mode( GUI );
    else if ( ! strcmp( picked, "Mode/Control Voltage" ) )
        mode( CV );
}

/** build the context menu for this control */
Fl_Menu_Button &
Controller_Module::menu ( void )
{
    static Fl_Menu_Button m( 0, 0, 0, 0, "Controller" );

    Fl_Menu_Item items[] =
        {
            { "Mode",             0, 0, 0,  FL_SUBMENU    },
            { "Manual",       0, 0, 0,  FL_MENU_RADIO | ( mode() == GUI ? FL_MENU_VALUE : 0 ) },
            { "Control Voltage",           0, 0, 0,  FL_MENU_RADIO | ( mode() == CV ? FL_MENU_VALUE : 0 ) },
//            { "Open Sound Control (OSC)",          0, 0, 0,  FL_MENU_RADIO | ( mode() == OSC  ? FL_MENU_VALUE : 0 ) },
            { 0                   },
            { 0 },
        };

    menu_set_callback( items, &Controller_Module::menu_cb, (void*)this );

    m.copy( items, (void*)this );

    return m;
}

int
Controller_Module::handle ( int m )
{

    switch ( m )
    {
        case FL_PUSH:
        {
                if ( test_press( FL_BUTTON3 ) )
                {
                    /* context menu */
                    menu_popup( &menu() );

                    return 1;
                }
                else
                    return Fl_Group::handle( m );
        }
    }

    return Fl_Group::handle( m );
}



/**********/
/* Engine */
/**********/

void
Controller_Module::process ( nframes_t nframes )
{
    THREAD_ASSERT( RT );

    if ( control_output[0].connected() )
    {
        float f = control_value;

        if ( mode() == CV )
        {
            f = *((float*)jack_input[0].buffer( nframes ));

            const Port *p = control_output[0].connected_port();

            if (p->hints.ranged )
            {
                // scale value to range.
                // we assume that CV values are between 0 and 1

                float scale = p->hints.maximum - p->hints.minimum;
                float offset = p->hints.minimum;

                f = ( f * scale ) + offset;
            }
        }
//        else
//            f =  *((float*)control_output[0].buffer());

        *((float*)control_output[0].buffer()) = f;

       control_value = f;
    }
}
