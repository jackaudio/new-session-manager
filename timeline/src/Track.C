
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
#include "../FL/Fl_Sometimes_Pack.H"
#include <FL/fl_ask.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/Fl.H>
#include "FL/Fl_Scalepack.H"
#include "FL/Fl_Blink_Button.H"

#include "Control_Sequence.H"
#include "Annotation_Sequence.H"

#include "Track_Header.H"

#include "const.h"
#include "debug.h"

#include <FL/Fl_Menu_Button.H>
#include "FL/menu_popup.H"

extern char *instance_name;



static Fl_Color
random_color ( void )
{
    return fl_rgb_color( rand() % 255, rand() % 255, rand() % 255 );
}

static Fl_Menu_Button _menu( 0, 0, 0, 0, "Track" );



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
        Track_Header *o = new Track_Header( x(), y(), w(), h() );

        name_field = o->name_input;
        record_button = o->rec_button;
        mute_button = o->mute_button;
        solo_button = o->solo_button;
        menu_button = o->menu_button;
        show_all_takes_button = o->show_all_takes_button;
        overlay_controls_button = o->overlay_controls_button;
        
        name_field->callback( cb_button, this );
        record_button->callback( cb_button, this );
        mute_button->callback( cb_button, this );
        solo_button->callback( cb_button, this );

        show_all_takes_button->callback( cb_button, this );
        overlay_controls_button->callback( cb_button, this );
        menu_button->callback( cb_button, this );

        resizable( o );
