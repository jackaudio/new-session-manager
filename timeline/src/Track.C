
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

/* A Track is a container for various sequences; the sequence, the
 * takes (inactive sequences), annotation sequences, control
 * sequences */
/* TODO: split into Track and Audio_Track (and maybe later Video_Track
 * and MIDI_Track */

#include <sys/time.h>
#include "Track.H"

#include "Transport.H"

#include "../FL/Fl_Sometimes_Input.H"
#include <FL/fl_ask.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/Fl.H>
#include "FL/Fl_Scalepack.H"
#include "FL/Fl_Blink_Button.H"

#include "Engine/Engine.H" // for lock()

#include "Control_Sequence.H"
#include "Annotation_Sequence.H"

#include "Track_Header.H"

#include "const.h"
#include "debug.h"


static Fl_Color
random_color ( void )
{
    return fl_rgb_color( rand() % 255, rand() % 255, rand() % 255 );
}

static Fl_Menu_Button _menu( 0, 0, 0, 0, "Track" );

class Fl_Sometimes_Pack : public Fl_Pack
{
    bool _pack;

public:

    Fl_Sometimes_Pack ( int X, int Y, int W, int H, const char *L=0 ) : Fl_Pack(X,Y,W,H,L)
        {
            _pack = true;
        }

    virtual ~Fl_Sometimes_Pack ( ) 
        {
        }

    void pack ( bool b )
        {
            if ( b != _pack )
                redraw();

            _pack = b;
        }

    bool pack ( void ) const
        {
            return _pack;
        }

    virtual void draw ( void )
        {
            /* draw_box(); */

            if ( _pack )
            {
                for ( int i = 0; i < children(); i++ )
                {
                    Fl_Widget *o = child( i );
                    
                    o->box( FL_FLAT_BOX );
                }
                
                Fl_Pack::draw();
            }
            else
            {
                if ( children() )
                {
                    for ( int i = 0; i < children(); i++ )
                    {
                        Fl_Widget *o = child( i );

                        if ( i != 0 )
                            o->box( FL_NO_BOX );

                        o->resize( x(),y(),w(), o->h() );
                    }
                    resize( x(), y(), w(), child(0)->h() );
                }

                Fl_Group::draw();
            }
        }
};



int Track::_soloing = 0;

const char *Track::capture_format = "Wav 24";
bool Track::colored_tracks = true;



Track::Track ( const char *L, int channels ) :
    Fl_Group ( 0, 0, 0, 0, 0 )
{
    init();

    if ( L )
        name( L );

    color( random_color() );

    configure_inputs( channels );
    configure_outputs( channels );

    log_create();
}


Track::Track ( ) : Fl_Group( 0, 0, 1, 1 )
{
    init();

    timeline->add_track( this );
}

Track::~Track ( )
{
    Loggable::block_start();

    /* must destroy sequences first to preserve proper log order */
    takes->clear();
    control->clear();
    annotation->clear();
    delete sequence();

    takes = NULL;
    control = NULL;
    annotation = NULL;

    log_destroy();

    /* ensure that soloing accounting is performed */
    solo( false );

    timeline->remove_track( this );

    /* give up our ports */
    configure_inputs( 0 );
    configure_outputs( 0 );

    _sequence = NULL;

    if ( _name )
        free( _name );

    Loggable::block_end();
}

