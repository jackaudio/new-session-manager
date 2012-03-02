
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

#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Menu_Item.H>
#include <FL/fl_show_colormap.H>

#include "Sequence.H"
#include "Audio_Region.H"
#include "Timeline.H"
#include "Waveform.H"
#include "Audio_Sequence.H"
#include "Track.H"

#include "Engine/Audio_File.H"

#include "const.h"
#include "debug.h"

#include <algorithm>
using std::min;
using std::max;



/* defined in timeline.C */
extern void draw_full_arrow_symbol ( Fl_Color color );

extern Timeline *timeline;

bool Audio_Region::inherit_track_color = true;

Fl_Boxtype Audio_Region::_box = FL_UP_BOX;

Fl_Color Audio_Region::_selection_color = FL_MAGENTA;



static Fl_Color fl_invert_color ( Fl_Color c )
{
    unsigned char r, g, b;

    Fl::get_color( c, r, g, b );

    return fl_rgb_color( 255 - r, 255 - g, 255 - b );
}



void
Audio_Region::get ( Log_Entry &e ) const
{
    e.add( ":source",            _clip            ? _clip->name() : "" );
    e.add( ":gain",              _scale           );
    e.add( ":fade-in-type",      _fade_in.type    );
    e.add( ":fade-in-duration",  _fade_in.length  );
    e.add( ":fade-out-type",     _fade_out.type   );
    e.add( ":fade-out-duration", _fade_out.length );

    Sequence_Region::get( e );

    e.add( ":offset", _r->offset );
    e.add( ":loop", _loop );
}

void
Audio_Region::set ( Log_Entry &e )
{
    for ( int i = 0; i < e.size(); ++i )
    {
        const char *s, *v;

        e.get( i, &s, &v );

        if ( ! strcmp( s, ":gain" ) )
            _scale = atof( v );
        else if ( ! strcmp( s, ":color" ) )
            _box_color = (Fl_Color)atoi( v );
        else if ( ! strcmp( s, ":fade-in-type" ) )
            _fade_in.type = (Fade::fade_type_e)atoi( v );
        else if ( ! strcmp( s, ":fade-in-duration" ) )
            _fade_in.length = atoll( v );
        else if ( ! strcmp( s, ":fade-out-type" ) )
            _fade_out.type = (Fade::fade_type_e)atoi( v );
        else if ( ! strcmp( s, ":fade-out-duration" ) )
            _fade_out.length = atoll( v );
        else if ( ! strcmp( s, ":offset" ) )
            _r->offset = atoll( v );
        else if ( ! strcmp( s, ":loop" ) )
            _loop = atoll( v );
        else if ( ! strcmp( s, ":source" ) )
        {
            if ( ! ( _clip = Audio_File::from_file( v ) ) )
            {
                printf( "Grave error: could not open source \"%s\"\n", v );
            }
        }
    }

    Sequence_Region::set( e );
}

void
Audio_Region::init ( void )
{
    _loop = 0;
    _sequence = NULL;
    _scale = 1.0f;
    _clip = NULL;

    _box_color = FL_CYAN;
    _color = FL_BLUE;

    _fade_in.length = 256;
    _fade_in.type = Fade::Sigmoid;

    _fade_out = _fade_in;
}

/* copy constructor */
Audio_Region::Audio_Region ( const Audio_Region & rhs ) : Sequence_Region( rhs )
{
//    *((Sequence_Region*)this) = (Sequence_Region &)rhs;

    _clip      = rhs._clip->duplicate();
    _scale     = rhs._scale;

    _fade_in   = rhs._fade_in;
    _fade_out  = rhs._fade_out;

    _loop  = rhs._loop;

    log_create();
}

/*  */
Audio_Region::Audio_Region ( Audio_File *c )
{
    init();
    _clip = c;
    _r->length = _clip->length();

    log_create();
}