//        o->color( (Fl_Color)53 );
    }
    
    {
        /* this pack holds the active sequence, annotation sequence, control sequences and takes */
        Fl_Pack *o = pack = new Fl_Pack( x(), y(), w(), h() );
        o->type( Fl_Pack::VERTICAL );
        o->labeltype( FL_NO_LABEL );
        /* o->resize( x() + width(), y(), w() - width(), h() ); */

        /* resizable( o ); */

        {
            Fl_Pack *o = annotation = new Fl_Pack( 0, 0, pack->w(), 1 );
            o->type( Fl_Pack::VERTICAL );
            o->end();
        }

        {
            {
                Fl_Group *o = controls_heading = new Fl_Group( 0, 0, pack->w(), 10 );
                o->box( FL_FLAT_BOX );
                o->color( fl_color_add_alpha( fl_rgb_color( 1,1,1 ), 127 ) );
                
                {
                    Fl_Box *o = new Fl_Box( 0,0, Track::width(), 10 );
                    o->label( "Controls" );
                    o->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE );
                    o->labelsize( 10 );
                }

                o->hide();
                o->end();
                o->resizable( 0 );
            }

            {
                Fl_Sometimes_Pack *o = control = new Fl_Sometimes_Pack( 0, 0, pack->w(), 1 );
                o->spacing( 1 );
                o->box( FL_NO_BOX );
                o->color( FL_BACKGROUND_COLOR );
                o->type( Fl_Pack::VERTICAL );
                o->pack( true );
                o->hide();
                o->align( FL_ALIGN_TOP | FL_ALIGN_LEFT );
                o->end();
            }
        }

        {
            {
                Fl_Group *o = takes_heading = new Fl_Group( 0, 0, pack->w(), 10 );
                o->box( FL_FLAT_BOX );
                o->color( fl_color_add_alpha( fl_rgb_color( 1,1,1 ), 127 ) );
                
                {
                    Fl_Box *o = new Fl_Box( 0,0, Track::width(), 10 );
                    o->label( "Takes" );
                    o->align( FL_ALIGN_RIGHT | FL_ALIGN_INSIDE );
                    o->labelsize( 10 );
                }
                o->hide();
                o->end();
                o->resizable( 0 );
            }

        {
            Fl_Pack *o = takes = new Fl_Pack( 0, 0, pack->w(), 1 );
            o->type( Fl_Pack::VERTICAL );
            o->end();
            o->hide();
            o->align( FL_ALIGN_TOP | FL_ALIGN_LEFT );
        }
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

    for ( int i = takes->children(); i--; )
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
    else if ( w == menu_button )
    {
        menu_popup( &menu(), menu_button->x(), menu_button->y() );
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

    if ( overlay_controls() )
    {
        for ( int i = 0; i < control->children(); i++ )
        {
            Control_Sequence *o = (Control_Sequence*)control->child(i);

            if ( i != 0 )
                o->box( FL_NO_BOX );

            o->header()->hide();
        }

        control->pack( false );
    }        
    else
    {
        for ( int i = 0; i < control->children(); i++ )
        {
            Control_Sequence *o = (Control_Sequence*)control->child(i);
            
            o->box( FL_FLAT_BOX );

            o->header()->show();
        }

        control->pack( true );
    }


    int TY = 0;

    if ( annotation->children() )
    {
        annotation->show();
        TY += 24 * pack_visible( annotation );
    }
    else
        annotation->hide();

    /* height of the sequence */
    TY += height();

    if ( control->children() )
    {
        int TH;

        /* calculate height of control pack */
        if ( overlay_controls() )
            TH = height() *  (control->children() ? 1 : 0);
        else
            TH = height() * pack_visible( control );

        TY += TH;

        control->show();
        controls_heading->show();
    }
    else
    {
        control->hide();
        controls_heading->hide();
    }

    if ( show_all_takes() )
    {
        /* calculate height of takes pack */
        const int TH = height() * takes->children();
        
        TY += TH;

        takes->show();
        takes_heading->show();
    }
    else
    {
        takes_heading->hide();
        takes->hide();
    }

    if ( takes_heading->visible() )
        TY += takes_heading->h();
    if ( controls_heading->visible() )
        TY += controls_heading->h();

    int TH;

    if ( ! size() )
    {
        takes->hide();
        control->hide();
        TH = height();
    }
    else
        TH = TY;

    Fl_Group::size( w(), TH );
    
    if ( sequence() )
        sequence()->size( w(), height() );

    /* FIXME: why is this necessary? */
    if ( parent() )
        parent()->parent()->redraw();
}

void
Track::name ( const char *name )
{
    if ( _name )
        free( _name );
    
    _name = timeline->get_unique_track_name(name);

    if ( name_field )
        name_field->value( _name );
    
    update_port_names();
}

const char * 
Track::name ( void ) const
{
    return _name; 
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
    t->track( this );

    takes->insert( *t, 0 );

    /* show the take header */
    t->child(0)->show();

    t->color( fl_color_average( FL_BLACK, FL_GRAY, 0.25f ) );

    t->labeltype( FL_ENGRAVED_LABEL );
}

void
Track::remove ( Audio_Sequence *t )
{
    if ( ! takes )
        return;

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

    control->remove( t );

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

    /* hide the take header */
    t->child(0)->hide();

    t->color( FL_GRAY );
    t->labeltype( FL_NO_LABEL );

    adjust_size();
}


void
Track::add ( Control_Sequence *t )
{
    DMESSAGE( "adding control sequence" );

    t->track( this );

    t->color( random_color() );

//    control->insert( *t, 0 );
    control->add( t );
    
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

    X -= Track::width();

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


void
Track::menu_cb ( Fl_Widget *w, void *v )
{
    ((Track*)v)->menu_cb( (Fl_Menu_*) w );
}

void
Track::command_configure_channels ( int n )
{
    /* due to locking this should only be invoked by direct user action */
    timeline->wrlock();
    configure_inputs( n );
    configure_outputs( n );
    timeline->unlock();
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
        command_configure_channels( 1 );
    }
    else if ( ! strcmp( picked, "Type/Stereo" ) )
    {
        command_configure_channels( 2 );
    }
    else if ( ! strcmp( picked, "Type/Quad" ) )
    {
        command_configure_channels( 4 );
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
                command_configure_channels(c);
            }
        }
    }
    else if ( ! strcmp( picked, "/Add Control" ) )
    {
        /* add audio track */
        char *name = get_unique_control_name( "Control" );

        timeline->wrlock();
        new Control_Sequence( this, name );
        timeline->unlock();
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
            timeline->command_remove_track( this );
             Fl::delete_widget( this );
        }
    }
    else if ( ! strcmp( picked, "/Rename" ) )
    {
        ((Fl_Sometimes_Input*)name_field)->take_focus();
    }
    else if ( ! strcmp( picked, "/Move Up" ) )
    {
        timeline->command_move_track_up( this );
    }
    else if ( ! strcmp( picked, "/Move Down" ) )
    {
        timeline->command_move_track_down( this );
    }
    else if ( !strcmp( picked, "Takes/Show all takes" ) )
    {
        show_all_takes( ! m->mvalue()->value() );
    }
    else if ( !strcmp( picked, "Takes/New" ) )
    {
        timeline->wrlock();
        sequence( (Audio_Sequence*)sequence()->clone_empty() );
        timeline->unlock();
    }
    else if ( !strcmp( picked, "Takes/Remove" ) )
    {
            if ( takes->children() )
            {
                Loggable::block_start();

                timeline->wrlock();

                Audio_Sequence *s = sequence();

                sequence( (Audio_Sequence*)takes->child( 0 ) );

                delete s;

                timeline->unlock();

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

        timeline->wrlock();
        sequence( s );
        timeline->unlock();
    }
}

/** retrun a pointer to the control sequence named /name/, or NULL if no control sequence is named /name/ */
Control_Sequence *
Track::control_by_name ( const char *name )
{
    for ( int i = control->children(); i-- ; )
    {
        Control_Sequence *t = (Control_Sequence*)control->child( i );

        if ( t->name() && name && ! strcmp( name, t->name() ) )
            return t;
    }
    
    return NULL;
}

