
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


#include <FL/Fl_Tabs.H>
#include "FL/Fl_Flowpack.H"
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
//    e.add( ":controllable",    controllable() );
//    e.add( ":inputs",          _in.size()     );
/*     e.add( ":gain",            gain_slider->value() ); */
    e.add( ":meter_point",      prepost_button->value() ? "pre" : "post" );
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
//        else if ( ! strcmp( s, ":controllable" ) )
//            controllable( atoi( v ) );
        else if ( ! strcmp( s, ":inputs" ) )
            configure_ports( atoi( v ) );
/*         else if ( ! strcmp( s, ":gain" ) ) */
/*             gain_slider->value( atof( v ) ); */
        else if ( ! strcmp( s, ":meter_point" ) )
            prepost_button->value( strcmp( v, "pre" ) == 0 );
        else if ( ! strcmp( s, ":color" ) )
        {
            color( (Fl_Color)atoll( v ) );
            redraw();
        }
    }

    if ( ! mixer->contains( this ) )
        mixer->add( this );
}


Mixer_Strip::Mixer_Strip( const char *strip_name, int channels ) : Fl_Group( 0, 0, 120, 600 )
{

    label( strdup( strip_name ) );

    init();

    color( (Fl_Color)rand() );

//    name( strdup( strip_name ) );

    configure_ports( channels );

    log_create();
}

Mixer_Strip::Mixer_Strip() : Fl_Group( 0, 0, 120, 600 )
{
    init();

    log_create();
}

Mixer_Strip::~Mixer_Strip ( )
{
    configure_ports( 0 );
}



