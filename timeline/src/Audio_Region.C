
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
#include "Transport.H"
#include "const.h"
#include "debug.h"
#include <FL/focus_frame.H>

#include <algorithm>
using std::min;
using std::max;



/* defined in timeline.C */
extern void draw_full_arrow_symbol ( Fl_Color color );

extern Timeline *timeline;
extern Transport *transport;

bool Audio_Region::inherit_track_color = true;
bool Audio_Region::show_box = true;

Fl_Boxtype Audio_Region::_box = FL_BORDER_BOX;

Fl_Color Audio_Region::_selection_color = FL_MAGENTA;



/* static Fl_Color fl_invert_color ( Fl_Color c ) */
/* { */
/*     unsigned char r, g, b; */

/*     Fl::get_color( c, r, g, b ); */

/*     return fl_rgb_color( 255 - r, 255 - g, 255 - b ); */
/* } */



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
    _adjusting_gain = 0;
    _loop = 0;
    _sequence = NULL;
    _scale = 1.0f;
    _clip = NULL;

    _color = FL_FOREGROUND_COLOR;
    _box_color = FL_GRAY;

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

    _box_color = rhs._box_color;
    _color = rhs._color;
    
    _adjusting_gain = 0.0f;

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

/* used when DND importing and when recording. must not invoke log_create() */
Audio_Region::Audio_Region ( Audio_File *c, Sequence *t, nframes_t o )
{
    init();
    _clip = c;
    _r->offset = 0;
    _r->start = o;
    _r->length = _clip->length();

    int sum = 0;
    const char *s = rindex( _clip->name(), '/' );
    if ( ! s )
        s = _clip->name();

    for ( int i = strlen( s ); i--; )
        sum += s[ i ];

    while ( sum >> 8 )
        sum = (sum & 0xFF) + (sum >> 8);

    _box_color = (Fl_Color)sum;

    t->add( this );
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
    else if ( ! strcmp( picked, "/Split at mouse" ) )
    {
	split( timeline->x_to_offset( Fl::event_x() ) );
    }
    else if ( ! strcmp( picked, "/Crop to range" ) )
    {
	trim_left( timeline->range_start() );
	trim_right( timeline->range_end() );
    }
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
    else if ( ! strcmp( picked, "/Gain with mouse vertical drag" ) )
    {
        /* float g = h() / (y() - Fl::event_y() ); */

        /* _scale = g; */
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
    else if ( ! strcmp( picked, "/Trim left to playhead" ) )
    {
        redraw();
        trim_left( transport->frame );
    }
    else if ( ! strcmp( picked, "/Trim right to playhead" ) )
    {
        redraw();
        trim_right( transport->frame );
    }
    else if ( ! strcmp( picked, "/Split at playhead" ) )
    {
        redraw();
	
        split( transport->frame );
    }
    else if ( ! strcmp( picked, "/Loop point at playhead" ) )
    {
        nframes_t f = transport->frame;

        _loop = f - _r->start;
    }
    else if ( ! strcmp( picked, "/Fade in to playhead" ) )
    {
        nframes_t offset = transport->frame - _r->start;

        if ( offset < length() )
            _fade_in.length = offset;
    }
    else if ( ! strcmp( picked, "/Fade out to playhead" ) )
    {
        nframes_t offset = _r->start + _r->length - transport->frame;
        
        if ( offset > 0 )
            _fade_out.length = offset;
    }
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
	    { "Crop to range", 'c', 0, 0, FL_MENU_DIVIDER },
	    { "Split at mouse", 's', 0, 0 },
            { "Gain with mouse vertical drag", 'g', 0, 0 },
            { "Fade in to mouse", FL_F + 3, 0, 0 },
            { "Fade out to mouse", FL_F + 4, 0, 0 },
            { "Loop point to mouse", 'l', 0, 0 },
            { "Clear loop point", 0, 0, 0, 0 == _loop ? FL_MENU_INACTIVE : 0 },
            { "Normalize", 'n', 0, 0 },
            { "Denormalize", FL_SHIFT + 'n', 0, 0, 1.0 == _scale ? FL_MENU_INACTIVE : 0 },
            { "Range from", FL_CTRL + 'r', 0, 0, FL_MENU_DIVIDER },
            { "Trim left to playhead", '{', 0, 0 },
            { "Trim right to playhead", '}', 0, 0 },
            { "Split at playhead", FL_SHIFT + 's', 0, 0 },
            { "Loop point at playhead", FL_SHIFT + 'l', 0, 0 },
            { "Fade in to playhead", FL_F + 3 + FL_SHIFT, 0, 0 },
            { "Fade out to playhead", FL_F + 4 + FL_SHIFT, 0, 0 },
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

    if ( width < 4 )
        /* too small to draw */
        return;
 
    int fx;
                                                
    if ( dir == Fade::In )
    {
        fx = line_x();
     
        if ( fx + width < X ||
                          fx > X + W )
            /* clipped */
            return;
    }
    else
    {
        fx = line_x() + abs_w();

        if ( fx - width > X + W || 
             fx < X )
            /* clipped */
            return;
    }

    if ( line )
        fl_begin_line();
    else
        fl_begin_polygon();

    fl_vertex( fx, dy );
    fl_vertex( fx, dy + height );

    {
        const float ti = 1.0f / (float)width;
        float ts = 0.0f;
        
        const int xi = dir == Fade::In ? 1 : -1;

        for ( int i = 0; i < width; i++, ts += ti, fx += xi )
            fl_vertex( fx, dy + height - ( height * fade.gain( ts )));
    }
    
    fl_vertex( fx, dy );

    if ( line )
        fl_end_line();
    else
        fl_end_polygon();
}

Fl_Color
Audio_Region::actual_box_color ( void ) const
{
    return Audio_Region::inherit_track_color ? sequence()->track()->color() :  _box_color;
}

void
Audio_Region::draw_box( void )
{
//    fl_push_clip( x(), y(), w(), h() );

    Fl_Color selection_color = _selection_color;

    Fl_Color color = actual_box_color();

    color = fl_color_average( color, sequence()->color(), 0.75f );

    if ( recording() )
    {
        color = FL_RED;
    }
    else if ( ! active_r() )
    {
        color = fl_inactive( color );
        selection_color = fl_inactive( selection_color );
    }

    Fl_Boxtype b;
    Fl_Color c = selected() ? fl_color_average( color, fl_rgb_color(10,10,10), 0.4f ) : color;

    if ( Audio_Region::show_box )
    {
        b = box();
    }
    else
    {
        b = FL_DOWN_FRAME;
    }

    fl_draw_box( b, line_x(), y(), abs_w(), h(), c );
 

//    fl_pop_clip();
}

void
Audio_Region::peaks_ready_callback ( void *v )
{
    /* this is called from the peak builder thread */

    DMESSAGE("Damaging region from peaks ready callback");
    Fl::lock();
    ((Audio_Region*)v)->redraw();
    Fl::unlock();
    Fl::awake();
}

bool
Audio_Region::recording ( void ) const
{
    return this == sequence()->track()->capture_region();
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

    
    if ( start() > timeline->xoffset + timeline->x_to_ts( sequence()->drawable_w() ) ||
         start() + length() < timeline->xoffset )
        /* not in viewport */
        return;

    fl_push_clip( X, Y, W, H );

    /* overdraw a little to avoid artifacts when scrolling */
    W += 2;

//    Fl_Color c = selected() ? fl_invert_color( _color ) : _color;

    if ( sequence()->damage() & FL_DAMAGE_USER1 && 
         recording() )
    {
        /* TODO: limit drawing. */
    }

    /* calculate waveform offset due to scrolling */
    /* offset is the number of frames into the waveform the value of X translates to */

    /* this is the timestamp at where we'll actually be drawing. */
    nframes_t x_frame = timeline->x_to_ts(
        timeline->ts_to_x( timeline->xoffset ) + ( X - _sequence->drawable_x() ) );

    nframes_t offset = 0;

    if ( x_frame > start() )
        offset = x_frame - start();

    nframes_t fo = 0;                                           
    nframes_t ostart = 0, oend = 0;
    const int total_peaks_needed = W;
    nframes_t total_frames_needed = timeline->x_to_ts( total_peaks_needed );

    {
        /* Fl_Color c = fl_color_average( FL_DARK1, */
        /*                                Audio_Region::inherit_track_color ? sequence()->track()->color() :  _box_color, */
        /*                                0.75f ); */
    
        fl_color( fl_color_add_alpha( FL_DARK1, 127 ) );

        draw_fade( _fade_in, Fade::In, false, X, W );
        draw_fade( _fade_out, Fade::Out, false, X, W );
    }

    int channels = 0;
    int peaks = 0;
    Peak *pbuf = NULL;  


    Fl_Color fg_color = FL_FOREGROUND_COLOR;
    Fl_Color bg_color = FL_BACKGROUND_COLOR;

    if ( !active_r() )
    {
        fg_color = fl_inactive(fg_color);
        bg_color = fl_inactive(bg_color);
    }
    
    do {

        nframes_t loop_frames_needed = _loop ? _loop : total_frames_needed;
        int loop_peaks_needed = timeline->ts_to_x( loop_frames_needed );

        nframes_t start = _r->offset;
        
        if ( ! fo )                                             /* first loop... */
        {
            if ( _loop )
            {
//                start += offset;
                start += offset % _loop;
                loop_frames_needed -= offset % loop_frames_needed;
                loop_peaks_needed = timeline->ts_to_x( loop_frames_needed );
            }
            else
                start += offset;

            assert( loop_peaks_needed >= 0 );
        }

        if ( fo + loop_frames_needed > total_frames_needed )
        {
            loop_frames_needed -= ( fo + loop_frames_needed ) - total_frames_needed;
            loop_peaks_needed = timeline->ts_to_x( loop_frames_needed );
        }

        if ( !loop_peaks_needed )
            break;

        const nframes_t end = start + loop_frames_needed;

        if ( start != ostart || end != oend )
        {
            _clip->peaks()->peakfile_ready();

            if ( _clip->read_peaks( timeline->fpp(),
                                    start,
                                    end,
                                    &peaks, &pbuf, &channels ) )
            {
                Waveform::scale( pbuf, peaks * channels, _scale );
                    
                ostart = start;
                oend = end;
            }
                        
            if ( _clip->peaks()->needs_more_peaks() && ! transport->rolling )
            {
                /* maybe create a thread to make the peaks */
                /* this function will just return if there's nothing to do. */
                _clip->peaks()->make_peaks_asynchronously( Audio_Region::peaks_ready_callback, this );
            }
        }
        else
        {
//            DMESSAGE( "using cached peaks" );
        }
        
        if ( peaks && pbuf )
        {
            int ch = (h() - Fl::box_dh( box() ))  / channels;
            int xo = timeline->ts_to_x( fo );

            for ( int i = 0; i < channels; ++i )
            {
                Waveform::draw( X + xo,
                                (y() + Fl::box_dy( box() )) + (i * ch),
                                loop_peaks_needed,
                                ch,
                                pbuf + i, peaks, channels,
                                fg_color, bg_color );
            }
        }
        else
            ;
//            WARNING( "Pbuf == %p, peaks = %lu", pbuf, (unsigned long)peaks );


        
        if ( _loop )
        {
            const int lx = sequence()->drawable_x() + timeline->ts_to_x( ( this->start() + _loop ) - timeline->xoffset );
            
            if ( lx < X + W )
            {
                fl_color( fl_darker( FL_CYAN ) );
                fl_line( lx, y(), lx, y() + h() );
                fl_line( lx - 3, y(), lx + 3, y() );
                fl_line( lx - 3, y() + h() - 1, lx + 3, y() + h() - 1 );
               
            }
        }
        
        if ( peaks < loop_peaks_needed )
        {
//            DMESSAGE( "Peak read came up %lu peaks short", (unsigned long)loop_peaks_needed - peaks );
        }

        fo += loop_frames_needed;
 
    }
    while ( _loop && fo < total_frames_needed );

    

    if ( _adjusting_gain > 0.0f )
    {
        fl_color( fl_color_add_alpha( FL_DARK1, 127 ) );

        fl_rectf( X, ( y() + h() ) - ( h() * ( _scale * 0.25 ) ), X + W, y() + h() );

        fl_line_style( FL_DASH, 1 );
        
        fl_color( fl_color_add_alpha( FL_GREEN, 200 ) );
        
        float j = 5;

        for ( int i = y() + h(); i > y(); i -= j, j *= 1.2 )
        {
            fl_line( X, i, X + W, i );
        }
        
        fl_line_style( FL_SOLID, 0 );
    }

    if ( selected() )
        draw_selection_frame( line_x() + Fl::box_dx(box()),
                              y() + Fl::box_dy(box()),
                              abs_w() - Fl::box_dw(box()),
                              h() - Fl::box_dh(box()),
                              selection_color() );

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
Audio_Region::draw_label ( void )
{
  if ( _clip->dummy() )
    {
        char pat[256];
        snprintf( pat, sizeof( pat ), "Missing Source!: %s", _clip->name() );
        draw_label( pat, align() );
    }
    else
        draw_label( _clip->name(), align() );
}

/** split region at absolute frame /where/ */
void
Audio_Region::split ( nframes_t where )
{
    block_start();
    
    nframes_t old_fade_in = _fade_in.length;

    _fade_in.length = 256;

    Audio_Region *copy = new Audio_Region( *this );

    {
        Logger _log( copy );

        _fade_in.length = old_fade_in;
        _fade_out.length = 256;

        Sequence_Region::split( copy, where );
    }

    log_end();
    
    block_end();

    log_start();
}

int
Audio_Region::handle ( int m )
{
    static int ox;

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
        case FL_KEYUP:
            if ( _adjusting_gain > 0 )
            {
                _adjusting_gain = 0;
                redraw();
                return 1;
            }
            break;
        case FL_KEYBOARD:
            if ( Fl::event_key() == 'g' )  
            {
                if ( _adjusting_gain <= 0 )
                {
                    _adjusting_gain = _scale;
                    redraw();
                }
                return 1;
            }
            return menu().test_shortcut() != 0;
        case FL_ENTER:
            return Sequence_Region::handle( m );
        case FL_LEAVE:
            if ( _adjusting_gain > 0 )
            {
                _adjusting_gain = 0;
                redraw();
            }
            return Sequence_Region::handle( m );
        case FL_PUSH:
        {
            if ( _adjusting_gain > 0.0f )
            {
                _adjusting_gain = _scale;
                return 1;
            }

            /* splitting  */
            if ( test_press( FL_BUTTON2 | FL_SHIFT ) )
            {
                /* split */
                if ( ! copied )
                {
                    split( timeline->x_to_offset( X ) );
                }

                return 0;
            }
            else
            {
                ox = x() - X;
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

            if ( Fl::event_is_click() )
                return 1;

            if ( ! _drag )
            {
                begin_drag( Drag( X, Y, x_to_offset( X ) ) );
                _log.hold();
            }

            if ( _adjusting_gain )
            {
                int d = _drag->y - Y;
                
                _scale = _adjusting_gain + ( 0.01f * d );

                if ( _scale < 0.01f )
                    _scale = 0.01f;

                redraw();
                return 1;
            }

            if ( test_press( FL_BUTTON1 | FL_SHIFT | FL_CTRL ) )
            {
                /* panning */
                int d = (ox + X) - x();

                if ( d < 0 )
                    _r->offset = os + timeline->x_to_ts( 0 - d );
                else
                {
                    if ( os < timeline->x_to_ts( d ) )
                        _r->offset = 0;
                    else
                        _r->offset = os - timeline->x_to_ts( d );
                }

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
    {
        _scale = 1000.0f;

        for ( int i = 0; i < channels; i++ )
        {
            float f = (pbuf + i)->normalization_factor();
            if ( f < _scale )
                _scale = f;
        }
    }

    /* FIXME: wrong place for this? */
    sequence()->handle_widget_change( start(), length() );
    redraw();
}
