
/*******************************************************************************/
/* Copyright (C) 2008 Jonathan Moore Liles                                     */
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
#include "debug.h"

#include <FL/fl_ask.H>

#include "Control_Sequence.H"
#include "Track.H"

#include "Engine/Engine.H" // for lock()

#include <list>
using std::list;

#include "Transport.H"

#include "OSC/Endpoint.H"

#include "string_util.h"



bool Control_Sequence::draw_with_grid = true;
bool Control_Sequence::draw_with_polygon = true;



Control_Sequence::Control_Sequence ( Track *track ) : Sequence( 0 )
{
    init();

    _track = track;

    mode( OSC );

    if ( track )
        track->add( this );

    log_create();
}


Control_Sequence::~Control_Sequence ( )
{
//    Fl::remove_timeout( &Control_Sequence::process_osc, this );

    Loggable::block_start();

    clear();

    log_destroy();

    engine->lock();

    track()->remove( this );

    engine->unlock();

    if ( _output )
    { 
        _output->shutdown();

        delete _output;
        
        _output = NULL;
    }

    if ( _osc_output )
    {
        delete _osc_output;
        
        _osc_output = NULL;
    }

    for ( list<char*>::iterator i = _persistent_osc_connections.begin();
          i != _persistent_osc_connections.end();
          ++i )
    {
        free( *i );
    }
    
    _persistent_osc_connections.clear();

    Loggable::block_end();
}

void
Control_Sequence::init ( void )
{
    _track = NULL;
    _highlighted = false;
    _output = NULL;
    _osc_output = NULL;
    _mode = (Mode)-1;

    interpolation( Linear );
}



void
Control_Sequence::get ( Log_Entry &e ) const
{
    e.add( ":track", _track );
    e.add( ":name", name() );
}

void
Control_Sequence::get_unjournaled ( Log_Entry &e ) const
{
    e.add( ":interpolation", _interpolation );

    if ( _osc_output && _osc_output->connected() )
    {
        DMESSAGE( "OSC Output connections: %i", _osc_output->noutput_connections() );

        for ( int i = 0; i < _osc_output->noutput_connections(); ++i )
        {
            char *s;

            s = _osc_output->get_output_connection_peer_name_and_path(i);

            e.add( ":osc-output", s );
        
            free( s );
        }
    }

    e.add( ":mode", mode() );
}

void
Control_Sequence::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( ":track", s ) )
        {
            int i;
            sscanf( v, "%X", &i );
            Track *t = (Track*)Loggable::find( i );

            assert( t );

            _output = new JACK::Port( engine, JACK::Port::Output, t->name(), t->ncontrols(), "cv" );

            if ( ! _output->activate() )
            {
                FATAL( "could not create JACK port" );
            }

            t->add( this );
        }
        else if ( ! strcmp( ":name", s ) )
        {
            name( v );
        }
        else if ( ! strcmp( ":interpolation", s ) )
        {
            interpolation( (Curve_Type)atoi( v ) );
        }
        else if ( ! strcmp( ":mode", s ) )
            mode( (Mode)atoi( v ) );
        else if ( ! strcmp( ":osc-output", s ) )
        {
            _persistent_osc_connections.push_back( strdup( v ) );
        }
    }
}

void
Control_Sequence::mode ( Mode m )
{
    if ( CV != m && mode() == CV )
    {
        if ( _output )
        {
            _output->shutdown();
            
            delete _output;
            
            _output = NULL;
        }
    }
    else if ( OSC != m && mode() == OSC )
    {
        if ( _osc_output )
        {
            delete _osc_output;
            
            _osc_output = NULL;
        }
    }

    if ( CV == m && mode() != CV )
    {
        _output = new JACK::Port( engine, JACK::Port::Output, track()->name(), track()->ncontrols(), "cv" );
        
        if ( ! _output->activate() )
        {
            fl_alert( "Could not create JACK port for control output on %s", track()->name() );
            delete _output;
            _output = NULL;
        }
    }
    else if ( OSC == m && mode() != OSC )
    {
        char *path;
        asprintf( &path, "/track/%s/control/%i", track()->name(), track()->ncontrols() );

        char *s = escape_url( path );

        free( path );

        path = s;
        
        _osc_output = timeline->osc->add_signal( path, OSC::Signal::Output, 0, 1, 0, NULL, NULL );
        
        free( path );

        connect_osc();
    }

    _mode = m;
}