void
Track::init ( void )
{
    _row = 0;
    _sequence = NULL;
    _name = NULL;
    _selected = false;
    _size = 1;

    record_ds = NULL;
    playback_ds = NULL;

    labeltype( FL_NO_LABEL );

//    clear_visible_focus();

    Fl_Group::size( timeline->w(), height() );

    Track *o = this;
    o->box( FL_FLAT_BOX );

    {
        Track_Header *o = new Track_Header( x(), y(), 200, 80 );

        name_field = o->name_input;
        record_button = o->rec_button;
        mute_button = o->mute_button;
        solo_button = o->solo_button;
        show_all_takes_button = o->show_all_takes_button;
        overlay_controls_button = o->overlay_controls_button;
        
        name_field->callback( cb_button, this );
        record_button->callback( cb_button, this );
        mute_button->callback( cb_button, this );
        solo_button->callback( cb_button, this );

        show_all_takes_button->callback( cb_button, this );
        overlay_controls_button->callback( cb_button, this );

        resizable( o );
        o->color( (Fl_Color)53 );
    }

    /* { */
    /*     Fl_Box *o = new Fl_Box( 0, 72, 149, 38 ); */
    /*     o->box( FL_NO_BOX ); */
    /*     Fl_Group::current()->resizable( o ); */
    /* } */

    {
        /* this pack holds the active sequence, annotation sequence, control sequences and takes */
        Fl_Pack *o = pack = new Fl_Pack( x() + width(), y(), w() - width(), h() );
        o->type( Fl_Pack::VERTICAL );
        o->labeltype( FL_NO_LABEL );
        /* o->resize( x() + width(), y(), w() - width(), h() ); */

        /* resizable( o ); */

        {
            Fl_Pack *o = annotation = new Fl_Pack( width(), 0, pack->w(), 1 );
            o->type( Fl_Pack::VERTICAL );
            o->end();
        }

        {
            Fl_Sometimes_Pack *o = control = new Fl_Sometimes_Pack( width(), 0, pack->w(), 1 );
            o->box( FL_NO_BOX );
            o->color( FL_BACKGROUND_COLOR );
            o->type( Fl_Pack::VERTICAL );
            o->pack( true );
            o->end();
        }

        {
            Fl_Pack *o = takes = new Fl_Pack( width(), 0, pack->w(), 1 );
            o->type( Fl_Pack::VERTICAL );
            o->end();
            o->hide();
        }

        o->end();
    }
    end();

}



void
Track::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":height" ) )
        {
            size( atoi( v ) );
            adjust_size();
        }
        else if ( ! strcmp( s, ":selected" ) )
            _selected = atoi( v );
//                else if ( ! strcmp( s, ":armed"
        else if ( ! strcmp( s, ":name" ) )
            name( v );
        else if ( ! strcmp( s, ":inputs" ) )
            configure_inputs( atoi( v ) );
        else if ( ! strcmp( s, ":outputs" ) )
            configure_outputs( atoi( v ) );
        else if ( ! strcmp( s, ":color" ) )
        {
            color( (Fl_Color)atoll( v ) );
            redraw();
        }
        else if ( ! strcmp( s, ":show-all-takes" ) )
            show_all_takes( atoi( v ) );
        else if ( ! strcmp( s, ":overlay-controls" ) )
            overlay_controls( atoi( v ) );
        else if ( ! strcmp( s, ":solo" ) )
            solo( atoi( v ) );
        else if ( ! strcmp( s, ":mute" ) )
            mute( atoi( v ) );
        else if ( ! strcmp( s, ":arm" ) )
            armed( atoi( v ) );
        else if ( ! strcmp( s, ":sequence" ) )
        {
            int i;
            sscanf( v, "%X", &i );

            if ( i )
            {
                Audio_Sequence *t = (Audio_Sequence*)Loggable::find( i );

                /* FIXME: our track might not have been
                 * defined yet... what should we do about this
                 * chicken/egg problem? */
                if ( t )
                {
//                        assert( t );

                    sequence( t );
                }

            }

        }
        else if ( ! strcmp( s, ":row" ) )
            row( atoi( v ) );
    }
}

void
Track::get ( Log_Entry &e ) const
{
    e.add( ":name",            _name            );
    e.add( ":sequence",        sequence()       );
    e.add( ":selected",        _selected        );
    e.add( ":color",           (unsigned long)color());
}

void
Track::get_unjournaled ( Log_Entry &e ) const
{
    e.add( ":height",          size()           );
    e.add( ":inputs",          input.size()     );
    e.add( ":outputs",         output.size()    );
    e.add( ":show-all-takes",  show_all_takes()  );
    e.add( ":overlay-controls", overlay_controls()  );    
    e.add( ":armed",           armed()          );
    e.add( ":mute",            mute()           );
    e.add( ":solo",            solo()           );
    e.add( ":row",             timeline->find_track( this ) );
}

int
Track::row ( void ) const 
{
    return _row;
}

void
Track::row ( int n )
{
    _row = n;
}

