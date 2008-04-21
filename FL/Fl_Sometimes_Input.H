
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



/* Just like an Fl_Input, except that when not being edited it
 * displays just like a label. */

class Fl_Sometimes_Input : public Fl_Input
{

    bool _editing;

public:

    Fl_Sometimes_Input ( int X, int Y, int W, int H, const char *L=0 )
        : Fl_Input( X, Y, W, H, L )
        {
            _editing = false;
        }

    virtual void
    draw ( void )
        {
            if ( _editing )
                Fl_Input::draw();
            else
            {
                fl_color( FL_WHITE /* textcolor() */ );
                fl_font( textfont(), textsize() );
                fl_draw( value(), x(), y(), w(), h(), FL_ALIGN_CENTER );
            }
        }

    virtual int
    handle ( int m )
        {
            int r = Fl_Input::handle( m );

            switch ( m )
            {
                case FL_FOCUS:
                    _editing = true;
                    redraw();
                    return 1;
                case FL_PUSH:
                    _editing = true;
                    redraw();
                    return r;
                case FL_UNFOCUS:
                    _editing = false;
                    {
                        /* lame, but we have to find the next parent
                         * with a box in order to clear the background
                         * properly */

                        Fl_Group *g = parent();

                        for ( ; g; g = g->parent() )
                        {
                            switch ( g->box() )
                            {
                                case FL_NO_BOX:
                                case FL_UP_FRAME:
                                case FL_DOWN_FRAME:
                                case FL_THIN_UP_FRAME:
                                case FL_THIN_DOWN_FRAME:
                                case FL_ENGRAVED_FRAME:
                                case FL_EMBOSSED_FRAME:
//                                case FL_SHADOW_FRAME:
//                                case FL_PLASTIC_UP_FRAME:
//                                case FL_PLASTIC_DOWN_FRAME:
//                                case FL_ROUNDED_FRAME:
//                                case FL_OVAL_FRAME:
                                    continue;
                                default:
                                    break;
                            }
                            break;
                        }

                        if ( g )
                        {
                            printf( "damaging parent\n" );
                            g->damage( FL_DAMAGE_EXPOSE, x(), y(), w(), h() );
                        }
                    }
                    redraw();
                    return r;
                default:
                    return r;
            }
        }

};
