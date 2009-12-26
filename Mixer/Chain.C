
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

#include "Chain.H"

#include "Module.H"
#include "Meter_Module.H"
#include "JACK_Module.H"
#include "Gain_Module.H"
#include "Plugin_Module.H"

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

#include <dsp.h>

Chain::Chain ( int X, int Y, int W, int H, const char *L ) :
    Fl_Group( X, Y, W, H, L)
{
    _outs = 1;
    _ins = 1;

    _configure_outputs_callback = NULL;

    _name = NULL;

    { Fl_Tabs *o = tabs = new Fl_Tabs( X, Y, W, H );
        { Fl_Group *o = new Fl_Group( X, Y + 24, W, H - 24, "Chain" );
            o->box( FL_FLAT_BOX );
            o->labelsize( 9 );
            { Fl_Pack *o = modules_pack = new Fl_Pack( X, Y + 24, W, H - 24 );
                o->type( Fl_Pack::VERTICAL );
                o->spacing( 10 );
                o->end();
            }
            o->end();
        }
        { Fl_Group *o = new Fl_Group( X, Y + 24, W, H - 24, "Controls" );
            o->labelsize( 9 );
            o->hide();
            { Fl_Scroll *o = new Fl_Scroll( X, Y + 24, W, H - 24 );
                o->type( Fl_Scroll::VERTICAL );
                { Fl_Flowpack *o = controls_pack = new Fl_Flowpack( X, Y + 24, W, H - 24 );
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
            Fl_Group::current()->resizable( o );
        }
        o->end();
        Fl_Group::current()->resizable( o );
    }

    end();
}

/* Fill this chain with JACK I/O, Gain, and Meter modules. */
void
Chain::initialize_with_default ( void )
{

    {
        JACK_Module *jm = new JACK_Module( 50, 50, "JACK" );
        jm->chain( this );
        jm->configure_outputs( 1 );

        jm->initialize();
        jm->color( FL_BLACK );
        insert( NULL, jm );
    }

    {
        JACK_Module *m = new JACK_Module( 50, 50, "JACK" );
        m->chain( this );
        m->initialize();
        m->color( FL_BLACK );
        insert( NULL, m );
    }
}


void Chain::cb_handle(Fl_Widget* o) {
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
    int old_outs = outs();
    int nouts = 0;

    engine->lock();

    for ( int i = 0; i < modules(); ++i )
    {
        module( i )->configure_inputs( nouts );
        nouts = module( i )->noutputs();
    }

    outs( nouts );

    int req_buffers = required_buffers();

    if ( outs() != old_outs )
    {
        if ( configure_outputs_callback() )
            configure_outputs_callback()( this, _configure_outputs_userdata );
    }

    DMESSAGE( "required_buffers = %i", req_buffers );

    if ( port.size() != req_buffers )
    {
        for ( unsigned int i = port.size(); i--; )
            delete[] (sample_t*)port[i].buffer();
        port.clear();

        for ( unsigned int i = 0; i < req_buffers; ++i )
        {
            Module::Port p( NULL, Module::Port::OUTPUT, Module::Port::AUDIO );
            p.connect_to( new sample_t[engine->nframes()] );
            buffer_fill_with_silence( (sample_t*)p.buffer(), engine->nframes() );
            port.push_back( p );
        }
    }

    build_process_queue();

    engine->unlock();

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

/* return true if this chain can be converted to support /n/ input channels  */
bool
Chain::can_support_input_channels ( int n )
{
    /* FIXME: implement */
    return true;
}

/* rename chain... we have to let our modules know our name has
 * changed so they can take the appropriate action (in particular the
 * JACK module). */
void
Chain::name ( const char *name )
{
    _name = name;

    for ( int i = 0; i < modules(); ++i )
        module( i )->handle_chain_name_changed();
}





#include "FL/menu_popup.H"

bool
Chain::insert ( Module *m, Module *n )
{

    engine->lock();

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
            goto err;
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

    DMESSAGE( "Module has %i:%i audio and %i:%i control ports",
              n->ninputs(),
              n->noutputs(),
              n->ncontrol_inputs(),
              n->ncontrol_outputs() );

    configure_ports();

    engine->unlock();

    return true;

err:

    engine->unlock();

    return false;
}

/* add a control to the control strip. Assumed to already be connected! */
void
Chain::add_control ( Module *m )
{
    controls_pack->add( m );
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
            m->audio_input[j].connect_to( &port[j] );
        }
        for ( unsigned int j = 0; j < m->audio_output.size(); ++j )
        {
            m->audio_output[j].connect_to( &port[j] );
        }

        m->handle_port_connection_change();
    }

    DMESSAGE( "Process queue looks like:" );

    for ( std::list<Module*>::const_iterator i = process_queue.begin(); i != process_queue.end(); ++i )
    {
        const Module* m = *i;

        if ( m->audio_input.size() || m->audio_output.size() )
            DMESSAGE( "\t%s", (*i)->name() );
        else if ( m->control_output.size() )
            DMESSAGE( "\t%s -->", (*i)->name() );
        else if ( m->control_input.size() )
            DMESSAGE( "\t%s <--", (*i)->name() );

        {
            char *s = m->describe_inputs();

            DMESSAGE( "(%s)", s );

            delete[] s;
        }
    }
}

void
Chain::draw ( void )
{
    Fl_Group::draw();

    if ( 0 == strcmp( "Chain", tabs->value()->label() ) )
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
                        if ( FL_BLACK == m->color() )
                        {
                            /* FIXME: hack */
                            fl_alert( "Cannot delete this module." );
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
                        Module *mod = (Module*)Plugin_Module::pick_plugin();
                        if ( mod )
                        {
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