void
Track::log_children ( void ) const
{
    log_create();

    for ( int i = 0; i < control->children(); i++ )
        ((Sequence*)control->child( i ))->log_children();

    for ( int i = 0; i < annotation->children(); i++ )
        ((Sequence*)annotation->child( i ))->log_children();

    for ( int i = 0; i < takes->children(); i++ )
        ((Sequence*)takes->child( i ))->log_children();

    sequence()->log_children();
}

void
Track::solo ( bool b )
{
    if ( b && ! solo_button->value() )
        ++_soloing;
    else if ( ! b && solo_button->value() )
        --_soloing;

    solo_button->value( b );
}


void
Track::cb_button ( Fl_Widget *w, void *v )
{
    ((Track*)v)->cb_button( w );
}

void
Track::cb_button ( Fl_Widget *w )
{
    Logger log(this);

    if ( w == name_field )
    {
        name( name_field->value() );
    }
    else if ( w == record_button )
    {

    }
    else if ( w == mute_button )
    {

    }
    else if ( w == solo_button )
    {
        if ( solo_button->value() )
            ++_soloing;
        else
            --_soloing;
    }
    else if ( w == show_all_takes_button )
    {
        show_all_takes( show_all_takes_button->value() );
    }
    else if ( w == overlay_controls_button )
    {
        overlay_controls( overlay_controls_button->value() );
    }
}

static int pack_visible( Fl_Pack *p )
{
    int v = 0;
    for ( int i = p->children(); i--; )
        if ( p->child( i )->visible() )
            v++;

    return v;
}

/* adjust size of widget and children */
void
Track::adjust_size ( void )
{
    for ( int i = takes->children(); i--; )
        takes->child( i )->size( w(), height()  );

    for ( int i = annotation->children(); i--; )
        annotation->child( i )->size( w(), 24 );

    for ( int i = control->children(); i--; )
        control->child( i )->size( w(), height()  );

    control->pack( ! overlay_controls() );

    int TH = height();

    if ( show_all_takes() )
    {
        takes->show();
        TH += height() * takes->children();
    }
    else
        takes->hide();

    if ( control->children() )
    {
        control->show();
        
        if ( overlay_controls() )
            TH += height() *  (control->children() ? 1 : 0);
        else
            TH += height() * pack_visible( control );
    }
    else
        control->hide();
    
    if ( annotation->children() )
    {
        annotation->show();
        TH += 24 * pack_visible( annotation );
    }
    else
        annotation->hide();

    if ( ! size() )
    {
        takes->hide();
        control->hide();
        Fl_Group::size( w(), height() );
    }
    else
        Fl_Group::size( w(), TH );

    if ( sequence() )
        sequence()->size( w(), height() );

    /* if ( controls->y() + controls->h() > y() + h() ) */
    /*     controls->hide(); */
    /* else */
    /*     controls->show(); */

    

    /* FIXME: why is this necessary? */
    if ( parent() )
        parent()->parent()->redraw();
}

void
Track::size ( int v )
{
    if ( v < 0 || v > 3 )
        return;

    _size = v;

    adjust_size();
}


void
Track::add ( Audio_Sequence * t )
{
    takes->insert( *t, 0 );

    t->color( fl_color_average( FL_BLACK, FL_GRAY, 0.25f ) );

    t->labeltype( FL_ENGRAVED_LABEL );
}

void
Track::remove ( Audio_Sequence *t )
{
    if ( ! takes )
        return;

    timeline->wrlock();

    if ( sequence() == t )
    {
        pack->remove( t );

        if ( takes->children() )
            sequence( (Audio_Sequence*)takes->child( 0 ) );
        else
            /* FIXME: should this ever happen? */
            _sequence = NULL;
    }
    else
        takes->remove( t );

/*     delete t; */

    timeline->unlock();

    adjust_size();
}

void
Track::remove ( Annotation_Sequence *t )
{
    if ( ! annotation )
        return;

    annotation->remove( t );

    adjust_size();
}

void
Track::remove ( Control_Sequence *t )
{
    if ( ! control )
        return;

    timeline->wrlock();

    engine->lock();

    control->remove( t );

    engine->unlock();

    timeline->unlock();

    adjust_size();
}

