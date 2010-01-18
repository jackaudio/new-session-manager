
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

/* Filter chain. This would all be much simpler if we chose not to
 * allow non 1:1 plugins to be mixed in a single chain...
 *
 * Supporting the mixture requires duplicating some inputs (to satisfy
 * stereo input plugins reading mono outputs) and duplicating some
 * plugins (to satisfy mono input plugins reading stereo outputs).
 *
 * Basically, what this means is that the intermediate number of
 * buffers need not have any relation to the starting and ending
 * buffer count. (Picture an ambisonic panner going into an ambisonic
 * decoder (1:6:2).
 *
 * The chain will allocate enough buffers to hold data from the
 * maximum number of channels used by a contained module.
 *
 * The process thread goes as follows:
 *
 * 1. Copy inputs to chain buffers.
 *
 * 2. process() each module in turn (reusing buffers in-place) (inputs
 * will be copied or plugins duplicated as necessary)
 *
 * 3. Copy chain buffers to outputs.
 *
 * For chains where the number of channels never exceeds the maximum
 * of the number of inputs and outputs, the first copy can be
 * optimized out.
 */

#include "const.h"

#include "Chain.H"

#include "Module.H"
#include "Meter_Module.H"
#include "JACK_Module.H"
#include "Gain_Module.H"
#include "Plugin_Module.H"
#include "Controller_Module.H"

#include <Fl/Fl_Box.H>
#include <FL/Fl_Menu.H>
#include <FL/fl_ask.H>

#include <stdlib.h>
#include "util/debug.h"

#include <stdio.h>

#include <FL/fl_draw.H>

#include "Engine/Engine.H"
#include <FL/Fl_Tabs.H>
#include "FL/Fl_Flowpack.H"
#include "FL/Fl_Scroll.H"
#include <string.h>

#include "Mixer_Strip.H"
#include <dsp.h>

#include <FL/Fl_Flip_Button.H>



void
Chain::get ( Log_Entry &e ) const
{
    e.add( ":strip", strip() );
    e.add( ":tab", tab_button->value() ? "controls" : "chain" );
}

void
Chain::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":tab" ) )
        {
            tab_button->value( strcmp( v, "controls" ) == 0 );
            tab_button->do_callback();
        }
        else if ( ! strcmp( s, ":strip" ) )
        {
            int i;
            sscanf( v, "%X", &i );
            Mixer_Strip *t = (Mixer_Strip*)Loggable::find( i );

            assert( t );

            t->chain( this );
        }
    }
}




/* Chain::Chain ( int X, int Y, int W, int H, const char *L ) : */
/*     Fl_Group( X, Y, W, H, L) */
Chain::Chain ( ) : Fl_Group( 0, 0, 100, 100, "")

{
    _engine = NULL;

    int X = 0;
    int Y = 0;
    int W = 100;
    int H = 100;

/*     _outs = 1; */
/*     _ins = 1; */

    _configure_outputs_callback = NULL;

    _strip = NULL;

    _name = NULL;

    labelsize( 10 );
    align( FL_ALIGN_TOP );

    { Fl_Flip_Button* o  = tab_button = new Fl_Flip_Button( X, Y, W, 16, "chain/controls");
        o->type(1);
        o->labelsize( 12 );
        o->callback( cb_handle, this );
    }

    Y += 18;
    H -= 18;

    { Fl_Group *o = chain_tab = new Fl_Group( X, Y, W, H, "" );
        o->labeltype( FL_NO_LABEL );
        o->box( FL_FLAT_BOX );
        { Fl_Pack *o = modules_pack = new Fl_Pack( X, Y, W, H );
            o->type( Fl_Pack::VERTICAL );
            o->spacing( 10 );
            o->end();
        }
        o->end();
    }
    { Fl_Group *o = control_tab = new Fl_Group( X, Y, W, H, "" );
        o->labeltype( FL_NO_LABEL );
        o->hide();
        { Fl_Scroll *o = new Fl_Scroll( X, Y, W, H );
            o->type( Fl_Scroll::VERTICAL );
            { Fl_Flowpack *o = controls_pack = new Fl_Flowpack( X, Y, W, H );
                o->hspacing( 10 );
                o->vspacing( 10 );
//                    o->box( FL_FLAT_BOX );
//                    o->color( FL_RED );
                o->end();
                Fl_Group::current()->resizable( o );
            }
            o->end();
            Fl_Group::current()->resizable( o );
        }
        o->end();
        o->hide();
        Fl_Group::current()->resizable( o );
    }

    end();

    log_create();
}

