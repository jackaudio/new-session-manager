
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

#include <string.h>

#include <FL/fl_ask.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Pack.H>
#include <FL/Fl_Scalepack.H>

#include "dsp.h"

#include "Engine/Engine.H"
#include "Chain.H"

#include "JACK_Module.H"
#include <FL/fl_draw.H>

#include <FL/Fl.H>
#include <FL/Fl_Browser.H>

#include <FL/Fl_PNG_Image.H>
#include <FL/img_io_input_connector_10x10_png.h>
#include <FL/img_io_output_connector_10x10_png.h>

static Fl_PNG_Image *input_connector_image = NULL;
static Fl_PNG_Image *output_connector_image = NULL;

extern char *instance_name;


static JACK_Module *receptive_to_drop = NULL;

JACK_Module::JACK_Module ( bool log )
    : Module ( 25, 25, name() )
{
    _prefix = 0;

    align( FL_ALIGN_TOP | FL_ALIGN_INSIDE );

    if ( log )
    {
        /* FIXME: how do Controls find out that a connected value has changed? How does this work in ladspa? */
        {
            Port p( this, Port::INPUT, Port::CONTROL, "Inputs" );
            p.hints.type = Port::Hints::INTEGER;
            p.hints.minimum = 0;
            p.hints.maximum = 16;
            p.hints.ranged = true;
            p.hints.visible = false;

            p.connect_to( new float );
            p.control_value_no_callback( 0 );

            add_port( p );
        }

        {
            Port p( this, Port::INPUT, Port::CONTROL, "Outputs" );
            p.hints.type = Port::Hints::INTEGER;
            p.hints.minimum = 0;
            p.hints.maximum = 16;
            p.hints.ranged = true;
            p.hints.visible = false;

            p.connect_to( new float );
            p.control_value_no_callback( 0 );

            add_port( p );
        }
        color( FL_BLACK );

        log_create();
    }


    { Fl_Scalepack *o = new Fl_Scalepack( x() +  Fl::box_dx(box()),
                                          y() + Fl::box_dy(box()),
                                          w(),
                                          24 - Fl::box_dh(box()) );
        o->type( Fl_Pack::HORIZONTAL );
    
        o->spacing( 0 );


        { Fl_Box *o = input_connection_handle = new Fl_Box( x(), y(), 18, 18 );
        o->tooltip( "Drag and drop to make and break JACK connections.");
        o->hide();
        o->image( input_connector_image ? input_connector_image : input_connector_image = new Fl_PNG_Image( "input_connector", img_io_input_connector_10x10_png, img_io_input_connector_10x10_png_len ) );
     
        }        

        { Fl_Box *o = new Fl_Box( x() + 10, y(), w() - 20, h() );
            Fl_Group::current()->resizable(o);
        }


        { Fl_Button *o = dec_button = new Fl_Button( 0, 0, 12, h(), "-" );
            o->callback( cb_button, this );
            o->labelsize(10);
            o->labelfont( FL_HELVETICA_BOLD );
            o->hide();
        }        
        { Fl_Button *o = inc_button = new Fl_Button( 0,0, 12, h(), "+" );
            o->labelsize(10);
            o->labelfont( FL_HELVETICA_BOLD );
            o->callback( cb_button, this );
            o->hide();
        }

        { Fl_Box *o = output_connection_handle = new Fl_Box( x(), y(), 18, 18 );
            o->tooltip( "Drag and drop to make and break JACK connections.");
            o->image( output_connector_image ? output_connector_image : output_connector_image = new Fl_PNG_Image( "output_connector", img_io_output_connector_10x10_png, img_io_output_connector_10x10_png_len ) );
            o->hide();
        }

        o->end();
        resizable(o);
    }

    {
        Fl_Browser *o = connection_display = new Fl_Browser( x() + Fl::box_dx(box()), y() + 25, w() - Fl::box_dw(box()), 300 );
        o->textsize( 11 );
        o->textcolor( FL_LIGHT3 );
        o->textfont( FL_COURIER );
        o->box( FL_FLAT_BOX );
        o->color( fl_color_add_alpha( fl_rgb_color( 10, 10, 10 ), 25 ));
    }

    end();    
}

