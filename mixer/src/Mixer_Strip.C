
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
#include <FL/Fl_Color_Chooser.H>
#include <FL/Fl.H>
#include "Mixer.H"

#include "Chain.H"
#include "Gain_Module.H"
#include "Meter_Module.H"
#include "Controller_Module.H"
#include "Meter_Indicator_Module.H"
#include "debug.h"

#include <FL/Fl_Menu_Button.H>
#include "FL/test_press.H"
#include "FL/menu_popup.H"

extern Mixer *mixer;



/* add a new mixer strip (with default configuration) */
Mixer_Strip::Mixer_Strip( const char *strip_name ) : Fl_Group( 0, 0, 120, 600 )
{
    label( strdup( strip_name ) );
    labeltype( FL_NO_LABEL );

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
    DMESSAGE( "Destroying mixer strip" );


    /* make sure this gets destroyed before the chain */
    fader_tab->clear();

    delete _chain;
    _chain = NULL;

    log_destroy();

    mixer->remove( this );
}



void
Mixer_Strip::get ( Log_Entry &e ) const
{
    e.add( ":name",            name()           );
    e.add( ":width",      width_button->value() ? "wide" : "narrow" );
    e.add( ":tab",      tab_button->value() ? "signal" : "fader" );
    e.add( ":color",           (unsigned long)color());
    /* since the default controllers aren't logged, we have to store
     * this setting as part of the mixer strip */
    e.add( ":gain_mode", gain_controller->mode() );

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
            width_button->value( strcmp( v, "wide" ) == 0 );
            width_button->do_callback();
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
        else if ( ! strcmp( s, ":gain_mode" ) )
        {
            _gain_controller_mode = atoi( v );
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


void Mixer_Strip::cb_handle(Fl_Widget* o) {
    // parent()->parent()->damage( FL_DAMAGE_ALL, x(), y(), w(), h() );
    DMESSAGE( "Callback for %s", o->label() );

    if ( o == tab_button )
    {
        if ( tab_button->value() == 0 )
        {
            fader_tab->resize( tab_group->x(), tab_group->y(), tab_group->w(), tab_group->h() );
            fader_tab->show();
            signal_tab->hide();
            tab_group->resizable( fader_tab );
        }
        else
        {
            signal_tab->resize( tab_group->x(), tab_group->y(), tab_group->w(), tab_group->h() );
            signal_tab->show();
            fader_tab->hide();
            tab_group->resizable( signal_tab );
        }

    }
    else if ( o == left_button )
        command_move_left();
    else if ( o == right_button )
        command_move_right();
    else if ( o == close_button )
    {
        if ( Fl::event_shift() || 1 == fl_choice( "Are you sure you want to remove this strip?\n\n(this action cannot be undone)", "Cancel", "Remove", NULL ) )
            command_close();
    }
    else if ( o == name_field )
        name( name_field->value() );
    else if ( o == width_button )
    {
        if ( width_button->value() )
            size( 220, h() );
        else
            size( 96, h() );

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

    if ( strlen( s ) > Chain::maximum_name_length() )
    {
        s[Chain::maximum_name_length() - 1] = '\0';

        fl_alert( "Name \"%s\" is too long, truncating to \"%s\"", name, s );
    }

    name_field->value( s );
    label( s );
    if ( _chain )
        _chain->name( s );
}

void
Mixer_Strip::configure_outputs ( Fl_Widget *, void *v )
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
            gain_controller->mode( (Controller_Module::Mode)_gain_controller_mode );
        }
        else if ( 0 == strcmp( m->name(), "Meter" ) )
        {
            meter_indicator->connect_to( &m->control_output[0] );
        }
    }
    else
    {
        if ( spatialization_controller->connect_spatializer_to( m ) )
        {
            spatialization_controller->show();
            DMESSAGE( "Connected spatializer to module \"%s\"", m->name() );
        }
    }
}


