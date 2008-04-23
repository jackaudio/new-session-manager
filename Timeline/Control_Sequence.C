
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

#include "Control_Sequence.H"
#include "Track.H"

bool Control_Sequence::draw_with_gradient = true;
bool Control_Sequence::draw_with_polygon = true;
bool Control_Sequence::draw_with_grid = true;

Control_Sequence::Control_Sequence ( Track *track ) : Sequence( 0, 0, 0, 0 )
{
    init();

    _track = track;

    if ( track )
        track->add( this );

    log_create();
}


Control_Sequence::~Control_Sequence ( )
{
    log_destroy();
}

void
Control_Sequence::init ( void )
{
    _track = NULL;

    color( fl_darker( FL_GREEN ) );
}

void
Control_Sequence::get ( Log_Entry &e )
{
    Sequence::get( e );

    e.add( ":t", _track );
}

void
Control_Sequence::set ( Log_Entry &e )
{
    Sequence::set( e );

    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( ":t", s ) )
        {
            int i;
            sscanf( v, "%X", &i );
            Track *t = (Track*)Loggable::find( i );

            assert( t );

            t->add( this );
        }
    }
}

void
Control_Sequence::draw ( void )
{

    if ( ! fl_not_clipped( x(), y(), w(), h() ) )
        return;

    fl_push_clip( x(), y(), w(), h() );

    /* draw the box with the ends cut off. */
    draw_box( box(), x() - Fl::box_dx( box() ), y(), w() + Fl::box_dw( box() ), h(), color() );


    const int bx = x();
    const int by = y() + Fl::box_dy( box() );
    const int bw = w();
    const int bh = h() - Fl::box_dh( box() );

    int X, Y, W, H;

    fl_clip_box( bx, by, bw, bh, X, Y, W, H );

    if ( draw_with_gradient )
    {
        Fl_Color target = fl_color_average( color(), FL_WHITE, 0.50f );

        for ( int gy = 0; gy < bh; gy++ )
        {
            fl_color( fl_color_average( target, selection_color(), gy / (float)bh) );
            fl_line( X, by + gy, X + W, by + gy );
        }
    }

    if ( draw_with_grid )
    {
        fl_color( fl_darker( color() ) );

        const int inc = bh / 10;
        if ( inc )
            for ( int gy = 0; gy < bh; gy += inc )
                fl_line( X, by + gy, X + W, by + gy );

    }

    fl_color( fl_color_average( selection_color(), color(), 0.90f ) );
    fl_line_style( FL_SOLID, 4 );

    if ( draw_with_polygon )
        fl_begin_complex_polygon();
    else
        fl_begin_line();


    list <Sequence_Widget *>::const_iterator e = _widgets.end();
    e--;

    if ( _widgets.size() )
        for ( list <Sequence_Widget *>::const_iterator r = _widgets.begin(); ; r++ )
        {
            const int ry = (*r)->y();

            if ( r == _widgets.begin() )
            {
                if ( draw_with_gradient )
                {
                    fl_vertex( bx, by );
                    fl_vertex( bx, ry );
                }
                else
                {
                    fl_vertex( bx, bh + by );
                    fl_vertex( bx, ry );
                }
            }

            fl_vertex( (*r)->x(), ry );

            if ( r == e )
            {
                if ( draw_with_gradient )
                {
                    fl_vertex( bx + bw, ry );
                    fl_vertex( bx + bw, by );
                }
                else
                {
                    fl_vertex( bx + bw, ry );
                    fl_vertex( bx + bw, bh + by );
                }
                break;
            }

        }

    if ( draw_with_polygon )
        fl_end_complex_polygon();
    else
        fl_end_line();

    fl_line_style( FL_SOLID, 0 );

    timeline->draw_measure_lines( x(), y(), w(), h(), color() );

    for ( list <Sequence_Widget *>::const_iterator r = _widgets.begin();  r != _widgets.end(); r++ )
        (*r)->draw_box();

    fl_pop_clip();
}

int
Control_Sequence::handle ( int m )
{
    int r = Sequence::handle( m );

    if ( r )
        return r;

    switch ( m )
    {
        case FL_PUSH:
        {
            if ( Fl::event_button1() )
            {
                Control_Point *r = new Control_Point( this, timeline->xoffset + timeline->x_to_ts( Fl::event_x() - x() ), (float)(Fl::event_y() - y()) / h() );

                add( r );
            }

            return 1;
        }
        default:
            return 0;
    }
}