void
Control_Sequence::draw_curve ( bool filled )
{
    const int bx = x();
    const int by = y() + Fl::box_dy( box() );
    const int bw = w();
    const int bh = h() - Fl::box_dh( box() );

    list <Sequence_Widget *>::const_iterator e = _widgets.end();
    e--;

    if ( _widgets.size() )
        for ( list <Sequence_Widget *>::const_iterator r = _widgets.begin(); ; r++ )
        {
            const int ry = (*r)->y();

            if ( r == _widgets.begin() )
            {
                if ( filled )
                    fl_vertex( bx, bh + by );
                fl_vertex( bx, ry );
            }

            fl_vertex( (*r)->line_x(), ry );

            if ( r == e )
            {
                fl_vertex( bx + bw, ry );
                if ( filled )
                    fl_vertex( bx + bw, bh + by );
                break;
            }

        }
}

void
Control_Sequence::draw ( void )
{
//    draw_box();

    fl_push_clip( x(), y(), w(), h() );

    
    /* draw the box with the ends cut off. */
//    draw_box( box(), x() - Fl::box_dx( box() ), y(), w() + Fl::box_dw( box() ) + 1, h(), color() );

    const int bx = x();
    const int by = y() + Fl::box_dy( box() );
    const int bw = w();
    const int bh = h() - Fl::box_dh( box() );

    int X, Y, W, H;

    fl_clip_box( bx, by, bw, bh, X, Y, W, H );

    bool active = active_r();

    const Fl_Color color = active ? this->color() : fl_inactive( this->color() );
//    const Fl_Color selection_color = active ? this->selection_color() : fl_inactive( this->selection_color() );

    fl_rectf( X, Y, W, H, fl_color_average( FL_WHITE, FL_BACKGROUND_COLOR, 0.3 ) );
    
    if ( draw_with_grid )
    {
        fl_color( FL_GRAY );

        const int inc = bh / 10;
        if ( inc )
            for ( int gy = 0; gy < bh; gy += inc )
                fl_line( X, by + gy, X + W, by + gy );

    }

    if ( interpolation() != None )
    {
        if ( draw_with_polygon )
        {
            fl_color( fl_color_add_alpha( color, 100 ) );

            fl_begin_complex_polygon();
            draw_curve( true );
            fl_end_complex_polygon();

        }

        fl_color( fl_color_average( FL_WHITE, color, 0.5 ) );
        fl_line_style( FL_SOLID, 3 );
        
        fl_begin_line();
        draw_curve( false );
        fl_end_line();

        fl_line_style( FL_SOLID, 0 );
    }

    timeline->draw_measure_lines( X, Y, W, H );

    if ( interpolation() == None || _highlighted || Fl::focus() == this )
        for ( list <Sequence_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); r++ )
            (*r)->draw_box();
    else
        for ( list <Sequence_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); r++ )
            if ( (*r)->selected() )
                (*r)->draw_box();

    fl_pop_clip();
}

#include "FL/menu_popup.H"

void
Control_Sequence::menu_cb ( Fl_Widget *w, void *v )
{
    ((Control_Sequence*)v)->menu_cb( (const Fl_Menu_*)w );
}

void
Control_Sequence::menu_cb ( const Fl_Menu_ *m )
{
    char picked[1024];

    if ( ! m->mvalue() ) // || m->mvalue()->flags & FL_SUBMENU_POINTER || m->mvalue()->flags & FL_SUBMENU )
        return;

    m->item_pathname( picked, sizeof( picked ), m->mvalue() );

    if ( ! strncmp( picked, "Connect To/", strlen( "Connect To/" ) ) )
    { 

        char *peer_name = index( picked, '/' ) + 1;
    
        *index( peer_name, '/' ) = 0;

        const char *path = ((OSC::Signal*)m->mvalue()->user_data())->path();

        char *peer_and_path;
        asprintf( &peer_and_path, "%s:%s", peer_name, path );

        if ( ! _osc_output->is_connected_to( ((OSC::Signal*)m->mvalue()->user_data()) ) )
        {
            _persistent_osc_connections.push_back( peer_and_path );
            
            connect_osc();
        }
        else
        {
            timeline->osc->disconnect_signal( _osc_output, peer_name, path );
            
            for ( std::list<char*>::iterator i = _persistent_osc_connections.begin();
                  i != _persistent_osc_connections.end();
                  ++i )
            {
                if ( !strcmp( *i, peer_and_path ) )
                {
                    free( *i );
                    i = _persistent_osc_connections.erase( i );
                    break;
                }
            }
            
            free( peer_and_path );
        }
        
    }
    else if ( ! strcmp( picked, "Interpolation/Linear" ) )
        interpolation( Linear );
    else if ( ! strcmp( picked, "Interpolation/None" ) )
        interpolation( None );
    else if ( ! strcmp( picked, "Mode/Control Signal (OSC)" ))
        mode( OSC );
    else if ( ! strcmp( picked, "Mode/Control Voltage (JACK)" ) )
        mode( CV );

    else if ( ! strcmp( picked, "/Rename" ) )
    {
        const char *s = fl_input( "Input new name for control sequence:", name() );
        
        if ( s )
            name( s );
        
        redraw();
    }
    else if ( !strcmp( picked, "/Remove" ) )
    {
        Fl::delete_widget( this );
    }
}