Chain::~Chain ( )
{
    DMESSAGE( "Destroying chain" );

    log_destroy();

    engine()->lock();

    /* if we leave this up to FLTK, it will happen after we've
     already destroyed the engine */
    modules_pack->clear();
    controls_pack->clear();

    delete _engine;
    _engine = NULL;
}



void
Chain::log_children ( void )
{
    log_create();

    for ( int i = 0; i < modules(); ++i )
    {
        module(i)->log_create();
    }

    for ( int i = 0; i < controls_pack->children(); ++i )
    {
        Controller_Module *cm = (Controller_Module*)controls_pack->child( i );

        cm->log_create();
    }
}

/* Fill this chain with JACK I/O, Gain, and Meter modules. */
void
Chain::initialize_with_default ( void )
{

    { JACK_Module *m = new JACK_Module();
        m->is_default( true );
        m->chain( this );
        m->configure_outputs( 1 );
        m->initialize();
        add( m );
    }

    { Module *m = new Gain_Module();
        m->is_default( true );
        m->initialize();
        add( m );
    }

    { Module *m = new Meter_Module();
        m->is_default( true );
        add( m );
    }

    { JACK_Module *m = new JACK_Module();
        m->is_default( true );
        m->chain( this );
        m->initialize();
        add( m );
    }
}


void Chain::cb_handle(Fl_Widget* o) {
    if ( o == tab_button )
    {
        Fl_Flip_Button *fb = (Fl_Flip_Button*)o;

        if ( fb->value() == 0 )
        {
            control_tab->hide();
            chain_tab->show();
        }
        else
        {
            chain_tab->hide();
            control_tab->show();
        }
    }

    /* if ( o == head_button ) */
    /* { */
    /*     Module *m = Module::pick_plugin(); */

    /*     insert_before( (Module*)modules_pack->child( 0 ), m ); */
    /* } */
    /* else if ( o == tail_button ) */
    /* { */
    /*     Module *m = Module::pick_plugin(); */
    /*     insert_before( 0, m ); */
    /* } */
}

void Chain::cb_handle(Fl_Widget* o, void* v) {
    ((Chain*)(v))->cb_handle(o);
}


/* remove a module from the chain. this isn't guaranteed to succeed,
 * because removing the module might result in an invalid routing */
void
Chain::remove ( Module *m )
{
    int i = modules_pack->find( m );

    int ins = 0;

    if ( i != 0 )
        ins = module( i - 1 )->noutputs();

    if ( ! can_configure_outputs( m, ins ) )
    {
        fl_alert( "Can't remove module at this point because the resultant chain is invalid" );
    }

    modules_pack->remove( m );

    configure_ports();
}

/* determine number of output ports, signal if changed.  */
void
Chain::configure_ports ( void )
{
     int nouts = 0;

    engine()->lock();

    for ( int i = 0; i < modules(); ++i )
    {
        module( i )->configure_inputs( nouts );
        nouts = module( i )->noutputs();
    }

    unsigned int req_buffers = required_buffers();

    DMESSAGE( "required_buffers = %i", req_buffers );

    if ( scratch_port.size() < req_buffers )
    {
        for ( unsigned int i = scratch_port.size(); i--; )
            delete[] (sample_t*)scratch_port[i].buffer();
        scratch_port.clear();

        for ( unsigned int i = 0; i < req_buffers; ++i )
        {
            Module::Port p( NULL, Module::Port::OUTPUT, Module::Port::AUDIO );
            p.connect_to( new sample_t[engine()->nframes()] );
            buffer_fill_with_silence( (sample_t*)p.buffer(), engine()->nframes() );
            scratch_port.push_back( p );
        }
    }

    build_process_queue();

    engine()->unlock();

    parent()->redraw();
}

/* calculate the minimum number of buffers required to satisfy this chain */
int
Chain::required_buffers ( void )
{
    int buffers = 0;
    int outs = 0;

    for ( int i = 0; i < modules(); ++i )
    {
        outs = module( i )->can_support_inputs( outs );

        if ( outs > buffers )
            buffers = outs;
    }

    return buffers;
}

/* called by a module when it wants to alter the number of its
 * outputs. Also used to test for chain validity when inserting /
 * removing modules */
bool
Chain::can_configure_outputs ( Module *m, int n ) const
{
    /* start at the requesting module */

    int outs = n;

    int i = modules_pack->find( m );

    if ( modules() - 1 == i )
        /* last module */
        return true;

    for ( i++ ; i < modules(); ++i )
    {
        outs = module( i )->can_support_inputs( outs );

        if ( outs < 0 )
            return false;
    }

    return true;
}

