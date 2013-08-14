
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Chain.H"
#include "Module.H"
#include "Meter_Module.H"
#include "JACK_Module.H"
#include "Gain_Module.H"
#include "Plugin_Module.H"
#include "Controller_Module.H"

#include <FL/Fl_Box.H>
#include <FL/Fl_Menu.H>
#include <FL/fl_ask.H>
#include <FL/Fl_Flip_Button.H>
#include <FL/Fl_Tabs.H>
#include "FL/Fl_Flowpack.H"
#include "FL/Fl_Scroll.H"
#include "FL/Fl_Packscroller.H"
#include <FL/fl_draw.H>
#include "FL/menu_popup.H"
#include "FL/test_press.H"

#include "debug.h"
#include "Group.H"

#include "Mixer_Strip.H"
#include <dsp.h>
#include "Mixer.H"
extern char *instance_name;



/* Chain::Chain ( int X, int Y, int W, int H, const char *L ) : */
/*     Fl_Group( X, Y, W, H, L) */
Chain::Chain ( ) : Fl_Group( 0, 0, 100, 100, "")

{
    _deleting = false;

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
//        o->color( fl_darker( FL_BACKGROUND_COLOR ) );
//        o->color( FL_BACKGROUND_COLOR );
//        o->box( FL_NO_BOX );
        { Fl_Packscroller *o = new Fl_Packscroller( X, Y, W, H );
            o->color( FL_BACKGROUND_COLOR );
//            o->box( FL_FLAT_BOX );
            o->box( FL_THIN_UP_BOX );
            o->type( Fl_Scroll::VERTICAL );
            { Fl_Pack *o = modules_pack = new Fl_Pack( X, Y, W, H );
                o->type( Fl_Pack::VERTICAL );
                o->spacing( 6 );
                o->end();
                Fl_Group::current()->resizable( o );
            }
            o->end();
        }
        o->end();
    }
    { Fl_Group *o = control_tab = new Fl_Group( X, Y, W, H, "" );
        o->box( FL_FLAT_BOX );
        o->color( FL_BACKGROUND_COLOR );
        o->labeltype( FL_NO_LABEL );
        o->hide();
        { Fl_Scroll *o = new Fl_Scroll( X, Y, W, H );
            o->color( FL_BACKGROUND_COLOR );
            o->box( FL_NO_BOX );
            o->type( Fl_Scroll::VERTICAL );
            { Fl_Flowpack *o = controls_pack = new Fl_Flowpack( X, Y, W, H );
                o->type( FL_VERTICAL );
                o->hspacing( 10 );
                o->vspacing( 10 );
                o->box( FL_NO_BOX );
//            o->color( FL_RED );
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

    _deleting = true;

    client()->lock();

    for ( unsigned int i = scratch_port.size(); i--; )
        free( (sample_t*)scratch_port[i].buffer() );
    
    /* if we leave this up to FLTK, it will happen after we've
     already destroyed the client */
    modules_pack->clear();
    controls_pack->clear();

    client()->unlock();
}

Group *
Chain::client ( void ) 
{
    return strip()->group();
}



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



void
Chain::log_children ( void ) const
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
        add( m );
    }

    { Module *m = new Gain_Module();
        m->is_default( true );
        m->chain(this);
        add( m );
    }

    { Module *m = new Meter_Module();
        m->is_default( true );
        add( m );
    }

    { JACK_Module *m = new JACK_Module();
        m->is_default( true );
        m->chain( this );
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
}

void Chain::cb_handle(Fl_Widget* o, void* v) {
    ((Chain*)(v))->cb_handle(o);
}

void
Chain::remove ( Controller_Module *m )
{
    DMESSAGE( "Removing controller module from chain" );

    client()->lock();

    m->disconnect();

    controls_pack->remove( m );
    modules_pack->remove( m );

    build_process_queue();

    client()->unlock();

    redraw();
}

void
Chain::send_feedback ( void )
{
    for ( int i = 0; i < modules(); i++ )
        module(i)->send_feedback();
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

    client()->lock();

    strip()->handle_module_removed( m );

    modules_pack->remove( m );

    configure_ports();

    client()->unlock();
}

/* determine number of output ports, signal if changed.  */
void
Chain::configure_ports ( void )
{
     int nouts = 0;

    client()->lock();

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
            free(scratch_port[i].buffer());
        scratch_port.clear();

        for ( unsigned int i = 0; i < req_buffers; ++i )
        {
            Module::Port p( NULL, Module::Port::OUTPUT, Module::Port::AUDIO );
            p.connect_to( buffer_alloc( client()->nframes() ) );
            buffer_fill_with_silence( (sample_t*)p.buffer(), client()->nframes() );
            scratch_port.push_back( p );
        }
    }

    build_process_queue();

    client()->unlock();

    parent()->redraw();
}