/** return a malloc'd string representing a unique name for a new control sequence */
char *
Track::get_unique_control_name ( const char *name )
{
    char pat[256];

    strcpy( pat, name );

    for ( int i = 1; control_by_name( pat ); ++i )
        snprintf( pat, sizeof( pat ), "%s.%d", name, i );

    return strdup( pat );
}


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

static Fl_Widget *receptive_to_drop = NULL;

void
Track::draw ( void )
{
    fl_push_clip( x(), y(), w(), h() );
    
    Fl_Color saved_color = color();

    if ( ! Track::colored_tracks )
        color( FL_GRAY );

    if ( _selected )
    {
        Fl_Color c = color();

        color( FL_RED );

        Fl_Group::draw();

        color( c );
    }
    else
        Fl_Group::draw();

    if ( ((Track_Header*)child(0))->input_connector_handle == receptive_to_drop )
    {
        Fl_Widget *o = ((Track_Header*)child(0))->input_connector_handle;
        fl_draw_box( FL_OVAL_BOX, o->x(), o->y(), o->w(), o->h(), fl_color_add_alpha( FL_GREEN, 127 ) );
    }

    if ( ! Track::colored_tracks )
        color( saved_color );

    fl_pop_clip();
}

int
Track::handle ( int m )
{

/*     if ( m != FL_NO_EVENT ) */
/*         DMESSAGE( "%s", event_name( m ) ); */
    static Fl_Widget *dragging = NULL;

    switch ( m )
    {
        case FL_DND_ENTER:
        case FL_DND_LEAVE:
        case FL_DND_DRAG:
        case FL_DND_RELEASE:
        case FL_PASTE:
            if ( Fl::event_x() > Track::width() )
                return sequence()->handle(m);
        default:
            break;
    }

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
            if ( Fl::event_button1() && Fl::event_inside( ((Track_Header*)child(0))->color_box ) )
            {
                dragging = this;
                return 1;
            }
            if ( Fl::event_button1() && Fl::event_inside( ((Track_Header*)child(0))->output_connector_handle ) )
                return 1;
            
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
        /* we have to prevent Fl_Group::handle() from getting these, otherwise it will mess up Fl::belowmouse() */
        case FL_ENTER:
        case FL_LEAVE:
        case FL_MOVE:
            if ( Fl::event_x() >= Track::width() )
            {
                return Fl_Group::handle(m);
            }
            return 1;
        case FL_DND_ENTER:
            return 1;
        case FL_DND_LEAVE:
    
            if ( ! Fl::event_inside(this) && this == receptive_to_drop )
            {
                receptive_to_drop = 0;
                redraw();
                Fl::selection_owner(0);
            }
            return 1;
        case FL_RELEASE:
            if ( dragging == this )
            {
                dragging = NULL;
                timeline->insert_track( this, timeline->event_inside() );
                return 1;
            }
            return 0;
            break;
        case FL_DND_RELEASE:
            receptive_to_drop = 0;
            redraw();
            Fl::selection_owner(0);
            return 1;
        case FL_DND_DRAG:
        {
            
            if ( receptive_to_drop == ((Track_Header*)child(0))->input_connector_handle )
                return 1;

           

            if ( Fl::event_inside( ((Track_Header*)child(0))->input_connector_handle )
                 && receptive_to_drop != ((Track_Header*)child(0))->input_connector_handle )
            
            {
                receptive_to_drop = ((Track_Header*)child(0))->input_connector_handle;
                redraw();
                return 1;
            }
            else
            {
                receptive_to_drop = NULL;
                redraw();
                return 0;
            }
        }
        case FL_PASTE:
        {
            receptive_to_drop = 0;
            redraw();

            if (! Fl::event_inside( ((Track_Header*)child(0))->input_connector_handle ) )
                return 0;

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

            for ( unsigned int i = 0; i < input.size() && i < port_names.size(); i++)
            {
                const char *pn = port_names[i].c_str();
                
                JACK::Port *ji = &input[i];
                
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
        case FL_DRAG:
        {
            if ( this != Fl::selection_owner() &&
                 Fl::event_inside( ((Track_Header*)child(0))->output_connector_handle ) )
            {
                char *s = (char*)malloc(256);
                s[0] = 0;
  
                for ( unsigned int i = 0; i < output.size(); ++i )
                {
                    char *s2;
                    asprintf(&s2, "jack.port://%s:%s\r\n", instance_name, output[i].name() );
                    
                    s = (char*)realloc( s, strlen( s ) + strlen( s2 ) + 1 ); 
                    strcat( s, s2 );

                    free( s2 );
                }
               
                Fl::copy(s, strlen(s) + 1, 0);
                Fl::selection_owner(this);

                free( s );

                Fl::dnd();
        
                return 1;
            }
            else
            {
                return 1;
            }
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
Track::update_osc_connection_state ( void )
{
    for ( int j = control->children(); j--; )
    {
        Control_Sequence *c = (Control_Sequence*)control->child( j );
        c->update_osc_connection_state();
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
