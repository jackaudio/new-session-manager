
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

#include "Track.H"

#include "Transport.H"

/* #include "Port.H" */

#include "../FL/Fl_Sometimes_Input.H"
#include <FL/fl_ask.H>
#include <FL/Fl_Color_Chooser.H>
// #include <FL/fl_draw.H>
#include <FL/Fl.H>

#include "Engine/Engine.H" // for lock()

#include "Control_Sequence.H"
#include "Annotation_Sequence.H"

int Track::_soloing = 0;

const char *Track::capture_format = "Wav 24";

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
                    return;
                case 1:                                         /* new */
                    sequence( (Audio_Sequence*)sequence()->clone_empty() );
                    return;
            }

            const char *s = take_menu->menu()[ v ].text;

            for ( int i = takes->children(); i--; )
            {
                Audio_Sequence *t = (Audio_Sequence*)takes->child( i );
                if ( ! strcmp( s, t->name() ) )
                {
                    sequence( t );
                    redraw();
                    break;
                }
            }
        }
}

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
                o->box( FL_THIN_UP_BOX );
                o->color( FL_LIGHT1 );
                o->selection_color( FL_RED );
                o->labelsize( 8 );
                o->callback( cb_button, this );
            }
            {
                Fl_Button *o = mute_button =
                    new Fl_Button( 35, 28, 26, 24, "m" );
                o->type( 1 );
                o->box( FL_THIN_UP_BOX );
                o->color( FL_LIGHT1 );
                o->labelsize( 11 );
                o->callback( cb_button, this );
            }
            {
                Fl_Button *o = solo_button =
                    new Fl_Button( 66, 28, 26, 24, "s" );
                o->type( 1 );
                o->box( FL_THIN_UP_BOX );
                o->color( FL_LIGHT1 );
                o->labelsize( 11 );
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

Track::~Track ( )
{
    /* FIXME: why is this necessary? */
    timeline->remove_track( this );

    /* give up our ports */
    configure_inputs( 0 );
    configure_outputs( 0 );

    /* controls too */
    for ( int i = control_out.size(); i--; )
    {
            control_out.back()->shutdown();
            delete control_out.back();
            control_out.pop_back();
    }

    log_destroy();

    if ( _name )
        free( _name );
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
Track::add ( Audio_Sequence * t )
{
    takes->insert( *t, 0 );
    if ( ! t->name() )
    {
        char pat[20];
        snprintf( pat, sizeof( pat ), "%d", takes->children() );
        t->name( strdup( pat ) );
    }

    take_menu->add( t->name() );

    t->labeltype( FL_ENGRAVED_LABEL );
}

void
Track::remove ( Audio_Sequence *t )
{
    timeline->wrlock();

    takes->remove( t );

    timeline->unlock();

    resize();

//            take_menu->remove( t->name() );
}

void
Track::remove ( Annotation_Sequence *t )
{
    annotation->remove( t );

    resize();
}

void
Track::remove ( Control_Sequence *t )
{
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
    switch ( m )
    {
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

            int X = Fl::event_x();
            int Y = Fl::event_y();

            if ( Fl_Group::handle( m ) )
                return 1;

            if ( Fl::test_shortcut( FL_BUTTON3 ) && ! Fl::event_shift() && X < Track::width() )
            {
                int c = output.size();

                /* context menu */
                Fl_Menu_Item menu[] =
                    {
                        { "Type",            0, 0, 0, FL_SUBMENU    },
                        { "Mono",            0, 0, 0, FL_MENU_RADIO | ( c == 1 ? FL_MENU_VALUE : 0 ) },
                        { "Stereo",          0, 0, 0, FL_MENU_RADIO | ( c == 2 ? FL_MENU_VALUE : 0 ) },
                        { "Quad",            0, 0, 0, FL_MENU_RADIO | ( c == 4 ? FL_MENU_VALUE : 0 ) },
                        { "...",             0, 0, 0, FL_MENU_RADIO | ( c == 3 || c > 4 ? FL_MENU_VALUE : 0 ) },
                        { 0                  },
                        { "Add Control"      },
                        { "Add Annotation"   },
                        { "Color"            },
                        { "Remove",          0, 0, 0 }, // transport->rolling ? FL_MENU_INACTIVE : 0 },
                        { 0 },
                    };

                const Fl_Menu_Item *r = menu->popup( X, Y, "Track" );

                if ( r && r > &menu[ 0 ] )
                {
                    if ( r < &menu[ 4 ] )
                    {
                        int c = r - &menu[1];
                        int ca[] = { 1, 2, 4 };

                        configure_inputs( ca[ c ] );
                        configure_outputs( ca[ c ] );
                    }
                    else
                    {
                        if ( r == &menu[ 4 ] )
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
                        else if ( r == &menu[ 6 ] )
                        {
                            new Control_Sequence( this );
                        }
                        else if ( r == &menu[ 7 ] )
                        {
                            add( new Annotation_Sequence( this ) );
                        }
                        else if ( r == &menu[ 8 ] )
                        {
                            unsigned char r, g, b;

                            Fl::get_color( color(), r, g, b );

                            if ( fl_color_chooser( "Track Color", r, g, b ) )
                            {
                                color( fl_rgb_color( r, g, b ) );
                            }

//                            color( fl_show_colormap( color() ) );
                            redraw();
                        }
                        else if ( r == &menu[ 9 ] )
                        {
                            int r = fl_choice( "Are you certain you want to remove track \"%s\"?", "Cancel", NULL, "Remove", name() );

                            if ( r == 2 )
                            {
                                timeline->remove_track( this );
                                Fl::delete_widget( this );
                            }
                        }
                    }
                    return 1;
                }

                return 1;
            }

            return 0;
        }
        default:
            return Fl_Group::handle( m );
    }

    return 0;
}
