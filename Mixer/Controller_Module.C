
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

#include "Controller_Module.H"

#include <FL/Fl.H>
#include "FL/Fl_Value_SliderX.H"
#include <FL/Fl_Box.H>
#include <FL/Fl_Counter.H>
#include "FL/Fl_Arc_Dial.H"
#include "FL/Fl_Light_Button.H"
#include "FL/Boxtypes.H"
#include <FL/fl_draw.H>
#include "FL/Fl_Labelpad_Group.H"
#include <stdio.h>
#include "Engine/Engine.H"
#include "Chain.H"



const float CONTROL_UPDATE_FREQ = 0.1f;



void
Controller_Module::get ( Log_Entry &e ) const
{

    Port *p = control_output[0].connected_port();
    Module *m = p->module();

    e.add( ":module", m );
    e.add( ":port", m->control_input_port_index( p ) );

    Module::get( e );
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
        control_output[0].connect_to( &module->control_input[port] );
        module->chain()->add_control( this );
        label( module->control_input[port].name() );
    }
}



Controller_Module::Controller_Module ( bool is_default ) : Module( is_default, 50, 100, name() )
{
//    label( "" );
    box( FL_NO_BOX );

    _pad = true;
    control = 0;
    control_value =0.0f;

    add_port( Port( this, Port::OUTPUT, Port::CONTROL ) );

    mode( GUI );
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
Controller_Module::connect_to ( Port *p )
{
    control_output[0].connect_to( p );

    if( mode() == CV )
    {
        chain()->engine()->lock();

//        char name[256];
//        snprintf( name, sizeof( name ), "%s-CV", p->name() );

        JACK::Port po( chain()->engine(), JACK::Port::Input, p->name(), 0, "CV" );

        if ( po.valid() )
        {
                jack_input.push_back( po );
        }

        chain()->engine()->unlock();
    }

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

//        o->type( FL_VERTICAL );
//        o->type(1);

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
//            o->box( FL_OVAL_BOX );
//            o->type( FL_FILL_DIAL );
            o->color( fl_darker( fl_darker( FL_GRAY ) ) );
            o->selection_color( FL_WHITE );
            o->value( p->control_value() );
        }

    }

    control_value = p->control_value();

    w->align(FL_ALIGN_TOP);
    w->labelsize( 10 );
    w->callback( cb_handle, this );

    if ( _pad )
    {
        Fl_Labelpad_Group *flg = new Fl_Labelpad_Group( w );
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

int
Controller_Module::handle ( int m )
{
    return Fl_Group::handle( m );
}



void
Controller_Module::process ( void )
{
    THREAD_ASSERT( RT );

    if ( control_output[0].connected() )
    {
        float f = control_value;

        if ( mode() == CV )
        {
            f = *((float*)jack_input[0].buffer( chain()->engine()->nframes() ));

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