/* used when DND importing */
Audio_Region::Audio_Region ( Audio_File *c, Sequence *t, nframes_t o )
{
    init();
    _clip = c;
    _sequence = t;
    _r->offset = 0;
    _r->start = o;
    _r->length = _clip->length();

    sequence()->add( this );

    int sum = 0;
    const char *s = rindex( _clip->name(), '/' );
    if ( ! s )
        s = _clip->name();

    for ( int i = strlen( s ); i--; )
        sum += s[ i ];

    while ( sum >> 8 )
        sum = (sum & 0xFF) + (sum >> 8);

    _color = (Fl_Color)sum;

/*     _color = fl_color_average( FL_YELLOW, (Fl_Color)sum, 0.80 ); */

//    _color = FL_YELLOW;

    _box_color = FL_WHITE;

    log_create();
}

Audio_Region::~Audio_Region ( )
{
    log_destroy();

    _clip->release();
}



void
Audio_Region::menu_cb ( Fl_Widget *w, void *v )
{
    ((Audio_Region*)v)->menu_cb( (Fl_Menu_*) w );
}

void
Audio_Region::menu_cb ( const Fl_Menu_ *m )
{
    char picked[256];

    m->item_pathname( picked, sizeof( picked ) );

    Logger log( this );

    if ( ! strcmp( picked, "Fade/In/Linear" ) )
        _fade_in.type = Fade::Linear;
    else if ( ! strcmp( picked, "Fade/In/Sigmoid" ) )
        _fade_in.type = Fade::Sigmoid;
    else if ( ! strcmp( picked, "Fade/In/Logarithmic" ) )
        _fade_in.type = Fade::Logarithmic;
    else if ( ! strcmp( picked, "Fade/In/Parabolic" ) )
        _fade_in.type = Fade::Parabolic;
    else if ( ! strcmp( picked, "Fade/Out/Linear" ) )
        _fade_out.type = Fade::Linear;
    else if ( ! strcmp( picked, "Fade/Out/Sigmoid" ) )
        _fade_out.type = Fade::Sigmoid;
    else if ( ! strcmp( picked, "Fade/Out/Logarithmic" ) )
        _fade_out.type = Fade::Logarithmic;
    else if ( ! strcmp( picked, "Fade/Out/Parabolic" ) )
        _fade_out.type = Fade::Parabolic;
    else if ( ! strcmp( picked, "/Color" ) )
        box_color( fl_show_colormap( box_color() ) );
    else if ( ! strcmp( picked, "/Fade in to mouse" ) )
    {
        nframes_t offset = x_to_offset( Fl::event_x() );

        if ( offset < length() )
            _fade_in.length = offset;

        DMESSAGE( "set fade in duration" );
    }
    else if ( ! strcmp( picked, "/Fade out to mouse" ) )
    {
        long offset = length() - x_to_offset( Fl::event_x() );

        if ( offset > 0 )
            _fade_out.length = offset;
    }
    else if ( ! strcmp( picked, "/Loop point to mouse" ) )
    {
        nframes_t offset = x_to_offset( Fl::event_x() );

        if ( offset > 0 )
        {
            nframes_t f = offset + _r->start;

            if ( timeline->nearest_line( &f, false ) )
                _loop = f - _r->start;
            else
                _loop = offset;
        }
    }
    else if ( ! strcmp( picked, "/Clear loop point" ) )
        _loop = 0;
    else if ( ! strcmp( picked, "/Normalize" ) )
        normalize();
    else if ( ! strcmp( picked, "/Denormalize" ) )
        _scale = 1.0;
    else if ( ! strcmp( picked, "/Range from" ) )
        timeline->range( start(), length() );
    else if ( ! strcmp( picked, "/Remove" ) )
        remove();
    else
        FATAL( "Unknown menu choice \"%s\"", picked );

    redraw();
}

#include "FL/test_press.H"
#include "FL/menu_popup.H"

