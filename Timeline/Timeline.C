
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
#include "Tempo_Sequence.H"
#include "Time_Sequence.H"
#include "Audio_Sequence.H"
#include "Control_Sequence.H"
#include <FL/Fl_Scrollbar.H>

#include "Annotation_Sequence.H"

// #include <FL/Fl_Image.H>
// #include <FL/Fl_RGB_Image.H> // needed for alpha blending

#include "Track.H"

bool Timeline::draw_with_measure_lines = true;
Timeline::snap_e Timeline::snap_to = Bars;
bool Timeline::snap_magnetic = true;
bool Timeline::follow_playhead = true;
bool Timeline::center_playhead = true;

const float UPDATE_FREQ = 0.02f;


#include "Playback_DS.H"
#include "Record_DS.H"

#include "Transport.H"

/** return the combined height of all visible children of (veritcal)
    pack, /p/. This is necessary because pack sizes are adjusted only
    when the relevant areas are exposes. */
static int
pack_visible_height ( const Fl_Pack *p )
{
    int th = 0;

    const Fl_Widget* const *w = p->array();
    for ( int i = p->children(); i--; ++w )
        if ( (*w)->visible() )
            th += (*w)->h() + p->spacing();

    return th;
}

/** recalculate the size of vertical scrolling area and inform scrollbar */
void
Timeline::adjust_vscroll ( void )
{
    vscroll->value( _yposition, h() - rulers->h() - hscroll->h(), 0, pack_visible_height( tracks ) );
}

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

        adjust_vscroll();
    }
    else
    {
        if ( hscroll->zoom_changed() )
        {

            nframes_t under_mouse = x_to_offset( Fl::event_x() );

            _fpp = hscroll->zoom();

            const int tw = tracks->w() - Track::width();
//            hscroll->value( ts_to_x( xoffset ), tw, 0, ts_to_x( _length ) );
            hscroll->value( max( 0, ts_to_x( under_mouse ) - ( Fl::event_x() - tracks->x() - Track::width() ) ),
                            tw, 0, ts_to_x( _length ) );

            redraw();
        }

        xposition( hscroll->value() );
    }
}


Timeline::Timeline ( int X, int Y, int W, int H, const char* L ) : Fl_Overlay_Window( X, Y, W, H, L )
{

    box( FL_FLAT_BOX );
    xoffset = 0;
    _yposition = 0;
    _old_yposition = 0;
    _old_xposition = 0;

    X = Y = 0;

    p1 = p2 = 0;

    {
        Scalebar *o = new Scalebar( X, Y + H - 18, W - 18, 18 );

        o->range( 0, 48000 * 300 );
//        o->zoom_range( 1, 16384 );
//        o->zoom_range( 1, 65536 << 4 );
        o->zoom_range( 1, 20 );

        o->zoom( 8 );

        o->type( FL_HORIZONTAL );
        o->callback( cb_scroll, this );

        hscroll = o;
    }

    {
        Fl_Scrollbar *o = new Fl_Scrollbar( X + W - 18, Y, 18, H - 18 );

        o->type( FL_VERTICAL );
        o->callback( cb_scroll, this );
        vscroll = o;
    }

    {
        Fl_Pack *o = new Fl_Pack( X + Track::width(), Y, (W - Track::width()) - vscroll->w(), H - hscroll->h(), "rulers" );
        o->type( Fl_Pack::VERTICAL );

        {
            Tempo_Sequence *o = new Tempo_Sequence( 0, 0, 800, 24 );

            o->color( fl_gray_ramp( 18 ) );

            o->label( "Tempo" );
            o->align( FL_ALIGN_LEFT );

            tempo_track = o;
        }

        {
            Time_Sequence *o = new Time_Sequence( 0, 24, 800, 24 );

            o->color( fl_gray_ramp( 16 ) );

            o->label( "Time" );
            o->align( FL_ALIGN_LEFT );

            time_track = o;
        }


/*         { */
/*             Annotation_Sequence *o = new Annotation_Sequence( 0, 24, 800, 24 ); */

/*             o->color( fl_gray_ramp( 'F' ) ); */

/*             o->label( "Ruler" ); */
/*             o->align( FL_ALIGN_LEFT ); */

/*             ruler_track = o; */
/*         } */

        o->size( o->w(), o->child( 0 )->h() * o->children() );
        rulers = o;
        o->end();
    }


    {
//        sample_rate() = engine->sample_rate();
        _fpp = 8;
//        _length = sample_rate() * 60 * 2;
        /* FIXME: hack */
        _length = -1;

        {
            Fl_Pack *o = new Fl_Pack( X, rulers->y() + rulers->h(), W - vscroll->w(), 1 );
            o->type( Fl_Pack::VERTICAL );
            o->spacing( 1 );

            tracks = o;
            o->end();
            resizable( o );
        }
    }

    /* rulers go above tracks... */
    add( rulers );

    /* make sure scrollbars are on top */
    add( vscroll );
    add( hscroll );

    vscroll->range( 0, tracks->h() );

    redraw();

    end();

    Fl::add_timeout( UPDATE_FREQ, update_cb, this );

}


