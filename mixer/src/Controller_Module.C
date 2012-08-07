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
#include <FL/fl_draw.H>
#include "FL/Fl_Dial.H"
#include "FL/Fl_Labelpad_Group.H"
#include "FL/Fl_Value_SliderX.H"
#include "Panner.H"
#include "FL/test_press.H"
#include "FL/menu_popup.H"

#include "Engine/Engine.H"
#include "Chain.H"
#include "OSC/Endpoint.H"

// needed for mixer->endpoint
#include "Mixer.H"



const float CONTROL_UPDATE_FREQ = 0.2f;



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

    /* shutdown JACK port, if we have one */
    mode( GUI );
}

void
Controller_Module::handle_chain_name_changed()
{
//    change_osc_path( generate_osc_path() );
}

void
Controller_Module::disconnect ( void )
{
    for ( std::vector<Module::Port>::iterator i = control_output.begin();
          i != control_output.end();
          ++i )
    {
        (*i).disconnect();
    }
}



void
Controller_Module::get ( Log_Entry &e ) const
{
    Module::get( e );

    Port *p = control_output[0].connected_port();

    if ( !p )
    {
        e.add( ":module", "" );
        e.add( ":port", "" );
        e.add( ":mode", "" );
    }
    else
    {
        Module *m = p->module();
        
        e.add( ":module", m );
        e.add( ":port", m->control_input_port_index( p ) );
        e.add( ":mode", mode() );
    }
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
                chain()->engine()->unlock();
                return;
            }

            if ( po.valid() )
            {
                jack_input.push_back( po );
            }

            chain()->engine()->unlock();
        }
    }
    else if ( mode() == CV && m != CV )
    {
        chain()->engine()->lock();

        jack_input.back().shutdown();
        jack_input.pop_back();

        chain()->engine()->unlock();
    }

    _mode = m ;
}

/** attempt to transform this controller into a spatialization
    controller and connect to the given module's spatialization
    control inputs. Returns true on success, false if given module
    does not accept spatialization inputs. */
bool
Controller_Module::connect_spatializer_to ( Module *m )
{
    /* these are for detecting related parameter groups which can be
       better represented by a single control */
    Port *azimuth_port = NULL;
    float azimuth_value = 0.0f;
    Port *elevation_port = NULL;
    float elevation_value = 0.0f;

    for ( unsigned int i = 0; i < m->control_input.size(); ++i )
    {
        Port *p = &m->control_input[i];

        if ( !strcasecmp( "Azimuth", p->name() ) &&
             180.0f == p->hints.maximum &&
             -180.0f == p->hints.minimum )
        {
            azimuth_port = p;
            azimuth_value = p->control_value();
            continue;
        }
        else if ( !strcasecmp( "Elevation", p->name() ) &&
                  90.0f == p->hints.maximum &&
                  -90.0f == p->hints.minimum )
        {
            elevation_port = p;
            elevation_value = p->control_value();
            continue;
        }
    }

    if ( ! ( azimuth_port && elevation_port ) )
        return false;

    control_output.clear();
    add_port( Port( this, Port::OUTPUT, Port::CONTROL ) );
    add_port( Port( this, Port::OUTPUT, Port::CONTROL ) );

    control_output[0].connect_to( azimuth_port );
    control_output[1].connect_to( elevation_port );

    {
        clear();

        Panner *o = new Panner( 0,0, 100, 100 );

        o->box(FL_THIN_UP_BOX);
        o->color(FL_GRAY0);
        o->selection_color(FL_BACKGROUND_COLOR);
        o->labeltype(FL_NORMAL_LABEL);
        o->labelfont(0);
        o->labelcolor(FL_FOREGROUND_COLOR);
        o->align(FL_ALIGN_TOP);
        o->when(FL_WHEN_CHANGED);
        o->label( "Spatialization" );

        o->align(FL_ALIGN_TOP);
        o->labelsize( 10 );
//        o->callback( cb_panner_value_handle, new callback_data( this, azimuth_port_number, elevation_port_number ) );

        o->point( 0 )->azimuth( azimuth_value );
        o->point( 0 )->elevation( elevation_value );

        o->callback( cb_spatializer_handle, this );

        control = (Fl_Valuator*)o;

        if ( _pad )
        {
            Fl_Labelpad_Group *flg = new Fl_Labelpad_Group( o );
            flg->position( x(), y() );
            flg->set_visible_focus();
            size( flg->w(), flg->h() );
            add( flg );
        }
        else
        {
            o->resize( x(), y(), w(), h() );
            add( o );
            resizable( o );
            init_sizes();
        }

        _type = SPATIALIZATION;
        return true;
    }
}

