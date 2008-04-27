
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

#include "Sequence_Widget.H"

class Sequence_Point : public Sequence_Widget
{

protected:

    char *_label;

    virtual void get ( Log_Entry &e ) const
        {
            e.add( ":x", _r->offset );
            e.add( ":t", _track );
        }

    virtual void set ( Log_Entry &e )
        {
            Sequence_Widget::set( e );

            for ( int i = 0; i < e.size(); ++i )
            {
                const char *s, *v;

                e.get( i, &s, &v );

                if ( ! strcmp( ":x", s ) )
                {
                    _track->sort();
                }

            }
        }

public:

    Fl_Align align ( void ) const { return FL_ALIGN_RIGHT; }
    virtual int abs_w ( void ) const { return 10; }
    nframes_t length ( void ) const { return timeline->x_to_ts( abs_w() ); }

    Sequence_Point ( )
        {
            _label = NULL;
        }


    virtual ~Sequence_Point ( )
        {
        }

    virtual void
    draw ( void )
        {
            Sequence_Widget::draw();

            draw_label( _label, align() );
        }
};