/** build the context menu for this region */
Fl_Menu_Button &
Audio_Region::menu ( void )
{
    static Fl_Menu_Button m( 0, 0, 0, 0, "Region" );

    Fade::fade_type_e it = _fade_in.type;
    Fade::fade_type_e ot = _fade_out.type;

    Fl_Menu_Item items[] =
        {
            { "Fade",             0, 0, 0,  FL_SUBMENU    },
            { "In",               0, 0, 0,  FL_SUBMENU    },
            { "Linear",           0, 0, 0,  FL_MENU_RADIO | ( it == Fade::Linear      ? FL_MENU_VALUE : 0 ) },
            { "Sigmoid",          0, 0, 0,  FL_MENU_RADIO | ( it == Fade::Sigmoid     ? FL_MENU_VALUE : 0 ) },
            { "Logarithmic",      0, 0, 0,  FL_MENU_RADIO | ( it == Fade::Logarithmic ? FL_MENU_VALUE : 0 ) },
            { "Parabolic",        0, 0, 0,  FL_MENU_RADIO | ( it == Fade::Parabolic   ? FL_MENU_VALUE : 0 ) },
            { 0                   },
            { "Out",              0, 0, 0,  FL_SUBMENU    },
            { "Linear",           0, 0, 0,  FL_MENU_RADIO | ( ot == Fade::Linear      ? FL_MENU_VALUE : 0 ) },
            { "Sigmoid",          0, 0, 0,  FL_MENU_RADIO | ( ot == Fade::Sigmoid     ? FL_MENU_VALUE : 0 ) },
            { "Logarithmic",      0, 0, 0,  FL_MENU_RADIO | ( ot == Fade::Logarithmic ? FL_MENU_VALUE : 0 ) },
            { "Parabolic",        0, 0, 0,  FL_MENU_RADIO | ( ot == Fade::Parabolic   ? FL_MENU_VALUE : 0 ) },
            { 0                   },
            { 0 },
            { "Color",        0, 0, 0,  inherit_track_color ? FL_MENU_INACTIVE : 0 },
            { "Fade in to mouse", FL_F + 3, 0, 0 },
            { "Fade out to mouse", FL_F + 4, 0, 0 },
            { "Loop point to mouse", 'l', 0, 0 },
            { "Clear loop point", FL_SHIFT + 'l', 0, 0, 0 == _loop ? FL_MENU_INACTIVE : 0 },
            { "Normalize", 'n', 0, 0 },
            { "Denormalize", FL_SHIFT + 'n', 0, 0, 1.0 == _scale ? FL_MENU_INACTIVE : 0 },
            { "Range from", FL_CTRL + 'r', 0, 0 },
            { "Remove", 0, 0, 0 },
            { 0 },
        };

    menu_set_callback( items, &Audio_Region::menu_cb, (void*)this );

    m.copy( items, (void*)this );

    return m;
}

/** Draws the curve for a single fade. /X/ and /W/ repersent the
    portion of the region covered by this draw, which may or may not
    cover the fade in question. */
void
Audio_Region::draw_fade ( const Fade &fade, Fade::fade_dir_e dir, bool line, int X, int W )
{
    const int dy = y() + Fl::box_dy( box() );
    const int dh = h() - Fl::box_dh( box() );
    const int height = dh;
    const int width = timeline->ts_to_x( fade.length );

    fl_color( fl_lighter( FL_BLACK ) );

    fl_push_matrix();

    if ( dir == Fade::In )
        fl_translate( line_x(), dy );
    else
    {
        fl_translate( line_x() + abs_w(), dy );
        /* flip */
        fl_scale( -1.0, 1.0 );
    }

    fl_scale( width, height );

    if ( line )
        fl_begin_line();
    else
        fl_begin_polygon();

    fl_vertex( 0.0, 0.0 );
    fl_vertex( 0.0, 1.0 );


//    if ( draw_real_fade_curve )
    {
        nframes_t tsx = timeline->x_to_ts( 1 );
        nframes_t ts = 0;

        for ( int i = 0; i < width; ++i, ts += tsx )
            fl_vertex( i / (float)width, 1.0f - fade.gain( ts / (float)fade.length ) );

    }

    fl_vertex( 1.0, 0.0 );

    if ( line )
        fl_end_line();
    else
        fl_end_polygon();

    fl_pop_matrix();
}

