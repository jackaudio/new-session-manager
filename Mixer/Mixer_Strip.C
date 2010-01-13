
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

/* Mixer strip control. Handles GUI and control I/O for this strip. */

/* A mixer strip is home to some (JACK) input ports, a fader, some
 * meters, and a filter chain which can terminate either at the input
 * to the spacializer or some (JACK) output ports. Since mixer strips
 * are not necessarily in a 1:1 association with Non-DAW tracks, there
 * is no need for busses per se. If you want to route the output of
 * several strips into a single fader or filter chain, then you just
 * gives those strips JACK outputs and connect them to the common
 * inputs. This mechanism can also do away with the need for 'sends'
 * and 'inserts'.

 */
/* Each mixer strip comprises a fader and a panner */

#include "Mixer_Strip.H"
#include "Engine/Engine.H"
#include <dsp.h>
#include <string.h>
#include "debug.h"


#include "FL/Fl_Flowpack.H"
#include <FL/Fl_Input.H>
#include <FL/fl_ask.H>
#include "Mixer.H"

#include "Chain.H"
#include "Gain_Module.H"
#include "Meter_Module.H"
#include "Controller_Module.H"
#include "Meter_Indicator_Module.H"
#include "util/debug.h"

extern Mixer *mixer;



void
Mixer_Strip::get ( Log_Entry &e ) const
{
    e.add( ":name",            name()           );
    e.add( ":width",      prepost_button->value() ? "wide" : "narrow" );
    e.add( ":tab",      tab_button->value() ? "signal" : "fader" );
    e.add( ":color",           (unsigned long)color());
}

void
Mixer_Strip::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":name" ) )
            name( v );
        else if ( ! strcmp( s, ":width" ) )
        {
            prepost_button->value( strcmp( v, "wide" ) == 0 );
            prepost_button->do_callback();
        }
        else if ( ! strcmp( s, ":tab" ) )
        {
            tab_button->value( strcmp( v, "signal" ) == 0 );
            tab_button->do_callback();
        }
        else if ( ! strcmp( s, ":color" ) )
        {
            color( (Fl_Color)atoll( v ) );
            redraw();
        }
    }

    if ( ! mixer->contains( this ) )
        mixer->add( this );
}

void
Mixer_Strip::log_children ( void )
{
    log_create();

    _chain->log_children();
}

void
Mixer_Strip::color ( Fl_Color c )
{
    _color = c;
    name_field->color( _color );
    name_field->redraw();
}

Fl_Color
Mixer_Strip::color ( void ) const
{
    return _color;
}

void
Mixer_Strip::chain ( Chain *c )
{
    if ( _chain )
        delete _chain;

    _chain = c;

    c->strip( this );

    Fl_Group *g = signal_tab;

    c->resize( g->x(), g->y(), g->w(), g->h() );
    g->add( c );
    g->resizable( c );

    c->labelsize( 10 );
    c->align( FL_ALIGN_TOP );
    c->color( FL_RED );
    c->configure_outputs_callback( configure_outputs, this );
    c->name( name() );

    gain_controller->chain( c );
    jack_input_controller->chain( c );
    meter_indicator->chain( c );
}

/* add a new mixer strip (with default configuration) */
Mixer_Strip::Mixer_Strip( const char *strip_name, int channels ) : Fl_Group( 0, 0, 120, 600 )
{
    label( strdup( strip_name ) );

    init();

    chain( new Chain() );

    _chain->initialize_with_default();

    _chain->configure_ports();

    color( (Fl_Color)rand() );

//    name( strdup( strip_name ) );

    log_create();
}

/* virgin strip created from journal */
Mixer_Strip::Mixer_Strip() : Fl_Group( 0, 0, 120, 600 )
{
    init();

    log_create();
}

Mixer_Strip::~Mixer_Strip ( )
{
    log_destroy();
}

