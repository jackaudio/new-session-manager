
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

#include "Track.H"
#include "Region.H"
#include "Timeline.H"
#include "Waveform.H"

#include <FL/fl_draw.H>
#include <FL/Fl.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Box.H>

#include <stdio.h>

#include <algorithm>
// using  std::algorithm;
using namespace std;


extern Timeline *timeline;



Fl_Boxtype Region::_box = FL_UP_BOX;

Fl_Color Region::_selection_color = FL_MAGENTA;

static Fl_Color fl_invert_color ( Fl_Color c )
{
    unsigned char r, g, b;

    Fl::get_color( c, r, g, b );

    return fl_rgb_color( 255 - r, 255 - g, 255 - b );
}


#if 0
/* perhaps use map? */
map_PRIM ( set )
{
/*     if ( narg % 2 != 0 ) */
/*         printf( "invalid number of arguments\n" ); */

    int id = atoi( arg );

    map_ARG_NEXT( arg, end );

    Logable *l = Loggable::find( id );

    char **sa = malloc( sizeof( char * ) * narg + 1 );

    for ( int i = 0; i < narg; ++i )
        sa[ i ] = strdup( map_ARG_NEXT( arg, end ) );

    l->set( sa );

    map_RESULT( "" );
}
#endif


void
Region::init ( void )
{

    _track = NULL;
    _r->offset = 0;
    _r->start = 0;
    _r->end = 0;
    _scale = 1.0f;
    _clip = NULL;

    _box_color = FL_CYAN;
    _color = FL_BLUE;

    _fade_in.length = 256;
    _fade_in.type = Fade::Sigmoid;

    _fade_out = _fade_in;
}

/* copy constructor */
Region::Region ( const Region & rhs )
{
    *((Track_Widget*)this) = (Track_Widget &)rhs;

    _clip      = rhs._clip;
    _scale     = rhs._scale;

    _fade_in   = rhs._fade_in;
    _fade_out  = rhs._fade_out;

    log_create();
}

Track_Widget *
Region::clone ( const Track_Widget *r )
{
    return new Region( *(Region*)r );
}

/*  */
Region::Region ( Audio_File *c )
{
    init();
    _clip = c;
    _r->end = _clip->length();

    log_create();
}


