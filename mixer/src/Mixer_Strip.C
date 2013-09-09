
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

#include "Project.H"
#include "Mixer_Strip.H"
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
#include "FL/focus_frame.H"
#include <FL/Fl_Menu_Button.H>
#include "FL/test_press.H"
#include "FL/menu_popup.H"
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Choice.H>
#include "Group.H"
#include "FL/focus_frame.H"

extern Mixer *mixer;



/* add a new mixer strip (with default configuration) */
Mixer_Strip::Mixer_Strip( const char *strip_name ) : Fl_Group( 0, 0, 120, 600 )
{
    label( strdup( strip_name ) );
    labeltype( FL_NO_LABEL );

    init();

    _group = new Group(strip_name, true);

    _group->add( this );

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

//    _chain->engine()->lock();

    log_destroy();

    mixer->remove( this );

    /* make sure this gets destroyed before the chain */
    fader_tab->clear();

    delete _chain;
    _chain = NULL;
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
    e.add( ":mute_mode", mute_controller->mode() );
    if ( ! _group->single() )
        e.add( ":group", _group );
    else
        e.add( ":group", (Loggable*)0 );
    e.add( ":auto_input", _auto_input );
    e.add( ":manual_connection", _manual_connection );
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
        else if ( ! strcmp( s, ":mute_mode" ) )
        {
            _mute_controller_mode = atoi( v );
        }
        else if ( ! strcmp( s, ":auto_input" ) )
        {
            auto_input( v );
        }
        else if ( ! strcmp( s, ":manual_connection" ) )
        {
            manual_connection( atoi( v ) );
        }
        else if ( ! strcmp( s, ":group" ) )
        {
            int i;
            sscanf( v, "%X", &i );

            if ( i )
            {
                Group *t = (Group*)Loggable::find( i );

                assert( t );
                
                group( t );
            }
            else
                group( 0 );
        }
    }

    if ( ! _group )
        group(0);

    if ( ! mixer->contains( this ) )
        mixer->add( this );
}

void
Mixer_Strip::log_children ( void ) const
{
    log_create();

    _chain->log_children();
}

void
Mixer_Strip::color ( Fl_Color c )
{
    _color = c;
    color_box->color( _color );
    color_box->redraw();
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

    /* FIXME: don't hardcode this list of modules */
    spatialization_controller->chain( c );
    gain_controller->chain( c );
    mute_controller->chain( c );
    jack_input_controller->chain( c );
    meter_indicator->chain( c );
}


void Mixer_Strip::cb_handle(Fl_Widget* o) {
    // parent()->parent()->damage( FL_DAMAGE_ALL, x(), y(), w(), h() );
    // DMESSAGE( "Callback for %s", o->label() );

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

        set_spatializer_visibility();
    }
    else if ( o == close_button )
    {
        if ( Fl::event_shift() || 1 == fl_choice( "Are you sure you want to remove this strip?\n\n(this action cannot be undone)", "Cancel", "Remove", NULL ) )
            command_close();
    }
    else if ( o == name_field )
    {
        name( name_field->value() );
        Fl::focus( this );
    }
    else if ( o == width_button )
    {
        if ( width_button->value() )
            size( 220, h() );
        else
            size( 96, h() );

        if ( parent() )
            parent()->parent()->redraw();
    }
    else if ( o == output_connection_button )
    {
        if ( output_connection_button->value() == 1 )
            o->label( output_connection_button->mvalue()->label() );
        else
            o->label( NULL );
            
        manual_connection( output_connection_button->value() );

//        _manual_connection = output_connection_button->value();
    }
    else if ( o == group_choice )
    {
        // group(group_choice->value());
        Group *g = NULL;

        if ( group_choice->value() == group_choice->size() - 2 )
        {
            /* create a new group */
            const char *s = fl_input( "Name for Group:" );
            if ( !s )
                return;
           
            char *n = mixer->get_unique_group_name( s );

            g = new Group( n, false );

            free( n );

            mixer->add_group( g );
        }
        else
        {
            g = (Group*)group_choice->mvalue()->user_data();
        }

        group(g);
    }
}