int 
Chain::get_module_instance_number ( Module *m )
{
    int n = 0;

    for ( int i = 0; i < modules() && module(i) != m; ++i )
        if ( ! strcmp( module(i)->label(), m->label() ) )
            n++;

    return n;
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

unsigned int
Chain::maximum_name_length ( void )
{
    return JACK::Client::maximum_name_length() - ( strlen( instance_name ) + 1 );
}

void
Chain::freeze_ports ( void )
{
    for ( int i = 0; i < modules(); i++ )
    {
        Module *m = module(i);
        m->freeze_ports();
    }
}

void
Chain::thaw_ports ( void )
{
    for ( int i = 0; i < modules(); i++ )
    {
        Module *m = module(i);
        m->thaw_ports();
    }
}

/* rename chain... we have to let our modules know our name has
 * changed so they can take the appropriate action (in particular the
 * JACK module). */
void
Chain::name ( const char *name )
{
    _name = name;

    if ( strip()->group() )
    {
        if ( strip()->group()->single() )
            strip()->group()->name(name);
    }
    
    for ( int i = 0; i < modules(); ++i )
    {
        module( i )->handle_chain_name_changed();
    }
}

bool
Chain::add ( Module *m )
{
    /* FIXME: hacky */
    if ( !strcmp( m->name(), "Controller" ) )
        return false;
    else
        return insert( NULL, m );
}

bool
Chain::add ( Controller_Module *m )
{
    DMESSAGE( "Adding control" );
    add_control(m);
    return true;
}

bool
Chain::insert ( Module *m, Module *n )
{
    client()->lock();

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


    strip()->handle_module_added( n );

    configure_ports();

    client()->unlock();

    DMESSAGE( "Module \"%s\" has %i:%i audio and %i:%i control ports",
              n->name(),
              n->ninputs(),
              n->noutputs(),
              n->ncontrol_inputs(),
              n->ncontrol_outputs() );

    n->initialize();
    return true;

err:

    client()->unlock();

    DMESSAGE( "Insert failed" );

    return false;
}

/* add a control to the control strip. Assumed to already be connected! */
void
Chain::add_control ( Controller_Module *m )
{
    client()->lock();

    controls_pack->add( m );

    configure_ports();

    client()->unlock();

    controls_pack->redraw();
}

void
Chain::draw_connections ( Module *m )
{
    int spacing;
    int offset;

    int X, Y, W, H;

    ((Fl_Packscroller*)chain_tab->child( 0 ))->bbox( X, Y, W, H );

    fl_push_clip( X, Y, W, H );

    Fl_Color c = FL_FOREGROUND_COLOR;
    fl_color( c );

    if ( m->ninputs() )
    {
        spacing = w() / m->ninputs();
        offset = spacing / 2;

        for ( int i = m->ninputs(); i--; )
            fl_rectf( m->x() + offset + ( spacing * i ), m->y() - 3, 2, 3 );
    }

    fl_color( fl_darker( c ) );

    if ( m->noutputs() )
    {
        spacing = w() / m->noutputs();
        offset = spacing / 2;
        for ( int i = m->noutputs(); i--; )
            fl_rectf( m->x() + offset + ( spacing * i ), m->y() + m->h(), 2, 3 );
    }

    fl_pop_clip();
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
Chain::strip ( Mixer_Strip * ms )
{
    _strip = ms;
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



/*****************/
/* Import/Export */
/*****************/

void
Chain::snapshot ( void *v )
{
    ((Chain*)v)->snapshot();
}

void
Chain::snapshot ( void )
{
    log_children();
}

bool
Chain::do_export ( const char *filename )
{
    MESSAGE( "Exporting chain state" );
    Loggable::snapshot_callback( &Chain::snapshot, this );
    Loggable::snapshot( filename );
    return true;
}



/**********/
/* Client */
/**********/

void
Chain::process ( nframes_t nframes )
{
    for ( std::list<Module*>::const_iterator i = process_queue.begin(); i != process_queue.end(); ++i )
    {
        Module *m = *i;

        m->process( nframes );
    }
}

void
Chain::buffer_size ( nframes_t nframes )
{
    for ( unsigned int i = scratch_port.size(); i--; )
        free(scratch_port[i].buffer());
    scratch_port.clear();

    configure_ports();

    for ( int i = 0; i < modules(); ++i )
    {
        Module *m = module(i);

        m->resize_buffers( nframes );
    }
}

int
Chain::sample_rate_change ( nframes_t nframes )
{
    Module::set_sample_rate ( nframes );
    for ( int i = 0; i < modules(); ++i )
    {
        Module *m = module(i);

        m->handle_sample_rate_change( nframes );
    }

    return 0;
}

/* handle jack port connection change */
void
Chain::port_connect ( jack_port_id_t a, jack_port_id_t b, int connect )
{
    if ( _deleting )
        return;

    /* this is called from JACK non-RT thread... */
   
    if ( jack_port_is_mine( client()->jack_client(), jack_port_by_id( client()->jack_client(), a ) ) ||
         jack_port_is_mine( client()->jack_client(), jack_port_by_id( client()->jack_client(), b ) ))
    {
        Fl::awake( Chain::update_connection_status, this );
    }
}

void
Chain::update ( void )
{
    for ( int i = 0; i < controls_pack->children(); ++i )
    {
        Controller_Module *cm = (Controller_Module*)controls_pack->child( i );
        cm->update();
    }

    for ( int i = 0; i < modules(); i++ )
    {
        Module *m = module(i);
        m->update();
    }
}

void
Chain::update_connection_status ( void *v )
{
    ((Chain*)v)->update_connection_status();
}

void
Chain::update_connection_status ( void )
{
    for ( int i = 0; i < modules(); i++ )
    {
        Module *m = module(i);
        
        if ( !strcmp( m->basename(), "JACK" ) )
        {
            ((JACK_Module*)m)->update_connection_status();
        }
    }
}