/* used when DND importing */
Region::Region ( Audio_File *c, Track *t, nframes_t o )
{
    init();
    _clip = c;
    _r->end = _clip->length();
    _track = t;
    _r->offset = o;

    _track->add( this );

    int sum = 0;
    const char *s = rindex( _clip->name(), '/' );
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

void
Region::trim ( enum trim_e t, int X )
{

    X -= _track->x();
    redraw();

    switch ( t )
    {
        case LEFT:
        {
/*             if ( d < 0 ) */
/* //                _track->damage( FL_DAMAGE_EXPOSE, x() + d, y(), 1 - d, h() ); */
/*                 _track->damage( FL_DAMAGE_EXPOSE, x(), y(), w(), h() ); */
/*             else */
/*                 _track->damage( FL_DAMAGE_EXPOSE, x(), y(), d, h() ); */

            int d = X - ( abs_x() - scroll_x() );

            long td = timeline->x_to_ts( d );

            if ( td < 0 && _r->start < 0 - td )
                td = 0 - _r->start;

            if ( _r->start + td >= _r->end )
                td = (_r->end - _r->start) - timeline->x_to_ts( 1 );

            _r->start += td;
            _r->offset += td;
            break;
        }
        case RIGHT:
        {
            int d = (( abs_x() - scroll_x() ) + abs_w() ) - X;

/*             _track->damage( FL_DAMAGE_EXPOSE, x() + w(), y(), d, h() ); */

            long td = timeline->x_to_ts( d );

//            printf( "%li %li\n", td, _r->end - _r->start );

            if ( td >= 0 && _r->end - _r->start < td )
                _r->end = _r->start + timeline->x_to_ts( 1 );
            else
                _r->end -= td;

            break;
        }
        default:
            return;

    }
}

/* convert a screen x coord into an offset into the region */
#define x_to_offset( X ) ( timeline->x_to_ts( scroll_x() + ( (X) - _track->x() ) ) - _r->offset )

int
Region::handle ( int m )
{
    static int ox, oy;
    static enum trim_e trimming;

    static bool copied = false;
    static nframes_t os;

//    int X = Fl::event_x() - _track->x();
    int X = Fl::event_x();
    int Y = Fl::event_y();

    int ret;

    Logger _log( this );
//log_r->start();

    switch ( m )
    {
        case FL_ENTER:
            Track_Widget::handle( m );
            redraw();
            break;
        case FL_LEAVE:
            Track_Widget::handle( m );
            redraw();
            break;
        case FL_KEYBOARD:
        {
            if ( Fl::event_key() == FL_F + 3 )
            {
                nframes_t offset = x_to_offset( X );

                if ( offset < length() )
                    _fade_in.length = offset;

                printf( "setting fade in length to %lu\n", _fade_in.length );
            }
            else
            if ( Fl::event_key() == FL_F + 4 )
            {
                long offset = length() - x_to_offset( X );

                if ( offset > 0 )
                    _fade_out.length = offset;

                printf( "setting fade out length to %lu\n", _fade_in.length );
            }


            redraw();
            return 1;
        }
        case FL_PUSH:
        {

            /* trimming / splitting  */
            if ( Fl::event_shift() && ! Fl::event_ctrl() )
            {
                switch ( Fl::event_button() )
                {
                    case 1:
                        trim( trimming = LEFT, X );
                        begin_drag( Drag( x() - X, y() - Y ) );
                        _log.hold();
                        break;
                    case 3:
                        trim( trimming = RIGHT, X );
                        begin_drag( Drag( x() - X, y() - Y ) );
                        _log.hold();
                        break;
                    case 2:
                    {
                        /* split */
                        if ( ! copied )
                        {
                            Loggable::block_start();

                            Region *copy = new Region( *this );

                            trim( RIGHT, X );
                            copy->trim( LEFT, X );

                            _track->add( copy );

                            log_end();

                            Loggable::block_end();
                            return 0;
                        }
                    }
                    default:
                        return 0;
                        break;
                }

                fl_cursor( FL_CURSOR_WE );
                return 1;
            }
            else
            {
                ox = x() - X;
                oy = y() - Y;
                /* for panning */
                os = _r->start;

                /* normalization and selection */
                if ( Fl::event_button2() )
                {
                    if ( Fl::event_ctrl() )
                        normalize();
                    else
                    {
                        if ( Track_Widget::current() == this )
                        {
                            if ( selected() )
                                deselect();
                            else
                                select();
                        }
                    }

                    redraw();
                    goto changed;
                }
                if ( Fl::event_button1() && Fl::event_ctrl() )
                {
                    /* duplication */
                    return 1;
                }
                else
                    return Track_Widget::handle( m );
            }
            break;
        }
        case FL_RELEASE:

        {
            Track_Widget::handle( m );

            copied = false;
            if ( trimming != NO )
                trimming = NO;

            goto changed;
        }
        case FL_DRAG:

            if ( ! _drag )
            {
                begin_drag( Drag( x() - X, y() - Y ) );
                _log.hold();
            }

            /* panning */
            if ( Fl::event_state() & FL_SHIFT &&
                 Fl::event_state() & FL_CTRL )
            {
                int d = (ox + X) - x();
                long td = timeline->x_to_ts( d );

                nframes_t W = _r->end - _r->start;

                if ( td > 0 && os < td )
                    _r->start = 0;
                else
                    _r->start = os - td;

                _r->end = _r->start + W;

                _track->redraw();
                return 1;
            }

            /* trimming */
            if ( Fl::event_state() & FL_SHIFT )
                if ( trimming )
                {
                    trim( trimming, X );
                    return 1;
                }
                else
                    return 0;

            /* duplication */
            if ( Fl::event_state() & FL_CTRL )
            {
                if ( _drag->state == 0 )
                {
                    _track->add( new Region( *this ) );
                    _drag->state = 1;
                    return 1;
                }
            }

            /* track jumping */
            if ( ! selected() )
            {
                if ( Y > y() + h() )
                {
                    Fl::copy( class_name(), strlen( class_name() ), 0 );
                    Fl::dnd();
                }
                else
                    if ( Y < y() )
                    {
                        Fl::copy( class_name(), strlen( class_name() ), 0 );
                        Fl::dnd();
                    }
            }

            ret = Track_Widget::handle( m );
            return ret | 1;
        default:
            return Track_Widget::handle( m );
            break;
    }

changed:

    return 1;

}


/** Draws the curve for a single fade. /X/ and /W/ repersent the
 portion of the region covered by this draw, which may or may not
 cover the fade in question. */
void
Region::draw_fade ( const Fade &fade, Fade::fade_dir_e dir, bool filled, int X, int W )
{
    const int dy = y() + Fl::box_dy( box() );
    const int dh = h() - Fl::box_dh( box() );
    const int height = dh;
    const int width = timeline->ts_to_x( fade.length );

    fl_color( fl_lighter( FL_BLACK ) );

    fl_push_matrix();

    if ( dir == Fade::In )
    {
        fl_translate( line_x(), dy );
        fl_scale( width, height );
    }
    else
    {
        //      fl_translate( line_x() + abs_w(), dy + height );
        fl_translate( line_x() + abs_w(), dy );
        fl_scale( width, height );

        /* flip */
        fl_scale( -1.0, 1.0 );
//        fl_scale( 1.0, -1.0 );
    }

    if ( filled )
        fl_begin_polygon();
    else
        fl_begin_line();

    fl_vertex( 0.0, 0.0 );
    fl_vertex( 0.0, 1.0 );

    for ( int i = 0; i < width; ++i )
    {
        const float x = i / (float)width;
        const float y = 1.0f - fade.gain( timeline->x_to_ts( i ) );

        fl_vertex( x, y );
    }

    if ( filled )
        fl_end_polygon();
    else
        fl_end_line();

    fl_pop_matrix();
}

void
Region::draw_box( int X, int Y, int W, int H )
{
    if ( ! shown() )
        return;

    /* dirty hack to keep the box from flipping to vertical at small sizes */

    fl_push_clip( x(), Y, w(), H );

    if ( selected() )
        fl_draw_box( fl_down( box() ), x() - 10, y(), w() + 50, h(), _selection_color );
//        fl_draw_box( fl_down( box() ), x() - 10, Y, w() + 50, H, fl_invert_color( _box_color ) );
    else
        fl_draw_box( box(), x() - 10, y(), w() + 50, h(), _box_color );

    /* draw fades */
    draw_fade( _fade_in, Fade::In, true, X, W );
    draw_fade( _fade_out, Fade::Out, true, X, W );

    fl_pop_clip();
}

/* Draw (part of) region. OX is pixel offset from start of timeline, X
   Y W and H are the portion of the widget to draw (arrived at by
   intersection of the clip and relative to OX) */
void
Region::draw ( int X, int Y, int W, int H )
{
    if ( ! shown() )
        return;

    if ( ! ( W > 0 && H > 0 ) )
        return;

    int OX = scroll_x();
    int ox = timeline->ts_to_x( _r->offset );

    if ( ox > OX + _track->w() ||
         ox < OX && ox + abs_w() < OX )
        return;

    int rw = timeline->ts_to_x( _r->end - _r->start );

//    nframes_t end = _r->offset + ( _r->end - _r->start );

    /* calculate waveform offset due to scrolling */
    nframes_t offset = 0;
    if ( ox < OX )
    {
        offset = timeline->x_to_ts( OX - ox );

        rw = timeline->ts_to_x( (_r->end - _r->start) - offset );
    }

    rw = min( rw, _track->w() );

    int rx = x();

    fl_push_clip( rx, Y, rw, H );

    /* get actual peak data */
    int channels;
    int peaks;
    Peak *pbuf;

    const nframes_t start = _r->start + offset + timeline->x_to_ts( X - rx );
    _clip->read_peaks( timeline->fpp(),
                       start,
                       start + timeline->x_to_ts( W ),
                       &peaks, &pbuf, &channels );

    assert( pbuf );

    /* draw fade curve outlines--this is only here because of crossfades */
    draw_fade( _fade_in, Fade::In, false, X, W );
    draw_fade( _fade_out, Fade::Out, false, X, W );

    int ch = (h() - Fl::box_dh( box() ))  / channels;

    for ( int i = 0; i < channels; ++i )
    {
        Peak *pb = pbuf + (peaks * i);

        /* scale it */
        for ( int j = peaks; j--; )
        {
            pb[ j ].min *= _scale;
            pb[ j ].max *= _scale;
        }

/*         int fw = timeline->ts_to_x( fade.length ); */

/*         /\* if ( draw_fade_waveform ) *\/ */
/*         for ( int j = min( fw, peaks ); j--; ) */
/*         { */
/*             const float g = fade.gain( j * timeline->fpp() ); */
/*             pb[ j ].min *= g; */
/*             pb[ j ].max *= g; */
/*         } */

        Waveform::draw( X, (y() + Fl::box_dy( box() )) + (i * ch), W, ch,
                        pb, peaks,
                        selected() ? fl_invert_color( _color ) : _color );
    }

    delete[] pbuf;

/*     for ( int i = _clip->channels(); i--; ) */
/*         Waveform::draw( rx, X, (y() + Fl::box_dy( box() )) + (i * ch), W, */
/*                         ch, _clip, i, timeline->fpp(), */
/*                        _r->start + offset, min( (_r->end - _r->start) - offset, _r->end), */
/*                        _scale, selected() ? fl_invert_color( _color ) : _color ); */




    timeline->draw_measure_lines( rx, Y, rw, H, _box_color );

    fl_color( FL_BLACK );
    fl_line( rx, Y, rx, Y + H );
    fl_line( rx + rw - 1, Y, rx + rw - 1, Y + H );

    draw_label( _clip->name(), align() );

    if ( current() )
    {
        /* draw length bubble */

        char pat[40];

        snprintf( pat, sizeof( pat ), "%dm:%.1fs", (int)(length() / timeline->sample_rate()) / 60, (double)length() / timeline->sample_rate() );

        draw_label( pat, (Fl_Align)(FL_ALIGN_INSIDE | FL_ALIGN_CENTER), FL_GREEN );
    }

    fl_pop_clip();

}


void
Region::normalize ( void )
{
    printf( "normalize: start=%lu end=%lu\n", _r->start, _r->end );

    /* FIXME: figure out a way to do this via the peak server */
/*     _scale = _clip->peaks( 0 )->normalization_factor( timeline->fpp(), _r->start, _r->end );  */

}




/**********/
/* Engine */
/**********/


/** Apply a (portion of) fade-out from /start/ to /end/ assuming a
 * buffer size of /nframes/. /start/ and /end/ are relative to the
 * given buffer, and /start/ may be negative. */
void
Region::Fade::apply ( sample_t *buf, Region::Fade::fade_dir_e dir, long start, nframes_t end, nframes_t nframes ) const
{
    printf( "apply fade %s: start=%ld end=%lu\n", dir == Fade::Out ? "out" : "in", start, end );

    nframes_t i = start > 0 ? start : 0;
    nframes_t e = end > nframes ? nframes : end;

    if ( dir == Fade::Out )
        for ( ; i < e; ++i )
            buf[ i ] *= gain( (length - 1) - (i - start) );
    else
        for ( ; i < e; ++i )
            buf[ i ] *= gain( i - start );
}


#if 0
/** Compute the gain value (0 to 1f) for a fade-in/out curve of /type/
 * (LINEAR, QUADRAIC, CUBIC), of /nframes/ in length at point
 * /offset/ */
static inline
float gain_on_curve ( int type, int dir, nframes_t nframes, nframes_t offset, nframes_t length )
{
    float a, b;

    /* FIXME: these first two sections should *definitely* be cached */

    /* calculate coefficients */
    if ( dir == FADE_OUT )
    {
        a = -1.0f / (double)nframes;
        /* fixme why would we need to know the clip length? */
        b = length / (double)nframes;
//        b = nframes;
    }
    else
    {
        a = 1.0f / (double)nframes;
        b = 0.0f;
    }

    float c[4];

    /* interpolate points */
    switch ( type )
    {
        case Linear:
            c[1] = a;
            c[0] = b;
            break;
        case Quadratic:
            c[2] = a * a;
            c[1] = 2.0f * a * b;
            c[0] = b * b;
            break;
        case Cubic:
        {
            const float a2 = a * a;
            const float b2 = b * b;
            c[3] = a * a2;
            c[2] = 3.0f * a2 * b;
            c[1] = 3.0f * a * b2;
            c[0] = b * b2;
            break;
        }
        default:
            printf( "unknown curve order\n" );
    }

    /* now get the gain for the given point */

    const float f  = offset;
    const float f2 = f * f;

    float g = 1.0f;

    switch ( type )
    {
        case Linear:
            g *= c[1] * f + c[0];
            break;
        case Quadratic:
            g *= c[2] * f2 + c[1] * f + c[0];
            break;
        case Cubic:
            g *= c[3] * f2 * f + c[2] * f2 + c[1] * f + c[0];
            break;
    }

    printf( "gain for %lu is %f\n", offset, g );

    return g;
}
#endif

/* THREAD: IO */
/** read the overlapping part of /channel/ at /pos/ for /nframes/ of
    this region into /buf/, where /pos/ is in timeline frames */
/* this runs in the diskstream thread. */
/* FIXME: it is far more efficient to read all the channels from a
 multichannel source at once... But how should we handle the case of a
 mismatch between the number of channels in this region's source and
 the number of channels on the track/buffer this data is being read
 for? Would it not be better to simply buffer and deinterlace the
 frames in the Audio_File class instead, so that sequential requests
 for different channels at the same position avoid hitting the disk
 again? */
nframes_t
Region::read ( sample_t *buf, nframes_t pos, nframes_t nframes, int channel ) const
{
    const Range r = _range;

    const nframes_t length = r.end - r.start;

    /* do nothing if we aren't covered by this frame range */
    if ( pos > r.offset + length || pos + nframes < r.offset )
        return 0;

    /* calculate offsets into file and sample buffer */

    nframes_t sofs, ofs, cnt;

    cnt = nframes;

    if ( pos < r.offset )
    {
        sofs = 0;
        ofs = r.offset - pos;
        cnt -= ofs;
    }
    else
    {
        ofs = 0;
        sofs = pos - r.offset;
    }

    if ( ofs >= nframes )
        return 0;

//    const nframes_t start = ofs + r.start + sofs;
    const nframes_t start =  r.start + sofs;
    const nframes_t len = min( cnt, nframes - ofs );
    const nframes_t end = start + len;

    if ( len == 0 )
        return 0;

    /* now that we know how much and where to read, get on with it */

    /* FIXME: seeking can be very expensive. Esp. with compressed
     * formats. We should attempt to avoid it. But here or in the
     * Audio_File class? */

    //    printf( "reading region ofs = %lu, sofs = %lu, %lu-%lu\n", ofs, sofs, start, end  );

    cnt = _clip->read( buf + ofs, channel, start, end );

    /* apply gain */

    if ( _scale != 1.0f )
        for ( int i = cnt; i--; )
            buf[i] *= _scale;

    /* perform declicking if necessary */

    /* FIXME: keep the declick defults someplace else */
    Fade declick;

    declick.length = 256;
    declick.type   = Fade::Linear;

    {
        Fade fade;

        fade = declick < _fade_in ? _fade_in : declick;

        /* do fade in if necessary */
        if ( sofs < fade.length )
        {
            const long d = 0 - sofs;

            fade.apply( buf + ofs, Fade::In, d, d + fade.length, cnt - ofs );
        }

        fade = declick < _fade_out ? _fade_out : declick;

        /* do fade out if necessary */
        if ( start + cnt + fade.length > r.end )
        {
            const nframes_t d = r.end - start;

            fade.apply( buf, Fade::Out, cnt + (long)d - fade.length, cnt + d, cnt );
        }
    }
//    printf( "read %lu frames\n", cnt );

    return cnt;
}
