
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
#include <FL/Fl_Color_Chooser.H>

#include "Control_Sequence.H"
#include "Track.H"

#include "Engine/Engine.H" // for lock()

#include "Track_Header.H"

#include <list>
using std::list;

#include "Transport.H"

#include "OSC/Endpoint.H"

#include "string_util.h"

#include "FL/event_name.H"
#include "FL/test_press.H"
#include <FL/Fl_Menu_Button.H>
#include "FL/menu_popup.H"



#define DAMAGE_SEQUENCE FL_DAMAGE_USER1
#define DAMAGE_HEADER FL_DAMAGE_USER2

bool Control_Sequence::draw_with_grid = true;
bool Control_Sequence::draw_with_polygon = true;
Fl_Widget * Control_Sequence::_highlighted = 0;

Control_Sequence::Control_Sequence (  ) : Sequence( 0 )
{
    init();
}



Control_Sequence::Control_Sequence ( Track *track, const char *name ) : Sequence( 0 )
{
    init();

    _track = track;

    mode( OSC );

    this->name( name );

    if ( track )
        track->add( this );

    log_create();
}


Control_Sequence::~Control_Sequence ( )
{
    Loggable::block_start();

    clear();

    log_destroy();

    track()->remove( this );

    if ( _output )
    { 
        _output->shutdown();

        delete _output;
        
        _output = NULL;
    }

    if ( _osc_output() )
    {
        OSC::Signal *t = _osc_output();
        
        _osc_output( NULL );
        
        delete t;
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

const char *
Control_Sequence::name ( void ) const
{
    return Sequence::name();
}

void
Control_Sequence::name ( const char *s )
{
    char *n = track()->get_unique_control_name( s );

    Sequence::name( n );
    header()->name_input->value( n );

    if ( mode() == CV )
        update_port_name();

    redraw();
}

void
Control_Sequence::update_port_name ( void )
{
    bool needs_activation = false;
    if ( ! _output )
    {
        _output = new JACK::Port( engine, JACK::Port::Output, track()->name(), track()->ncontrols(), "cv" );
        needs_activation = true;
    }
    
    if ( name() )
    {
        char n[1024];
        snprintf( n, sizeof(n), "%s/%s-cv", track()->name(), name() );
        
        _output->name( n );
    }

    if ( needs_activation )
    {
        if ( ! _output->activate() )
        {
            fl_alert( "Could not create JACK port for control output on track \"%s\"", track()->name() );
            delete _output;
            _output = NULL;
        }
    }
}

void
Control_Sequence::cb_button ( Fl_Widget *w, void *v )
{
    ((Control_Sequence*)v)->cb_button( w );
}

void
Control_Sequence::cb_button ( Fl_Widget *w )
{
    Logger log(this);

    if ( w == header()->name_input )
    {
        name( header()->name_input->value() );
    }
    else if ( w == header()->delete_button )
    {
        Fl::delete_widget( this );
    }
    else if ( w == header()->menu_button )
    {
        menu_popup( &menu(), header()->menu_button->x(), header()->menu_button->y() );
    }
    /* else if ( w == header()->promote_button ) */
    /* { */
    /*     track()->sequence( this ); */
    /* } */
}

void
Control_Sequence::init ( void )
{
    labeltype( FL_NO_LABEL );
    {
        Control_Sequence_Header *o = new Control_Sequence_Header( x(), y(), Track::width(), 52 );

        o->name_input->callback( cb_button, this );
        o->delete_button->callback( cb_button, this );
        o->menu_button->callback( cb_button, this );
        /* o->promote_button->callback( cb_button, this ); */
        Fl_Group::add( o );
    }
    resizable(0);

    box( FL_NO_BOX );

    _track = NULL;
    _output = NULL;
    __osc_output = NULL;
    _mode = (Mode)-1;

    interpolation( Linear );
}



void
Control_Sequence::get ( Log_Entry &e ) const
{
    e.add( ":track", _track );
    e.add( ":name", name() );
    e.add( ":color", color() );
}

void
Control_Sequence::get_unjournaled ( Log_Entry &e ) const
{
    e.add( ":interpolation", _interpolation );

    if ( _osc_output() && _osc_output()->connected() )
    {
        DMESSAGE( "OSC Output connections: %i", _osc_output()->noutput_connections() );

        for ( int i = 0; i < _osc_output()->noutput_connections(); ++i )
        {
            char *s;

            s = _osc_output()->get_output_connection_peer_name_and_path(i);

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
        else if ( ! strcmp( ":color", s ) )
        {
            color( (Fl_Color)atol( v ) );
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
            
            JACK::Port *t = _output;

            _output = NULL;

            delete t;
        }
    }
    else if ( OSC != m && mode() == OSC )
    {
        if ( _osc_output() )
        {
            OSC::Signal *t = _osc_output();

            _osc_output( NULL );

            delete t;
        }
    }

    if ( CV == m && mode() != CV )
    {
        update_port_name();

        header()->outputs_indicator->label( "cv" );
    }
    else if ( OSC == m && mode() != OSC )
    {
        /* FIXME: use name here... */
        char *path;
        asprintf( &path, "/track/%s/control/%i", track()->name(), track()->ncontrols() );

        char *s = escape_url( path );

        free( path );

        path = s;
        
        OSC::Signal *t = timeline->osc->add_signal( path, OSC::Signal::Output, 0, 1, 0, NULL, NULL );
        
        free( path );

        _osc_output( t );

        DMESSAGE( "osc_output: %p", _osc_output() );

        connect_osc();

        header()->outputs_indicator->label( "osc" );
    }

    _mode = m;
}

void
Control_Sequence::draw_curve ( bool filled )
{
    const int bx = drawable_x();
    const int by = y() + Fl::box_dy( box() );
    const int bw = drawable_w();
    const int bh = h() - Fl::box_dh( box() );

    /* make a copy of the list for drawing and sort it... */
    list <Sequence_Widget *> wl;

    std::copy( _widgets.begin(), _widgets.end(), std::back_inserter( wl ) );

//= new list <const Sequence_Widget *>(_widgets);
    
    wl.sort( Sequence_Widget::sort_func );

    list <Sequence_Widget *>::const_iterator e = wl.end();
    e--;

    if ( wl.size() )
        for ( list <Sequence_Widget *>::const_iterator r = wl.begin(); ; r++ )
        {
            const int ry = (*r)->y();
            const int rx = (*r)->line_x();

            if ( r == wl.begin() )
            {
                if ( filled )
                    fl_vertex( bx, bh + by );
                fl_vertex( bx, ry );
            }

            fl_vertex( rx, ry );

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
Control_Sequence::draw_box ( void )
{
    const int bx = drawable_x();
    const int by = y();
    const int bw = drawable_w();
    const int bh = h();

    int X, Y, W, H;

    fl_clip_box( bx, by, bw, bh, X, Y, W, H );

//    fl_rectf( X, Y, W, H, fl_color_average( FL_BLACK, FL_BACKGROUND_COLOR, 0.3 ) );
//    fl_rectf( X,Y,W,H, fl_color_average( FL_BLACK, FL_WHITE, 0.90 ) );
    fl_rectf( X,Y,W,H, FL_DARK1 );
    
    
    if ( draw_with_grid )
    {
        fl_color( FL_GRAY );

        const int inc = bh / 10;
        if ( inc )
            for ( int gy = 0; gy < bh; gy += inc )
                fl_line( X, by + gy, X + W, by + gy );

    }
   
    timeline->draw_measure_lines( X, Y, W, H );
}

void
Control_Sequence::draw ( void )
{
    fl_push_clip( drawable_x(), y(), drawable_w(), h() );
    
    const int bx = x();
    const int by = y() + Fl::box_dy( box() );
    const int bw = w();
    const int bh = h() - Fl::box_dh( box() );

    int X, Y, W, H;

    fl_clip_box( bx, by, bw, bh, X, Y, W, H );

    bool active = active_r();

    const Fl_Color color = active ? this->color() : fl_inactive( this->color() );
//    const Fl_Color selection_color = active ? this->selection_color() : fl_inactive( this->selection_color() );

    if ( box() != FL_NO_BOX )
        draw_box();

    if ( interpolation() != None )
    {
        if ( draw_with_polygon )
        {
            fl_color( fl_color_add_alpha( color, 60 ) );

            fl_begin_complex_polygon();
            draw_curve( true );
            fl_end_complex_polygon();

        }

        fl_color( fl_color_average( FL_WHITE, color, 0.5 ) );
        fl_line_style( FL_SOLID, 2 );
        
        fl_begin_line();
        draw_curve( false );
        fl_end_line();

        fl_line_style( FL_SOLID, 0 );
    }

    if ( interpolation() == None || _highlighted == this || Fl::focus() == this )
        for ( list <Sequence_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); r++ )
            if ( (*r)->x() + (*r)->w() >= bx && 
                 (*r)->x() <= bw + bw )
                (*r)->draw_box();
    else
        for ( list <Sequence_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); r++ )
            if ( (*r)->selected() )
                if ( (*r)->x() + (*r)->w() >= bx && 
                     (*r)->x() <= bw + bw )
                    (*r)->draw_box();

    fl_pop_clip();
    
    if ( damage() & ~DAMAGE_SEQUENCE )
    {
        Fl_Group::draw_children();
    }
}


void
Control_Sequence::menu_cb ( Fl_Widget *w, void *v )
{
    ((Control_Sequence*)v)->menu_cb( (const Fl_Menu_*)w );
}

void
Control_Sequence::menu_cb ( const Fl_Menu_ *m )
{
    char picked[1024];

    DMESSAGE( "Control_Sequence: menu_cb" );

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

        if ( ! _osc_output()->is_connected_to( ((OSC::Signal*)m->mvalue()->user_data()) ) )
        {
            _persistent_osc_connections.push_back( peer_and_path );
            
            connect_osc();
        }
        else
        {
            timeline->osc->disconnect_signal( _osc_output(), peer_name, path );
            
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
        ((Fl_Sometimes_Input*)header()->name_input)->take_focus();
    }
    else if ( !strcmp( picked, "/Remove" ) )
    {
        Fl::delete_widget( this );
    }
    else if ( ! strcmp( picked, "/Color" ) )
    {
        unsigned char r, g, b;

        Fl::get_color( color(), r, g, b );

        if ( fl_color_chooser( "Track Color", r, g, b ) )
        {
            color( fl_rgb_color( r, g, b ) );
        }

        redraw();
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
            if ( ! timeline->osc->connect_signal( _osc_output(), *i ) )
            {
//                MESSAGE( "Failed to connect output %s to ", _osc_output->path(), *i );
            }
            else
            {
                MESSAGE( "Connected output %s to %s", _osc_output()->path(), *i );
//                tooltip( _osc_connected_path );
            }
        }
    }
}

void
Control_Sequence::process_osc ( void )
{
    if ( mode() != OSC )
        return;

    header()->outputs_indicator->value( _osc_output() && _osc_output()->connected() );

    if ( _osc_output() && _osc_output()->connected() )
    {
        sample_t buf[1];
        
        play( buf, (nframes_t)transport->frame, (nframes_t) 1 );
        _osc_output()->value( (float)buf[0] );
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
                    ( _osc_output()->is_connected_to( sig ) ? FL_MENU_VALUE : 0 ) );

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

Fl_Menu_Button &
Control_Sequence::menu ( void ) 
{
    static Fl_Menu_Button _menu( 0, 0, 0, 0, "Control Sequence" );

    _menu.clear();
    
    if ( mode() == OSC )
    {
        add_osc_peers_to_menu( &_menu, "Connect To" );
    }
    
    _menu.add( "Interpolation/None", 0, 0, 0, FL_MENU_RADIO | ( interpolation() == None ? FL_MENU_VALUE : 0 ) );
    _menu.add( "Interpolation/Linear", 0, 0, 0, FL_MENU_RADIO | ( interpolation() == Linear ? FL_MENU_VALUE : 0 ) );
    _menu.add( "Mode/Control Voltage (JACK)", 0, 0, 0 ,FL_MENU_RADIO | ( mode() == CV ? FL_MENU_VALUE : 0 ) );
    _menu.add( "Mode/Control Signal (OSC)", 0, 0, 0 , FL_MENU_RADIO | ( mode() == OSC ? FL_MENU_VALUE : 0 ) );
    
    _menu.add( "Rename", 0, 0, 0 );
    _menu.add( "Color", 0, 0, 0 );
    _menu.add( "Remove", 0, 0, 0 );
    
    _menu.callback( &Control_Sequence::menu_cb, (void*)this);

    return _menu;
}

int
Control_Sequence::handle ( int m )
{
    switch ( m )
    {
        case FL_ENTER:
            break;
        case FL_LEAVE:
            _highlighted = 0;
            damage( DAMAGE_SEQUENCE );
            fl_cursor( FL_CURSOR_DEFAULT );
            break;
        case FL_MOVE:
            if ( Fl::event_x() > drawable_x() )
            {
                if ( _highlighted != this )
                {
                    _highlighted = this;
                    damage( DAMAGE_SEQUENCE );
                    fl_cursor( FL_CURSOR_CROSS );
                }
            }
            else
            {
                if ( _highlighted == this )
                {
                    _highlighted = 0;
                    damage( DAMAGE_SEQUENCE );
                    fl_cursor( FL_CURSOR_DEFAULT );
                }
            }
        default:
            break;
    }

    Logger log(this);

    int r = Sequence::handle( m );

    if ( r )
        return r;

    switch ( m )
    {
        case FL_PUSH:
        {

            if ( Fl::event_x() >= drawable_x() &&
                test_press( FL_BUTTON1 ) )
            {
                /* insert new control point */
                timeline->wrlock();

                Control_Point *r = new Control_Point( this, timeline->xoffset + timeline->x_to_ts( Fl::event_x() - drawable_x() ), (float)(Fl::event_y() - y()) / h() );

                add( r );

               timeline->unlock();

                return 1;
            }
            else if ( Fl::event_x() < drawable_x() &&
                      test_press( FL_BUTTON3 ) )
            {
                menu_popup( &menu() );

                return 1;
            }

            return Fl_Group::handle( m );
        }
        default:
            return 0;
    }
}