void Mixer_Strip::cb_handle(Fl_Widget* o) {
    // parent()->parent()->damage( FL_DAMAGE_ALL, x(), y(), w(), h() );
    if ( o == close_button )
        ((Mixer*)parent())->remove( this );
    else if ( o == inputs_counter )
        configure_ports( ((Fl_Counter*)o)->value() );
    else if ( o == name_field )
        name( name_field->value() );
/*     else if ( o == controllable_button ) */
/*     { */
/*         controllable( controllable_button->value() ); */
/* //        configure_ports( channels() ); */
/*     } */
    else if ( o == prepost_button )
    {
        if ( ((Fl_Button*)o)->value() )
            size( 300, h() );
        else
            size( 120, h() );

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
    chain->name( s );
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

bool
Mixer_Strip::configure_ports ( int n )
{
/*     /\* figure out how many buffers we have to create *\/ */
/*     int required_buffers = chain->required_buffers(); */

/*     engine->lock(); */

/*     if ( chain_buffers > 0 ) */
/*     { */
/*         for ( int i = chain_buffers; --i; ) */
/*         { */
/*             delete chain_buffer[i]; */
/*             chain_buffer[i] = NULL; */
/*         } */
/*         delete chain_buffer; */
/*         chain_buffer = NULL; */
/*         chain_buffers = 0; */
/*     } */

/*     sample_t **buf = new sample_t*[required_buffers]; */
/*     for ( int i = 0; i < required_buffers; ++i ) */
/*         buf[i] = new sample_t[nframes]; */

/*     chain_buffers = required_buffers; */
/*     chain_buffer = buf; */

/*     engine->unlock(); */

/*     /\* FIXME: bogus *\/ */
/*     return true; */

}



void
Mixer_Strip::process ( nframes_t nframes )
{
    THREAD_ASSERT( RT );

/*     sample_t *gain_buf = NULL; */
/*     float g = gain_slider->value(); */

/*     if ( _control && _control->connected() ) */
/*     { */
/*         gain_buf = (sample_t*)_control->buffer( nframes ); */

/* /\*         // bring it up to 0.0-2.0f *\/ */
/* /\*         for ( int i = nframes; i--; ) *\/ */
/* /\*             gain_buf[i] += 1.0f; *\/ */

/*         // apply gain from slider */
/*         buffer_apply_gain( gain_buf, nframes, g ); */

/*         /\* FIXME: bullshit! *\/ */
/*         _control_peak = gain_buf[0]; */
/*     } */
/*     else */
/*     { */
/*         _control_peak = 0; */
/*     } */

/*     for ( int i = channels(); i--; ) */
/*     { */
/*         if ( _in[i].connected()) */
/*         { */
/*             if ( gain_buf ) */
/*                 buffer_copy_and_apply_gain_buffer( (sample_t*)_out[i].buffer( nframes ), (sample_t*)_in[i].buffer( nframes ), gain_buf, nframes ); */
/*             else */
/*                 buffer_copy_and_apply_gain( (sample_t*)_out[i].buffer( nframes ), (sample_t*)_in[i].buffer( nframes ),  nframes, g ); */

/*             sample_t *meter_buffer = prepost_button->value() == 1 ? (sample_t*)_in[i].buffer( nframes ) : (sample_t*)_out[i].buffer( nframes ); */

/*             /\* set peak value (in dB) *\/ */
/*             _peak[i] = 20 * log10( get_peak_sample( meter_buffer, nframes ) / 2.0f ); */
/*         } */
/*         else */
/*         { */
/*             buffer_fill_with_silence( (sample_t*)_out[i].buffer( nframes ), nframes ); */
/*         } */
/*     } */

    chain->process( nframes );
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
            { Fl_Button* o = close_button = new Fl_Button(7, 143, 35, 25, "X");
                o->tooltip( "Remove strip" );
                o->type(0);
                o->labeltype( FL_EMBOSSED_LABEL );
                o->color( FL_LIGHT1 );
                o->selection_color( FL_RED );
                o->labelsize(10);
                o->callback( ((Fl_Callback*)cb_handle), this );
            } // Fl_Button* o
            o->end();
        } // Fl_Group* o
        { Fl_Flip_Button* o = prepost_button = new Fl_Flip_Button(61, 183, 45, 22, "narrow/wide");
            o->type(1);
//            o->box(FL_ROUNDED_BOX);
            o->box( FL_THIN_DOWN_BOX );
            o->color((Fl_Color)106);
            o->selection_color((Fl_Color)65);
            o->labeltype(FL_NORMAL_LABEL);
            o->labelfont(0);
            o->labelsize(14);
            o->labelcolor(FL_FOREGROUND_COLOR);
            o->align(FL_ALIGN_CLIP);
            o->callback( ((Fl_Callback*)cb_handle), this );
            o->when(FL_WHEN_RELEASE);
        } // Fl_Flip_Button* o
//    { Fl_Pack* o = new Fl_Pack(8, 208, 103, 471);
//    { Fl_Pack* o = gain_pack = new Fl_Pack(8, 208, 103, 516 );

        o->end();
    }

    Fl_Pack *fader_pack;

    { Fl_Tabs *o = new Fl_Tabs( 4, 104, 110, 330 );
        o->clip_children( 1 );
        o->box( FL_NO_BOX );
        { Fl_Group *o = new Fl_Group( 4, 114, 110, 330, "Fader" );
            o->labelsize( 9 );
            o->box( FL_NO_BOX );
            { Fl_Pack* o = fader_pack = new Fl_Pack(4, 116, 103, 330 );
                o->spacing( 20 );
                o->type( Fl_Pack::HORIZONTAL );
                o->end();
                Fl_Group::current()->resizable(o);
            } // Fl_Group* o
            o->end();
            Fl_Group::current()->resizable(o);
        }
        { Fl_Group *o = new Fl_Group( 4, 114, 110, 330, "Signal" );
            o->labelsize( 9 );
            o->hide();
            { Chain *o = chain = new Chain( 4, 116, 110, 330 );
                o->labelsize( 10 );
                o->align( FL_ALIGN_TOP );
                o->color( FL_RED );
                o->configure_outputs_callback( configure_outputs, this );
                o->name( name() );
                o->initialize_with_default();
                Fl_Group::current()->resizable(o);
            }
            o->end();
        }

        o->end();
        Fl_Group::current()->resizable(o);
    }

    { Fl_Pack *o = new Fl_Pack( 2, 440, 114, 40 );
        o->spacing( 2 );
        o->type( Fl_Pack::VERTICAL );

        { Fl_Box *o = new Fl_Box( 0, 0, 100, 24 );
            o->align( (Fl_Align)(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE) );
            o->labelsize( 10 );
            o->label( "Pan" );
        }
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
        {
            Controller_Module *m = new Controller_Module( 100, 24, "Inputs" );
            m->chain( chain );
            m->pad( false );
            m->connect_to( &chain->module( 0 )->control_input[1] );
            m->size( 33, 24 );
        }
        o->end();
    }

    end();

    color( FL_BLACK );
//    controllable( true );

    {
        Module *gain_module;

        {
            Module *m = gain_module = new Gain_Module( 50, 50, "Gain" );
            m->initialize();
            chain->insert( chain->module( chain->modules() - 1 ), m );
        }

        {
            Controller_Module *m = new Controller_Module( 100, 0, "Gain" );
            m->chain( chain );
            m->pad( false );
            m->connect_to( &gain_module->control_input[0] );
            m->size( 33, 0 );

            fader_pack->add( m );
        }

        Module *meter_module;

        {
            Module *m = meter_module = new Meter_Module( 50, 50, "Meter" );
            chain->insert( chain->module( chain->modules() - 1 ), m );
        }
        {
            Meter_Indicator_Module *m = new Meter_Indicator_Module( 100, 0, "" );
            m->chain( chain );
            m->pad( false );
            m->connect_to( &meter_module->control_output[0] );
            m->size( 58, 0 );
            m->clip_children( 0 );

            fader_pack->add( m );

            fader_pack->resizable( m );
        }

        chain->configure_ports();
    }

}


int
Mixer_Strip::handle ( int m )
{
    Logger log( this );

    static Fl_Color orig_color;

    switch ( m )
    {
        case FL_ENTER:
//            orig_color = color();
//            color( FL_BLACK );
            redraw();
            return 1;
            break;
        case FL_LEAVE:
//            color( orig_color );
            redraw();
            return 1;
            break;
        default:
            return Fl_Group::handle( m );

    }

    return 0;
}