JACK_Module::~JACK_Module ( )
{
    log_destroy();
    configure_inputs( 0 );
    configure_outputs( 0 );
    if ( _prefix )
        free( _prefix );
}



void
JACK_Module::draw ( void )
{
    Module::draw();
    if ( this == receptive_to_drop )
    {
        Fl_Widget *o = input_connection_handle;
        fl_draw_box( FL_OVAL_BOX, o->x(), o->y(), o->w(), o->h(), fl_color_add_alpha( FL_GREEN, 127 ) );
    }
}

static std::list<std::string>
get_connections_for_ports ( std::vector<JACK::Port> ports )
{
    std::list<std::string> names;

    for ( unsigned int i = 0; i < ports.size(); ++i )
    {
        const char **connections = ports[i].connections();

        if ( ! connections )
            return names;

        bool is_output = ports[i].type() == JACK::Port::Output;

        for ( const char **c = connections; *c; c++ )
        {
            char *client_id = 0;
            char *strip_name = 0;
            //      char *client_name = 0;
           
            if ( 2 == sscanf( *c, "Non-Mixer.%a[^:/]/%a[^:]:", &client_id, &strip_name ) )
            {
                free( client_id );
                char *s = NULL;
                asprintf( &s, "%s%s", is_output ? "@r" : "", strip_name );
                free( strip_name );
                strip_name = s;
            }
            else
            if ( 2 == sscanf( *c, "Non-Timeline.%a[^:/]:%a[^/]/", &client_id, &strip_name ) )
            {
                free( client_id );
                char *s = NULL;
                asprintf( &s, "@C2%s%s", is_output ? "@r" : "", strip_name );
                free( strip_name );
                strip_name = s;
            }
            else
            if ( 2 == sscanf( *c, "Non-DAW.%a[^:/]:%a[^/]/", &client_id, &strip_name ) )
            {
                free( client_id );
                char *s = NULL;
                asprintf( &s, "@C2%s%s", is_output ? "@r" : "", strip_name );
                free( strip_name );
                strip_name = s;
            }
            else if ( 1 == sscanf( *c, "%a[^:]:", &strip_name ) )
            {   
                char *s = NULL;
                asprintf( &s, "@C3%s%s",  is_output ? "@r" : "", strip_name );
                free( strip_name );
                strip_name = s;
            }
            else
            {
                continue;
            }
            
            for ( std::list<std::string>::const_iterator j = names.begin();
                  j != names.end();
                  j++ )
            {
                if ( !strcmp( j->c_str(), strip_name ) )
                {
                    goto skip;
                }       
            }
            
            names.push_back( strip_name );

        skip:
            free( strip_name );
                
            ;
            
        }
    }

    names.sort();
    return names;
}

void
JACK_Module::update_connection_status ( void )
{
    std::list<std::string> output_names = get_connections_for_ports( jack_output );
    std::list<std::string> input_names = get_connections_for_ports( jack_input );

    connection_display->clear();

    int n = 0;
    for ( std::list<std::string>::const_iterator j = input_names.begin();
          j != input_names.end();
          j++ )
    {
        connection_display->add( j->c_str() );
        n++;
    }
    for ( std::list<std::string>::const_iterator j = output_names.begin();
          j != output_names.end();
          j++ )
    {
        connection_display->add( j->c_str() );
        n++;
    }
  
    h( 25 + ( n * 13 ) );

    parent()->parent()->redraw();
}

void
JACK_Module::cb_button ( Fl_Widget *w, void *v )
{
    ((JACK_Module*)v)->cb_button( w );
}

void
JACK_Module::cb_button( Fl_Widget *w )
{
    int n = audio_output.size();

    if ( w == dec_button )
    {
        --n;
    }
    else if ( w == inc_button )
    {
        ++n;
    }
    
    if ( chain()->can_configure_outputs( this, n ) )
    {
        configure_outputs( n );
        chain()->configure_ports();
    }
}