void
Track::sequence ( Audio_Sequence * t )
{
    if ( sequence() == t )
    {
        DMESSAGE( "Attempt to set sequence twice" );
        return;
    }

    t->track( this );

    if ( sequence() )
        add( sequence() );

    _sequence = t;
    /* insert following the annotation pack */
    pack->insert( *t, 1 );

    t->color( FL_GRAY );
    t->labeltype( FL_NO_LABEL );

    adjust_size();
}


void
Track::add ( Control_Sequence *t )
{
    DMESSAGE( "adding control sequence" );

    engine->lock();

    t->track( this );

    t->color( random_color() );

//    control->insert( *t, 0 );
    control->add( t );
    
    engine->unlock();

    adjust_size();
}

void
Track::add ( Annotation_Sequence *t )
{
    DMESSAGE( "adding annotation sequence" );

    t->track( this );

    annotation->add( t );

    adjust_size();
}

/** add all widget on this track falling within the given rectangle to
    the selection.  */
void
Track::select ( int X, int Y, int W, int H,
                bool include_control, bool merge_control )
{

    Sequence *t = sequence();

    if ( ! ( t->y() > Y + H || t->y() + t->h() < Y ) )
        t->select_range( X, W );
    else
        include_control = true;

    if ( include_control )
        for ( int i = control->children(); i--; )
        {
            Control_Sequence *c = (Control_Sequence*)control->child( i );

            if ( merge_control ||
                 ( c->y() >= Y && c->y() + c->h() <= Y + H  ) )
                c->select_range( X, W );
        }
}


#include <FL/Fl_Menu_Button.H>

void
Track::menu_cb ( Fl_Widget *w, void *v )
{
    ((Track*)v)->menu_cb( (Fl_Menu_*) w );
}