/* called by the chain to let us know that a module has been removed */
void
Mixer_Strip::handle_module_removed ( Module *m )
{
    if ( spatialization_controller->control_output[0].connected_port()->module() == m )
    {
        spatialization_controller->hide();
        DMESSAGE( "Module \"%s\" disconnected from spatialization controller", m->name() );
    }
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
    selection_color( FL_RED );

    _gain_controller_mode = 0;
    _chain = 0;

//    box(FL_THIN_UP_BOX);
    box( FL_RFLAT_BOX );
    labeltype( FL_NO_LABEL );

    Fl_Group::color( FL_BACKGROUND_COLOR );

    set_visible_focus();

     { Fl_Scalepack *o = new Fl_Scalepack( 2, 2, 116, 595 );
         o->type( FL_VERTICAL );
         o->spacing( 2 );

        { Fl_Pack *o = new Fl_Pack( 2, 2, 114, 100 );
            o->type( Fl_Pack::VERTICAL );
            o->spacing( 2 );
            {
                Fl_Sometimes_Input *o = new Fl_Sometimes_Input( 2, 2, 144, 24 );
                name_field = o;

                o->color( color() );
                o->up_box( FL_ROUNDED_BOX );
                o->box( FL_ROUNDED_BOX );
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
            { Fl_Flip_Button* o = tab_button = new Fl_Flip_Button(61, 183, 45, 22, "fader/signal");
                o->type(1);
                o->labelsize( 14 );
                o->callback( ((Fl_Callback*)cb_handle), this );
                o->when(FL_WHEN_RELEASE);
            }
            { Fl_Flip_Button* o = width_button = new Fl_Flip_Button(61, 183, 45, 22, "narrow/wide");
                o->type(1);
                o->labelsize( 14 );
                o->callback( ((Fl_Callback*)cb_handle), this );
                o->when(FL_WHEN_RELEASE);
            }
            o->end();
        }

/*         { Fl_Scalepack *o = new Fl_Scalepack( 2, 103, 114, 490 ); */
/*             o->type( FL_VERTICAL ); */
//        o->box( FL_FLAT_BOX );
//        o->color( FL_BACKGROUND_COLOR );
        { Fl_Group *o = tab_group = new Fl_Group( 2, 116, 105, 330 );
            o->box( FL_NO_BOX );
            { Fl_Group *o = fader_tab = new Fl_Group( 2, 116, 105, 330, "Fader" );
                o->box( FL_NO_BOX );
                o->labeltype( FL_NO_LABEL );
                { Fl_Scalepack* o = new Fl_Scalepack(2, 116, 105, 330 );
                    // o->box( FL_BORDER_BOX );
//                        o->color( FL_RED );
                    o->spacing( 20 );
                    o->type( Fl_Scalepack::HORIZONTAL );
                    { Controller_Module *o = gain_controller = new Controller_Module( true );
                        o->pad( false );
                        o->size( 33, 100 );
                    }
                    { Meter_Indicator_Module *o = meter_indicator = new Meter_Indicator_Module( true );
                        o->pad( false );
                        o->size( 38, 100 );
                        Fl_Group::current()->resizable(o);
                    }
                    o->end();
                    Fl_Group::current()->resizable(o);
                } // Fl_Group* o
                o->end();
                Fl_Group::current()->resizable(o);
            }
            { Fl_Group *o = signal_tab = new Fl_Group( 2, 116, 105, 330 );
                o->box( FL_NO_BOX );
                o->labeltype( FL_NO_LABEL );
                o->hide();
                o->end();
            }
            o->end();
            Fl_Group::current()->resizable( o );
        }
/*         { Fl_Pack *o = panner_pack = new Fl_Pack( 2, 465, 114, 40 ); */
/*             o->spacing( 2 ); */
/*             o->type( Fl_Pack::VERTICAL ); */
            { Fl_Box *o = new Fl_Box( 0, 0, 100, 12 );
                o->align( (Fl_Align)(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE) );
                o->labelsize( 10 );
//                o->label( "Spatialization" );
            }
            { Controller_Module *o = spatialization_controller = new Controller_Module( true );
                o->hide();
                o->pad( false );
                o->size( 100, 100 );
            }
            { Fl_Box *o = new Fl_Box( 0, 0, 100, 12 );
                o->align( (Fl_Align)(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE) );
                o->labelsize( 10 );
                o->label( "Inputs" );
            }
            {
                Controller_Module *m = jack_input_controller = new Controller_Module( true );
                m->labeltype( FL_NO_LABEL );
                m->chain( _chain );
                m->pad( false );
                m->size( 33, 24 );
             }
/*             o->end(); */
/*         } */
        o->end();
    }

    end();

    color( FL_BLACK );

    size( 96, h() );

    redraw();

    //  _chain->configure_ports();
}

void
Mixer_Strip::draw ( void )
{
    if ( !fl_not_clipped( x(), y(), w(), h() ) )
        return;

     /* don't bother drawing anything else, all we're doing is drawing the focus. */
    if ( damage() & FL_DAMAGE_ALL ||
         damage() & FL_DAMAGE_CHILD )
        Fl_Group::draw();

    Fl_Group::draw_box( FL_UP_FRAME, x(), y(), w(), h(), Fl::focus() == this ? Fl_Group::selection_color() : FL_BLACK );
}



void
Mixer_Strip::menu_cb ( const Fl_Menu_ *m )
{
    char picked[256];

    m->item_pathname( picked, sizeof( picked ) );

    Logger log( this );

    if ( ! strcmp( picked, "Width/Narrow" ) )
        command_width( false );
    else if ( ! strcmp( picked, "Width/Wide" ) )
        command_width( true );
    else if ( ! strcmp( picked, "View/Fader" ) )
        command_view( false );
    else if ( ! strcmp( picked, "View/Signal" ) )
        command_view( true );
    else if ( ! strcmp( picked, "/Move Left" ) )
        command_move_left();
    else if ( ! strcmp( picked, "/Move Right" ) )
        command_move_right();
    else if ( ! strcmp( picked, "/Rename" ) )
    {
        ((Fl_Sometimes_Input*)name_field)->take_focus();
    }
    else if ( ! strcmp( picked, "/Color" ) )
    {
        unsigned char r, g, b;

        Fl::get_color( color(), r, g, b );

        if ( fl_color_chooser( "Strip Color", r, g, b ) )
            color( fl_rgb_color( r, g, b ) );

        redraw();
    }
    else if ( ! strcmp( picked, "/Remove" ) )
    {
        if ( Fl::event_shift() || 1 == fl_choice( "Are you sure you want to remove this strip?\n\n(this action cannot be undone)", "Cancel", "Remove", NULL ) )
            command_close();
    }
}

void
Mixer_Strip::menu_cb ( Fl_Widget *w, void *v )
{
    ((Mixer_Strip*)v)->menu_cb( (Fl_Menu_*) w );
}


/** build the context menu */
Fl_Menu_Button &
Mixer_Strip::menu ( void ) const
{
    static Fl_Menu_Button m( 0, 0, 0, 0, "Strip" );
    static char label[256];

    snprintf( label, sizeof(label), "Strip/%s", name() );
    m.label( label );

//    int c = output.size();

    Fl_Menu_Item menu[] =
        {
            { "Width",            0, 0, 0, FL_SUBMENU    },
            { "Narrow",         'n', 0, 0, FL_MENU_RADIO | ( ! width_button->value() ? FL_MENU_VALUE : 0 ) },
            { "Wide",           'w', 0, 0, FL_MENU_RADIO | ( width_button->value() ? FL_MENU_VALUE : 0 ) },
            { 0                  },
            { "View",            0, 0, 0, FL_SUBMENU    },
            { "Fader",          'f', 0, 0, FL_MENU_RADIO | ( 0 == tab_button->value() ? FL_MENU_VALUE : 0 ) },
            { "Signal",         's', 0, 0, FL_MENU_RADIO | ( 1 == tab_button->value() ? FL_MENU_VALUE : 0 ) },
            { 0                  },
            { "Move Left",      '[', 0, 0  },
            { "Move Right",     ']', 0, 0 },
            { "Color",           0, 0, 0 },
            { "Rename",          FL_CTRL + 'n', 0, 0 },
            { "Remove",          FL_Delete, 0, 0 },
            { 0 },
        };

    menu_set_callback( menu, &Mixer_Strip::menu_cb, (void*)this );

    m.copy( menu, (void*)this );

    return m;
}

int
Mixer_Strip::handle ( int m )
{
    Logger log( this );

    switch ( m )
    {
        case FL_KEYBOARD:
        {
            if ( Fl_Group::handle( m ) )
                return 1;

            if ( Fl::event_key() == FL_Menu )
            {
                menu_popup( &menu(), x(), y() );
                return 1;
            }
             else
                return menu().test_shortcut() != 0;
            break;
        }
        case FL_PUSH:
        {
            int r = 0;
            if ( Fl::event_button1() )
            {
                take_focus();
                r = 1;
            }

            if ( Fl_Group::handle( m ) )
                return 1;
            else if ( test_press( FL_BUTTON3 ) )
            {
                menu_popup( &menu() );
                return 1;
            }
            else
                return r;
            break;
        }
        case FL_FOCUS:
            damage( FL_DAMAGE_USER1 );
            return Fl_Group::handle( m ) || 1;
        case FL_UNFOCUS:
            damage( FL_DAMAGE_USER1 );
            return Fl_Group::handle( m ) || 1;
    }

    return Fl_Group::handle( m );
}


/************/
/* Commands */
/************/

void
Mixer_Strip::command_move_left ( void )
{
    mixer->move_left( this );
}

void
Mixer_Strip::command_move_right ( void )
{
    mixer->move_right( this );
}

void
Mixer_Strip::command_close ( void )
{
        mixer->remove( this );
        Fl::delete_widget( this );
}

void
Mixer_Strip::command_rename ( const char * s )
{
    name( s );
}

void
Mixer_Strip::command_width ( bool b )
{
    width_button->value( b );
    width_button->do_callback();
}

void
Mixer_Strip::command_view ( bool b )
{
    tab_button->value( b );
    tab_button->do_callback();
}
