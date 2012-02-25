
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



bool Control_Sequence::draw_with_gradient = true;
bool Control_Sequence::draw_with_polygon = true;
bool Control_Sequence::draw_with_grid = true;



Control_Sequence::Control_Sequence ( Track *track ) : Sequence( 0 )
{
    init();

    _osc_connected_peer = _osc_connected_path = 0;

    _track = track;

    _output = new JACK::Port( engine, JACK::Port::Output, track->name(), track->ncontrols(), "cv" );

    if ( ! _output->activate() )
    {
        FATAL( "could not create JACK port" );
    }

    /* { */
    /*     char *path; */
    /*     asprintf( &path, "/non/daw/%s/control/%i", track->name(), track->ncontrols() ); */
        
    /*     _osc_output = timeline->osc->add_signal( path, OSC::Signal::Output, NULL, NULL ); */
        
    /*     free( path ); */
    /* } */

    _osc_output = 0;

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

    _output->shutdown();

    delete _output;

    _output = NULL;

    delete _osc_output;
    
    _osc_output = NULL;

    if ( _osc_connected_peer )
        free( _osc_connected_peer );
        
    _osc_connected_peer = NULL;

    if ( _osc_connected_path )
        free( _osc_connected_path );

    _osc_connected_path = NULL;

    Loggable::block_end();
}

void
Control_Sequence::init ( void )
{
    _track = NULL;
    _highlighted = false;
    _output = NULL;
    _osc_output = NULL;
    color( fl_darker( FL_YELLOW ) );

    interpolation( Linear );
    frequency( 10 );
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
    if ( _osc_output->connected() )
    {
        char *path;
        char *peer;
        
        _osc_output->get_connected_peer_name_and_path( &peer, &path );
        
        e.add( ":osc-peer", peer );
 
        e.add( ":osc-path", path );
        
        free( path );
        free( peer );
    }
    /* e.add( ":frequency", frequency() ); */
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
            interpolation( (curve_type_e)atoi( v ) );
        }
        /* else if ( ! strcmp( ":frequency", s ) ) */
        /*     frequency( atoi( v ) ); */
        else if ( ! strcmp( ":osc-peer", s ) )
        {
            _osc_connected_peer = strdup( v );
        }
        else if ( !strcmp( ":osc-path", s ) )
        {
            _osc_connected_path = strdup( v );
        }
    }
}

void
Control_Sequence::draw_curve ( bool flip, bool filled )
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
                if ( flip )
                {
                    if ( filled )
                        fl_vertex( bx, by );
                    fl_vertex( bx, ry );
                }
                else
                {
                    if ( filled )
                        fl_vertex( bx, bh + by );
                    fl_vertex( bx, ry );
                }
            }

            fl_vertex( (*r)->line_x(), ry );

            if ( r == e )
            {
                if ( flip )
                {
                    fl_vertex( bx + bw, ry );
                    if ( filled )
                        fl_vertex( bx + bw, by );
                }
                else
                {
                    fl_vertex( bx + bw, ry );
                    if ( filled )
                        fl_vertex( bx + bw, bh + by );
                }
                break;
            }

        }
}

