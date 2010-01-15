
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

/* Filter module. Can host LADPSA Plugins, or can be inherited from to make internal
   modules with special features and appearance. */

#include "Plugin_Module.H"

#include <Fl/fl_draw.H>
#include <FL/Fl_Group.H>

#include "util/debug.h"

#include <string.h>

#define HAVE_LIBLRDF 1
#include "LADSPAInfo.h"
#include <vector>
#include <string>
#include <ladspa.h>
#include <stdlib.h>
#include <math.h>

#include "Engine/Engine.H"



static LADSPAInfo *ladspainfo;

/* keep this out of the header to avoid spreading ladspa.h dependency */
struct Plugin_Module::ImplementationData
{
    const LADSPA_Descriptor     *descriptor;
//    std::vector<LADSPA_Data*>    m_LADSPABufVec;
    std::vector<LADSPA_Handle>   handle;
};




Plugin_Module::Plugin_Module ( ) : Module( 50, 50, name() )
{
    init();

    end();

    log_create();
}

Plugin_Module::~Plugin_Module ( )
{
    log_destroy();
    plugin_instances( 0 );
}




void
Plugin_Module::get ( Log_Entry &e ) const
{
//    char s[512];
//    snprintf( s, sizeof( s ), "ladspa:%lu", _idata->descriptor->UniqueID );
    e.add( ":plugin_id", _idata->descriptor->UniqueID );

    Module::get( e );
}

void
Plugin_Module::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":plugin_id" ) )
        {
            load( (unsigned long) atoll ( v ) );
        }
    }

    Module::set( e );
}




#include <FL/Fl_Menu_Button.H>

/* allow the user to pick a plugin */
Plugin_Module *
Plugin_Module::pick_plugin ( void )
{
    /**************/
    /* build menu */
    /**************/

    Fl_Menu_Button *menu = new Fl_Menu_Button( 0, 0, 400, 400 );
    menu->type( Fl_Menu_Button::POPUP3 );

    Plugin_Module::Plugin_Info *pia = Plugin_Module::discover();

    for ( Plugin_Module::Plugin_Info *pi = pia; pi->path; ++pi )
    {
        menu->add(pi->path, 0, NULL, pi, 0 );
    }

    menu->popup();

    if ( menu->value() <= 0 )
        return NULL;

    /************************/
    /* load selected plugin */
    /************************/

    Plugin_Module::Plugin_Info *pi = (Plugin_Module::Plugin_Info*)menu->menu()[ menu->value() ].user_data();

    Plugin_Module *m = new Plugin_Module();

    m->load( pi->id );

    delete[] pia;

    return m;
}


void
Plugin_Module::init ( void )
{
    _idata = new Plugin_Module::ImplementationData();
    _idata->handle.clear();
    _active = false;
    _crosswire = false;

    align( (Fl_Align)FL_ALIGN_CENTER | FL_ALIGN_INSIDE );
    color( (Fl_Color)fl_color_average( FL_BLUE, FL_GREEN, 0.5f ) );
    int tw, th, tx, ty;

    bbox( tx, ty, tw, th );
}

#include "FL/test_press.H"

int
Plugin_Module::handle ( int m )
{
    switch ( m )
    {
        case FL_ENTER:
        case FL_LEAVE:
            redraw();
            return 1;
            break;
        default:
            return Module::handle( m );
    }

    return 0;
}

/* There are two possible adaptations that can be made at Plugin_Module input to account for a mismatch
   between channel configurations.

   The two scenarios are as follows.

   1. The preceding module has fewer outputs than this module has inputs. If
   the preceding module has 1 output (MONO) then it will be duplicated
   for this module's addition inputs. If the preceding module has more
   than one output, then the chain is in error.

   2. The preceding module has more outputs than this module has inputs
   If this module has 1 output (MONO) then it will create the required number of
   instances of its plugin.


   Stereo plugins are never run with more than one instance.  Mono
   plugins will have their outputs brought up to stereo for plugins with
   stereo input.

*/

