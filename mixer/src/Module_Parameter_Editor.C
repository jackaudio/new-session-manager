
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

#include <FL/Fl.H>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <FL/fl_draw.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_Counter.H>
#include "FL/Fl_Flowpack.H"
#include "FL/Fl_Labelpad_Group.H"
#include "FL/Fl_Value_SliderX.H"
#include "FL/Fl_DialX.H"
#include <FL/Fl_Scroll.H>
#include "Module.H"
#include "Module_Parameter_Editor.H"
#include "Controller_Module.H"
#include "Chain.H"
#include "Panner.H"

#include "debug.h"





Module_Parameter_Editor::Module_Parameter_Editor ( Module *module ) : Fl_Double_Window( 900,900 )
{
    _module = module;
    _resized = false;
    _min_width = 100;

    char lab[256];
    if ( strcmp( module->name(), module->label() ) )
    {
        snprintf( lab, sizeof( lab ), "%s : %s", module->name(), module->label() );
    }
    else
        strcpy( lab, module->label() );

    char title[512];
    snprintf( title, sizeof( title ), "%s - %s - %s", "Mixer", module->chain()->name(), lab );

    copy_label( title );

//    fl_font( FL_HELVETICA, 14 );

    _min_width = 30 + fl_width( module->label() );
    
    { Fl_Group *o = new Fl_Group( 0, 0, w(), 25 );
        o->label( module->label() );
        o->labelfont( 2 );
        o->labeltype( FL_SHADOW_LABEL );
        o->labelsize( 14 );
        o->align( FL_ALIGN_TOP | FL_ALIGN_RIGHT | FL_ALIGN_INSIDE );

        { Fl_Menu_Button *o = mode_choice = new Fl_Menu_Button( 0, 0, 25, 25 );
            o->add( "Knobs" );
            o->add( "Horizontal Sliders" );
            o->add( "Vertical Sliders" );
            o->label( NULL );
            o->value( 0 );
            o->when( FL_WHEN_CHANGED );
            o->callback( cb_mode_handle, this );
        }
        o->resizable(0);
        o->end();
    }

    { Fl_Scroll *o = control_scroll = new Fl_Scroll( 0, 40, w(), h() - 40 );
        { Fl_Group *o = new Fl_Group( 0, 40, w(), h() - 40 );
            { Fl_Flowpack *o = control_pack = new Fl_Flowpack( 50, 40, w() - 100, h() - 40 );
                o->type( FL_HORIZONTAL );
                o->flow( true );
                o->vspacing( 5 );
                o->hspacing( 5 );
                o->end();
            }
            o->resizable( 0 );
            o->end();
        }
        o->end();
    }
    resizable(control_scroll);

    end();

    make_controls();
}

Module_Parameter_Editor::~Module_Parameter_Editor ( )
{
}