struct Peaks_Redraw_Request {

    Audio_Region *region;

    nframes_t start;
    nframes_t end;

    Peaks_Redraw_Request ( Audio_Region *region, nframes_t start, nframes_t end ) : region( region ), start( start), end( end )
        {
        }
};

/* static wrapper */
void
Audio_Region::peaks_pending_cb ( void *v )
{
    Peaks_Redraw_Request *r = (Peaks_Redraw_Request*)v;

    r->region->peaks_pending_cb( r );
}

void
Audio_Region::peaks_pending_cb ( Peaks_Redraw_Request *r )
{
    int npeaks = timeline->ts_to_x( r->end - r->start );

    if ( _clip->peaks()->ready( r->start, npeaks, timeline->fpp() ) )
    {
        printf( "damaging from timeout\n" );
        /* FIXME: only need to damage the affected area! */
        timeline->damage( FL_DAMAGE_ALL, x(), y(), w(), h() );

        delete r;
    }
    else
        Fl::repeat_timeout( 0.1f, &Audio_Region::peaks_pending_cb, (void*)r );
}

void
Audio_Region::draw_box( void )
{
    /* dirty hack to keep the box from flipping to vertical at small sizes */

    fl_push_clip( x(), y(), w(), h() );

    Fl_Color selection_color = _selection_color;

    Fl_Color color = Audio_Region::inherit_track_color ? sequence()->track()->color() :  _box_color;

    color = fl_color_average( color, sequence()->color(), 0.75f );

    if ( this == ((Audio_Sequence*)sequence())->capture_region() )
    {
        color = FL_RED;
    }
    else if ( ! active_r() )
    {
        color = fl_inactive( color );
        selection_color = fl_inactive( selection_color );
    }

    if ( selected() )
        fl_draw_box( fl_down( box() ), x() - ( h() >> 1 ), y(), w() + ( h() >> 1 ) + 50, h(), selection_color );
    else
        fl_draw_box( box(), x() - ( h() >> 1 ), y(), w() + ( h() >> 1 ) + 50, h(), color );

    /* draw fades */
    draw_fade( _fade_in, Fade::In, false, x(), w() );
    draw_fade( _fade_out, Fade::Out, false, x(), w() );

    fl_pop_clip();
}