float
Timeline::beats_per_minute ( nframes_t when ) const
{

/*     return tempo_track->beats_per_minute( when ); */

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

void
Timeline::time ( nframes_t when, int bpb, int note_type )
{
    time_track->add( new Time_Point( when, bpb, note_type ) );
}

/* struct BBT */
/* { */
/*     int bar, beat, tick; */

/*     BBT ( int bar, int beat, int tick ) : bar( bar ), beat( beat ), tick( tick ) */
/*         { */
/*         } */
/* }; */

/* /\** returns the BBT value for timestamp /when/ by examining the tempo/time maps *\/ */
/* BBT */
/* Timeline::bbt ( nframes_t when ) */
/* { */
/*     Tempo_Sequence *tempo = (Tempo_Sequence*)rulers->child( 0 ); */

/*     BBT bbt; */

/*     for ( list <Sequence_Widget *>::const_iterator i = tempo.widgets.begin(); */
/*           i != tempo.widgets.end(); ++i ) */
/*     { */
/*         Tempo_Point *p = *i; */

/*     } */
/* } */



/************/
/* Snapping */
/************/

struct nearest_line_arg
{
    nframes_t original;
    nframes_t closest;
};

const int snap_pixel = 10;

static nframes_t
abs_diff ( nframes_t n1, nframes_t n2 )
{
    return n1 > n2 ? n1 - n2 : n2 - n1;
}

void
nearest_line_cb ( nframes_t frame, const BBT &bbt, void *arg )
{
    nearest_line_arg *n = (nearest_line_arg *)arg;

    if ( Timeline::snap_to == Timeline::Bars && bbt.beat )
        return;

    if ( Timeline::snap_magnetic &&
         abs_diff( frame, n->original ) > timeline->x_to_ts( snap_pixel ) )
        return;

    if ( abs_diff( frame, n->original ) < abs_diff( n->original, n->closest ) )
        n->closest = frame;
}

/** Set the value pointed to by /frame/ to the frame number of the of
 the nearest measure line to /when/. Returns true if the new value of
 *frame is valid, false otherwise. */
bool
Timeline::nearest_line ( nframes_t when, nframes_t *frame ) const
{
    if ( snap_to == None )
        return false;

    nearest_line_arg n = { when, -1 };

    render_tempomap( when - x_to_ts( w() >> 1 ), x_to_ts( w() ), nearest_line_cb, &n );

    *frame = n.closest;
    return *frame != (nframes_t)-1;
}



nframes_t
Timeline::x_to_offset ( int x ) const
{
    return x_to_ts( max( 0, x - Track::width() ) ) + xoffset;
}


/** draws a single measure line */
static void
draw_measure_cb ( nframes_t frame, const BBT &bbt, void *arg )
{
    Fl_Color *color = (Fl_Color*)arg;

    fl_color( FL_BLACK );
    fl_line_style( FL_DASH, 0 );

    if ( bbt.beat )
        ++color;

    fl_color( *color );

    const int x = timeline->ts_to_x( frame - timeline->xoffset ) + Track::width();

    fl_line( x, 0, x, 5000 );

    fl_line_style( FL_SOLID, 0 );
}

/* FIXME: wrong place for this */
const float ticks_per_beat = 1920.0;

void
Timeline::update_tempomap ( void )
{
    _tempomap.clear();

    for ( list <Sequence_Widget *>::const_iterator i = time_track->_widgets.begin();
          i != time_track->_widgets.end(); ++i )
        _tempomap.push_back( *i );

    for ( list <Sequence_Widget *>::const_iterator i = tempo_track->_widgets.begin();
          i != tempo_track->_widgets.end(); ++i )
        _tempomap.push_back( *i );

    /* FIXME: shouldn't we ensure that time points always precede
     tempo points at the same position? */
    _tempomap.sort( Sequence_Widget::sort_func );
}

position_info
Timeline::solve_tempomap ( nframes_t frame ) const
{
    return render_tempomap( frame, 0, 0, 0 );
}

/** draw appropriate measure lines inside the given bounding box */
position_info
Timeline::render_tempomap( nframes_t start, nframes_t length, measure_line_callback * cb, void *arg ) const
{

    const nframes_t end = start + length;

    position_info pos;
    memset( &pos, 0, sizeof( pos ) );

    BBT &bbt = pos.bbt;

    const nframes_t samples_per_minute = sample_rate() * 60;

    float bpm = 120.0f;

    time_sig sig;

    sig.beats_per_bar = 4;
    sig.beat_type = 4;

    nframes_t f = 0;
    nframes_t next = 0;

    nframes_t frames_per_beat = samples_per_minute / bpm;

    /* FIXME: don't we need to sort so that Time_Points always preceed Tempo_Points? */

    for ( list <const Sequence_Widget *>::const_iterator i = _tempomap.begin();
          i != _tempomap.end(); ++i )
    {

        if ( ! strcmp( (*i)->class_name(), "Tempo_Point" ) )
        {
            const Tempo_Point *p = (Tempo_Point*)(*i);

            bpm = p->tempo();
            frames_per_beat = samples_per_minute / bpm;
        }
        else
        {
            const Time_Point *p = (Time_Point*)(*i);

            sig = p->time();
        }

        {
            list <const Sequence_Widget *>::const_iterator n = i;
            ++n;
            if ( n == _tempomap.end() )
                next = end;
            else
//                next = min( (*n)->start(), end );
                /* points may not always be aligned with beat boundaries, so we must align here */
                next = (*n)->start() - ( ( (*n)->start() - (*i)->start() ) % frames_per_beat );
        }


        for ( ; f < next; ++bbt.beat, f += frames_per_beat )
        {

            if ( bbt.beat == sig.beats_per_bar )
            {
                bbt.beat = 0;
                ++bbt.bar;
            }

            if ( f >= start )
            {
                /* in the zone */
                if ( cb )
                    cb( f, bbt, arg );
            }

            if ( f + frames_per_beat >= end )
                goto done;

        }
    }

done:

    pos.frame = f;
    pos.tempo = bpm;
    pos.beats_per_bar = sig.beats_per_bar;
    pos.beat_type = sig.beat_type;

    assert( f <= end );

    assert( end - f <= frames_per_beat );

    /* FIXME: this this right? */

    const nframes_t frames_per_tick = frames_per_beat / ticks_per_beat;
    bbt.tick = ( end - f ) / frames_per_tick;

    return pos;
}

void
Timeline::draw_measure_lines ( int X, int Y, int W, int H, Fl_Color color )
{
    if ( ! draw_with_measure_lines )
        return;

    Fl_Color colors[] = {  fl_color_average( FL_RED, color, 0.65f ),
                           fl_color_average( FL_BLACK, color, 0.65f ) };

    const nframes_t start = x_to_offset( X );
    const nframes_t length = x_to_ts( W );

    fl_push_clip( X, Y, W, H );

    render_tempomap( start, length, draw_measure_cb, &colors );

    fl_pop_clip();

}

/** just like draw mesure lines except that it also draws the BBT values.  */
void
Timeline::draw_measure_BBT ( int X, int Y, int W, int H, Fl_Color color )
{
//    render_tempomap( X, Y, W, H, color, true );
}

void
Timeline::xposition ( int X )
{
//    _old_xposition = xoffset;

    /* FIXME: shouldn't have to do this... */
    X = min( X, ts_to_x( _length ) - tracks->w() - Track::width() );

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

    fl_push_clip( X, Y, W, H );

/*     fl_color( rand() ); */
/*     fl_rectf( X, Y, X + W, Y + H ); */

    tl->draw_box();

    tl->draw_child( *tl->rulers );

    fl_push_clip( tl->tracks->x(), tl->rulers->y() + tl->rulers->h(), tl->tracks->w(), tl->h() - tl->rulers->h() - tl->hscroll->h() );
    tl->draw_child( *tl->tracks );
    fl_pop_clip();

    fl_pop_clip();
}


// static unsigned char *rect_image;

void
Timeline::resize ( int X, int Y, int W, int H )
{
    Fl_Overlay_Window::resize( X, Y, W, H );

    /* why is this necessary? */
    rulers->resize( Track::width(), 0, W - Track::width() - vscroll->w(), rulers->h() );

    /* why is THIS necessary? */
    hscroll->resize( 0, H - 18, hscroll->w(), 18 );
    vscroll->size( vscroll->w(), H - 18 );
}

void
Timeline::draw ( void )
{
    int X, Y, W, H;

    int bdx = 0;
    int bdw = 0;

    X = tracks->x() + bdx + 1;
    Y = tracks->y();
    W = tracks->w() - bdw - 1;
    H = tracks->h();

    adjust_vscroll();

/*     if ( damage() & FL_DAMAGE_USER1 ) */
/*     { */

/*         /\* save the rectangle so we can draw it (darkened) in the overlay *\/ */
/*         Rectangle &r = _selection; */
/*         make_current(); */
/*         rect_image = fl_read_image( NULL, r.x, r.y, r.w, r.h, 0 ); */

/*         return; */
/*     } */


    if ( ( damage() & FL_DAMAGE_ALL ) || ( damage() & FL_DAMAGE_EXPOSE ) )
    {
        DMESSAGE( "complete redraw" );

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


/*     Rectangle &r = _selection; */

/*     unsigned char *data = fl_read_image( NULL, r.x, r.y, r.w, r.h, 0 ); */

/*     Fl_RGB_Image bi( data, r.w, r.h, 3 ); */

/*     bi.color_average( FL_BLACK, 0.50f ); */

/*     bi.draw( r.x, r.y ); */

/*     delete[] data; */


/*     if ( r.w && r.h ) */
/*     { */
/*     const unsigned char data[] = { 0, 127, 0, 96, */
/*                                    0, 96, 0, 127 }; */
/*     Fl_RGB_Image bi( data, 2, 2, 2 ); */

/*     Fl_Image *bi2 = bi.copy( r.w, r.h ); */

/*     bi2->draw( r.x, r.y ); */

/*     delete bi2; */
/*     } */

        goto done;
    }

    if ( damage() & FL_DAMAGE_SCROLL )
    {
        int dx = ts_to_x( _old_xposition ) - ts_to_x( xoffset );
        int dy = _old_yposition - _yposition;

/*         draw_child( *rulers ); */

        if ( ! dy )
            fl_scroll( rulers->x(), rulers->y(), rulers->w(), rulers->h(), dx, 0, draw_clip, this );

        Y = rulers->y() + rulers->h();
        H = h() - rulers->h() - hscroll->h();

        if ( dy == 0 )
            fl_scroll( X + Track::width(), Y, W - Track::width(), H, dx, dy, draw_clip, this );
        else
            fl_scroll( X, Y, W, H, dx, dy, draw_clip, this );
    }

    if ( damage() & FL_DAMAGE_CHILD )
    {
        fl_push_clip( rulers->x(), rulers->y(), rulers->w(), rulers->h() );
        update_child( *rulers );
        fl_pop_clip();

        fl_push_clip( tracks->x(), rulers->y() + rulers->h(), tracks->w(), h() - rulers->h() - hscroll->h() );
        update_child( *tracks );
        fl_pop_clip();

        update_child( *hscroll );
        update_child( *vscroll );
    }

done:

    _old_xposition = xoffset;
    _old_yposition = _yposition;

}

void
Timeline::draw_cursor ( nframes_t frame, Fl_Color color )
{
//    int x = ( ts_to_x( frame ) - ts_to_x( xoffset ) ) + tracks->x() + Track::width();

    if ( frame < xoffset )
        return;

    const int x = ts_to_x( frame - xoffset ) + tracks->x() + Track::width();

    if ( x > tracks->x() + tracks->w() )
        return;

    fl_color( color );

    const int y = rulers->y() + rulers->h();
    const int h = this->h() - hscroll->h();

    fl_push_clip( Track::width(), y, tracks->w(), h );

    fl_line( x, y, x, h );

    fl_color( fl_darker( color ) );

    fl_line( x - 1, y, x - 1, h );

    fl_color( FL_BLACK );

    fl_line( x + 1, y, x + 1, h );

    /* draw cap */
    fl_color( color );
    fl_begin_polygon();

    fl_vertex( x - 8, y );
    fl_vertex( x, y + 8 );
    fl_vertex( x + 8, y );

    fl_end_polygon();


    /* draw cap outline */
    fl_color( FL_BLACK );
    fl_begin_line();

    fl_vertex( x - 8, y );
    fl_vertex( x, y + 8 );
    fl_vertex( x + 8, y );

    fl_end_line();

    fl_pop_clip();

}

void
Timeline::draw_playhead ( void )
{
    if ( p1 != p2 )
    {
        draw_cursor( p1, FL_BLUE );
        draw_cursor( p2, FL_GREEN );
    }

    draw_cursor( transport->frame, FL_RED );
}

void
Timeline::redraw_playhead ( void )
{
    static nframes_t last_playhead = -1;

    if ( last_playhead != transport->frame )
    {
        redraw_overlay();
        last_playhead = transport->frame;

        if ( follow_playhead )
        {
            if ( center_playhead && active() )
                xposition( max( 0, ts_to_x( transport->frame ) - ( ( tracks->w() - Track::width() ) >> 1 ) ) );
            else if ( ts_to_x( transport->frame ) > ts_to_x( xoffset ) + ( tracks->w() - Track::width() ) )
                xposition( ts_to_x( transport->frame ) );
        }
    }
}


/** called so many times a second to redraw the playhead etc.  */
void
Timeline::update_cb ( void *arg )
{
    Fl::repeat_timeout( UPDATE_FREQ, update_cb, arg );

    Timeline *tl = (Timeline *)arg;

    tl->redraw_playhead();
}

void
Timeline::draw_overlay ( void )
{

    draw_playhead();

    if ( ! ( _selection.w && _selection.h ) )
        return;

    fl_push_clip( tracks->x() + Track::width(), rulers->y() + rulers->h(),  tracks->w() - Track::width(), h() - rulers->h() - hscroll->h() );

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


/*     const unsigned char data[] = { 127, 127, 127, 96, */
/*                                    127, 96, 127, 40 }; */
/*     Fl_RGB_Image bi( data, 2, 2, 2 ); */

/*     Fl_Image *bi2 = bi.copy( r.w, r.h ); */

/*     bi2->draw( r.x, r.y ); */

/*     delete bi2; */


/*     unsigned char *data = fl_read_image( NULL, r.x, r.y, r.w, r.h, 0 ); */



/*     Fl_RGB_Image bi( rect_image, r.w, r.h, 3 ); */

/*     bi.color_average( FL_BLACK, 0.50f ); */

/*     bi.draw( r.x, r.y ); */

/*     delete[] rect_image; */
/*     rect_image = NULL; */

    fl_pop_clip();

}

// #include "Sequence_Widget.H"

/** select all widgets in inside rectangle /r/ */
void
Timeline::select ( const Rectangle &r )
{
    const int Y = r.y;

    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( ! ( t->y() > Y + r.h || t->y() + t->h() < Y ) )
            t->select( r.x, r.y, r.w, r.h, true, true );
    }
}

