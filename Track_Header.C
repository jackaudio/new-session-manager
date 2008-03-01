
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

Track_Header::Track_Header ( int X, int Y, int W, int H, const char *L ) :
    Fl_Group ( X, Y, W, H, L )
{
    Track_Header *o = this;
    o->box( FL_THIN_UP_BOX );
    {
        Fl_Group *o = new Fl_Group( 2, 2, 149, 113 );
        o->color( ( Fl_Color ) 53 );
        {
            Fl_Group *o = new Fl_Group( 4, 4, 144, 66 );
            {
                Fl_Input *o = name_field = new Fl_Input( 4, 9, 144, 25 );
                o->color( ( Fl_Color ) 33 );
                o->labeltype( FL_NO_LABEL );
                o->labelcolor( FL_GRAY0 );
                o->textcolor( 32 );
            }
            {
                Fl_Button *o = record_button =
                    new Fl_Button( 6, 38, 26, 27, "@circle" );
                o->type( 1 );
                o->box( FL_ROUNDED_BOX );
                o->color( FL_LIGHT1 );
                o->labelsize( 8 );
            }
            {
                Fl_Button *o = mute_button =
                    new Fl_Button( 35, 38, 26, 27, "m" );
                o->type( 1 );
                o->box( FL_ROUNDED_BOX );
                o->color( FL_LIGHT1 );
                o->labelsize( 11 );
            }
            {
                Fl_Button *o = solo_button =
                    new Fl_Button( 66, 38, 26, 27, "s" );
                o->type( 1 );
                o->box( FL_ROUNDED_BOX );
                o->color( FL_LIGHT1 );
                o->labelsize( 11 );
            }
            {
                Fl_Menu_Button *o = take_menu =
                    new Fl_Menu_Button( 97, 38, 47, 27, "T" );
                o->box( FL_ROUNDED_BOX );
                o->color( FL_LIGHT1 );
                o->align( FL_ALIGN_LEFT | FL_ALIGN_INSIDE );
            }
            o->end();
        }
        o->size( Track_Header::width(), o->h() );
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