/** Draw (part of) region. X, Y, W and H are the rectangle we're clipped to. */
void
Audio_Region::draw ( void )
{
    /* intersect clip with region */

    int X, Y, W, H;

    fl_clip_box( x(), y(), w(), h(), X, Y, W, H );

    if ( ! ( W > 0 && H > 0 ) )
        /* no coverage */
        return;

    if ( start() > timeline->xoffset + timeline->x_to_ts( sequence()->w() ) ||
         start() + length() < timeline->xoffset )
        /* not in viewport */
        return;

    fl_push_clip( X, Y, W, H );

    /* account for waveform outlines... */
    X -= 2;
    W += 4;

    /* start with region length... */
//    int rw = timeline->ts_to_x( min( length(), timeline->x_to_ts( sequence()->w() ) ) );
    int rw = W;

    /* calculate waveform offset due to scrolling */
    nframes_t offset = 0;
    if ( start() < timeline->xoffset )
    {
        offset = timeline->xoffset - start();

//        rw -= timeline->ts_to_x( offset );
    }

/*     DMESSAGE( "rw = %d", rw ); */

    const int rx = x();

/*     fl_color( FL_RED ); */
/*     fl_line( rx + rw, y(), rx + rw, y() + h() ); */

    /* draw fade curve outlines--this is only here because of crossfades */
    draw_fade( _fade_in, Fade::In, true, X, W );
    draw_fade( _fade_out, Fade::Out, true, X, W );

    int xo = 0;

    nframes_t ostart = 0, oend = 0;

    const int total_peaks_needed = rw;

    /* compensate for scrolling */
    if ( X - rx > 0 )
        offset += timeline->x_to_ts( X - rx );


    do {

        int channels;
        int peaks;
        Peak *pbuf;

        nframes_t start = _r->offset;

        int loop_peaks_needed = _loop ? timeline->ts_to_x( _loop ) : timeline->ts_to_x( _clip->length() );

        if ( ! loop_peaks_needed )
            break;

        if ( ! xo )                                             /* first loop... */
        {
            if ( _loop )
                start += offset % _loop;
            else
                start += offset;

/*             DMESSAGE( "offset = %lu", (unsigned long) offset ); */
/*             DMESSAGE( "loop peaks needed = %d", loop_peaks_needed ); */

            loop_peaks_needed -= timeline->ts_to_x( offset % timeline->x_to_ts( loop_peaks_needed ) );

            loop_peaks_needed = min( loop_peaks_needed, total_peaks_needed );

/*             DMESSAGE( "loop peaks needed = %d", loop_peaks_needed ); */

            assert( loop_peaks_needed >= 0 );

            if ( _loop && offset < _loop )
            {
                const int x = timeline->ts_to_x( _loop - offset );

                /* FIXME: is there no way to draw these symbols direclty? */
                fl_color( FL_WHITE );
                
                fl_push_matrix();
                
                fl_translate( X + x + 2, y() + h() - 14 );
                fl_scale( - 16, 8 );
                
                draw_full_arrow_symbol( FL_BLACK );
                
                fl_pop_matrix();
            }
        }

        if ( xo + loop_peaks_needed > total_peaks_needed )
        {
            loop_peaks_needed -= ( xo + loop_peaks_needed ) - total_peaks_needed;
        }

        if ( 0 == loop_peaks_needed )
            break;

        const nframes_t end = start + timeline->x_to_ts( loop_peaks_needed );

        if ( start != ostart || end != oend )
        {
            if ( _clip->read_peaks( timeline->fpp(),
                                    start,
                                    end,
                                    &peaks, &pbuf, &channels ) )
            {
                Waveform::scale( pbuf, peaks * channels, _scale );

                ostart = start;
                oend = end;
            }
        }
        else
        {
//            DMESSAGE( "using cached peaks" );
        }

        if ( peaks && pbuf )
        {
            int ch = (h() - Fl::box_dh( box() ))  / channels;


            for ( int i = 0; i < channels; ++i )
            {
                Waveform::draw( X + xo,
                                (y() + Fl::box_dy( box() )) + (i * ch),
                                loop_peaks_needed,
                                ch,
                                pbuf + i, peaks, channels,
                                selected() ? fl_invert_color( _color ) : _color );
            }
        }

        if ( peaks < loop_peaks_needed )
        {
            /* couldn't read peaks--perhaps they're being generated. Try again later. */
            Fl::add_timeout( 0.1f, &Audio_Region::peaks_pending_cb,
                             new Peaks_Redraw_Request( this, start + timeline->x_to_ts( peaks ), end ) );
        }

        xo += loop_peaks_needed;

    }
    while ( _loop && xo < W );

    timeline->draw_measure_lines( X, Y, W, H, _box_color );

/*     fl_color( FL_BLACK ); */
/*     fl_line( rx, Y, rx, Y + H ); */
/*     fl_line( rx + rw - 1, Y, rx + rw - 1, Y + H ); */

    if ( _clip->dummy() )
    {
        char pat[256];
        snprintf( pat, sizeof( pat ), "Missing Source!: %s", _clip->name() );
        draw_label( pat, align() );
    }
    else
        draw_label( _clip->name(), align() );

/*     if ( current() ) */
/*     { */
/*         /\* draw length bubble *\/ */

/*         char pat[40]; */

/*         snprintf( pat, sizeof( pat ), "%dm:%.1fs", (int)(length() / timeline->sample_rate()) / 60, (double)length() / timeline->sample_rate() ); */

/*         draw_label( pat, (Fl_Align)(FL_ALIGN_INSIDE | FL_ALIGN_CENTER), FL_GREEN ); */
/*     } */

    fl_pop_clip();

}

