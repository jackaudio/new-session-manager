
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


#include "Timeline.H"
#include "Tempo_Track.H"
#include "Time_Track.H"
#include "Audio_Track.H"

Timeline::Timeline ( int X, int Y, int W, int H, const char* L ) : Fl_Group( X, Y, W, H, L )
{

    xoffset = 0;

    {
        Fl_Pack *o = new Fl_Pack( 0, 0, 800, 600, "rulers" );
        o->type( Fl_Pack::VERTICAL );

        {
            Tempo_Track *o = new Tempo_Track( 0, 0, 800, 24 );

            o->color( FL_RED );

            o->add( new Tempo_Point( 0, 120 ) );
            o->add( new Tempo_Point( 56000, 250 ) );


            tempo_track = o;
            o->end();

        }

        {
            Time_Track *o = new Time_Track( 0, 24, 800, 24 );

            o->color( fl_color_average( FL_RED, FL_WHITE, 0.50f ) );

            o->add( new Time_Point( 0, 4, 4 ) );
            o->add( new Time_Point( 345344, 6, 8 ) );

            time_track = o;
            o->end();

        }

        rulers = o;
        o->end();
    }

    {
        Fl_Scroll *o = new Fl_Scroll( 0, 24 * 2, 800, 600 - (24 * 3) );
        o->type( Fl_Scroll::VERTICAL_ALWAYS );

        sample_rate = 44100;
        fpp = 256;
        _beats_per_minute = 120;
        length = sample_rate * 60 * 2;

        {
            Fl_Pack *o = new Fl_Pack( 0, 0, 800, 5000 );
            o->type( Fl_Pack::VERTICAL );
            o->spacing( 10 );

            Track *l = NULL;
            for ( int i = 6; i--;  )
            {

                Track *o = new Audio_Track( 0, 0, 800, 100 );
                o->prev( l );
                if ( l )
                    l->next( o );
                l = o;
                o->end();
            }

            tracks = o;
            o->end();
        }

        scroll = o;
        o->end();
    }

    redraw();

    end();
}


float
Timeline::beats_per_minute ( nframes_t when ) const
{
    return tempo_track->beats_per_minute( when );
}

void
Timeline::beats_per_minute ( nframes_t when, float bpm )
{
    tempo_track->add( new Tempo_Point( when, bpm ) );
}

/* draw appropriate measure lines inside the given bounding box */
void
Timeline::draw_measure_lines ( int X, int Y, int W, int H, Fl_Color color )
{
    fl_line_style( FL_DASH, 2 );
    fl_color( fl_color_average( FL_BLACK, color, 0.65f ) );

//            int measure = ts_to_x( sample_rate * 60 / beats_per_minute() );

    int measure;

    for ( int x = X; x < X + W; ++x )
    {
        measure = ts_to_x( (double)(sample_rate * 60) / beats_per_minute( x_to_ts( x ) + xoffset ));

        /* don't bother with lines this close together */
        if ( measure < 4 )
            break;

        if ( 0 == (ts_to_x( xoffset ) + x) % measure )
            fl_line( x, Y, x, Y + H );

    }
    fl_line_style( FL_SOLID, 0 );
}