int
JACK_Module::can_support_inputs ( int )
{
    return audio_output.size();
}

bool
JACK_Module::configure_inputs ( int n )
{
   if ( n > 0 )
   {
       if ( is_default() )
           control_input[0].hints.minimum = 1;

       output_connection_handle->show();
   }

  
    int on = audio_input.size();

    if ( n > on )
    {
        for ( int i = on; i < n; ++i )
        {
            JACK::Port *po = NULL;

            if ( !_prefix )
                po = new JACK::Port( chain()->engine(), JACK::Port::Output, i );
            else
                po = new JACK::Port( chain()->engine(), JACK::Port::Output, _prefix, i );

            if ( ! po->activate() )
            {
                jack_port_activation_error( po );
                return false;
            }

            if ( po->valid() )
            {
                add_port( Port( this, Port::INPUT, Port::AUDIO ) );
                jack_output.push_back( *po );
            }

            delete po;
        }
    }
    else
    {
        for ( int i = on; i > n; --i )
        {
            audio_input.back().disconnect();
            audio_input.pop_back();
            jack_output.back().shutdown();
            jack_output.pop_back();
        }
    }

    if ( is_default() )
        control_input[0].control_value_no_callback( n );

    return true;
}

void
JACK_Module::jack_port_activation_error ( JACK::Port *p )
{
    fl_alert( "Could not activate JACK port \"%s\"", p->name() );
}

bool
JACK_Module::configure_outputs ( int n )
{
   int on = audio_output.size();
   
   if ( n > 0 )
   {
       input_connection_handle->show();
   }
   
    if ( n > on )
    {
        for ( int i = on; i < n; ++i )
        {
            JACK::Port *po = NULL;

            if ( !_prefix )
                po = new JACK::Port( chain()->engine(), JACK::Port::Input, i );
            else
                po = new JACK::Port( chain()->engine(), JACK::Port::Input, _prefix, i );

            if ( ! po->activate() )
            {
                jack_port_activation_error( po );
                return false;
            }

            if ( po->valid() )
            {
                add_port( Port( this, Port::OUTPUT, Port::AUDIO ) );
                jack_input.push_back( *po );
            }

            delete po;
        }
    }
    else
    {
        for ( int i = on; i > n; --i )
        {
            audio_output.back().disconnect();
            audio_output.pop_back();
            jack_input.back().shutdown();
            jack_input.pop_back();
        }
    }

    if ( is_default() )
        control_input[1].control_value_no_callback( n );

    if ( n > 0 && is_default() )
    {
        dec_button->show();
        inc_button->show();
    }
    return true;
}

bool
JACK_Module::initialize ( void )
{
    return true;
}

void
JACK_Module::handle_control_changed ( Port *p )
{
    THREAD_ASSERT( UI );

    if ( 0 == strcmp( p->name(), "Inputs" ) )
    {
        DMESSAGE( "Adjusting number of inputs (JACK outputs)" );
        configure_inputs( p->control_value() );
        if ( chain() )
            chain()->configure_ports();
    }
    else if ( 0 == strcmp( p->name(), "Outputs" ) )
    {
        DMESSAGE( "Adjusting number of outputs (JACK inputs)" );

        if ( ! chain() )
        {
            configure_outputs( p->control_value() );
        }
        else if ( chain()->can_configure_outputs( this, p->control_value() ) )
        {
            configure_outputs( p->control_value() );
            chain()->configure_ports();
        }
        else
        {
            p->connected_port()->control_value( noutputs() );
        }
    }
}

void
JACK_Module::handle_chain_name_changed ( void )
{
    for ( unsigned int i = 0; i < jack_output.size(); ++i )
        jack_output[ i ].name( NULL, i  );

    for ( unsigned int i = 0; i < jack_input.size(); ++i )
        jack_input[ i ].name( NULL, i );

    Module::handle_chain_name_changed();
}

