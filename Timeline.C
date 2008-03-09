
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
#include "Control_Track.H"
#include <FL/Fl_Scrollbar.H>

#include "Track_Header.H"

void
Timeline::cb_scroll ( Fl_Widget *w, void *v )
{
    ((Timeline*)v)->cb_scroll( w );
}

void
Timeline::cb_scroll ( Fl_Widget *w )
{
    if ( w == vscroll )
    {
        tracks->position( tracks->x(), (rulers->y() + rulers->h()) - vscroll->value() );

        yposition( vscroll->value() );

        int rh = h() - rulers->h();

        vscroll->value( vscroll->value(), 30, 0, max( tracks->h() - rh, rh) );
    }
    else
    {
        if ( hscroll->zoom_changed() )
        {
            _fpp = hscroll->zoom() * 1;

            int maxx = ts_to_x( _length );
            hscroll->range( 0, maxx );

            redraw();
        }
        else
        {
            xposition( hscroll->value() );
        }
    }
}


Timeline::Timeline ( int X, int Y, int W, int H, const char* L ) : Fl_Overlay_Window( X, Y, W, H, L )
{

    box( FL_FLAT_BOX );
    xoffset = 0;

    _enable_measure_lines = true;

    X = Y = 0;
    {
        Scalebar *o = new Scalebar( X, Y + H - 18, W - 18, 18 );

        o->range( 0, 48000 * 300 );
        o->zoom_range( 2, 8192 );
        o->zoom( 256 );
        o->type( FL_HORIZONTAL );
        o->callback( cb_scroll, this );

        hscroll = o;
    }

    {
        Fl_Scrollbar *o = new Fl_Scrollbar( X + W - 18, Y, 18, H - 18 );

        o->type( FL_VERTICAL );
//        o->step( 10 );
        o->callback( cb_scroll, this );
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

        _sample_rate = 44100;
        _fpp = 256;
        _length = _sample_rate * 60 * 2;

        {
            Fl_Pack *o = new Fl_Pack( X, rulers->y() + rulers->h(), W - vscroll->w(), 5000 );
            o->type( Fl_Pack::VERTICAL );
            o->spacing( 0 );

            for ( int i = 8; i--;  )
            {
//                Track_Header *t = new Track_Header( 0, 0, W, 75 );
                Track_Header *t = new Track_Header( 0, 0, W, 30 );
                Track *o = new Audio_Track( 0, 0, 1, 100 );

                t->track( o );
                t->add( new Audio_Track( 0, 0, 1, 100 ) );
                t->add( new Audio_Track( 0, 0, 1, 100 ) );
                t->add_control( new Control_Track( 0, 0, 1, 100 ) );
                t->color( (Fl_Color)rand() );
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
        const int measure = ts_to_x( (double)(_sample_rate * 60) / beats_per_minute( x_to_ts( x - Track_Header::width() ) + xoffset ));

//        const int abs_x = ts_to_x( xoffset ) + x;

        if ( 0 == x % measure )
            return x;
    }

    return -1;
}

/** draw appropriate measure lines inside the given bounding box */
/* FIXME: this function *really* needs to be optimized. Currently it
   searched both the time and tempo lists once for every horiontal
   pixel and performs a number of calculations--this is slow. */
void
Timeline::draw_measure_lines ( int X, int Y, int W, int H, Fl_Color color )
{
    if ( ! _enable_measure_lines )
        return;

//    fl_line_style( FL_DASH, 2 );
    fl_line_style( FL_DASH, 0 );

     Fl_Color beat = fl_color_average( FL_BLACK, color, 0.65f );
     Fl_Color bar  = fl_color_average( FL_RED, color, 0.65f );

    int measure;

    for ( int x = X; x < X + W; ++x )
    {
        measure = ts_to_x( (double)(_sample_rate * 60) / beats_per_minute( x_to_ts( x - Track_Header::width() ) + xoffset ));

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
Timeline::xposition ( int X )
{
//    _old_xposition = xoffset;

    xoffset = x_to_ts( X );

    damage( FL_DAMAGE_SCROLL );
}

void
Timeline::yposition ( int Y )
{
//    _old_yposition = _yposition;

    _yposition = Y;

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

    if ( (damage() & FL_DAMAGE_ALL)
         ||
         damage() & FL_DAMAGE_EXPOSE )
    {
        draw_box( box(), 0, 0, w(), h(), color() );

        fl_push_clip( 0, rulers->y(), w(), rulers->h() );
        draw_child( *rulers );
        fl_pop_clip();

        fl_push_clip( tracks->x(), rulers->y() + rulers->h(), tracks->w(), hscroll->y() - (rulers->y() + rulers->h()) );
        draw_child( *tracks );
        fl_pop_clip();

        draw_child( *hscroll );
        draw_child( *vscroll );

        redraw_overlay();
        return;
    }

    if ( damage() & FL_DAMAGE_CHILD )
    {
//        draw_box( box(), 0, 0, w(), h(), color() );

        fl_push_clip( rulers->x(), rulers->y(), rulers->w() - vscroll->w(), rulers->h() );
        update_child( *rulers );
        fl_pop_clip();

        fl_push_clip( tracks->x(), rulers->y() + rulers->h(), tracks->w(), hscroll->y() - (rulers->y() + rulers->h()) );
        update_child( *tracks );
        fl_pop_clip();

        update_child( *hscroll );
        update_child( *vscroll );
    }


    if ( damage() & FL_DAMAGE_SCROLL )
    {
        int dx = ts_to_x( _old_xposition ) - ts_to_x( xoffset );
        int dy = _old_yposition - _yposition;

        if ( ! dy )
            fl_scroll( X + Track_Header::width(), rulers->y(), rulers->w() - Fl::box_dw( rulers->child(0)->box() ), rulers->h(), dx, 0, draw_clip, this );

        Y = rulers->y() + rulers->h();
        H = h() - rulers->h() - hscroll->h();

        if ( dy == 0 )
            fl_scroll( X + Track_Header::width(), Y, W - Track_Header::width(), H, dx, dy, draw_clip, this );
        else
            fl_scroll( X, Y, W, H, dx, dy, draw_clip, this );

        _old_xposition = xoffset;
        _old_yposition = _yposition;

    }
}

void
Timeline::draw_overlay ( void )
{

    fl_push_clip( tracks->x() + Track_Header::width(), rulers->y() + rulers->h(),  tracks->w() - Track_Header::width(), h() - rulers->h() - hscroll->h() );

    const Rectangle &r = _selection;

    fl_color( FL_BLACK );

    fl_line_style( FL_SOLID, 2 );

    fl_rect( r.x + 2, r.y + 2, r.w, r.h );
    fl_color( FL_MAGENTA );
    fl_line_style( FL_DASH, 2 );
    fl_rect( r.x, r.y, r.w, r.h );

    fl_line( r.x, r.y, r.x + r.w, r.y + r.h );

    fl_line( r.x + r.w, r.y, r.x, r.y + r.h );

/*     fl_overlay_rect( r.x, r.y, r.w, r.h ); */

    fl_line_style( FL_SOLID, 0 );


    fl_pop_clip();


}

// #include "Track_Widget.H"

/** select all widgets in inside rectangle /r/ */
void
Timeline::select( const Rectangle &r )
{
    const int Y = r.y;

    for ( int i = tracks->children(); i-- ; )
    {
        Track_Header *t = (Track_Header*)tracks->child( i );

        if ( ! ( t->y() > Y + r.h || t->y() + t->h() < Y ) )
            t->track()->select_range( r.x, r.w );
    }
}

int
Timeline::handle ( int m )
{
    static Drag *drag = NULL;

    switch ( m )
    {
        case FL_KEYBOARD:
        {
            switch ( Fl::event_key() )
            {
                case FL_Delete:
                {
                    Track_Widget::delete_selected();

                    return 1;
                }
            }

            return 0;
        }
        default:
        {
            int r = Fl_Overlay_Window::handle( m );

            if ( m != FL_RELEASE && r )
                return r;

            const int X = Fl::event_x();
            const int Y = Fl::event_y();

            switch ( m )
            {
                case FL_PUSH:
                {
                    if ( ! Fl::event_button1() )
                        return 0;

                    assert( ! drag );

                    drag = new Drag( X - x(), Y - y() );
                    _selection.x = drag->x;
                    _selection.y = drag->y;
                    break;
                }
                case FL_DRAG:
                {
                    int ox = X - drag->x;
                    int oy = Y - drag->y;

                    if ( ox < 0 )
                        _selection.x = X;
                    if ( oy < 0 )
                        _selection.y = Y;

                    _selection.w = abs( ox );
                    _selection.h = abs( oy );
                    break;
                }
                case FL_RELEASE:
                {
                    delete drag;
                    drag = NULL;

                    select( _selection );

                    _selection.w = _selection.h = 0;
                    break;
                }
                default:
                    return 0;
                    break;
            }

            redraw_overlay();
            return 1;
        }
    }

}
