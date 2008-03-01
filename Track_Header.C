
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

void
Track_Header::cb_input_field ( Fl_Widget *w, void *v )
{
    ((Track_Header*)v)->cb_input_field();
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

Track_Header::Track_Header ( int X, int Y, int W, int H, const char *L ) :
    Fl_Group ( X, Y, W, H, L )
{


    _name = NULL;
    _track = NULL;
    _selected = false;
    _size = 3;

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
            }
            {
                Fl_Button *o = mute_button =
                    new Fl_Button( 35, 28, 26, 24, "m" );
                o->type( 1 );
                o->box( FL_THIN_UP_BOX );
                o->color( FL_LIGHT1 );
                o->labelsize( 11 );
            }
            {
                Fl_Button *o = solo_button =
                    new Fl_Button( 66, 28, 26, 24, "s" );
                o->type( 1 );
                o->box( FL_THIN_UP_BOX );
                o->color( FL_LIGHT1 );
                o->labelsize( 11 );
            }
            {
                Fl_Menu_Button *o = take_menu =
                    new Fl_Menu_Button( 97, 28, 47, 24, "T" );
                o->box( FL_THIN_UP_BOX );
                o->color( FL_LIGHT1 );
                o->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE );
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
        Fl_Pack *o = takes = new Fl_Pack( 150, 0, 1006, 115 );
        o->labeltype( FL_NO_LABEL );
        o->align( FL_ALIGN_CLIP );
        o->resize( x() + width(), y(), w() - width(), h() );
        o->end();
        Fl_Group::current()->resizable( o );
    }
    end();

    log_create();
}

Track_Header::~Track_Header ( )
{
    log_destroy();
}

int
Track_Header::width()
{
    return 150;
}

void
Track_Header::track( Track * t )
{
    _track = t;

    t->size( 1, takes->h() );
    takes->add( t );
}