void
Track::menu_cb ( const Fl_Menu_ *m )
{
    char picked[256];

    m->item_pathname( picked, sizeof( picked ) );

    DMESSAGE( "Picked: %s", picked );

    Logger log( this );

    if ( ! strcmp( picked, "Type/Mono" ) )
    {
        configure_inputs( 1 );
        configure_outputs( 1 );
    }
    else if ( ! strcmp( picked, "Type/Stereo" ) )
    {
        configure_inputs( 2 );
        configure_outputs( 2 );
    }
    else if ( ! strcmp( picked, "Type/Quad" ) )
    {
        configure_inputs( 4 );
        configure_outputs( 4 );
    }
    else if ( ! strcmp( picked, "Type/..." ) )
    {
        const char *s = fl_input( "How many channels?", "3" );
        if ( s )
        {

            int c = atoi( s );

            if ( c <= 0 || c > 10 )
                fl_alert( "Invalid number of channels." );
            else
            {
                configure_inputs( c );
                configure_outputs( c );
            }
        }
    }
    else if ( ! strcmp( picked, "/Add Control" ) )
    {
        new Control_Sequence( this );
    }
    else if ( ! strcmp( picked, "/Overlay controls" ) )
    {
        overlay_controls( ! m->mvalue()->value() );
    }
    else if ( ! strcmp( picked, "/Add Annotation" ) )
    {
        add( new Annotation_Sequence( this ) );
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
    else if ( ! strcmp( picked, "Flags/Record" ) )
    {
        armed( m->mvalue()->flags & FL_MENU_VALUE );
    }
    else if ( ! strcmp( picked, "Flags/Mute" ) )
    {
        mute( m->mvalue()->flags & FL_MENU_VALUE );
    }
    else if ( ! strcmp( picked, "Flags/Solo" ) )
    {
        solo( m->mvalue()->flags & FL_MENU_VALUE );
    }
    else if ( ! strcmp( picked, "Size/Small" ) )
    {
        size( 0 );
    }
    else if ( ! strcmp( picked, "Size/Medium" ) )
    {
        size( 1 );
    }
    else if ( ! strcmp( picked, "Size/Large" ) )
    {
        size( 2 );
    }
    else if ( ! strcmp( picked, "Size/Huge" ) )
    {
        size( 3 );
    }
    else if ( ! strcmp( picked, "/Remove" ) )
    {
        int r = fl_choice( "Are you certain you want to remove track \"%s\"?", "Cancel", NULL, "Remove", name() );

        if ( r == 2 )
        {
            timeline->remove_track( this );
            Fl::delete_widget( this );
        }
    }
    else if ( ! strcmp( picked, "/Rename" ) )
    {
        ((Fl_Sometimes_Input*)name_field)->take_focus();
    }
    else if ( ! strcmp( picked, "/Move Up" ) )
    {
        timeline->move_track_up( this );
    }
    else if ( ! strcmp( picked, "/Move Down" ) )
    {
        timeline->move_track_down( this );
    }
    else if ( !strcmp( picked, "Takes/Show all takes" ) )
    {
        show_all_takes( ! m->mvalue()->value() );
    }
    else if ( !strcmp( picked, "Takes/New" ) )
    {
        sequence( (Audio_Sequence*)sequence()->clone_empty() );
    }
    else if ( !strcmp( picked, "Takes/Remove" ) )
    {
            if ( takes->children() )
            {
                Loggable::block_start();

                Audio_Sequence *s = sequence();

                sequence( (Audio_Sequence*)takes->child( 0 ) );

                delete s;

                Loggable::block_end();
            }
    }
    else if ( !strcmp( picked, "Takes/Remove others" ))
    {
        if ( takes->children() )
            {
                Loggable::block_start();

                takes->clear();

                Loggable::block_end();
            }
    }
    else if ( !strncmp( picked, "Takes/", sizeof( "Takes/" ) - 1 ) )
    {
        Audio_Sequence* s = (Audio_Sequence*)m->mvalue()->user_data();

        sequence( s );
    }
}

#include "FL/menu_popup.H"

/** build the context menu */
Fl_Menu_Button &
Track::menu ( void ) const
{

    int c = output.size();
    int s = size();

    _menu.clear();

    _menu.add( "Takes/Show all takes", 0, 0, 0, FL_MENU_TOGGLE | ( show_all_takes() ? FL_MENU_VALUE : 0 ) );
    _menu.add( "Takes/New", 0, 0, 0 );

    if ( takes->children() )
    {
        _menu.add( "Takes/Remove", 0, 0, 0 );
        _menu.add( "Takes/Remove others", 0, 0, 0, FL_MENU_DIVIDER );
        
        for ( int i = 0; i < takes->children(); ++i )
        {
            Sequence *s = (Sequence *)takes->child( i );
            
            char n[256];
            snprintf( n, sizeof(n), "Takes/%s", s->name() );

            _menu.add( n, 0, 0, s);
        }
    }

    _menu.add( "Type/Mono",  0, 0, 0, FL_MENU_RADIO | ( c == 1 ? FL_MENU_VALUE : 0 ) );
    _menu.add( "Type/Stereo", 0, 0, 0, FL_MENU_RADIO | ( c == 2 ? FL_MENU_VALUE : 0 ));
    _menu.add( "Type/Quad",            0, 0, 0, FL_MENU_RADIO | ( c == 4 ? FL_MENU_VALUE : 0 ) );
    _menu.add( "Type/...",             0, 0, 0, FL_MENU_RADIO | ( c == 3 || c > 4 ? FL_MENU_VALUE : 0 ) );
    _menu.add( "Overlay controls",   0, 0, 0, FL_MENU_TOGGLE | ( overlay_controls() ? FL_MENU_VALUE : 0 ) );
    _menu.add( "Add Control",     0, 0, 0 );
    _menu.add( "Add Annotation",  0, 0, 0 );
    _menu.add( "Color",           0, 0, 0 );
    _menu.add( "Rename",          FL_CTRL + 'n', 0, 0 );
    _menu.add( "Size/Small",           FL_ALT + '1', 0, 0, FL_MENU_RADIO | ( s == 0 ? FL_MENU_VALUE : 0 ) );
    _menu.add( "Size/Medium",          FL_ALT + '2', 0, 0, FL_MENU_RADIO | ( s == 1 ? FL_MENU_VALUE : 0 ) );
    _menu.add( "Size/Large",           FL_ALT + '3', 0, 0, FL_MENU_RADIO | ( s == 2 ? FL_MENU_VALUE : 0 ) );
    _menu.add( "Size/Huge",           FL_ALT + '4', 0, 0, FL_MENU_RADIO | ( s == 3 ? FL_MENU_VALUE : 0 ) );
    _menu.add( "Flags/Record",         FL_CTRL + 'r', 0, 0, FL_MENU_TOGGLE | ( armed() ? FL_MENU_VALUE : 0 ) );
    _menu.add( "Flags/Mute",            FL_CTRL + 'm', 0, 0, FL_MENU_TOGGLE | ( mute() ? FL_MENU_VALUE : 0 ) );
    _menu.add( "Flags/Solo",           FL_CTRL + 's', 0, 0, FL_MENU_TOGGLE | ( solo() ? FL_MENU_VALUE : 0 ) );
    _menu.add( "Move Up",        FL_SHIFT + '1', 0, 0 );
    _menu.add( "Move Down",        FL_SHIFT + '2', 0, 0 );
    _menu.add( "Remove",          0, 0, 0 ); // transport->rolling ? FL_MENU_INACTIVE : 0 );
  
    _menu.callback( &Track::menu_cb, (void*)this );

    return _menu;
}

#include "FL/event_name.H"
#include "FL/test_press.H"

void
Track::internal_draw ( void )
{
    draw_box();

    /* we have to do this first because the pack(s) size isn't known until draw() */
    draw_child( *pack );


    {
        Track_Header *o = (Track_Header *)child( 0 );
        
        o->controls_header_group->resize( x(), control->y(), o->controls_header_group->w(), control->h() );
        o->takes_header_group->resize( x(), takes->y(), o->takes_header_group->w(), takes->h() );

        if ( takes->visible() )
            o->takes_header_group->show();
        else
            o->takes_header_group->hide();

        if ( control->visible() )
            o->controls_header_group->show();
        else
            o->controls_header_group->hide();

        /* override stupid group resize effect. */
        o->takes_header_group->child(0)->size( 195, 12 );
        o->controls_header_group->child(0)->size( 195, 12 );

    }

    child(0)->size( Track::width(), h() );

    draw_child( *child(0));

    if ( takes->visible() )
        for ( int i = 0; i < takes->children(); i++ )
            draw_outside_label( *takes->child( i ) );
    
    if ( control->visible() )
        for ( int i = 0; i < control->children(); i++ )
            draw_outside_label( *control->child( i ) );
}

void
Track::draw ( void )
{
    int X, Y, W, H;
    
    fl_push_clip( x(), y(), w(), h() );
    
    fl_clip_box( x(), y(), w(), h(), X, Y, W, H );

    Fl_Color saved_color = color();

    if ( ! Track::colored_tracks )
        color( FL_GRAY );

    if ( _selected )
    {
        Fl_Color c = color();

        color( FL_RED );

        internal_draw();

        color( c );
    }
    else
        internal_draw();

    if ( ! Track::colored_tracks )
        color( saved_color );

    fl_pop_clip();
}

int
Track::handle ( int m )
{

/*     if ( m != FL_NO_EVENT ) */
/*         DMESSAGE( "%s", event_name( m ) ); */

    switch ( m )
    {
        case FL_KEYBOARD:
        {
            Fl_Menu_Button * men = &menu();

            if ( Fl::event_key() == FL_Menu )
            {
                menu_popup( men );
                return 1;
            }
            else
                return men->test_shortcut() || Fl_Group::handle( m );
        }
        case FL_MOUSEWHEEL:
        {
            Logger log( this );

            if ( ! Fl::event_shift() )
                return Fl_Group::handle( m );

            int d = Fl::event_dy();

            if ( d < 0 )
                size( size() - 1 );
            else
                size( size() + 1 );

            return 1;
        }
        case FL_PUSH:
        {
            Logger log( this );

            if ( Fl_Group::handle( m ) )
                return 1;

            if ( test_press( FL_BUTTON3 ) && Fl::event_x() < Track::width() )
            {
                menu_popup( &menu() );
                return 1;
            }

            return 0;
        }
        default:
            return Fl_Group::handle( m );
    }

    return 0;
}

void
Track::connect_osc ( void ) 
{
    for ( int j = control->children(); j--; )
    {
        Control_Sequence *c = (Control_Sequence*)control->child( j );
        c->connect_osc();
    }
}

void
Track::process_osc ( void ) 
{
    for ( int j = control->children(); j--; )
    {
        Control_Sequence *c = (Control_Sequence*)control->child( j );
        c->process_osc();
    }
}