void
Module_Parameter_Editor::make_controls ( void )
{
    Module *module = _module;

    control_pack->clear();

    controls_by_port.clear();

    /* these are for detecting related parameter groups which can be
       better represented by a single control */
    azimuth_port_number = -1;
    float azimuth_value = 0.0f;
    elevation_port_number = -1;
    float elevation_value = 0.0f;
    radius_port_number = -1;
    float radius_value = 0.0f;
    
    Fl_Color fc = fl_color_add_alpha( FL_CYAN, 200 );
    Fl_Color bc = FL_BACKGROUND2_COLOR;

    controls_by_port.resize( module->control_input.size() );
    
    if ( mode_choice->value() == 1 )
    {
        control_pack->flow(false);
        control_pack->type( FL_VERTICAL );
        control_pack->size( 450, 24 );
    }
    else if ( mode_choice->value() == 2 )
    {
        control_pack->flow(false);
        control_pack->type( FL_HORIZONTAL );
        control_pack->size( 24, 350 );
    }
    else if ( mode_choice->value() == 0 )
    {
        control_pack->flow(true);
        control_pack->type( FL_HORIZONTAL );
        control_pack->size( 700, 50 );
        
    }
        
    for ( unsigned int i = 0; i < module->control_input.size(); ++i )
    {
        Fl_Widget *w;

        Module::Port *p = &module->control_input[i];

        /* if ( !p->hints.visible ) */
        /*     continue; */

        if ( !strcasecmp( "Azimuth", p->name() ) &&
             180.0f == p->hints.maximum &&
             -180.0f == p->hints.minimum )
        {
            azimuth_port_number = i;
            azimuth_value = p->control_value();
            continue;
        }
        else if ( !strcasecmp( "Elevation", p->name() ) &&
                  90.0f == p->hints.maximum &&
                  -90.0f == p->hints.minimum )
        {
            elevation_port_number = i;
            elevation_value = p->control_value();
            continue;
        } 
        else if ( !strcasecmp( "Radius", p->name() ) )
        {
            radius_port_number = i;
            radius_value = p->control_value();
            continue;
        }

        if ( p->hints.type == Module::Port::Hints::BOOLEAN )
        {
            Fl_Button *o = new Fl_Button( 0, 0, 30, 30, p->name() );
            w = o;
            o->selection_color( fc );
            o->type( FL_TOGGLE_BUTTON );
            o->value( p->control_value() );
            o->align(FL_ALIGN_TOP);
        }
        else if ( p->hints.type == Module::Port::Hints::INTEGER )
        {

            Fl_Counter *o = new Fl_Counter(0, 0, 58, 24, p->name() );
            w = o;
            
            o->type(1);
            o->step(1);
            o->align(FL_ALIGN_TOP);

            if ( p->hints.ranged )
            {
                o->minimum( p->hints.minimum );
                o->maximum( p->hints.maximum );
            }

            o->value( p->control_value() );

        }
        else
        {
            if ( mode_choice->value() == 0 )
            {
                Fl_DialX *o = new Fl_DialX( 0, 0, 60, 60, p->name() );
                w = o;

                if ( p->hints.ranged )
                {
                    DMESSAGE( "Min: %f, max: %f", p->hints.minimum, p->hints.maximum );

                    o->minimum( p->hints.minimum );
                    o->maximum( p->hints.maximum );
                }
                o->color( bc );
                o->selection_color( fc );
                o->value( p->control_value() );
                o->align(FL_ALIGN_TOP);

                o->precision( 2 );
                /* a couple of plugins have ridiculously small units */
                if ( p->hints.maximum < 0.5f )
                    o->precision( 5 );

//                o->step( fabs( ( o->maximum() - o->minimum() ) ) / 32.0f );
            }
            else
            {
                Fl_Value_SliderX *o = new Fl_Value_SliderX( 0, 0, 120, 24, p->name() );
                w = o;

                if ( mode_choice->value() == 1 )
                {
                    o->type( FL_HORIZONTAL );

                    o->align( FL_ALIGN_RIGHT );
                    o->size( 300, 24 );
                    if ( p->hints.ranged )
                    {
                        o->minimum( p->hints.minimum );
                        o->maximum( p->hints.maximum );
                    }
                }
                else
                {
                    o->type( FL_VERTICAL );
                    o->align(FL_ALIGN_TOP);

                    o->size( 24, 300 );
                    /* have to reverse the meaning of these to get the
                     * orientation of the slider right */
                    o->maximum( p->hints.minimum );
                    o->minimum( p->hints.maximum );
                }
                
                o->precision( 2 );
                /* a couple of plugins have ridiculously small units */
                if ( p->hints.maximum < 0.5f )
                    o->precision( 5 );

                o->textsize( 8 );
                o->box( FL_NO_BOX );
                o->slider( FL_UP_BOX );
                o->color( bc );
                o->selection_color( fc );
                o->value( p->control_value() );
            }

        }
//        w->align(FL_ALIGN_TOP);
        w->labelsize( 10 );

        controls_by_port[i] = w;

        w->tooltip( p->osc_path() );

        Fl_Button *bound;


        _callback_data.push_back( callback_data( this, i ) );

        if ( p->hints.type == Module::Port::Hints::BOOLEAN )
            w->callback( cb_button_handle, &_callback_data.back() );
        else
            w->callback( cb_value_handle, &_callback_data.back() );

        { Fl_Group *o = new Fl_Group( 0, 0, 50, 75 );
            {
                Fl_Labelpad_Group *flg = new Fl_Labelpad_Group( w );

                { Fl_Button *o = bound = new Fl_Button( 0, 50, 14, 14 );
                    o->selection_color( FL_YELLOW );
                    o->type( 0 );
                    o->labelsize( 8 );

                    o->value( p->connected() );

                    o->callback( cb_bound_handle, &_callback_data.back() );
                }

//                flg->resizable(w);
                o->resizable( 0 );
                o->end();

                o->set_visible_focus();
                flg->set_visible_focus();

                flg->position( o->x(), o->y() );

                if ( mode_choice->value() == 1 )
                {
                    bound->position( flg->x() + 450 - bound->w(), o->y() );
                    o->size( flg->w() + bound->w(), flg->h() );
                }
                else
                {
                    bound->position( o->x(), flg->y() + flg->h() );
                    o->size( flg->w(), flg->h() + bound->h() );
                }
                o->init_sizes();
                //  o->resizable(flg);
            }

            if (! p->hints.visible )
                o->hide();

            control_pack->add( o );
        }

    }

    if ( azimuth_port_number >= 0 && elevation_port_number >= 0 )
    {
        Panner *o = new Panner( 0,0, 502,502 );
        o->box(FL_FLAT_BOX);
        o->color(FL_GRAY0);
        o->selection_color(FL_BACKGROUND_COLOR);
        o->labeltype(FL_NORMAL_LABEL);
        o->labelfont(0);
        o->labelcolor(FL_FOREGROUND_COLOR);
        o->align(FL_ALIGN_TOP);
        o->when(FL_WHEN_CHANGED);
        o->label( "Spatialization" );
        o->labelsize( 10 );

        _callback_data.push_back( callback_data( this, azimuth_port_number, elevation_port_number, radius_port_number ) );
        o->callback( cb_panner_value_handle, &_callback_data.back() );

        o->point( 0 )->azimuth( azimuth_value );
        o->point( 0 )->elevation( elevation_value );
        if ( radius_port_number >= 0 )
        {
            o->point( 0 )->radius_enabled = true;
            o->point( 0 )->radius( radius_value );
        }

        Fl_Labelpad_Group *flg = new Fl_Labelpad_Group( o );

        control_pack->add( flg );

        controls_by_port[azimuth_port_number] = o;
        controls_by_port[elevation_port_number] = o;
        if ( radius_port_number >= 0 )
            controls_by_port[radius_port_number] = o;
    }

    update_control_visibility();

    control_pack->dolayout();

    int width = control_pack->w() + 100;
    int height = control_pack->h() + 60;

    if ( width < _min_width )
        width = _min_width;

    control_pack->parent()->size( control_pack->w() + 100, control_pack->h() );
    
    control_scroll->scroll_to(0, 0 );

    size( width, height );
    size_range( width, height, width, height );
}