int
JACK_Module::handle ( int m )
{
    static JACK_Module *drag_source = 0;

    switch ( m ) 
    {
        case FL_PUSH:
            return Module::handle(m) || 1;
        case FL_RELEASE:
            return Module::handle(m) || 1;
        case FL_DRAG:
        {
            if ( ! Fl::event_inside( this ) && this != drag_source )
            {
                DMESSAGE( "initiation of drag" );

                char *s = (char*)malloc(256);
                s[0] = 0;
  
                for ( unsigned int i = 0; i < jack_output.size(); ++i )
                {
                    char *s2;
                    asprintf(&s2, "jack.port://%s/%s:%s\r\n", instance_name, chain()->name(), jack_output[i].name() );
                    
                    s = (char*)realloc( s, strlen( s ) + strlen( s2 ) + 1 ); 
                    strcat( s, s2 );

                    free( s2 );
                }
               
                Fl::copy(s, strlen(s) + 1, 0);

                free( s );

                Fl::dnd();
                
                drag_source = this;

                return 1;
            }
            
            return 1;
        }
        /* we have to prevent Fl_Group::handle() from getting these, otherwise it will mess up Fl::belowmouse() */
        case FL_MOVE:
            return 0;
        case FL_ENTER:
        case FL_DND_ENTER:
            return 1;
        case FL_LEAVE:
        case FL_DND_LEAVE:
            if ( this == receptive_to_drop )
            {
                receptive_to_drop = NULL;
                redraw();
            }
            return 1;
        case FL_DND_RELEASE:
            receptive_to_drop = NULL;
            redraw();
            return 1;
        case FL_DND_DRAG:
        {
          if ( this == drag_source )
                return 0;

            if ( this == receptive_to_drop )
                return 1;

            if ( jack_input.size() )
            {
     
                receptive_to_drop = this;
                redraw();
                return 1;
            }
                
            return 0;
        }
        case FL_PASTE:
        {
            receptive_to_drop = NULL;
            redraw();

            drag_source = NULL;

            /* NOW we get the text... */
            const char *text = Fl::event_text();

            DMESSAGE( "Got drop text \"%s\"",text);

            if ( strncmp( text, "jack.port://", strlen( "jack.port://" ) ) )
            {
                return 0;
            }
                        
            std::vector<std::string> port_names;

            char *port_name;
            int end;
            while (  sscanf( text, "jack.port://%a[^\r\n]\r\n%n", &port_name, &end ) > 0 )
            {
                DMESSAGE( "Scanning %s", port_name );
                port_names.push_back( port_name );
                free(port_name );
                
                text += end;
            }
            
            for ( unsigned int i = 0; i < jack_input.size() && i < port_names.size(); i++)
            {
                const char *pn = port_names[i].c_str();
                
                JACK::Port *ji = &jack_input[i];
                
                if ( ji->connected_to( pn ) )
                {
                    
                    DMESSAGE( "Disconnecting from \"%s\"", pn );
                    ji->disconnect( pn );
                }
                else
                {
                    DMESSAGE( "Connecting to %s", pn );
                    ji->connect( pn );
                }
            }
          
            Fl::selection_owner(0);
            return 1;
        }
    }
    
    return Module::handle(m);
}


/**********/
/* Engine */
/**********/

void
JACK_Module::process ( nframes_t nframes )
{
    for ( unsigned int i = 0; i < audio_input.size(); ++i )
    {
        if ( audio_input[i].connected() )
            buffer_copy( (sample_t*)jack_output[i].buffer( nframes ),
                         (sample_t*)audio_input[i].buffer(),
                         nframes );
                         
    }

    for ( unsigned int i = 0; i < audio_output.size(); ++i )
    {
        if ( audio_output[i].connected() )
            buffer_copy( (sample_t*)audio_output[i].buffer(),
                         (sample_t*)jack_input[i].buffer( nframes ),
                         nframes );
    }
}
