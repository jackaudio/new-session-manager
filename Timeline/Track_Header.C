
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

#include "Track_Header.H"

#include "Disk_Stream.H"
#include "Engine.H"

void
Track_Header::cb_input_field ( Fl_Widget *w, void *v )
{
    ((Track_Header*)v)->cb_input_field();
}

void
Track_Header::cb_button ( Fl_Widget *w, void *v )
{
    ((Track_Header*)v)->cb_button( w );
}


void
Track_Header::cb_input_field ( void )
{
    log_start();

    if ( _name )
        free( _name );

    _name = strdup( name_field->value() );

    log_end();
}

void
Track_Header::cb_button ( Fl_Widget *w )
{

    printf( "FIXME: inform mixer here\n" );
    if ( w == record_button )
    {


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
                    track( track()->clone_empty() );
                    return;
            }

            const char *s = take_menu->menu()[ v ].text;

            for ( int i = takes->children(); i--; )
            {
                Track *t = (Track*)takes->child( i );
                if ( ! strcmp( s, t->name() ) )
                {
                    track( t );
                    redraw();
                    break;
                }
            }
        }
}

#include "Port.H"

Track_Header::Track_Header ( int X, int Y, int W, int H, const char *L ) :
    Fl_Group ( X, Y, W, H, L )
{

    _track = NULL;
    _name = NULL;
    _selected = false;
    _show_all_takes = false;
    _size = 1;

    {
        char pname[40];
        static int n = 0;
        snprintf( pname, sizeof( pname ), "out-%d", n++ );

        output.push_back( Port( strdup( pname ) ) );
    }

    diskstream = new Disk_Stream( this, engine->frame_rate(), engine->nframes(), 1 );

    Fl_Group::size( w(), height() );

    Track_Header *o = this;
    o->box( FL_THIN_UP_BOX );
    {
        Fl_Group *o = new Fl_Group( 2, 2, 149, 70 );
        o->color( ( Fl_Color ) 53 );
        {
            Fl_Input *o = name_field = new Fl_Input( 2, 2, 144, 24 );
            o->color( ( Fl_Color ) 33 );
            o->labeltype( FL_NO_LABEL );
            o->labelcolor( FL_GRAY0 );
            o->textcolor( 32 );

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
            Fl_Box *o = new Fl_Box( 0, 76, 149, 38 );
            o->box( FL_FLAT_BOX );
            Fl_Group::current()->resizable( o );
        }

        o->size( Track_Header::width(), h() );
        o->end();
    }
    {
        Fl_Pack *o = pack = new Fl_Pack( width(), 0, 1006, 115 );
        o->labeltype( FL_NO_LABEL );
        o->resize( x() + width(), y(), w() - width(), h() );
        Fl_Group::current()->resizable( o );

        {
            Fl_Pack *o = control = new Fl_Pack( width(), 0, pack->w(), 115 );
            o->end();
        }

        {
            Fl_Pack *o = takes = new Fl_Pack( width(), 0, pack->w(), 115 );
            o->end();
            o->hide();
        }

        o->end();
    }
    end();

    log_create();
}

Track_Header::~Track_Header ( )
{
    log_destroy();
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
Track_Header::resize ( void )
{
    for ( int i = takes->children(); i--; )
        takes->child( i )->size( w(), height()  );

    for ( int i = control->children(); i--; )
        control->child( i )->size( w(), height()  );

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

    if ( track() )
        track()->size( w(), height() );


    if ( controls->y() + controls->h() > y() + h() )
        controls->hide();
    else
        controls->show();

    parent()->redraw();
}

void
Track_Header::size ( int v )
{
    if ( v < 0 || v > 3 )
        return;

    _size = v;

    resize();
}



void
Track_Header::track( Track * t )
{
//    t->size( 1, h() );
    if ( track() )
        add( track() );

//        takes->insert( *track(), 0 );

    _track = t;
    pack->insert( *t, 0 );

    resize();
}

void
Track_Header::add_control( Track *t )
{
    control->add( t );

    resize();
}


/**********/
/* Engine */
/**********/

/* THREAD: RT */
nframes_t
Track_Header::process ( nframes_t nframes )
{
    if ( diskstream )
        return diskstream->process( nframes );
    else
        return 0;
}

/* THREAD: RT */
void
Track_Header::seek ( nframes_t frame )
{
    if ( diskstream )
        return diskstream->seek( frame );
}
