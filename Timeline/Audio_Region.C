
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
#include <FL/Fl_Group.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Menu_Item.H>
#include <stdio.h>

#include "Sequence.H"
#include "Audio_Region.H"
#include "Timeline.H"
#include "Waveform.H"
#include "Audio_Sequence.H"
#include "Track.H"

#include "Engine/Audio_File.H"

#include <algorithm>
using std::min;
using std::max;



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

    _clip      = rhs._clip;
    _scale     = rhs._scale;

    _fade_in   = rhs._fade_in;
    _fade_out  = rhs._fade_out;

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

const char *
Audio_Region::source_name ( void ) const
{
    return _clip->name();
}

#include <FL/fl_show_colormap.H>
#include "FL/test_press.H"

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
        case FL_KEYBOARD:
        {
            if ( Fl::event_key() == FL_F + 3 )
            {
                nframes_t offset = x_to_offset( X );

                if ( offset < length() )
                    _fade_in.length = offset;

                DMESSAGE( "setting fade in length to %lu", _fade_in.length );

                redraw();

                return 1;
            }
            else if ( Fl::event_key() == FL_F + 4 )
            {
                long offset = length() - x_to_offset( X );

                if ( offset > 0 )
                    _fade_out.length = offset;

                DMESSAGE( "setting fade out length to %lu", _fade_in.length );

                redraw();

                return 1;
            }

            return 0;

        }
        case FL_PUSH:
        {
            /* splitting  */
            if ( test_press( FL_BUTTON2 | FL_SHIFT ) )
            {
                /* split */
                if ( ! copied )
                {
                    Loggable::block_start();

                    Audio_Region *copy = new Audio_Region( *this );

                    trim( RIGHT, X );
                    copy->trim( LEFT, X );

                    sequence()->add( copy );

                    log_end();

                    Loggable::block_end();
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
                    redraw();
                    return 1;
                }
                else if ( test_press( FL_BUTTON3 ) )
                {
                    /* context menu */

                    Fade::fade_type_e it = _fade_in.type;
                    Fade::fade_type_e ot = _fade_out.type;

                    Fl_Menu_Item menu[] =
                        {
                            { "Fade",        0, 0, 0, FL_SUBMENU    },
                            { "In",          0, 0, 0, FL_SUBMENU    },
                            { "Linear",           0, 0, 0, FL_MENU_RADIO | ( it == Fade::Linear      ? FL_MENU_VALUE : 0 ) },
                            { "Sigmoid",          0, 0, 0, FL_MENU_RADIO | ( it == Fade::Sigmoid     ? FL_MENU_VALUE : 0 ) },
                            { "Logarithmic",      0, 0, 0, FL_MENU_RADIO | ( it == Fade::Logarithmic ? FL_MENU_VALUE : 0 ) },
                            { "Parabolic",        0, 0, 0, FL_MENU_RADIO | ( it == Fade::Parabolic   ? FL_MENU_VALUE : 0 ) },
                            { 0 },
                            { "Out",         0, 0, 0, FL_SUBMENU    },
                            { "Linear",           0, 0, 0, FL_MENU_RADIO | ( ot == Fade::Linear      ? FL_MENU_VALUE : 0 ) },
                            { "Sigmoid",          0, 0, 0, FL_MENU_RADIO | ( ot == Fade::Sigmoid     ? FL_MENU_VALUE : 0 ) },
                            { "Logarothmic",      0, 0, 0, FL_MENU_RADIO | ( ot == Fade::Logarithmic ? FL_MENU_VALUE : 0 ) },
                            { "Parabolic",        0, 0, 0, FL_MENU_RADIO | ( ot == Fade::Parabolic   ? FL_MENU_VALUE : 0 ) },
                            { 0 },
                            { 0 },
                            { "Color",        0, 0, 0, inherit_track_color ? FL_MENU_INACTIVE : 0 },
                            { 0 },
                        };

                    const Fl_Menu_Item *r = menu->popup( X, Y, "Audio_Region" );

                    if ( r )
                    {
                        if ( r > &menu[1] && r < &menu[6] )
                            _fade_in.type = (Fade::fade_type_e)(int)(r - &menu[2]);
                        else if ( r > &menu[7] && r < &menu[12] )
                            _fade_out.type = (Fade::fade_type_e)(int)(r - &menu[8]);
                        else if ( r == &menu[ 14 ] )
                        {
                            box_color( fl_show_colormap( box_color() ) );
                        }

                        redraw();
                    }

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
        fl_draw_box( fl_down( box() ), x() - 10, y(), w() + 50, h(), selection_color );
    else
        fl_draw_box( box(), x() - 10, y(), w() + 50, h(), color );

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

    /* account for waveform outlines... */
    X -= 2;
    W += 4;

    int OX = scroll_x();
    int ox = timeline->ts_to_x( _r->start );

    if ( ox > OX + sequence()->w() ||
         ox < OX && ox + abs_w() < OX )
        /* not in viewport */
        return;

    int rw = timeline->ts_to_x( _r->length );

    /* calculate waveform offset due to scrolling */
    nframes_t offset = 0;
    if ( ox < OX )
    {
        offset = timeline->x_to_ts( OX - ox );

        rw -= OX - ox;
    }

    rw = min( rw, sequence()->w() );

    int rx = x();

    fl_push_clip( rx, Y, rw, H );

    /* get actual peak data */
    int channels;
    int peaks;
    Peak *pbuf;


//    const nframes_t start = _r->start + offset + timeline->x_to_ts( X - rx );
//    nframes_t start = _r->start + offset;
    nframes_t start = _r->offset + offset;

    /* compensate for ??? */
    if ( X - rx > 0 )
        start += timeline->x_to_ts( X - rx );

    const int peaks_needed = min( timeline->ts_to_x( _clip->length() - start ), W );

    const nframes_t end = start + timeline->x_to_ts( peaks_needed );

    if ( _clip->read_peaks( timeline->fpp(),
                            start,
                            end,
                            &peaks, &pbuf, &channels ) &&
         peaks )
    {

        assert( pbuf );

        /* draw fade curve outlines--this is only here because of crossfades */
        draw_fade( _fade_in, Fade::In, true, X, W );
        draw_fade( _fade_out, Fade::Out, true, X, W );

        int ch = (h() - Fl::box_dh( box() ))  / channels;

        Waveform::scale( pbuf, peaks * channels, _scale );


        for ( int i = 0; i < channels; ++i )
        {
//            Peak *pb = pbuf + (peaks * i);

/*         int fw = timeline->ts_to_x( fade.length ); */

/*         /\* if ( draw_fade_waveform ) *\/ */
/*         for ( int j = min( fw, peaks ); j--; ) */
/*         { */
/*             const float g = fade.gain( j * timeline->fpp() ); */
/*             pb[ j ].min *= g; */
/*             pb[ j ].max *= g; */
/*         } */

            Waveform::draw( X,
                            (y() + Fl::box_dy( box() )) + (i * ch),
                            W,
                            ch,
                            pbuf + i, peaks, channels,
                            selected() ? fl_invert_color( _color ) : _color );
        }
    }

    if ( peaks < peaks_needed )
    {
        /* couldn't read peaks--perhaps they're being generated. Try again later. */
        Fl::add_timeout( 0.1f, &Audio_Region::peaks_pending_cb,
                         new Peaks_Redraw_Request( this, start + timeline->x_to_ts( peaks ), end ) );
    }

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


void
Audio_Region::normalize ( void )
{
    int peaks, channels;
    Peak *pbuf;

    if ( _clip->read_peaks( length(), offset(), offset() + length(), &peaks, &pbuf, &channels ) &&
         peaks )
        _scale = pbuf->normalization_factor();
}