void
Controller_Module::connect_to ( Port *p )
{
    control_output[0].connect_to( p );

    clear();

    Fl_Widget *w;

    if ( p->hints.type == Module::Port::Hints::BOOLEAN )
    {
        Fl_Light_Button *o = new Fl_Light_Button( 0, 0, 40, 40, p->name() );
        w = o;
        o->value( p->control_value() );

        _type = TOGGLE;

        /* FIXME: hack */
        control = (Fl_Valuator*)o;
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

        _type = SPINNER;

        o->value( p->control_value() );
    }
    else if ( p->hints.type == Module::Port::Hints::LOGARITHMIC )
    {
        Fl_Value_SliderX *o = new Fl_Value_SliderX(0, 0, 30, 250, p->name() );
        control = o;
        w = o;

        o->type(4);
        o->color(FL_BACKGROUND_COLOR);
        o->selection_color((Fl_Color)1);
        o->minimum(1.5);
        o->maximum(0);
        o->value(1);
        o->textsize(6);

        if ( p->hints.ranged )
        {
            o->minimum( p->hints.maximum );
            o->maximum( p->hints.minimum );
        }

        o->value( p->control_value() );

        _type = SLIDER;
    }
    else
    {
        { Fl_Dial *o = new Fl_Dial( 0, 0, 50, 50, p->name() );
            w = o;
            control = o;

            if ( p->hints.ranged )
            {
                DMESSAGE( "Min: %f, max: %f", p->hints.minimum, p->hints.maximum );
                o->minimum( p->hints.minimum );
                o->maximum( p->hints.maximum );
            }
            
            o->color( fl_darker( FL_GRAY ) );
            o->selection_color( FL_WHITE );
            o->value( p->control_value() );
        }

        _type = KNOB;
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
        flg->position( x(), y() );
        add( flg );
    }
    else
    {
        /* HACK: hide label */
        w->labeltype( FL_NO_LABEL );
        w->resize( x(), y(), this->w(), h() );
        add( w );
        resizable( w );
//       init_sizes();
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

    /* we only need this in CV (JACK) mode, because with other forms
     * of control the change happens in the GUI thread and we know it */
    if ( mode() != CV )
        return;

    /* ensures that port value change callbacks are run */
    if ( control && control_output.size() > 0 && control_output[0].connected() )
        control_output[0].connected_port()->control_value( control_value );
}

void
Controller_Module::cb_handle ( Fl_Widget *w, void *v )
{
    ((Controller_Module*)v)->cb_handle( w );
}

void
Controller_Module::cb_handle ( Fl_Widget *w )
{
    if ( type() == TOGGLE )
    {
        control_value = ((Fl_Button*)w)->value();
    }
    else
        control_value = ((Fl_Valuator*)w)->value();

    if ( control_output[0].connected() )
        control_output[0].connected_port()->control_value( control_value );
}


void
Controller_Module::cb_spatializer_handle ( Fl_Widget *w, void *v )
{
    ((Controller_Module*)v)->cb_spatializer_handle( w );
}

void
Controller_Module::cb_spatializer_handle ( Fl_Widget *w )
{
    Panner *pan = (Panner*)w;

    if ( control_output[0].connected() &&
         control_output[1].connected() )
    {
        control_output[0].connected_port()->control_value( pan->point( 0 )->azimuth() );
        control_output[1].connected_port()->control_value( pan->point( 0 )->elevation() );
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

    if ( ! strcmp( picked, "Mode/GUI + OSC" ) )
        mode( GUI );
    else if ( ! strcmp( picked, "Mode/Control Voltage (JACK)" ) )
        mode( CV );
    else if ( ! strcmp( picked, "/Remove" ) )
        command_remove();
}

/** build the context menu for this control */
Fl_Menu_Button &
Controller_Module::menu ( void )
{
    static Fl_Menu_Button m( 0, 0, 0, 0, "Controller" );

    Fl_Menu_Item items[] =
        {
            { "Mode",             0, 0, 0,  FL_SUBMENU    },
            { "GUI + OSC",       0, 0, 0,  FL_MENU_RADIO | ( mode() == GUI ? FL_MENU_VALUE : 0 ) },
            { "Control Voltage (JACK)",           0, 0, 0,  FL_MENU_RADIO | ( mode() == CV ? FL_MENU_VALUE : 0 ) },
            { 0 },
            { "Remove", 0, 0, 0, 0 },
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
                if ( type() != SPATIALIZATION )
                    menu_popup( &menu() );

                return 1;
            }
            else
                return Fl_Group::handle( m );
        }
    }

    return Fl_Group::handle( m );
}

void
Controller_Module::handle_control_changed ( Port *p )
{
    /* ignore changes initiated while mouse is over widget */
    if ( contains( Fl::pushed() ) )
        return;

    if ( p )
        control_value = p->control_value();

    if ( control->value() == control_value )
        return;

    /* if ( control->value() != control_value ) */
    /* { */
    /*     redraw(); */
    /* } */

    if ( type() == SPATIALIZATION )
    {
        Panner *pan = (Panner*)control;

        pan->point( 0 )->azimuth( control_output[0].control_value() );
        pan->point( 0 )->elevation( control_output[1].control_value() );

        pan->redraw();
    }
    else
    {
        if ( type() == TOGGLE )
            ((Fl_Button*)control)->value(control_value);
        else
            control->value(control_value);
    }
}

void
Controller_Module::command_remove ( void )
{
    if ( is_default() )
        fl_alert( "Default modules may not be deleted." );
    else
    {
        chain()->remove( this );
        Fl::delete_widget( this );
    }
}

/**********/
/* Engine */
/**********/

void
Controller_Module::process ( nframes_t nframes )
{
    THREAD_ASSERT( RT );

    if ( type() == SPATIALIZATION )
    {
        return;
    }

    if (  control_output[0].connected() )
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