void
Timeline::delete_selected ( void )
{
    Sequence_Widget::delete_selected();
}

void
Timeline::select_none ( void )
{
    Sequence_Widget::select_none();
}


/** An unfortunate necessity for implementing our own DND aside from
 * the (bogus) native FLTK system */
Track *
Timeline::track_under ( int Y )
{
    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( ! ( t->y() > Y  || t->y() + t->h() < Y ) )
            return t;
    }

    return NULL;
}


int
Timeline::handle ( int m )
{
    static Drag *drag = NULL;

    switch ( m )
    {
        case FL_FOCUS:
        case FL_UNFOCUS:
//            redraw();
            return 1;
        case FL_KEYBOARD:
        case FL_SHORTCUT:
        {
            if ( Fl::event_state() & ( FL_ALT | FL_CTRL | FL_SHIFT ) )
                /* we don't want any keys with modifiers... */
                return 0;

            switch ( Fl::event_key() )
            {
                case FL_Delete:
                case FL_Home:
                case FL_End:
                    /* keep scrollbar from eating these. */
                    return 0;
                case 'p':
                {
                    int X = Fl::event_x() - Track::width();

                    if ( X > 0 )
                    {
                        transport->locate( xoffset + x_to_ts( X ) );
                    }

                    return 1;
                }
                case '[':
                {
                    int X = Fl::event_x() - Track::width();

                    if ( X > 0 )
                    {
                        p1 = xoffset + x_to_ts( X );
                    }

                    return 1;
                }
                case ']':
                {
                    int X = Fl::event_x() - Track::width();

                    if ( X > 0 )
                    {
                        p2 = xoffset + x_to_ts( X );
                    }

                    return 1;
                }
                default:
                    return Fl_Overlay_Window::handle( m );
            }

            return 0;
        }
        default:
        {

            if ( m == FL_PUSH )
                Fl::focus( this );

            int r = Fl_Overlay_Window::handle( m );

            if ( m != FL_RELEASE && r )
                return r;

            const int X = Fl::event_x();
            const int Y = Fl::event_y();

            switch ( m )
            {
                case FL_PUSH:
                {
//                    take_focus();

                    if ( Fl::event_state() & ( FL_ALT | FL_CTRL | FL_SHIFT ) )
                        return 0;

                    if ( Fl::event_button1() )
                    {
                        assert( ! drag );

                        drag = new Drag( X - x(), Y - y() );
                        _selection.x = drag->x;
                        _selection.y = drag->y;
                    }
                    else if ( Fl::test_shortcut( FL_BUTTON3 ) && ! Fl::event_shift() )
                    {
                        Fl_Menu_Item menu[] =
                            {
                                { "Add Track",        0, 0, 0, FL_SUBMENU    },
                                { "Audio",           0, 0, 0 },
                                { 0 },
                                { 0 },
                            };

                        const Fl_Menu_Item *r = menu->popup( X, Y, "Timeline" );

                        if ( r == &menu[1] )
                        {
                            /* FIXME: prompt for I/O config? */

                            Loggable::block_start();

                            /* add audio track */
                            char *name = get_unique_track_name( "Audio" );

                            Track *t = new Track( name );

                            Audio_Sequence *o = new Audio_Sequence( t );

//                            new Control_Sequence( t );

                            add_track( t );

                            t->sequence( o );

                            Loggable::block_end();
                        }

                    }
                    else
                        return 0;

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


void
Timeline::zoom_in ( void )
{
    hscroll->zoom_in();
}

void
Timeline::zoom_out ( void )
{
    hscroll->zoom_out();
}

/** zoom the display to show /secs/ seconds per screen */
void
Timeline::zoom ( float secs )
{
    const int sw = tracks->w() - Track::width();

    int fpp = (int)((secs * sample_rate()) / sw);

    int p = 0;
    while ( 1 << p < fpp ) p++;

    hscroll->zoom( p );

    redraw();
}

void
Timeline::zoom_fit ( void )
{
    zoom( _length / (float)sample_rate() );
}

Track *
Timeline::track_by_name ( const char *name )
{
    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( ! strcmp( name, t->name() ) )
            return t;
    }

    return NULL;
}

char *
Timeline::get_unique_track_name ( const char *name )
{
    char pat[256];

    strcpy( pat, name );

    for ( int i = 1; track_by_name( pat ); ++i )
        snprintf( pat, sizeof( pat ), "%s.%d", name, i );

    return strdup( pat );
}

void
Timeline::add_track ( Track *track )
{
    DMESSAGE( "added new track to the timeline" );

    /* FIXME: do locking */
    tracks->add( track );

    /* FIXME: why is this necessary? doesn't the above add do DAMAGE_CHILD? */
    redraw();

}

void
Timeline::remove_track ( Track *track )
{
    DMESSAGE( "removed track from the timeline" );

    /* FIXME: do locking */

    /* FIXME: what to do about track contents? */
    tracks->remove( track );

    /* FIXME: why is this necessary? doesn't the above add do DAMAGE_CHILD? */
    redraw();
}

/** Initiate recording for all armed tracks */
bool
Timeline::record ( void )
{
    /* FIXME: right place for this? */
    transport->recording = true;

    Loggable::block_start();

    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( t->armed() && t->record_ds )
            t->record_ds->start( transport->frame );
    }

    deactivate();

    return true;
}

/** stop recording for all armed tracks */
void
Timeline::stop ( void )
{
    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( t->armed() && t->record_ds )
            t->record_ds->stop( transport->frame );
    }

    Loggable::block_end();

    activate();

    transport->recording = false;
}


/**********/
/* Engine */
/**********/

/** call process() on each track header */
nframes_t
Timeline::process ( nframes_t nframes )
{
    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        t->process( nframes );
    }

    /* FIXME: BOGUS */
    return nframes;
}

/* THREAD: RT */
void
Timeline::seek ( nframes_t frame )
{
    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        t->seek( frame );
    }
}