void Mixer_Strip::cb_handle(Fl_Widget* o) {
    // parent()->parent()->damage( FL_DAMAGE_ALL, x(), y(), w(), h() );
    if ( o == tab_button )
    {
        if ( tab_button->value() == 0 )
        {
            fader_tab->show();
            signal_tab->hide();
        }
        else
        {
            signal_tab->show();
            fader_tab->hide();
        }

    }
    else if ( o == left_button )
    {
        mixer->move_left( this );
    }
    else if ( o == right_button )
    {
        mixer->move_right( this );
    }
    else if ( o == close_button )
    {
        if ( Fl::event_shift() || 1 == fl_choice( "Are you sure you want to remove this strip?\n\n(this action cannot be undone)", "Cancel", "Remove", NULL ) )
        {
            ((Mixer*)parent())->remove( this );
        }
    }
    else if ( o == name_field )
        name( name_field->value() );
    else if ( o == prepost_button )
    {
        if ( ((Fl_Button*)o)->value() )
            size( 300, h() );
        else
            size( 120, h() );

        if ( parent() )
            parent()->parent()->redraw();
    }
}

void Mixer_Strip::cb_handle(Fl_Widget* o, void* v) {
    ((Mixer_Strip*)(v))->cb_handle(o);
}


void
Mixer_Strip::name ( const char *name ) {
    char *s = strdup( name );
    name_field->value( s );
    label( s );
    if ( _chain )
        _chain->name( s );
}

void
Mixer_Strip::configure_outputs ( Fl_Widget *o, void *v )
{
    ((Mixer_Strip*)v)->configure_outputs();
}

void
Mixer_Strip::configure_outputs ( void )
{
    DMESSAGE( "Got signal to configure outputs" );
}

/* called by the chain to let us know that a module has been added */
void
Mixer_Strip::handle_module_added ( Module *m )
{
    if ( m->is_default() )
    {
        DMESSAGE( "Connecting controls to default module \"%s\"", m->name() );

        /* connect default modules to their default controllers/indicators */
        if ( 0 == strcmp( m->name(), "JACK" ) && m->ninputs() == 0 )
        {
            if ( !jack_input_controller->control_output[0].connected() )
                jack_input_controller->connect_to( &m->control_input[1] );
        }
        else if ( 0 == strcmp( m->name(), "Gain" ) )
        {
            gain_controller->connect_to( &m->control_input[0] );
        }
        else if ( 0 == strcmp( m->name(), "Meter" ) )
        {
            meter_indicator->connect_to( &m->control_output[0] );
        }
    }
}


void
Mixer_Strip::process ( nframes_t nframes )
{
    THREAD_ASSERT( RT );

    _chain->process( nframes );
}

/* update GUI with values from RT thread */
void
Mixer_Strip::update ( void )
{
    THREAD_ASSERT( UI );
}

