
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

#include <FL/Fl_Scrollbar.H>

#include "Track_Header.H"

void
cb_hscroll ( Fl_Widget *w, void *v )
{
    Scalebar *sb = (Scalebar*)w;

    if ( sb->zoom_changed() )
    {
        timeline->fpp = sb->zoom() * 1;

        int maxx = timeline->ts_to_x( timeline->length );
        sb->range( 0, maxx );

        timeline->redraw();
    }
    else
    {
        timeline->position( sb->value() );
    }

    printf( "%lu\n", timeline->xoffset );
}

void
cb_vscroll ( Fl_Widget *w, void *v )
{
    Fl_Scrollbar *sb = (Fl_Scrollbar*)w;

    timeline->tracks->position( timeline->tracks->x(), (timeline->rulers->y() + timeline->rulers->h()) - sb->value() );
    timeline->yposition = sb->value();

//    timeline->vscroll->range( 0, timeline->tracks->h() - timeline->h() - timeline->rulers->h() );

    sb->value( sb->value(), 30, 0, min( timeline->tracks->h(),  timeline->tracks->h() - timeline->h() - timeline->rulers->h() ) );

    timeline->damage( FL_DAMAGE_SCROLL );
}


Timeline::Timeline ( int X, int Y, int W, int H, const char* L ) : Fl_Group( X, Y, W, H, L )
{

    box( FL_FLAT_BOX );
    xoffset = 0;

    {
        Scalebar *o = new Scalebar( X, Y + H - 18, W - 18, 18 );

        o->range( 0, 48000 * 2 );
        o->zoom_range( 2, 8192 );
        o->zoom( 256 );
        o->type( FL_HORIZONTAL );
        o->callback( cb_hscroll, 0 );

        hscroll = o;
    }

    {
        Fl_Scrollbar *o = new Fl_Scrollbar( X + W - 18, Y, 18, H - 18 );

        o->type( FL_VERTICAL );
//        o->step( 10 );
        o->callback( cb_vscroll, 0 );
        vscroll = o;
    }

    {
        Fl_Pack *o = new Fl_Pack( X + Track_Header::width(), Y, (W - Track_Header::width()) - vscroll->w(), H - hscroll->h(), "rulers" );
        o->type( Fl_Pack::VERTICAL );

        {
            Tempo_Track *o = new Tempo_Track( 0, 0, 800, 24 );

            o->color( FL_RED );

            o->add( new Tempo_Point( 0, 120 ) );
            o->add( new Tempo_Point( 56000, 250 ) );

            o->label( "Tempo" );
            o->align( FL_ALIGN_LEFT );

            tempo_track = o;
            o->end();

        }

        {
            Time_Track *o = new Time_Track( 0, 24, 800, 24 );

            o->color( fl_color_average( FL_RED, FL_WHITE, 0.50f ) );

            o->add( new Time_Point( 0, 4, 4 ) );
            o->add( new Time_Point( 345344, 6, 8 ) );

            o->label( "Time" );
            o->align( FL_ALIGN_LEFT );

            time_track = o;
            o->end();

        }

        o->size( o->w(), o->child( 0 )->h() * o->children() );
        rulers = o;
        o->end();
    }


    {

/*         Fl_Scroll *o = new Fl_Scroll( 0, 24 * 2, 800, 600 - (24 * 3) ); */
/*         o->type( Fl_Scroll::VERTICAL_ALWAYS ); */

        sample_rate = 44100;
        fpp = 256;
        _beats_per_minute = 120;
        length = sample_rate * 60 * 2;

        {
            Fl_Pack *o = new Fl_Pack( X, rulers->y() + rulers->h(), W - vscroll->w(), 5000 );
            o->type( Fl_Pack::VERTICAL );
            o->spacing( 5 );

            Track *l = NULL;
            for ( int i = 16; i--;  )
            {
                Track_Header *t = new Track_Header( 0, 0, W, 75 );
                Track *o = new Audio_Track( 0, 0, 1, 100 );
                o->prev( l );
                if ( l )
                    l->next( o );
                l = o;
//                o->end();

                t->track( o );
                t->color( FL_GREEN );
            }

            tracks = o;
            o->end();
        }

/*         scroll = o; */
/*         o->end(); */

    }

    /* make sure scrollbars are on top */
    add( vscroll );
    add( hscroll );

    vscroll->range( 0, tracks->h() );

    redraw();

    end();
}


float
Timeline::beats_per_minute ( nframes_t when ) const
{
    return tempo_track->beats_per_minute( when );
}

int
Timeline::beats_per_bar ( nframes_t when ) const
{
    time_sig t = time_track->time( when );

    return t.beats_per_bar;
}

void
Timeline::beats_per_minute ( nframes_t when, float bpm )
{
    tempo_track->add( new Tempo_Point( when, bpm ) );
}

/** return the absolute pixel of the nearest measure line to /x/ */
int
Timeline::nearest_line ( int ix )
{
    for ( int x = ix - 10; x < ix + 10; ++x )
    {
        const int measure = ts_to_x( (double)(sample_rate * 60) / beats_per_minute( x_to_ts( x - Track_Header::width() ) + xoffset ));

//        const int abs_x = ts_to_x( xoffset ) + x;

        if ( 0 == x % measure )
            return x;
    }

    return -1;
}