/* rename chain... we have to let our modules know our name has
 * changed so they can take the appropriate action (in particular the
 * JACK module). */
void
Chain::name ( const char *name )
{
    char ename[512];
    snprintf( ename, sizeof(ename), "%s/%s", APP_NAME, name );


    if ( ! _engine )
    {
        _engine = new Engine( &Chain::process, this );

        engine()->init( ename );
    }
    else
    {
        DMESSAGE( "Renaming JACK client from \"%s\" to \"%s\"", _name, ename );

        _name = engine()->name( ename );
        /* FIXME: discarding the name jack picked is technically wrong! */

    }

    _name = name;

    for ( int i = 0; i < modules(); ++i )
        module( i )->handle_chain_name_changed();
}





#include "FL/menu_popup.H"

bool
Chain::add ( Module *m )
{
    return insert( NULL, m );
}

bool
Chain::insert ( Module *m, Module *n )
{

    engine()->lock();

    if ( !m )
    {
        if ( modules() == 0 && n->can_support_inputs( 0 ) >= 0 )
        {
            n->configure_inputs( 0 );
            modules_pack->add( n );
            n->chain( this );
        }
        else if ( n->can_support_inputs( module( modules() - 1 )->noutputs() ) >= 0 )
        {
            n->configure_inputs( module( modules() - 1 )->noutputs() );
            modules_pack->add( n );
            n->chain( this );
        }
        else
        {
            DMESSAGE( "Module says it can't support %i inputs", module( modules() - 1 )->noutputs() );

            goto err;
        }
    }
    else
    {
        int i = modules_pack->find( m );

        if ( 0 == i )
        {
            /* inserting to head of chain*/
            if ( n->can_support_inputs( 0 ) >= 0 )
                n->configure_inputs( 0 );
            else
                goto err;
        }
        else
        {
            if ( n->can_support_inputs(  module( i - 1 )->noutputs() ) >= 0 )
            {
                n->configure_inputs(  module( i - 1 )->noutputs() );

                m->configure_inputs( n->noutputs() );

                for ( int j = i + 1; j < modules(); ++j )
                    module( j )->configure_inputs( module( j - 1 )->noutputs() );
            }
            else
                goto err;
        }

        modules_pack->insert( *n, i );
        n->chain( this );
    }

    DMESSAGE( "Module \"%s\" has %i:%i audio and %i:%i control ports",
              n->name(),
              n->ninputs(),
              n->noutputs(),
              n->ncontrol_inputs(),
              n->ncontrol_outputs() );

    strip()->handle_module_added( n );

    configure_ports();

    engine()->unlock();

    return true;

err:

    DMESSAGE( "Insert failed" );

    engine()->unlock();

    return false;
}

/* add a control to the control strip. Assumed to already be connected! */
void
Chain::add_control ( Module *m )
{
    engine()->lock();

    controls_pack->add( m );

    engine()->unlock();

    controls_pack->redraw();
}

void
Chain::draw_connections ( Module *m )
{
    int spacing;
    int offset;

    Fl_Color c =fl_color_average( FL_WHITE, FL_YELLOW, 0.50 );
    fl_color( c );

    if ( m->ninputs() )
    {
        spacing = w() / m->ninputs();
        offset = spacing / 2;

        for ( int i = m->ninputs(); i--; )
            fl_rectf( m->x() + offset + ( spacing * i ), m->y() - 5, 2, 5 );
    }

    fl_color( fl_darker( c ) );

    if ( m->noutputs() )
    {
        spacing = w() / m->noutputs();
        offset = spacing / 2;
        for ( int i = m->noutputs(); i--; )
            fl_rectf( m->x() + offset + ( spacing * i ), m->y() + m->h(), 2, 5 );
    }
}

void
Chain::add_to_process_queue ( Module *m )
{
    for ( std::list<Module*>::const_iterator i = process_queue.begin(); i != process_queue.end(); ++i )
        if ( m == *i )
            return;

    process_queue.push_back( m );
}

/* run any time the internal connection graph might have
 * changed... Tells the process thread what order modules need to be
 * run in. */