void
Mixer_Strip::init ( )
{
    _chain = 0;

    chain_buffers = 0;
    chain_buffer = NULL;

    box(FL_THIN_UP_BOX);
    clip_children( 1 );

    Fl_Pack *gain_pack;

    { Fl_Pack *o = new Fl_Pack( 2, 2, 114, 100 );
        o->type( Fl_Pack::VERTICAL );
        o->spacing( 2 );
        {
            Fl_Input *o = name_field = new Fl_Sometimes_Input( 2, 2, 144, 24 );
            o->color( color() );
            o->box( FL_FLAT_BOX );
            o->labeltype( FL_NO_LABEL );
            o->labelcolor( FL_GRAY0 );
            o->textcolor( FL_FOREGROUND_COLOR );
            o->value( name() );
            o->callback( cb_handle, (void*)this );

        }
        { Fl_Scalepack *o = new Fl_Scalepack( 7, 143, 110, 25 );
            o->type( Fl_Pack::HORIZONTAL );
            { Fl_Button* o = left_button = new Fl_Button(7, 143, 35, 25, "@<-");
                o->tooltip( "Move left" );
                o->type(0);
                o->labelsize(10);
                o->when( FL_WHEN_RELEASE );
                o->callback( ((Fl_Callback*)cb_handle), this );
            } // Fl_Button* o

            { Fl_Button* o = close_button = new Fl_Button(7, 143, 35, 25, "X");
                o->tooltip( "Remove strip" );
                o->type(0);
                o->labeltype( FL_EMBOSSED_LABEL );
                o->color( FL_LIGHT1 );
                o->selection_color( FL_RED );
                o->labelsize(10);
                o->when( FL_WHEN_RELEASE );
                o->callback( ((Fl_Callback*)cb_handle), this );
            } // Fl_Button* o

            { Fl_Button* o = right_button = new Fl_Button(7, 143, 35, 25, "@->");
                o->tooltip( "Move right" );
                o->type(0);
                o->labelsize(10);
                o->when( FL_WHEN_RELEASE );
                o->callback( ((Fl_Callback*)cb_handle), this );
            } // Fl_Button* o

            o->end();
        } // Fl_Group* o
        { Fl_Flip_Button* o = prepost_button = new Fl_Flip_Button(61, 183, 45, 22, "narrow/wide");
            o->type(1);
            o->labelsize(14);
            o->callback( ((Fl_Callback*)cb_handle), this );
            o->when(FL_WHEN_RELEASE);
        } // Fl_Flip_Button* o
        { Fl_Flip_Button* o = tab_button = new Fl_Flip_Button(61, 183, 45, 22, "fader/signal");
            o->type(1);
            o->labelsize( 14 );
            o->callback( cb_handle, this );
            o->when(FL_WHEN_RELEASE);
        }
//    { Fl_Pack* o = new Fl_Pack(8, 208, 103, 471);
//    { Fl_Pack* o = gain_pack = new Fl_Pack(8, 208, 103, 516 );

        o->end();
    }

    Fl_Pack *fader_pack;

    { Fl_Group *o = fader_tab = new Fl_Group( 7, 115, 105, 330, "Fader" );
        o->labeltype( FL_NO_LABEL );
        { Fl_Pack* o = fader_pack = new Fl_Pack(7, 116, 103, 330 );
            o->spacing( 20 );
            o->type( Fl_Pack::HORIZONTAL );
            { Controller_Module *o = gain_controller = new Controller_Module( true );
                o->pad( false );
                o->size( 33, 0 );
            }
            { Meter_Indicator_Module *o = meter_indicator = new Meter_Indicator_Module( true );
                o->pad( false );
                o->size( 58, 0 );
                o->clip_children( 0 );
                Fl_Group::current()->resizable(o);

            }
            o->end();
            Fl_Group::current()->resizable(o);
        } // Fl_Group* o
        o->end();
        Fl_Group::current()->resizable(o);
    }
    { Fl_Group *o = signal_tab = new Fl_Group( 7, 115, 105, 330 );
        o->labeltype( FL_NO_LABEL );
        o->hide();
        o->end();
    }
    { Fl_Pack *o = panner_pack = new Fl_Pack( 2, 440, 114, 40 );
        o->spacing( 2 );
        o->type( Fl_Pack::VERTICAL );

        { Fl_Box *o = new Fl_Box( 0, 0, 100, 24 );
            o->align( (Fl_Align)(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE) );
            o->labelsize( 10 );
            o->label( "Pan" );
        }
#if 0
        { Panner* o = new Panner(0, 0, 110, 90);
            o->box(FL_THIN_UP_BOX);
            o->color(FL_GRAY0);
            o->selection_color(FL_BACKGROUND_COLOR);
            o->labeltype(FL_NORMAL_LABEL);
            o->labelfont(0);
            o->labelsize(11);
            o->labelcolor(FL_FOREGROUND_COLOR);
            o->align(FL_ALIGN_TOP);
            o->when(FL_WHEN_RELEASE);
        } // Panner* o
#endif
        {
            Controller_Module *m = jack_input_controller = new Controller_Module( true );
            m->label( "Inputs" );
            m->chain( _chain );
            m->pad( false );
//            m->connect_to( &_chain->module( 0 )->control_input[1] );
            m->size( 33, 24 );
        }
        o->end();
    }

    end();

    color( FL_BLACK );

    //  _chain->configure_ports();
}


int
Mixer_Strip::handle ( int m )
{
    Logger log( this );

    switch ( m )
    {
        case FL_ENTER:
            name_field->color( FL_BLACK );
            name_field->redraw();
            return 1;
            break;
        case FL_LEAVE:
            name_field->color( _color );
            name_field->redraw();
            return 1;
            break;
        default:
            return Fl_Group::handle( m );

    }

    return 0;
}
