
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

#pragma once

#include "Sequence.H"
#include "Annotation_Point.H"
#include "Annotation_Region.H"
#include "Timeline.H"

#include "Track.H"

class Annotation_Sequence : public Sequence
{

protected:

    virtual void get ( Log_Entry &e ) const
        {
            e.add( ":track", _track );
        }

    void
    set ( Log_Entry &e )
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
            }
        }

    Annotation_Sequence ( ) : Sequence ( 0 )
        {
            color( fl_darker( FL_GREEN ) );
        }

public:


    LOG_CREATE_FUNC( Annotation_Sequence );

    Fl_Cursor cursor ( void ) const { return FL_CURSOR_INSERT; }

    Annotation_Sequence ( Track *track ) : Sequence( track )
        {
            color( fl_darker(  FL_GREEN ) );

            log_create();
        }

/*     Annotation_Sequence ( int X, int Y, int W, int H ) : Sequence ( 0 ) */
/*         { */
/*         } */

    ~Annotation_Sequence ( )
        {
            Loggable::block_start();

            clear();

            log_destroy();

            track()->remove( this );

            Loggable::block_end();
        }

    int handle ( int m )
        {

            if ( Sequence::handle( m ) )
                return 1;

            switch ( m )
            {
                case FL_PUSH:
                {
                    Logger log( this );

                    if ( Fl::event_button1() )
                    {
                        add( new Annotation_Point( this, x_to_offset( Fl::event_x() ), "mark" ) );
                        redraw();
                    }
                    if ( Fl::event_button3() && Fl::event_shift() )
                    {
                        Annotation_Region *r = new Annotation_Region( this, x_to_offset( Fl::event_x() ), "mark" );

                        add( r );

                        Sequence_Widget::pushed( r );

                        r->handle( m );

                        redraw();

                        return 1;

                    }
                    else if ( Fl::event_button3() && ! ( Fl::event_state() & ( FL_ALT | FL_SHIFT | FL_CTRL ) ) )
                    {

                        Fl_Menu_Item menu[] =
                            {
                                { "Remove" },
                                { 0 }
                            };

                        const Fl_Menu_Item *r = menu->popup( Fl::event_x(), Fl::event_y(), "Annotation Sequence" );

                        if ( r )
                        {
                            if ( r == &menu[ 0 ] )
                            {
                                Fl::delete_widget( this );
                            }

                        }

                        return 1;
                    }
                    break;
                }
                default:
                    break;
            }

            return 0;
        }
};