int
Plugin_Module::can_support_inputs ( int n )
{
    /* this is the simple case */
    if ( plugin_ins() == n )
        return plugin_outs();
    /* e.g. MONO going into STEREO */
    /* we'll duplicate our inputs */
    else if ( n < plugin_ins() &&
              1 == n  )
    {
        return plugin_outs();
    }
    /* e.g. STEREO going into MONO */
    /* we'll run multiple instances of the plugin */
    else if ( n > plugin_ins() &&
              ( plugin_ins() == 1 && plugin_outs() == 1 ) )
    {
        return n;
    }

    return -1;
}

bool
Plugin_Module::configure_inputs( int n )
{
    if ( ninputs() == n )
        return true;

    int inst = 1;
    _crosswire = false;

    if ( 1 == n && plugin_ins() > 1 )
    {
        DMESSAGE( "Cross-wiring plugin inputs" );
        _crosswire = true;

        audio_input.clear();

        for ( int i = n; i--; )
            audio_input.push_back( Port( this, Port::INPUT, Port::AUDIO ) );
    }
    else if ( n >= plugin_ins() &&
              ( plugin_ins() == 1 && plugin_outs() == 1 ) )
    {
        DMESSAGE( "Running multiple instances of plugin" );

        audio_input.clear();
        audio_output.clear();

        for ( int i = n; i--; )
        {
            add_port( Port( this, Port::INPUT, Port::AUDIO ) );
            add_port( Port( this, Port::OUTPUT, Port::AUDIO ) );
        }

        inst = n;
    }
    if ( n == plugin_ins() )
    {

    }
    else
    {
        // unsupported configuration
        return false;
    }

    if ( _active )
        deactivate();

    if ( plugin_instances( inst ) )
        instances( inst );
    else
        return false;

    if ( ! _active )
        activate();

    return true;
}

/* return a list of available plugins */
Plugin_Module::Plugin_Info *
Plugin_Module::discover ( void )
{
    if ( !ladspainfo )
        ladspainfo = new LADSPAInfo();

    std::vector<LADSPAInfo::PluginEntry> plugins = ladspainfo->GetMenuList();

    Plugin_Info* pi = new Plugin_Info[plugins.size() + 1];

    int j = 0;
    for (std::vector<LADSPAInfo::PluginEntry>::iterator i=plugins.begin();
         i!=plugins.end(); i++, j++)
    {
        pi[j].path = i->Name.c_str();
        pi[j].id = i->UniqueID;
    }

    return pi;
}

bool
Plugin_Module::plugin_instances ( unsigned int n )
{
    if ( _idata->handle.size() > n )
    {
        for ( int i = _idata->handle.size() - n; i--; )
        {
            DMESSAGE( "Destroying plugin instance" );

            LADSPA_Handle h = _idata->handle.back();

            if ( _idata->descriptor->deactivate )
                _idata->descriptor->deactivate( h );
            if ( _idata->descriptor->cleanup )
                _idata->descriptor->cleanup( h );

            _idata->handle.pop_back();
        }
    }
    else if ( _idata->handle.size() < n )
    {
        for ( int i = n - _idata->handle.size(); i--; )
        {
            LADSPA_Handle h;

            DMESSAGE( "Instantiating plugin..." );

            if ( ! (h = _idata->descriptor->instantiate( _idata->descriptor, engine->sample_rate() ) ) )
            {
                WARNING( "Failed to instantiate plugin" );
                return false;
            }

            DMESSAGE( "Instantiated: %p", h );

            _idata->handle.push_back( h );

            DMESSAGE( "Connecting control ports..." );

            int ij = 0;
            int oj = 0;
            for ( unsigned int k = 0; k < _idata->descriptor->PortCount; ++k )
            {
                if ( LADSPA_IS_PORT_CONTROL( _idata->descriptor->PortDescriptors[k] ) )
                {
                    if ( LADSPA_IS_PORT_INPUT( _idata->descriptor->PortDescriptors[k] ) )
                        _idata->descriptor->connect_port( h, k, (LADSPA_Data*)control_input[ij++].buffer() );
                    else if ( LADSPA_IS_PORT_OUTPUT( _idata->descriptor->PortDescriptors[k] ) )
                        _idata->descriptor->connect_port( h, k, (LADSPA_Data*)control_output[oj++].buffer() );
                }
            }

            // connect ports to magic bogus value to aid debugging.
            for ( unsigned int k = 0; k < _idata->descriptor->PortCount; ++k )
                if ( LADSPA_IS_PORT_AUDIO( _idata->descriptor->PortDescriptors[k] ) )
                    _idata->descriptor->connect_port( h, k, (LADSPA_Data*)0x42 );

        }
    }

    return true;
}