void
Chain::build_process_queue ( void )
{
    process_queue.clear();

    for ( int i = 0; i < modules(); ++i )
    {
        Module *m = (Module*)module( i );

        /* controllers */
        for ( unsigned int j = 0; j < m->control_input.size(); ++j )
        {
            if ( m->control_input[j].connected() )
            {
                add_to_process_queue( m->control_input[j].connected_port()->module() );
            }
        }

        /* audio modules */
        add_to_process_queue( m );

        /* indicators */
        for ( unsigned int j = 0; j < m->control_output.size(); ++j )
        {
            if ( m->control_output[j].connected() )
            {
                add_to_process_queue( m->control_output[j].connected_port()->module() );
            }
        }
    }

    /* connect all the ports to the buffers */
    for ( int i = 0; i < modules(); ++i )
    {
        Module *m = module( i );
        for ( unsigned int j = 0; j < m->audio_input.size(); ++j )
        {
            m->audio_input[j].connect_to( &scratch_port[j] );
        }
        for ( unsigned int j = 0; j < m->audio_output.size(); ++j )
        {
            m->audio_output[j].connect_to( &scratch_port[j] );
        }

        m->handle_port_connection_change();
    }

/*     DMESSAGE( "Process queue looks like:" ); */

/*     for ( std::list<Module*>::const_iterator i = process_queue.begin(); i != process_queue.end(); ++i ) */
/*     { */
/*         const Module* m = *i; */

/*         if ( m->audio_input.size() || m->audio_output.size() ) */
/*             DMESSAGE( "\t%s", (*i)->name() ); */
/*         else if ( m->control_output.size() ) */
/*             DMESSAGE( "\t%s -->", (*i)->name() ); */
/*         else if ( m->control_input.size() ) */
/*             DMESSAGE( "\t%s <--", (*i)->name() ); */

/*         { */
/*             char *s = m->get_parameters(); */

/*             DMESSAGE( "(%s)", s ); */

/*             delete[] s; */
/*         } */
/*     } */
}

void
Chain::draw ( void )
{
    Fl_Group::draw();

/*     if ( 0 == strcmp( "Chain", tabs->value()->label() ) ) */
    if ( chain_tab->visible() )
        for ( int i = 0; i < modules(); ++i )
            draw_connections( module( i ) );
}

void
Chain::resize ( int X, int Y, int W, int H )
{
    Fl_Group::resize( X, Y, W, H );

/* this won't naturally resize because it's inside of an Fl_Scroll... */
    controls_pack->size( W, controls_pack->h() );
}

#include "FL/test_press.H"

int
Chain::handle ( int m )
{
    switch ( m )
    {
        case FL_PUSH:
        {
            if ( Fl::belowmouse() != this )
            {
                Module *m = NULL;

                for ( int i = 0; i < modules(); ++i )
                    if ( Fl::event_inside( module( i ) ) )
                    {
                        m = module( i );
                        break;
                    }

                if ( m )
                {
                    if ( test_press( FL_BUTTON3 | FL_CTRL ) )
                    {
                        if ( m->is_default() )
                        {
                            fl_alert( "Default modules may not be deleted." );
                        }
                        else
                        {
                            remove( m );
                            delete m;
                            redraw();
                        }
                        return 1;
                    }
                    else if ( test_press( FL_BUTTON1 | FL_SHIFT ) )
                    {
//                        Module *mod = (Module*)Plugin_Module::pick_plugin();
                        Module *mod = Module::pick_module();
                        if ( mod )
                        {
                            if ( !strcmp( mod->name(), "JACK" ) )
                            {
                                DMESSAGE( "Special casing JACK module" );
                                JACK_Module *jm = (JACK_Module*)mod;
                                jm->chain( this );
                                jm->configure_inputs( m->ninputs() );
                                jm->configure_outputs( m->ninputs() );
                            }

                            if ( ! insert( m, mod ) )
                                fl_alert( "Cannot insert this module at this point in the chain" );
                            redraw();
                        }
                        return 1;
                    }
                    else if ( test_press( FL_BUTTON1 | FL_CTRL ) )
                    {
                        if ( m->active() )
                            m->deactivate();
                        else
                            m->activate();
                        return 1;
                    }
                }
            }
            break;
        }
    }

    return Fl_Group::handle( m );
}
void
Chain::strip ( Mixer_Strip * ms )
{
    _strip = ms;
}


void
Chain::process ( nframes_t nframes, void *v )
{
    ((Chain*)v)->process( nframes );
}

void
Chain::process ( nframes_t nframes )
{
    for ( std::list<Module*>::const_iterator i = process_queue.begin(); i != process_queue.end(); ++i )
    {
        Module *m = *i;

        m->nframes( nframes );
        if ( m->active() )
            m->process();
    }
}
