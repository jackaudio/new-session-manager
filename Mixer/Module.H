
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

#pragma once

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Group.H>

#include <stdlib.h>
#include "util/debug.h"
#include <vector>

#include "util/Thread.H"

#include "Loggable.H"

class Chain;
class Module_Parameter_Editor;

class Module : public Fl_Group, public Loggable {

    int _ins;
    int _outs;
    int _instances;
    unsigned long _nframes;
    Chain *_chain;
    bool _is_default;

    Module_Parameter_Editor *_editor;

    void cb_handle(Fl_Widget*);
    static void cb_handle(Fl_Widget*, void*);

    void init ( void );

public:

    /* true if this module was added by default and not under normal user control */
    bool is_default ( void ) const { return _is_default; }
    void is_default ( bool v ) { _is_default = v; }

    class Port
    {
        /* char *type_names[] = { "Audio", "Control" }; */
        /* char *direction_names[] = { "Input", "Output" }; */

        void update_connected_port_buffer ( void )
            {
                if ( connected() )
                    connected_port()->_buf = _buf;
            }

    public:

        enum Direction { INPUT, OUTPUT };
        enum Type { AUDIO, CONTROL };

        /* hints for control ports (specifically control inputs) */
        struct Hints
        {
            enum Type { LINEAR, LOGARITHMIC, BOOLEAN, INTEGER };

            Type type;
            bool ranged;
            float minimum;
            float maximum;
            float default_value;
            int dimensions;

            Hints ( )
                {
                    type = LINEAR;
                    ranged = false;
                    minimum = 0;
                    maximum = 0;
                    default_value = 0.0f;
                    dimensions = 1;
                }
        };

        Hints hints;

        Port ( Module *module, Direction direction, Type type, const char *name = 0 )
            {
                _name = name;
                _direction = direction;
                _type = type;
                _buf = 0;
                _nframes = 0;
                _connected = 0;
                _module = module;
            }

        Port ( const Port& p )
            {
                _name = p._name;
                _direction = p._direction;
                _type = p._type;
                _buf = p._buf;
                _nframes = p._nframes;
                _connected = p._connected;
                _module = p._module;
                hints = p.hints;
            }

        virtual ~Port ( )
            {
                //   disconnect();
            }

        const char *name ( void ) const { return _name; }
        Type type ( void ) const { return _type; }
        Direction direction ( void ) const  { return _direction; }

        Module * module ( void ) const { return _module; }
        unsigned long nframes ( void ) const { return _nframes; }

        void buffer ( void *buf, unsigned long nframes ) { _buf = buf; _nframes = nframes; };
        void *buffer ( void ) const { return _buf; }

        void control_value_no_callback ( float f )
            {
                THREAD_ASSERT( UI );

                if ( buffer() )
                {
                    *((float*)buffer()) = f;
                }
            }

        void control_value ( float f )
            {
                control_value_no_callback( f );
                _module->handle_control_changed( this );
            }

        float control_value ( ) const
            {
                if ( buffer() )
                    return *((float*)buffer());
                else
                    return 0.0f;
            }

        bool connected ( void ) const { return _connected; }

        Port *connected_port ( void ) const
            {
                return _connected;
            }

        void connect_to ( Port *to )
            {
                _buf = to->_buf;
                to->_connected = this;
                _connected = to;
            }

        void connect_to ( void *buf )
            {
                _buf = buf;
                update_connected_port_buffer();
            }

        void disconnect ( void )
            {
                if ( _connected && _connected != (void*)0x01 )
                {
                    _connected->_connected = NULL;
                    _connected = NULL;
                }
                else
                    _connected = NULL;

                /* FIXME: do something! */
            }

    private:

        Port *_connected;
        Type _type;
        Direction _direction;
        const char *_name;
        void *_buf;
        unsigned long _nframes;
        Module *_module;
    };

    void bbox ( int &X, int &Y, int &W, int &H )
        {
            X = x() + 5;
            Y = y() + 5;
            W = w() - 10;
            H = h() - 10;
        }

    Module ( int W, int H, const char *L = 0 );
    Module ( );
    Module ( bool is_default, int W, int H, const char *L = 0 );

    virtual ~Module ( );

    LOG_NAME_FUNC( Module );

//    static Module * pick_plugin ( void );


    unsigned long nframes ( void ) const { return _nframes; }
    void nframes ( unsigned long v ) { _nframes = v; }


    int instances ( void ) const { return _instances; }
    void instances ( int i ) { _instances = i; }

    bool is_being_controlled ( void ) const
        {
            for ( unsigned int i = control_input.size(); i--; )
                if ( control_input[i].connected() )
                    return true;
            return false;
        }

    bool is_controlling ( void ) const
        {
            for ( unsigned int i = control_output.size(); i--; )
                if ( control_output[i].connected() )
                    return true;
            return false;
        }

    virtual const char *name ( void ) const = 0;

    std::vector<Port> audio_input;
    std::vector<Port> audio_output;
    std::vector<Port> control_input;
    std::vector<Port> control_output;

    void add_port ( const Port &p )
        {
            if ( p.type() == Port::AUDIO && p.direction() == Port::INPUT )
                audio_input.push_back( p );
            else if ( p.type() == Port::AUDIO && p.direction() == Port::OUTPUT )
                audio_output.push_back( p );
            else if ( p.type() == Port::CONTROL && p.direction() == Port::INPUT )
                control_input.push_back( p );
            else if ( p.type() == Port::CONTROL && p.direction() == Port::OUTPUT )
                control_output.push_back( p );
        }

    int noutputs ( void ) const
        {
            return audio_output.size();
        }

    int ninputs ( void ) const
        {
            return audio_input.size();
        }

    int ncontrol_inputs ( void ) const
        {
            return control_input.size();
        }

    int ncontrol_outputs ( void ) const
        {
            return control_output.size();
        }


    int control_input_port_index ( Port *p )
        {
            for ( unsigned int i = control_input.size(); i--; )
                if ( &control_input[i] == p )
                    return i;

            return -1;
        }

    int control_output_port_index ( Port *p )
        {
            for ( unsigned int i = control_output.size(); i--; )
                if ( &control_output[i] == p )
                    return i;

            return -1;
        }

    Chain *chain ( void ) const { return _chain; }
    void chain ( Chain * v ) { _chain = v; }

    char *get_parameters ( void ) const;
    void set_parameters ( const char * );

    virtual bool initialize ( void ) { return true; }

    /* for the given number of inputs, return how many outputs this
     * plugin would have. -1 if this plugin can't support so many
     * inputs. */
    virtual int can_support_inputs ( int n ) = 0;
    /* called by the chain whenever we need to adjust our input
     * channel configuration, but only if can_support_inputs() returns
     * true */
    virtual bool configure_inputs ( int n ) = 0;

    virtual void process ( void ) { }

    /* called whenever the value of a control port is changed.
       This can be used to take appropriate action from the GUI thread */
    virtual void handle_control_changed ( Port * ) { }

    /* called whenever the name of the chain changes (usually because
     * the name of the mixer strip changed). */
    virtual void handle_chain_name_changed () {}

    virtual void handle_port_connection_change () {}

protected:

    void draw_connections ( void );
    void draw_label ( void );
    void draw_box ( void );

    virtual void draw ( void ) { Module::draw_box(); Module::draw_label(); }
    virtual int handle ( int m );

    virtual void get ( Log_Entry &e ) const;
    virtual void set ( Log_Entry &e );

};