bool
Plugin_Module::load ( unsigned long id )
{
    if ( !ladspainfo )
        ladspainfo = new LADSPAInfo();

    _idata->descriptor = ladspainfo->GetDescriptorByID( id );

    label( _idata->descriptor->Name );

    _plugin_ins = _plugin_outs = 0;

    if ( _idata->descriptor )
    {
        if ( LADSPA_IS_INPLACE_BROKEN( _idata->descriptor->Properties ) )
        {
            WARNING( "Cannot use this plugin because it is incapable of processing audio in-place" );
            return false;
        }
        else if ( ! LADSPA_IS_HARD_RT_CAPABLE( _idata->descriptor->Properties ) )
        {
            WARNING( "Cannot use this plugin because it is incapable of hard real-time operation" );
            return false;
        }

        MESSAGE( "Name: %s", _idata->descriptor->Name );

        for ( unsigned int i = 0; i < _idata->descriptor->PortCount; ++i )
        {
            if ( LADSPA_IS_PORT_AUDIO( _idata->descriptor->PortDescriptors[i] ) )
            {
                if ( LADSPA_IS_PORT_INPUT( _idata->descriptor->PortDescriptors[i] ) )
                {
                    add_port( Port( this, Port::INPUT, Port::AUDIO, _idata->descriptor->PortNames[ i ] ) );
                    _plugin_ins++;
                }
                else if (LADSPA_IS_PORT_OUTPUT(_idata->descriptor->PortDescriptors[i]))
                {
                    _plugin_outs++;
                    add_port( Port( this, Port::OUTPUT, Port::AUDIO, _idata->descriptor->PortNames[ i ] ) );
                }
            }
        }

        MESSAGE( "Plugin has %i inputs and %i outputs", _plugin_ins, _plugin_outs);

        for ( unsigned int i = 0; i < _idata->descriptor->PortCount; ++i )
        {
            if ( LADSPA_IS_PORT_CONTROL( _idata->descriptor->PortDescriptors[i] ) )
            {
                Port::Direction d = Port::INPUT;

                if ( LADSPA_IS_PORT_INPUT( _idata->descriptor->PortDescriptors[i] ) )
                {
                    d = Port::INPUT;
                }
                else if ( LADSPA_IS_PORT_OUTPUT( _idata->descriptor->PortDescriptors[i] ) )
                {
                    d = Port::OUTPUT;
                }

                Port p( this, d, Port::CONTROL, _idata->descriptor->PortNames[ i ] );


                LADSPA_PortRangeHintDescriptor hd = _idata->descriptor->PortRangeHints[i].HintDescriptor;

                if ( LADSPA_IS_HINT_BOUNDED_BELOW(hd) )
                {
                    p.hints.ranged = true;
                    p.hints.minimum = _idata->descriptor->PortRangeHints[i].LowerBound;
                }
                if ( LADSPA_IS_HINT_BOUNDED_ABOVE(hd) )
                {
                    p.hints.ranged = true;
                    p.hints.maximum = _idata->descriptor->PortRangeHints[i].UpperBound;
                }

                if ( LADSPA_IS_HINT_HAS_DEFAULT(hd) )
                {

                    float Max=1.0f, Min=-1.0f, Default=0.0f;
                    int Port=i;

                    // Get the bounding hints for the port
                    LADSPA_PortRangeHintDescriptor HintDesc=_idata->descriptor->PortRangeHints[Port].HintDescriptor;
                    if (LADSPA_IS_HINT_BOUNDED_BELOW(HintDesc))
                    {
                        Min=_idata->descriptor->PortRangeHints[Port].LowerBound;
                        if (LADSPA_IS_HINT_SAMPLE_RATE(HintDesc))
                        {
                            Min*=engine->sample_rate();
                        }
                    }
                    if (LADSPA_IS_HINT_BOUNDED_ABOVE(HintDesc))
                    {
                        Max=_idata->descriptor->PortRangeHints[Port].UpperBound;
                        if (LADSPA_IS_HINT_SAMPLE_RATE(HintDesc))
                        {
                            Max*=engine->sample_rate();
                        }
                    }

#ifdef LADSPA_VERSION
// We've got a version of the header that supports port defaults
                    if (LADSPA_IS_HINT_HAS_DEFAULT(HintDesc)) {
                        // LADSPA_HINT_DEFAULT_0 is assumed anyway, so we don't check for it
                        if (LADSPA_IS_HINT_DEFAULT_1(HintDesc)) {
                            Default = 1.0f;
                        } else if (LADSPA_IS_HINT_DEFAULT_100(HintDesc)) {
                            Default = 100.0f;
                        } else if (LADSPA_IS_HINT_DEFAULT_440(HintDesc)) {
                            Default = 440.0f;
                        } else {
                            // These hints may be affected by SAMPLERATE, LOGARITHMIC and INTEGER
                            if (LADSPA_IS_HINT_DEFAULT_MINIMUM(HintDesc) &&
                                LADSPA_IS_HINT_BOUNDED_BELOW(HintDesc)) {
                                Default=_idata->descriptor->PortRangeHints[Port].LowerBound;
                            } else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(HintDesc) &&
                                       LADSPA_IS_HINT_BOUNDED_ABOVE(HintDesc)) {
                                Default=_idata->descriptor->PortRangeHints[Port].UpperBound;
                            } else if (LADSPA_IS_HINT_BOUNDED_BELOW(HintDesc) &&
                                       LADSPA_IS_HINT_BOUNDED_ABOVE(HintDesc)) {
                                // These hints require both upper and lower bounds
                                float lp = 0.0f, up = 0.0f;
                                float min = _idata->descriptor->PortRangeHints[Port].LowerBound;
                                float max = _idata->descriptor->PortRangeHints[Port].UpperBound;
                                if (LADSPA_IS_HINT_DEFAULT_LOW(HintDesc)) {
                                    lp = 0.75f;
                                    up = 0.25f;
                                } else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(HintDesc)) {
                                    lp = 0.5f;
                                    up = 0.5f;
                                } else if (LADSPA_IS_HINT_DEFAULT_HIGH(HintDesc)) {
                                    lp = 0.25f;
                                    up = 0.75f;
                                }

                                if (LADSPA_IS_HINT_LOGARITHMIC(HintDesc)) {

                                    p.hints.type = Port::Hints::LOGARITHMIC;

                                    if (min==0.0f || max==0.0f) {
                                        // Zero at either end means zero no matter
                                        // where hint is at, since:
                                        //  log(n->0) -> Infinity
                                        Default = 0.0f;
                                    } else {
                                        // Catch negatives
                                        bool neg_min = min < 0.0f ? true : false;
                                        bool neg_max = max < 0.0f ? true : false;

                                        if (!neg_min && !neg_max) {
                                            Default = exp(::log(min) * lp + ::log(max) * up);
                                        } else if (neg_min && neg_max) {
                                            Default = -exp(::log(-min) * lp + ::log(-max) * up);
                                        } else {
                                            // Logarithmic range has asymptote
                                            // so just use linear scale
                                            Default = min * lp + max * up;
                                        }
                                    }
                                } else {
                                    Default = min * lp + max * up;
                                }
                            }
                            if (LADSPA_IS_HINT_SAMPLE_RATE(HintDesc)) {
                                Default *= engine->sample_rate();
                            }
                            if (LADSPA_IS_HINT_INTEGER(HintDesc)) {
                                if ( p.hints.ranged &&
                                     0 == p.hints.minimum &&
                                     1 == p.hints.maximum )
                                    p.hints.type = Port::Hints::BOOLEAN;
                                else
                                    p.hints.type = Port::Hints::INTEGER;
                                Default = floorf(Default);
                            }
                            if (LADSPA_IS_HINT_TOGGLED(HintDesc)){
                                p.hints.type = Port::Hints::BOOLEAN;
                            }
                        }
                    }
#else
                    Default = 0.0f;
#endif
                    p.hints.default_value = Default;
                }

                float *control_value = new float;

                *control_value = p.hints.default_value;

                p.connect_to( control_value );

                add_port( p );

                DMESSAGE( "Plugin has control port \"%s\" (default: %f)", _idata->descriptor->PortNames[ i ], p.hints.default_value );
            }
        }
    }
    else
    {
        WARNING( "Failed to load plugin" );
        return false;
    }

    return plugin_instances( 1 );
}