/** split region at absolute frame /where/ */
void
Audio_Region::split ( nframes_t where )
{
    nframes_t old_fade_in = _fade_in.length;

    _fade_in.length = 256;

    Audio_Region *copy = new Audio_Region( *this );

    _fade_in.length = old_fade_in;
    _fade_out.length = 256;

    Sequence_Region::split( copy, where );
}

int
Audio_Region::handle ( int m )
{
    static int ox, oy;

    static bool copied = false;
    static nframes_t os;

    int X = Fl::event_x();
    int Y = Fl::event_y();

    Logger _log( this );

    switch ( m )
    {
        case FL_FOCUS:
        case FL_UNFOCUS:
            return 1;
        case FL_KEYBOARD:
            return menu().test_shortcut() != 0;
        case FL_ENTER:
            return Sequence_Region::handle( m );
        case FL_LEAVE:
            return Sequence_Region::handle( m );
        case FL_PUSH:
        {
            /* splitting  */
            if ( test_press( FL_BUTTON2 | FL_SHIFT ) )
            {
                /* split */
                if ( ! copied )
                {
                    Loggable::block_start();

                    split( timeline->x_to_offset( X ) );

                    log_end();

                    Loggable::block_end();

                    log_start();
                }

                return 0;
            }
            else
            {
                ox = x() - X;
                oy = y() - Y;
                /* for panning */
                os = _r->offset;

                if ( test_press( FL_BUTTON2 | FL_CTRL ) )
                {
                    normalize();
                    return 1;
                }
                else if ( test_press( FL_BUTTON3 ) )
                {
                    /* context menu */
                    menu_popup( &menu() );

                    return 1;
                }
                else
                    return Sequence_Region::handle( m );
            }

            break;
        }
        case FL_RELEASE:
        {
            Sequence_Region::handle( m );

            copied = false;

            return 1;
        }
        case FL_DRAG:
            if ( ! _drag )
            {
                begin_drag( Drag( x() - X, y() - Y, x_to_offset( X ) ) );
                _log.hold();
            }

            if ( test_press( FL_BUTTON1 | FL_SHIFT | FL_CTRL ) )
            {
                /* panning */
                int d = (ox + X) - x();
                long td = timeline->x_to_ts( d );

                if ( td > 0 && os < (nframes_t)td )
                    _r->offset = 0;
                else
                    _r->offset = os - td;

                redraw();
                return 1;
            }

            return Sequence_Region::handle( m );

        default:
            return Sequence_Region::handle( m );
            break;
    }

    return 0;
}



/**********/
/* Public */
/**********/

/** return the name of the audio source this region represents */
const char *
Audio_Region::source_name ( void ) const
{
    return _clip->name();
}

/** set the amplitude scaling for this region from the normalization
 * factor for the range of samples represented by this region */
void
Audio_Region::normalize ( void )
{
    int peaks, channels;
    Peak *pbuf;

    const nframes_t npeaks = _loop ? _loop : length();

    if ( _clip->read_peaks( npeaks, offset(), offset() + npeaks, &peaks, &pbuf, &channels ) &&
         peaks )
        _scale = pbuf->normalization_factor();

    /* FIXME: wrong place for this? */
    sequence()->handle_widget_change( start(), length() );
    redraw();
}