void
Control_Sequence::connect_osc ( void )
{
    if ( _persistent_osc_connections.size() )
    {
        for ( std::list<char*>::iterator i = _persistent_osc_connections.begin();
              i != _persistent_osc_connections.end();
              ++i )
        {
            if ( ! timeline->osc->connect_signal( _osc_output, *i ) )
            {
//                MESSAGE( "Failed to connect output %s to ", _osc_output->path(), *i );
            }
            else
            {
                MESSAGE( "Connected output %s to %s", _osc_output->path(), *i );

//                tooltip( _osc_connected_path );
            }
        }
    }
}

void
Control_Sequence::process_osc ( void *v )
{
    ((Control_Sequence*)v)->process_osc();
}

void
Control_Sequence::process_osc ( void )
{
    if ( _osc_output && _osc_output->connected() )
    {
        sample_t buf[1];
        
        play( buf, (nframes_t)transport->frame, (nframes_t) 1 );
        _osc_output->value( (float)buf[0] );
    }
}

void
Control_Sequence::peer_callback( const char *name, const OSC::Signal *sig, void *v )
{
    ((Control_Sequence*)v)->peer_callback( name, sig );
}

static Fl_Menu_Button *peer_menu;
static const char *peer_prefix;

void
Control_Sequence::peer_callback( const char *name, const OSC::Signal *sig )
{
    char *s;

    /* only show inputs */
    if ( sig->direction() != OSC::Signal::Input )
        return;

    /* only list CV signals for now */
    if ( ! ( sig->parameter_limits().min == 0.0 &&
             sig->parameter_limits().max == 1.0 ) )
        return;         
    

    assert( sig->path() );

    char *path = strdup( sig->path() );

    unescape_url( path );

    asprintf( &s, "%s/%s%s", peer_prefix, name, path );

    peer_menu->add( s, 0, NULL, (void*)( sig ),
                    FL_MENU_TOGGLE |
                    ( _osc_output->is_connected_to( sig ) ? FL_MENU_VALUE : 0 ) );

    free( path );

    free( s );

    connect_osc();
}

void
Control_Sequence::add_osc_peers_to_menu ( Fl_Menu_Button *m, const char *prefix )
{
    peer_menu = m;
    peer_prefix = prefix;

    timeline->osc->list_peer_signals( &Control_Sequence::peer_callback, this );
}

int
Control_Sequence::handle ( int m )
{
    switch ( m )
    {
        case FL_ENTER:
            _highlighted = true;
            fl_cursor( cursor() );
            redraw();
            return 1;
        case FL_LEAVE:
            _highlighted = false;
            redraw();
            return 1;
        default:
            break;
    }

    int r = Sequence::handle( m );

    if ( r )
        return r;

    switch ( m )
    {
        case FL_PUSH:
        {
            Logger log( this );

            if ( Fl::event_button1() )
            {
                Control_Point *r = new Control_Point( this, timeline->xoffset + timeline->x_to_ts( Fl::event_x() - x() ), (float)(Fl::event_y() - y()) / h() );

                add( r );
            }
            else if ( Fl::event_button3() && ! ( Fl::event_state() & ( FL_ALT | FL_SHIFT | FL_CTRL ) ) )
            {

                Fl_Menu_Button menu( 0, 0, 0, 0, "Control Sequence" );

                menu.clear();

                if ( mode() == OSC )
                {
                    add_osc_peers_to_menu( &menu, "Connect To" );
                }
                
                menu.add( "Interpolation/None", 0, 0, 0, FL_MENU_RADIO | ( interpolation() == None ? FL_MENU_VALUE : 0 ) );
                menu.add( "Interpolation/Linear", 0, 0, 0, FL_MENU_RADIO | ( interpolation() == Linear ? FL_MENU_VALUE : 0 ) );
                menu.add( "Mode/Control Voltage (JACK)", 0, 0, 0 ,FL_MENU_RADIO | ( mode() == CV ? FL_MENU_VALUE : 0 ) );
                menu.add( "Mode/Control Signal (OSC)", 0, 0, 0 , FL_MENU_RADIO | ( mode() == OSC ? FL_MENU_VALUE : 0 ) );

                menu.add( "Rename", 0, 0, 0 );
                menu.add( "Remove", 0, 0, 0 );

                menu.callback( &Control_Sequence::menu_cb, (void*)this);

                menu_popup( &menu, x(), y() );

                return 1;
            }

            return 1;
        }
        default:
            return 0;
    }
}