void
Plugin_Module::set_input_buffer ( int n, void *buf )
{
    LADSPA_Handle h;

    if ( instances() > 1 )
    {
        h = _idata->handle[n];
        n = 0;
    }
    else
        h = _idata->handle[0];

    for ( unsigned int i = 0; i < _idata->descriptor->PortCount; ++i )
        if ( LADSPA_IS_PORT_INPUT( _idata->descriptor->PortDescriptors[i] ) &&
             LADSPA_IS_PORT_AUDIO( _idata->descriptor->PortDescriptors[i] ) )
            if ( n-- == 0 )
                _idata->descriptor->connect_port( h, i, (LADSPA_Data*)buf );
}

void
Plugin_Module::set_output_buffer ( int n, void *buf )
{
    LADSPA_Handle h;

    if ( instances() > 1 )
    {
        h = _idata->handle[n];
        n = 0;
    }
    else
        h = _idata->handle[0];

    for ( unsigned int i = 0; i < _idata->descriptor->PortCount; ++i )
        if ( LADSPA_IS_PORT_OUTPUT( _idata->descriptor->PortDescriptors[i] ) &&
             LADSPA_IS_PORT_AUDIO( _idata->descriptor->PortDescriptors[i] ) )
            if ( n-- == 0 )
                _idata->descriptor->connect_port( h, i, (LADSPA_Data*)buf );
}