void Mixer_Strip::cb_handle(Fl_Widget* o, void* v) {
    ((Mixer_Strip*)(v))->cb_handle(o);
}

void
Mixer_Strip::group ( Group *g )
{
    if ( !g && _group && _group->single() )
        return;

    if ( _group )
    {
        _group->remove(this);
        if ( ! _group->nstrips() )
        {
            if ( ! _group->single() )
                mixer->remove_group( _group );

            delete _group;

            _group = NULL;
        }
    }

    if ( ! g )
        g = new Group(name(), true);

    const Fl_Menu_Item *menu = group_choice->menu();
        
    for ( unsigned int i = 0; menu[i].text; i++ )
        if ( menu[i].user_data() == g )
            group_choice->value( i );

//    group_choice->color( (Fl_Color)n );
//    group_choice->value( n );

    _group = g;

    g->add(this);
}

void
Mixer_Strip::name ( const char *name )
{
    if ( this->name() && !strcmp( name, this->name() ) )
        return;

    name = mixer->get_unique_track_name( name );

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

void
Mixer_Strip::set_spatializer_visibility ( void )
{
    if ( fader_tab->visible() && spatialization_controller->is_controlling() )
    {
        spatialization_controller->show();
        spatialization_label->show();
    }
    else
    {
        spatialization_controller->hide();
        spatialization_label->hide();
    }
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
            mute_controller->connect_to( &m->control_input[1] );
            mute_controller->mode( (Controller_Module::Mode)_mute_controller_mode );
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
            DMESSAGE( "Connected spatializer to module \"%s\"", m->name() );
            set_spatializer_visibility();
        }
    }
}


/* called by the chain to let us know that a module has been removed */
void
Mixer_Strip::handle_module_removed ( Module *m )
{
    if ( spatialization_controller->control_output[0].connected() &&
	 spatialization_controller->control_output[0].connected_port()->module() == m )
    {
        set_spatializer_visibility();
        DMESSAGE( "Module \"%s\" disconnected from spatialization controller", m->label() );
    }
}

/* update GUI with values from RT thread */
void
Mixer_Strip::update ( void )
{
    THREAD_ASSERT( UI );

    meter_indicator->update();
    gain_controller->update();
    mute_controller->update();

    if ( _chain )
    {
        _chain->update();
    }
    if ( group() )
    {
        if ( ( _dsp_load_index++ % 10 ) == 0 )
        {
            float l = group()->dsp_load();
   
            dsp_load_progress->value( l );

            {
                char pat[20];
                snprintf( pat, sizeof(pat), "%.1f%%", l * 100.0f );
                dsp_load_progress->copy_tooltip( pat );
            }
            
            if ( l <= 0.15f )
                dsp_load_progress->color2( fl_rgb_color( 127,127,127 ) );
            else
                dsp_load_progress->color2( FL_RED );
        }
    }
}