/* draw appropriate measure lines inside the given bounding box */
void
Timeline::draw_measure_lines ( int X, int Y, int W, int H, Fl_Color color )
{
    fl_line_style( FL_DASH, 2 );

    Fl_Color beat = fl_color_average( FL_BLACK, color, 0.65f );
    Fl_Color bar  = fl_color_average( FL_RED, color, 0.65f );

//            int measure = ts_to_x( sample_rate * 60 / beats_per_minute() );

    int measure;

    for ( int x = X; x < X + W; ++x )
    {
        measure = ts_to_x( (double)(sample_rate * 60) / beats_per_minute( x_to_ts( x - Track_Header::width() ) + xoffset ));

        const int abs_x = ts_to_x( xoffset ) + x - Track_Header::width();

        if ( 0 == abs_x % measure )
        {
            int bpb = beats_per_bar( x_to_ts( x -Track_Header::width() ) + xoffset );

            if ( 0 == (abs_x / measure) % bpb )
            {
                if ( measure * bpb < 8 )
                    break;

                fl_color( bar );
            }
            else
            {
                if ( measure < 8 )
                    continue;

                fl_color( beat );
            }


            fl_line( x, Y, x, Y + H );
        }

    }
    fl_line_style( FL_SOLID, 0 );
}


void
Timeline::position ( int X )
{
    _old_xposition = xoffset;

    xoffset = x_to_ts( X );

    damage( FL_DAMAGE_SCROLL );
}

void
Timeline::draw_clip ( void * v, int X, int Y, int W, int H )
{
    Timeline *tl = (Timeline *)v;

//            printf( "draw_clip: %d,%d %dx%d\n", X, Y, W, H );
    fl_push_clip( X, Y, W, H );

    fl_color( rand() );
    fl_rectf( X, Y, X + W, Y + H );


    tl->draw_child( *tl->rulers );

    /* headers */
    fl_push_clip( tl->tracks->x(), tl->rulers->y() + tl->rulers->h(), Track_Header::width(), tl->h() - tl->rulers->h()  - tl->hscroll->h() );
    tl->draw_child( *tl->tracks );
    fl_pop_clip();

    /* track bodies */
    fl_push_clip( tl->tracks->x() + Track_Header::width(), tl->rulers->y() + tl->rulers->h(), tl->tracks->w() - Track_Header::width(), tl->h() - tl->rulers->h()  - tl->hscroll->h() );
    tl->draw_child( *tl->tracks );
    fl_pop_clip();

//    tl->draw_child( *tl->tracks );

    fl_pop_clip();
}


void
Timeline::draw ( void )
{
    int X, Y, W, H;

    X = tracks->x() + Fl::box_dx( tracks->child( 0 )->box() ) + 1;
    Y = tracks->y();
    W = tracks->w() - Fl::box_dw( tracks->child( 0 )->box() ) - 1;
    H = tracks->h();


/*             fl_color( FL_RED ); */
/*             fl_rect( X, Y, X + W, Y + H ); */

    if ( damage() & FL_DAMAGE_ALL )
//                 ( damage() & ( FL_DAMAGE_CHILD | FL_DAMAGE_SCROLL ) ) )
    {

        draw_box( box(), x(), y(), w(), h(), color() );

        fl_push_clip( x(), rulers->y(), w(), rulers->h() );
        draw_child( *rulers );
        fl_pop_clip();

        fl_push_clip( tracks->x(), rulers->y() + rulers->h(), tracks->w(), h() - hscroll->h() );
        draw_child( *tracks );
        fl_pop_clip();

        draw_child( *hscroll );
        draw_child( *vscroll );

        return;
    }

    if ( damage() & FL_DAMAGE_CHILD )
    {

/*         if ( damage() & FL_DAMAGE_SCROLL ) */
/*             fl_push_no_clip(); */

        fl_push_clip( rulers->x(), rulers->y(), rulers->w() - vscroll->w(), rulers->h() );
        update_child( *rulers );
        fl_pop_clip();

        /* headers */
        fl_push_clip( tracks->x(), rulers->y() + rulers->h(), Track_Header::width(), h() - rulers->h()  - hscroll->h() );
        update_child( *tracks );
        fl_pop_clip();

        /* track bodies */
        fl_push_clip( tracks->x() + Track_Header::width(), rulers->y() + rulers->h(), tracks->w() - Track_Header::width(), h() - rulers->h()  - hscroll->h() );
        update_child( *tracks );
        fl_pop_clip();

        update_child( *hscroll );
        update_child( *vscroll );

/*         if ( damage() & FL_DAMAGE_SCROLL ) */
/*             fl_pop_clip(); */

    }


    if ( damage() & FL_DAMAGE_SCROLL )
    {
        int dx = ts_to_x( _old_xposition ) - ts_to_x( xoffset );
        int dy = _old_yposition - yposition;

        if ( ! dy )
            fl_scroll( X + Track_Header::width(), rulers->y(), rulers->w() - Fl::box_dw( rulers->child(0)->box() ), rulers->h(), dx, 0, draw_clip, this );

        Y = rulers->y() + rulers->h();
        H = h() - rulers->h() - hscroll->h();

        if ( dy == 0 )
            fl_scroll( X + Track_Header::width(), Y, W - Track_Header::width(), H, dx, dy, draw_clip, this );
        else
            fl_scroll( X, Y, W, H, dx, dy, draw_clip, this );

        _old_xposition = xoffset;
        _old_yposition = yposition;

    }
}


int
Timeline::handle ( int m )
{
    switch ( m )
    {
        case FL_MOUSEWHEEL:
        {
            if ( hscroll->handle( m ) )
                return 1;

            return vscroll->handle( m );
        }
        default:
            return Fl_Group::handle( m );
    }
}