void
Plugin_Module::activate ( void )
{
    if ( _active )
        FATAL( "Attempt to activate already active plugin" );

    if ( _idata->descriptor->activate )
        for ( unsigned int i = 0; i < _idata->handle.size(); ++i )
            _idata->descriptor->activate( _idata->handle[i] );

    _active = true;
}

void
Plugin_Module::deactivate( void )
{
    if ( _idata->descriptor->deactivate )
        for ( unsigned int i = 0; i < _idata->handle.size(); ++i )
            _idata->descriptor->activate( _idata->handle[i] );

    _active = false;
}

void
Plugin_Module::handle_port_connection_change ( void )
{
//    DMESSAGE( "Connecting audio ports" );

    if ( _crosswire )
    {
        for ( int i = 0; i < plugin_ins(); ++i )
            set_input_buffer( i, audio_input[0].buffer() );
    }
    else
    {
        for ( unsigned int i = 0; i < audio_input.size(); ++i )
            set_input_buffer( i, audio_input[i].buffer() );
    }

    for ( unsigned int i = 0; i < audio_output.size(); ++i )
        set_output_buffer( i, audio_output[i].buffer() );
}

void
Plugin_Module::process ( )
{
    handle_port_connection_change();

    if ( _active )
        for ( unsigned int i = 0; i < _idata->handle.size(); ++i )
        {
            volatile int n = i;
            _idata->descriptor->run( _idata->handle[n], nframes() );
        }
}