void
Control_Sequence::draw ( void )
{
    if ( ! fl_not_clipped( x(), y(), w(), h() ) )
        return;

    fl_push_clip( x(), y(), w(), h() );

    /* draw the box with the ends cut off. */
    draw_box( box(), x() - Fl::box_dx( box() ), y(), w() + Fl::box_dw( box() ) + 1, h(), color() );

    const int bx = x();
    const int by = y() + Fl::box_dy( box() );
    const int bw = w();
    const int bh = h() - Fl::box_dh( box() );

    int X, Y, W, H;

    fl_clip_box( bx, by, bw, bh, X, Y, W, H );

    bool active = active_r();

    const Fl_Color color = active ? this->color() : fl_inactive( this->color() );
    const Fl_Color selection_color = active ? this->selection_color() : fl_inactive( this->selection_color() );


        if ( draw_with_gradient )
        {
/*         const Fl_Color c2 = fl_color_average( selection_color, FL_WHITE, 0.90f ); */
/*         const Fl_Color c1 = fl_color_average( color, c2, 0.60f ); */

            const Fl_Color c1 = fl_color_average( selection_color, FL_WHITE, 0.90f );
            const Fl_Color c2 = fl_color_average( color, c1, 0.60f );

            for ( int gy = 0; gy < bh; gy++ )
            {
                fl_color( fl_color_average( c1, c2, gy / (float)bh) );
                fl_line( X, by + gy, X + W, by + gy );
            }
        }

        if ( draw_with_grid )
        {
            fl_color( fl_darker( color ) );

            const int inc = bh / 10;
            if ( inc )
                for ( int gy = 0; gy < bh; gy += inc )
                    fl_line( X, by + gy, X + W, by + gy );

        }

    if ( interpolation() != None )
    {
        if ( draw_with_polygon )
        {
            fl_color( draw_with_gradient ? color : fl_color_average( color, selection_color, 0.45f ) );

            fl_begin_complex_polygon();
            draw_curve( draw_with_gradient, true );
            fl_end_complex_polygon();

            fl_color( selection_color );
            fl_line_style( FL_SOLID, 2 );

            fl_begin_line();
            draw_curve( draw_with_gradient, false );
            fl_end_line();
        }
        else
        {
//        fl_color( fl_color_average( selection_color, color, 0.70f ) );
            fl_color( selection_color );
            fl_line_style( FL_SOLID, 2 );

            fl_begin_line();
            draw_curve( draw_with_gradient, false );
            fl_end_line();
        }

        fl_line_style( FL_SOLID, 0 );
    }

    timeline->draw_measure_lines( x(), y(), w(), h(), color );

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

        _osc_connected_peer = strdup( peer_name );

        _osc_connected_path = strdup( ((OSC::Signal*)m->mvalue()->user_data())->path() );

        if ( ! _osc_output->is_connected_to( ((OSC::Signal*)m->mvalue()->user_data()) ) )
        {
            connect_osc();
        }
        else
        {
            timeline->osc->disconnect_signal( _osc_output, _osc_connected_peer, _osc_connected_path );

            free( _osc_connected_path );
            free( _osc_connected_peer );
            _osc_connected_peer = _osc_connected_path = NULL;
        }
        
    }
    else if ( ! strcmp( picked, "Interpolation/Linear" ) )
        interpolation( Linear );
    else if ( ! strcmp( picked, "Interpolation/None" ) )
        interpolation( None );
    /* else if ( ! strcmp( picked, "Frequency/1Hz" ) ) */
    /*     frequency( 1 ); */
    /* else if ( ! strcmp( picked, "Frequency/5Hz" ) ) */
    /*     frequency( 5 ); */
    /* else if ( ! strcmp( picked, "Frequency/10Hz" ) ) */
    /*     frequency( 10 ); */
    /* else if ( ! strcmp( picked, "Frequency/20Hz" ) ) */
    /*     frequency( 20 ); */
    /* else if ( ! strcmp( picked, "Frequency/30Hz" ) ) */
    /*     frequency( 30 ); */
    /* else if ( ! strcmp( picked, "Frequency/60Hz" ) ) */
    /*     frequency( 60 ); */

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
    if ( ! _osc_output )
    {
        char *path;
        asprintf( &path, "/non/daw/%s/control/%i", track()->name(), track()->ncontrols() );
        
        _osc_output = timeline->osc->add_signal( path, OSC::Signal::Output, NULL, NULL );
        
        free( path );
    }

    if ( _osc_connected_peer && _osc_connected_path )
    {
        if ( ! timeline->osc->connect_signal( _osc_output, _osc_connected_peer, _osc_connected_path ) )
        {
            //  MESSAGE( "Failed to connect output %s to %s:%s", _osc_output->path(), _osc_connected_peer, _osc_connected_path );
        }
        else
        {
            tooltip( _osc_connected_path );

//            _osc_connected_peer = _osc_connected_path = 

            MESSAGE( "Connected output %s to %s:%s", _osc_output->path(), _osc_connected_peer, _osc_connected_path );
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

    asprintf( &s, "%s/%s%s", peer_prefix, name, sig->path() );

    peer_menu->add( s, 0, NULL, (void*)( sig ),
                     FL_MENU_TOGGLE |
                    ( _osc_output->is_connected_to( sig ) ? FL_MENU_VALUE : 0 ) );

    free( s );

    connect_osc();
}

void
Control_Sequence::add_osc_peers_to_menu ( Fl_Menu_Button *m, const char *prefix )
{
   peer_menu = m;
   peer_prefix = prefix;

   timeline->osc->list_peers( &Control_Sequence::peer_callback, this );
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
                timeline->discover_peers();

                timeline->osc->wait( 500 );

                Fl_Menu_Button menu( 0, 0, 0, 0, "Control Sequence" );

                /* Fl_Menu_Button *con = new Fl_Menu_Button( 0, 0, 0, 0 ); */

//                con->callback( &Control_Sequence::menu_cb, (void*)this );

                menu.clear();

                add_osc_peers_to_menu( &menu, "Connect To" );
                
                /* menu.add( "Connect To", 0, 0, 0); */
                /* menu.add( "Connect To", 0, 0, const_cast< Fl_Menu_Item *>( con->menu() ), FL_SUBMENU_POINTER ); */
                menu.add( "Interpolation/None", 0, 0, 0, FL_MENU_RADIO | ( interpolation() == None ? FL_MENU_VALUE : 0 ) );
                menu.add( "Interpolation/Linear", 0, 0, 0, FL_MENU_RADIO | ( interpolation() == Linear ? FL_MENU_VALUE : 0 ) );

                /* menu.add( "Frequency/1Hz", 0, 0, 0, FL_MENU_RADIO | ( frequency() == 1 ? FL_MENU_VALUE : 0 ) ); */
                /* menu.add( "Frequency/5Hz", 0, 0, 0, FL_MENU_RADIO | ( frequency() == 5 ? FL_MENU_VALUE : 0 ) ); */
                /* menu.add( "Frequency/10Hz", 0, 0, 0, FL_MENU_RADIO | ( frequency() == 10 ? FL_MENU_VALUE : 0 ) ); */
                /* menu.add( "Frequency/20Hz", 0, 0, 0, FL_MENU_RADIO | ( frequency() == 20 ? FL_MENU_VALUE : 0 ) ); */
                /* menu.add( "Frequency/30Hz", 0, 0, 0, FL_MENU_RADIO | ( frequency() == 30 ? FL_MENU_VALUE : 0 ) ); */
                /* menu.add( "Frequency/60Hz", 0, 0, 0, FL_MENU_RADIO | ( frequency() == 60 ? FL_MENU_VALUE : 0 ) ); */


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