void
Mixer_Strip::init ( )
{
    selection_color( FL_YELLOW );
    _manual_connection = 0;
    _auto_input = 0;
    _mute_controller_mode = 0;
    _gain_controller_mode = 0;
    _chain = 0;
    _group = 0;

    _dsp_load_index = 0;

    box( FL_FLAT_BOX );
    labeltype( FL_NO_LABEL );

    Fl_Group::color( FL_BACKGROUND_COLOR );

    set_visible_focus();

    { Fl_Scalepack *o = new Fl_Scalepack( 2, 2, 116, 595 );
        o->type( FL_VERTICAL );
        o->spacing( 2 );
         
        { Fl_Box *o = color_box = new Fl_Box( 0,0, 25, 10 );
            o->box(FL_FLAT_BOX);
            o->tooltip( "Drag and drop to move strip" );
        }

        { Fl_Pack *o = new Fl_Pack( 2, 2, 114, 100 );
            o->type( Fl_Pack::VERTICAL );
            o->spacing( 2 );
            {
                Fl_Sometimes_Input *o = new Fl_Sometimes_Input( 2, 2, 144, 15 );
                name_field = o;

                o->up_box( FL_NO_BOX );
                o->box( FL_FLAT_BOX );
                o->selection_color( FL_BLACK );
                o->labeltype( FL_NO_LABEL );
                o->labelcolor( FL_GRAY0 );
                o->textcolor( FL_FOREGROUND_COLOR );
                o->textsize( 12 );
                o->value( name() );
                o->callback( cb_handle, (void*)this );
            }
            { Fl_Scalepack *o = new Fl_Scalepack( 7, 143, 110, 18 );
                o->type( Fl_Pack::HORIZONTAL );
              
                { Fl_Flip_Button* o = width_button = new Fl_Flip_Button(61, 183, 45, 22, "[]/[-]");
                    o->type(1);
                    o->tooltip( "Switch between wide and narrow views" );
                    o->labelfont( FL_COURIER_BOLD );
                    o->labelsize(10);
                    o->callback( ((Fl_Callback*)cb_handle), this );
                    o->when(FL_WHEN_RELEASE);
                }

                { Fl_Button* o = close_button = new Fl_Button(7, 143, 35, 25, "X");
                    o->tooltip( "Remove strip" );
                    o->type(0);
                    o->labelfont( FL_COURIER_BOLD );
                    o->selection_color( FL_RED );
                    o->labelsize(10);
                    o->when( FL_WHEN_RELEASE );
                    o->callback( ((Fl_Callback*)cb_handle), this );
                } // Fl_Button* o

                o->end();
            } // Fl_Group* o
            { Fl_Progress* o = dsp_load_progress = new Fl_Progress(61, 183, 45, 10, "group dsp");
                o->box(FL_FLAT_BOX);
                o->type(FL_HORIZONTAL);
                o->labelsize( 9 );
                o->minimum( 0 );
//                o->maximum( 0.25f );
                o->maximum( 1 );
                o->color2(FL_CYAN);
            }
            { Fl_Choice* o = group_choice = new Fl_Choice(61, 183, 45, 22);
                o->tooltip( "Create or assign group" );
                o->labeltype(FL_NO_LABEL);
                o->labelsize(10);
                o->textsize(10);
                o->add("---");
                o->value(0);
                o->callback( ((Fl_Callback*)cb_handle), this );
            }
            { Fl_Scalepack *o = new Fl_Scalepack( 0,0, 45, 22 );
                o->type( FL_HORIZONTAL );
                { Fl_Flip_Button* o = tab_button = new Fl_Flip_Button(61, 183, 45, 22, "Fadr/Signl");
                    o->tooltip( "Switch between fader and signal views" );
                    o->type(1);
                    o->labelsize( 10 );
                    o->callback( ((Fl_Callback*)cb_handle), this );
                    o->when(FL_WHEN_RELEASE);
                }
                { Controller_Module *o = mute_controller = new Controller_Module( true );
                    o->pad( false );
                    o->size( 45, 22 );
                }
                o->end();
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
                { Fl_Pack *o = new Fl_Pack( 2, 116, 105, 15 );
                    o->type( FL_VERTICAL );
                    {
                        Controller_Module *m = jack_input_controller = new Controller_Module( true );
                        m->labeltype( FL_NO_LABEL );
                        m->chain( _chain );
                        m->pad( false );
                        m->size( 105, 15 );
                    }
                    o->resizable(0);
                    o->end();
                }
                { Fl_Scalepack* o = new Fl_Scalepack(2, 135, 105, 311 );
                    // o->box( FL_BORDER_BOX );
//                        o->color( FL_RED );
                    o->spacing( 20 );
                    o->type( Fl_Scalepack::HORIZONTAL );
                    { Controller_Module *o = gain_controller = new Controller_Module( true );
                        o->pad( false );
                        o->size( 33, 100 );
                    }
                    { Meter_Indicator_Module *o = meter_indicator = new Meter_Indicator_Module( true );
                        o->disable_context_menu( true );
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
        { Fl_Box *o = spatialization_label = new Fl_Box( 0, 0, 100, 12 );
            o->align( (Fl_Align)(FL_ALIGN_BOTTOM | FL_ALIGN_INSIDE) );
            o->labelsize( 10 );
            o->hide();
            o->label( "Spatialization" );
        }
        { Controller_Module *o = spatialization_controller = new Controller_Module( true );
            o->hide();
            o->label( 0 );
            o->pad( false );
            o->size( 92,92 );
        }
/*             o->end(); */
/*         } */

        { Fl_Menu_Button *o = output_connection_button = new Fl_Menu_Button( 0, 0, 10, 18 );
            o->labelsize( 9 );
            o->add("- auto -");
            o->add("- manual -");
            o->align( FL_ALIGN_CLIP );
            o->labelcolor( FL_YELLOW );
            o->callback( cb_handle, this );
            o->hide();
        }

        o->end();
    }

    end();

    color( FL_BLACK );

    size( 96, h() );

    update_group_choice();
//    redraw();

    //  _chain->configure_ports();
}

void
Mixer_Strip::update_group_choice ( void )
{
    Fl_Choice *o = group_choice;

    o->clear();
    o->add( "---" );
     
    for ( std::list<Group*>::iterator i = mixer->groups.begin(); i != mixer->groups.end(); )
    {
        Group *g = *i;

        i++;

        if ( i == mixer->groups.end() )
        {
            o->add( g->name(), 0, 0, (void*)g, FL_MENU_DIVIDER );
            break;
        }
        else
            o->add( g->name(), 0, 0, (void*)g );
    }

    o->add( "New Group" );

    const Fl_Menu_Item *menu = o->menu();

    if ( ! group() || ( group() && group()->single() ) )
        o->value(0);
    else
    {
        for ( unsigned int i = 0; menu[i].text; i++ )
            if ( menu[i].user_data() == group() )
                o->value( i );
    }
}

void
Mixer_Strip::draw ( void )
{
    /* don't bother drawing anything else, all we're doing is drawing the focus. */
//    if ( damage() & ~FL_DAMAGE_USER1 )
    Fl_Group::draw();

    if ( focused_r( this ) )
        draw_focus_frame( x(),y(),w(),h(), Fl_Group::selection_color() );
    /* else */
    /*     clear_focus_frame( x(),y(),w(),h(), FL_BACKGROUND_COLOR ); */
}

/*****************/
/* Import/Export */
/*****************/

void
Mixer_Strip::snapshot ( void *v )
{
    ((Mixer_Strip*)v)->snapshot();
}

void
Mixer_Strip::snapshot ( void )
{
    log_children();
}

bool
Mixer_Strip::export_strip ( const char *filename )
{
    MESSAGE( "Exporting chain state" );
    Loggable::snapshot_callback( &Mixer_Strip::snapshot, this );
    Loggable::snapshot( filename );
    return true;
}

bool
Mixer_Strip::import_strip ( const char *filename )
{
    MESSAGE( "Importing new chain state" );
    Loggable::begin_relative_id_mode();
    int r = Loggable::replay( filename );
    Loggable::end_relative_id_mode();
    return r;
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
    else if ( ! strcmp( picked, "/Copy" ) )
    {
        export_strip( "clipboard.strip" );

        char *s;
        asprintf( &s, "file://%s/%s\r\n", Project::path(), "clipboard.strip" );

        Fl::copy( s, strlen(s), 0 );

        free(s);
    }
    else if ( ! strcmp( picked, "/Color" ) )
    {
        unsigned char r, g, b;

        Fl::get_color( color(), r, g, b );

        if ( fl_color_chooser( "Strip Color", r, g, b ) )
            color( fl_rgb_color( r, g, b ) );

        redraw();
    }
    else if ( !strcmp( picked, "/Export Strip" ) )
    {
        char *suggested_name;
        asprintf( &suggested_name, "%s.strip", name() );

        const char *s = fl_file_chooser( "Export strip to filename:", "*.strip", suggested_name, 0 );

        free( suggested_name );

        if ( s )
            export_strip( s );

        fl_message( "Strip exported." );
    }
    else if ( ! strcmp( picked, "/Remove" ) )
    {
        if ( Fl::event_shift() || 1 == fl_choice( "Are you sure you want to remove this strip?\n\n(this action cannot be undone)", "Cancel", "Remove", NULL ) )
            command_close();
    }
    else if ( ! strcmp( picked, "Auto Output/On" ) )
    {
        manual_connection( false );
    }
    else if ( ! strcmp( picked, "Auto Output/Off" ) )
    {
        manual_connection( true );
    }
    else if ( ! strncmp( picked, "Auto Input/", strlen( "Auto Input/" ) ))
    {
        const char *s = index( picked, '/' ) + 1;
        
        if ( ! strcmp( s, "Off" ) )
            auto_input( NULL );
        else
            auto_input( s );
    }
}

void
Mixer_Strip::menu_cb ( Fl_Widget *w, void *v )
{
    ((Mixer_Strip*)v)->menu_cb( (Fl_Menu_*) w );
}

void
Mixer_Strip::auto_input ( const char *s )
{
    if ( _auto_input )
        free( _auto_input );
    
    _auto_input = NULL;

    if ( s )
        _auto_input = strdup( s );

    mixer->auto_connect();
}

void
Mixer_Strip::manual_connection ( bool b )
{
    _manual_connection = b;
    output_connection_button->value(b);
   
    if ( chain() )
    {
        if ( b )
            chain()->auto_disconnect_outputs();
        else
            chain()->auto_connect_outputs();
    }
}

static bool matches_pattern ( const char *pattern, Module::Port *p )
{
    char group_name[256];
    char port_group[256];
    
    if ( 2 == sscanf( pattern, "%[^/]/%[^\n]", group_name, port_group ))
    {
        if ( strcmp( group_name, "*" ) && 
             strcmp( group_name, p->module()->chain()->strip()->group()->name() ))
        {
            /* group mismatch */
            return false;
        }

        /* group matches... try port group */
        if ( ! strcmp( port_group, "mains" ) )
        { 
            if ( index( p->jack_port()->name(), '/' ) )
                return false;
            else
                return true;
        }
        else
        {
            const char *pn = p->jack_port()->name();
            const char *n = rindex( pn, '/' );
            
            if ( n )
            {
//                *n = 0;
                if ( ! strncmp( port_group, pn, ( n - 1 ) - pn ) )
                    return true;
                else
                    return false;
            }
            else
                return false;
        }
    }

    return false;
}


#include "JACK_Module.H"

void
Mixer_Strip::auto_connect_outputs ( void )
{
    chain()->auto_connect_outputs();
}

bool
Mixer_Strip::has_group_affinity ( void ) const
{
    return _auto_input && strncmp( _auto_input, "*/", 2 );
}


bool
Mixer_Strip::maybe_auto_connect_output ( Module::Port *p )
{
    if ( p->module()->chain()->strip()->_manual_connection )
        return true;

    if ( p->module()->chain()->strip() == this )
        /* don't auto connect to self! */
        return false;

    if ( ! _auto_input )
    {
        if ( p->connected_port() && p->connected_port()->module()->chain()->strip() == this )
        {
            /* first break previous auto connection */
            p->connected_port()->jack_port()->disconnect( p->jack_port()->jack_name() );
            p->disconnect();
        }
    }
    
    if ( _auto_input && matches_pattern( _auto_input, p ) )
    {
        DMESSAGE( "Auto connecting port" );

        if ( p->connected_port() )
        {
            /* first break previous auto connection */
            p->connected_port()->jack_port()->disconnect( p->jack_port()->jack_name() );
            p->disconnect();
        }

        const char* jack_name = p->jack_port()->jack_name();
       
        /* get port number */
        const char *s = rindex( jack_name, '-' );
        unsigned int n = atoi( s + 1 ) - 1;

        /* FIXME: safe assumption? */
        JACK_Module *m = (JACK_Module*)chain()->module(0);
        
        if ( n < m->aux_audio_input.size() )
        {
            m->aux_audio_input[n].jack_port()->connect( jack_name );
            /* make a note of the connection so we know to disconnected later */
            m->aux_audio_input[n].connect_to( p );
        }

        if ( p->module()->is_default() )
        {
            /* only do this for mains */
            p->module()->chain()->strip()->output_connection_button->copy_label( name() );
            p->module()->chain()->strip()->output_connection_button->labelcolor( FL_FOREGROUND_COLOR );
        }

        return true;
    }

    return false;
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
    
    m.clear();

    std::list<std::string> sl = mixer->get_auto_connect_targets();


    m.add( "Auto Output/On", 0, 0, 0, FL_MENU_RADIO | ( _manual_connection ? 0 : FL_MENU_VALUE )  );
    m.add( "Auto Output/Off", 0, 0, 0, FL_MENU_RADIO | ( ! _manual_connection ? 0 : FL_MENU_VALUE )  );
    m.add( "Auto Input/Off", 0, 0, 0, FL_MENU_DIVIDER | FL_MENU_RADIO | ( _auto_input ? 0 : FL_MENU_VALUE )  );

    for ( std::list<std::string>::iterator i = sl.begin(); i != sl.end(); i++ )
    {
        char *s;
        asprintf( &s, "Auto Input/%s", i->c_str() );

        m.add( s, 0,0,0, FL_MENU_RADIO | ( _auto_input && !strcmp( _auto_input, i->c_str() ) ? FL_MENU_VALUE : 0 ));
        free(s );
    }

    m.add( "Width/Narrow",  'n', 0, 0, FL_MENU_RADIO | ( ! width_button->value() ? FL_MENU_VALUE : 0 ));
    m.add( "Width/Wide", 'w', 0, 0, FL_MENU_RADIO | ( width_button->value() ? FL_MENU_VALUE : 0 ) );
    m.add( "View/Fader",          'f', 0, 0, FL_MENU_RADIO | ( 0 == tab_button->value() ? FL_MENU_VALUE : 0 ) );
    m.add( "View/Signal",      's', 0, 0, FL_MENU_RADIO | ( 1 == tab_button->value() ? FL_MENU_VALUE : 0 ) );
    m.add( "Move Left",      '[', 0, 0  );
    m.add( "Move Right",     ']', 0, 0 );
    m.add( "Color",           0, 0, 0 );
    m.add( "Copy",            FL_CTRL + 'c', 0, 0 );
    m.add( "Export Strip",           0, 0, 0 );
    m.add( "Rename",          FL_CTRL + 'n', 0, 0 );
    m.add( "Remove",          FL_Delete, 0, 0 );
    
    menu_set_callback( const_cast<Fl_Menu_Item*>(m.menu()), &Mixer_Strip::menu_cb, (void*)this );

    return m;
}

Controller_Module *
Mixer_Strip::spatializer ( void )
{
    return spatialization_controller;
}

void
Mixer_Strip::get_output_ports ( std::list<std::string> &ports )
{
    _chain->get_output_ports(ports);
}

int
Mixer_Strip::handle ( int m )
{
    static int _button = 0;

    Logger log( this );
    
    static Fl_Widget *dragging = NULL;

    if ( Fl_Group::handle( m ) )
        return 1;

    switch ( m )
    {
        case FL_FOCUS:
            damage( FL_DAMAGE_USER1 );
            return 1;
        case FL_UNFOCUS:
            damage( FL_DAMAGE_USER1 );
            return 1;
    }

    /* if ( m == FL_PUSH ) */
    /*     take_focus(); */

    switch ( m )
    {
        case FL_KEYBOARD:
        {
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
            if ( Fl::event_button1() && Fl::event_inside( color_box ) )
                dragging = this;
            else
                dragging = NULL;

            _button = Fl::event_button();
          
            return 1;
       
            break;
        
        case FL_DRAG:
            return 1;
            break;
        case FL_RELEASE:
            if ( dragging == this && ! Fl::event_is_click() )
            {
                mixer->insert( this, mixer->event_inside() );
                /* FIXME: do better! */
                mixer->redraw();
                dragging = NULL;
                return 1;
            }
            
            dragging = NULL;
   
            int b = _button;
            _button = 0;
 
            /* if ( 1 == b ) */
            /* { */
            /*     take_focus(); */
            /* } */
            /* else */
            if ( 3 == b )
            {
                menu_popup( &menu() );
                return 1;
            }
            break;
    }
    
    return 0;
}

void
Mixer_Strip::send_feedback ( void )
{
    if ( _chain )
        _chain->send_feedback();
}

int
Mixer_Strip::number ( void ) const
{
    return mixer->find_strip( this );
}

/************/
/* Commands */
/************/

void
Mixer_Strip::command_toggle_fader_view ( void )
{
    tab_button->value( ! tab_button->value() );
    tab_button->do_callback();
}

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
