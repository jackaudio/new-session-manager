
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

#include "Track.H"

#include "Transport.H"

#include "../FL/Fl_Sometimes_Input.H"
#include <FL/fl_ask.H>
#include <FL/Fl_Color_Chooser.H>
#include <FL/Fl.H>

#include "Engine/Engine.H" // for lock()

#include "Control_Sequence.H"
#include "Annotation_Sequence.H"

#include "const.h"
#include "util/debug.h"



int Track::_soloing = 0;

const char *Track::capture_format = "Wav 24";



Track::Track ( const char *L, int channels ) :
    Fl_Group ( 0, 0, 0, 0, 0 )
{
    init();

    if ( L )
        name( L );

    color( (Fl_Color)rand() );

    configure_inputs( channels );
    configure_outputs( channels );

    log_create();
}


Track::Track ( ) : Fl_Group( 0, 0, 1, 1 )
{
    init();

    timeline->add_track( this );
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

Track::~Track ( )
{
    Loggable::block_start();

    takes = NULL;
    control = NULL;
    annotation = NULL;

    solo( false );

    Fl_Group::clear();

    log_destroy();

    timeline->remove_track( this );

    /* give up our ports */
    configure_inputs( 0 );
    configure_outputs( 0 );

    _sequence = NULL;

    if ( _name )
        free( _name );

    Loggable::block_end();
}
#include "FL/Boxtypes.H"

void
Track::init ( void )
{
    _sequence = NULL;
    _name = NULL;
    _selected = false;
    _show_all_takes = false;
    _size = 1;

    record_ds = NULL;
    playback_ds = NULL;

    labeltype( FL_NO_LABEL );

//    clear_visible_focus();

    Fl_Group::size( timeline->w(), height() );

    Track *o = this;
    o->box( FL_THIN_UP_BOX );
    {
        Fl_Group *o = new Fl_Group( 0, 0, 149, 70 );
        o->color( ( Fl_Color ) 53 );

        {
            Fl_Input *o = name_field = new Fl_Sometimes_Input( 2, 2, 144, 24 );
            o->color( FL_BACKGROUND_COLOR );
            o->labeltype( FL_NO_LABEL );
            o->labelcolor( FL_GRAY0 );
            o->textcolor( FL_FOREGROUND_COLOR );

            o->callback( cb_input_field, (void*)this );
        }

        {
            Fl_Group *o = controls = new Fl_Group( 2, 28, 149, 24 );

            {
                Fl_Button *o = record_button =
                    new Fl_Button( 6, 28, 26, 24, "@circle" );
                o->type( 1 );
                o->box( FL_ROUNDED_BOX );
                o->down_box( FL_ROUNDED_BOX );
                o->color( FL_LIGHT1 );
                o->selection_color( FL_RED );
                o->labelsize( 9 );
                o->callback( cb_button, this );
            }
            {
                Fl_Button *o = mute_button =
                    new Fl_Button( 35, 28, 26, 24, "m" );
                o->selection_color( fl_color_average( FL_YELLOW, FL_GREEN, 0.50 ) );
                o->type( 1 );
                o->box( FL_ROUNDED_BOX );
                o->down_box( FL_ROUNDED_BOX );
                o->color( FL_LIGHT1 );
                o->labelsize( 15 );
                o->callback( cb_button, this );
            }
            {
                Fl_Button *o = solo_button =
                    new Fl_Button( 66, 28, 26, 24, "s" );
                o->selection_color( fl_color_average( FL_YELLOW, FL_RED, 0.50 ) );
                o->type( 1 );
                o->box( FL_ROUNDED_BOX );
                o->down_box( FL_ROUNDED_BOX );
                o->color( FL_LIGHT1 );
                o->labelsize( 15 );
                o->callback( cb_button, this );
            }
            {
                Fl_Menu_Button *o = take_menu =
                    new Fl_Menu_Button( 97, 28, 47, 24, "T" );
                o->box( FL_THIN_UP_BOX );
                o->color( FL_LIGHT1 );
                o->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE );
                o->callback( cb_button, this );

                o->add( "Show all takes", 0, 0, 0, FL_MENU_TOGGLE );
                o->add( "New", 0, 0, 0, FL_MENU_DIVIDER );
            }
            o->end();
        }

        {
            Fl_Box *o = new Fl_Box( 0, 72, 149, 38 );
            o->box( FL_NO_BOX );
            Fl_Group::current()->resizable( o );
        }

        o->size( Track::width(), h() );
        o->end();
    }
    {
        Fl_Pack *o = pack = new Fl_Pack( width(), 0, 1006, 115 );
        o->labeltype( FL_NO_LABEL );
        o->resize( x() + width(), y(), w() - width(), h() );

        resizable( o );

        {
//            Fl_Pack *o = annotation = new Fl_Pack( width(), 0, pack->w(), 0 );
            Fl_Pack *o = annotation = new Fl_Pack( width(), 0, pack->w(), 1 );
            o->end();
        }

        {
            Fl_Pack *o = control = new Fl_Pack( width(), 0, pack->w(), 0 );
            o->end();
        }

        {
            Fl_Pack *o = takes = new Fl_Pack( width(), 0, pack->w(), 0 );
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
            resize();
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

    }
}

void
Track::get ( Log_Entry &e ) const
{
    e.add( ":name",            _name            );
    e.add( ":sequence",        sequence()       );
    e.add( ":selected",        _selected        );
    e.add( ":height",          size()           );
    e.add( ":inputs",          input.size()     );
    e.add( ":outputs",         output.size()    );
    e.add( ":color",           (unsigned long)color());
    e.add( ":show-all-takes",  _show_all_takes  );
}


void
Track::cb_input_field ( Fl_Widget *, void *v )
{
    ((Track*)v)->cb_input_field();
}

void
Track::cb_button ( Fl_Widget *w, void *v )
{
    ((Track*)v)->cb_button( w );
}

void
Track::cb_input_field ( void )
{
    log_start();

    name( name_field->value() );

    log_end();
}

void
Track::cb_button ( Fl_Widget *w )
{

    if ( w == record_button )
    {

    }
    if ( w == mute_button )
    {

    }
    if ( w == solo_button )
    {
        if ( solo_button->value() )
            ++_soloing;
        else
            --_soloing;
    }
    else
        if ( w == take_menu )
        {
            int v = take_menu->value();

            switch ( v )
            {
                case 0:                                         /* show all takes */
                    show_all_takes( take_menu->menu()[ v ].value() );
                    break;
                case 1:                                         /* new */
                    sequence( (Audio_Sequence*)sequence()->clone_empty() );
                    break;
                case 2:                                         /* remove */
                    if ( takes->children() )
                    {
                        Loggable::block_start();

                        Sequence *s = sequence();

                        sequence( (Audio_Sequence*)takes->child( 0 ) );

                        delete s;

                        Loggable::block_end();
                    }
                    break;
                default:
                    sequence( (Audio_Sequence*)take_menu->menu()[ v ].user_data() );
            }

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
Track::resize ( void )
{
    for ( int i = takes->children(); i--; )
        takes->child( i )->size( w(), height()  );

    for ( int i = annotation->children(); i--; )
        annotation->child( i )->size( w(), 24 );

    for ( int i = control->children(); i--; )
        control->child( i )->size( w(), height()  );

    /* FIXME: hack! */
    if ( annotation->children() )
        annotation->show();
    else
        annotation->hide();

    if ( _show_all_takes )
    {
        takes->show();
        Fl_Group::size( w(), height() * ( 1 + takes->children() + pack_visible( control ) ) );
    }
    else
    {
        takes->hide();
        Fl_Group::size( w(), height() * ( 1 + pack_visible( control ) ) );
    }

    Fl_Group::size( w(), h() + ( ( 24 ) * pack_visible( annotation ) ) );

    if ( sequence() )
        sequence()->size( w(), height() );


    if ( controls->y() + controls->h() > y() + h() )
        controls->hide();
    else
        controls->show();

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

    resize();
}

void
Track::update_take_menu ( void )
{
    take_menu->clear();

    take_menu->add( "Show all takes", 0, 0, 0, FL_MENU_TOGGLE );
    take_menu->add( "New", 0, 0, 0 );
    take_menu->add( "Remove", 0, 0, 0, FL_MENU_DIVIDER );

    for ( int i = 0; i < takes->children(); ++i )
    {
        Sequence *s = (Sequence *)takes->child( i );

        take_menu->add( s->name(), 0, 0, s );
    }
}

void
Track::add ( Audio_Sequence * t )
{
    takes->insert( *t, 0 );
    if ( ! t->name() )
    {
        char pat[20];
        snprintf( pat, sizeof( pat ), "%d", 1 + takes->children() );
        t->name( strdup( pat ) );
    }

    t->labeltype( FL_ENGRAVED_LABEL );

    update_take_menu();
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

    resize();

    update_take_menu();
}

void
Track::remove ( Annotation_Sequence *t )
{
    if ( ! annotation )
        return;

    annotation->remove( t );

    resize();
}

void
Track::remove ( Control_Sequence *t )
{
    if ( ! control )
        return;

    engine->lock();

    control->remove( t );

    engine->unlock();

    resize();
}

void
Track::sequence ( Audio_Sequence * t )
{
    t->track( this );

    if ( sequence() )
        add( sequence() );

    _sequence = t;
    pack->insert( *t, 1 );

    t->labeltype( FL_NO_LABEL );

    resize();
}

void
Track::add ( Control_Sequence *t )
{
    DMESSAGE( "adding control sequence" );

    engine->lock();

    t->track( this );

    control->add( t );

    engine->unlock();

    resize();
}

void
Track::add ( Annotation_Sequence *t )
{
    DMESSAGE( "adding annotation sequence" );

    t->track( this );

    annotation->add( t );

    resize();
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
       int c = atoi( s );

       if ( c <= 0 || c > 10 )
           fl_alert( "Invalid number of channels." );
       else
       {
                                configure_inputs( c );
                                configure_outputs( c );
       }
   }
   else if ( ! strcmp( picked, "/Add Control" ) )
   {
       new Control_Sequence( this );
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
}

#include "FL/menu_popup.H"

/** build the context menu */
Fl_Menu_Button &
Track::menu ( void ) const
{
    static Fl_Menu_Button m( 0, 0, 0, 0, "Track" );

    int c = output.size();

    Fl_Menu_Item menu[] =
        {
            { "Type",            0, 0, 0, FL_SUBMENU    },
            { "Mono",            0, 0, 0, FL_MENU_RADIO | ( c == 1 ? FL_MENU_VALUE : 0 ) },
            { "Stereo",          0, 0, 0, FL_MENU_RADIO | ( c == 2 ? FL_MENU_VALUE : 0 ) },
            { "Quad",            0, 0, 0, FL_MENU_RADIO | ( c == 4 ? FL_MENU_VALUE : 0 ) },
            { "...",             0, 0, 0, FL_MENU_RADIO | ( c == 3 || c > 4 ? FL_MENU_VALUE : 0 ) },
            { 0                  },
            { "Add Control",     0, 0, 0 },
            { "Add Annotation",  0, 0, 0 },
            { "Color",           0, 0, 0 },
            { "Rename",          FL_CTRL + 'n', 0, 0 },
            { "Remove",          0, 0, 0 }, // transport->rolling ? FL_MENU_INACTIVE : 0 },
            { 0 },
        };

    menu_set_callback( menu, &Track::menu_cb, (void*)this );

    m.copy( menu, (void*)this );

    return m;
}

#include "FL/event_name.H"
#include "FL/test_press.H"

void
Track::draw ( void )
{
    if ( _selected )
    {
        Fl_Color c = color();

        color( FL_RED );

        Fl_Group::draw();

        color( c );
    }
    else
        Fl_Group::draw();
}

int
Track::handle ( int m )
{

/*     if ( m != FL_NO_EVENT ) */
/*         DMESSAGE( "%s", event_name( m ) ); */

    switch ( m )
    {
        case FL_KEYBOARD:
            return menu().test_shortcut() || Fl_Group::handle( m );
        case FL_MOUSEWHEEL:
        {
            Logger log( this );

            if ( ! Fl::event_shift() )
                return 0;

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
#include "const.h"
}
