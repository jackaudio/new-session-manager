
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
#include "Sequence_Widget.H"

/* Base class representing a /region/ of time on a /sequence/, with
   controls for the start and end points. */

class Sequence_Region : public Sequence_Widget
{

protected:

    virtual void get ( Log_Entry &e ) const
        {
            e.add( ":color",             (int)_box_color  );

            Sequence_Widget::get( e );
        }


    void
    virtual set ( Log_Entry &e )
        {
            for ( int i = 0; i < e.size(); ++i )
            {
                const char *s, *v;

                e.get( i, &s, &v );

                if ( ! strcmp( s, ":color" ) )
                    _box_color = (Fl_Color)atoll( v );
            }

            Sequence_Widget::set( e );
        }

    Sequence_Region ( )
        {
            color( FL_CYAN );
        }

    virtual ~Sequence_Region ( )
        {

        }

public:

    LOG_NAME_FUNC( Region );

    enum trim_e { NO, LEFT, RIGHT };
    void trim ( enum trim_e t, int X );

    virtual int handle ( int m );
    virtual void draw_box( void );
    virtual void draw ( void );

};
