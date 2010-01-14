
/*******************************************************************************/
/* Copyright (C) 2009 Jonathan Moore Liles                                     */
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

/* wrap a widget in a group as wide as the widget's label. This is
 * useful when you want to put labeled widgets into a pack */

#include <FL/Fl_Widget.H>
#include <FL/Fl_Group.H>
#include <FL/fl_draw.H>

class Fl_Labelpad_Group : public Fl_Group
{
public:

    static void measure_label ( Fl_Widget *o, int &W, int &H )
        {
            W = fl_width( o->label() );
            H = fl_height();
        }

    Fl_Labelpad_Group ( Fl_Widget *o ) : Fl_Group( 0, 0, 50, 50, 0 )
        {
            resizable( 0 );

            end();

            add( o );

            fl_font( o->labelfont(), o->labelsize() );

            int W, H;

            measure_label( o, W, H );

            // set size to contain widget
            size( W > o->w() ? W : o->w(), o->h() + H );

            // center widget in group
            if ( o->align() & FL_ALIGN_TOP )
                o->position( x() + w() / 2 - (o->w() / 2 ), y() + H );
            else if ( o->align() & FL_ALIGN_BOTTOM )
                o->position( x() + w() / 2 - (o->w() / 2 ), y() );
            else
            {
                /* TODO: other alignments */
            }

        }

    virtual ~Fl_Labelpad_Group ( )
        {
        }
};