/* THREAD: RT */
int
Timeline::seek_pending ( void )
{
    int r = 0;

    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( t->playback_ds )
            r += t->playback_ds->buffer_percent() < 50;
    }

    return r;
}


/* FIXME: shouldn't these belong to the engine? */
int
Timeline::total_input_buffer_percent ( void )
{
    int r = 0;

    int cnt = 0;

    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( t->record_ds )
        {
            ++cnt;
            r += t->record_ds->buffer_percent();
        }
    }

    if ( ! cnt )
        return 0;

    return r / cnt;
}

int
Timeline::total_output_buffer_percent ( void )
{

    int r = 0;

    int cnt = 0;

    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( t->playback_ds )
        {
            ++cnt;
            r += t->playback_ds->buffer_percent();
        }
    }

    if ( ! cnt )
        return 0;

    return r / cnt;
}

int
Timeline::total_playback_xruns ( void )
{
    int r = 0;

    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( t->playback_ds )
            r += t->playback_ds->xruns();
    }

    return r;
}

int
Timeline::total_capture_xruns ( void )
{
    int r = 0;

    for ( int i = tracks->children(); i-- ; )
    {
        Track *t = (Track*)tracks->child( i );

        if ( t->record_ds )
            r += t->record_ds->xruns();
    }

    return r;
}