void 
Module_Parameter_Editor::update_control_visibility ( void )
{
    for ( unsigned int i = 0; i < _module->control_input.size(); ++i )
    {
        const Module::Port *p = &_module->control_input[i];

        if ( p->hints.visible )
            controls_by_port[i]->parent()->parent()->show();
        else
            controls_by_port[i]->parent()->parent()->hide();
    }
}

void
Module_Parameter_Editor::cb_value_handle ( Fl_Widget *w, void *v )
{
    callback_data *cd = (callback_data*)v;

    cd->base_widget->set_value( cd->port_number[0], ((Fl_Valuator*)w)->value() );
}

void
Module_Parameter_Editor::cb_button_handle ( Fl_Widget *w, void *v )
{
    callback_data *cd = (callback_data*)v;

    cd->base_widget->set_value( cd->port_number[0], ((Fl_Button*)w)->value() );
}


void
Module_Parameter_Editor::cb_panner_value_handle ( Fl_Widget *w, void *v )
{
    callback_data *cd = (callback_data*)v;

    cd->base_widget->set_value( cd->port_number[0], ((Panner*)w)->point( 0 )->azimuth() );
    cd->base_widget->set_value( cd->port_number[1], ((Panner*)w)->point( 0 )->elevation() );
    cd->base_widget->set_value( cd->port_number[2], ((Panner*)w)->point( 0 )->radius() );

}

void
Module_Parameter_Editor::cb_mode_handle ( Fl_Widget *, void *v )
{
    ((Module_Parameter_Editor*)v)->make_controls();
}

void
Module_Parameter_Editor::cb_bound_handle ( Fl_Widget *w, void *v )
{
    callback_data *cd = (callback_data*)v;

    Fl_Button *fv = (Fl_Button*)w;

   fv->value( 1 );

   cd->base_widget->bind_control( cd->port_number[0] );
}

void
Module_Parameter_Editor::bind_control ( int i )
{
    Module::Port *p = &_module->control_input[i];

    if ( p->connected() )
        /* can only bind once */
        return;

    Controller_Module *o = new Controller_Module();
    o->label( p->name() );
    o->chain( _module->chain() );

    o->connect_to( p );

    _module->chain()->add_control( o );
    _module->redraw();
}

/* Display changes initiated via automation or from other parts of the GUI */
void
Module_Parameter_Editor::handle_control_changed ( Module::Port *p )
{
    int i = _module->control_input_port_index( p );
   
    Fl_Widget *w = controls_by_port[i];

    if ( i == azimuth_port_number ||
         i == elevation_port_number ||
        i == radius_port_number )
    {
        Panner *_panner = (Panner*)w;

        if ( i == azimuth_port_number )
            _panner->point(0)->azimuth( p->control_value() );
        else if ( i == elevation_port_number )
            _panner->point(0)->elevation( p->control_value() );
        else if ( i == radius_port_number )
            _panner->point(0)->radius( p->control_value() );

        _panner->redraw();

        return;
    }


    if ( p->hints.type == Module::Port::Hints::BOOLEAN )
    {
        Fl_Button *v = (Fl_Button*)w;

        v->value( p->control_value() );
    }        
    else
    {
        Fl_Valuator *v = (Fl_Valuator*)w;
    
        v->value( p->control_value() );
    }
}

void
Module_Parameter_Editor::reload ( void )
{
//    make_controls();
    update_control_visibility();
    redraw();
}

void
Module_Parameter_Editor::set_value (int i, float value )
{
    if ( i >= 0 )
    {
        _module->control_input[i].control_value( value );
        if ( _module->control_input[i].connected() )
            _module->control_input[i].connected_port()->module()->handle_control_changed( _module->control_input[i].connected_port() );
    }
//    _module->handle_control_changed( &_module->control_input[i] );
}
