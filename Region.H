
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

#pragma once

// class Track;

// #include "Waveform.H"

#include "Clip.H"
#include "Track.H"
#include "Timeline.H"

#include <algorithm>
using namespace std;
/* Regions are "virtual" FLTK widgets; this is necessary because the
 * dimensions of real FLTK widgets are limited to 16-bits, which is
 * far too little for our purposes */


/* Base class for virtual widget on a track */
class Track_Widget
{

protected:

    Track *_track;                                              /* track this region belongs to */

    nframes_t _offset;                                          /* where on the timeline */
    nframes_t _start;                                           /* first sample from clip */
    nframes_t _end;                                             /* last sample from clip */

    bool _selected;

    Fl_Color _color;                                            /* color of waveform */
    Fl_Color _box_color;                                        /* color of background (box) */

public:

    Track_Widget ( )
        {
            _track = NULL;

            _offset = _start = _end = 0;

            _selected = false;
        }


    Fl_Group * parent ( void ) const { return _track; }

    int scroll_x ( void ) const { return timeline.ts_to_x( timeline.xoffset ); }
    nframes_t scroll_ts ( void ) const { return timeline.xoffset; }

    int y ( void ) const { return _track->y(); }
    int h ( void ) const { return _track->h(); }
    int x ( void ) const { return _offset < timeline.xoffset ? -1 : min( 32767, _track->x() + timeline.ts_to_x( _offset - timeline.xoffset ) ); }
//    int x ( void ) const { return (short)( _track->x() + timeline.ts_to_x( _offset - timeline.xoffset )); }

    virtual int w ( void ) const { return timeline.ts_to_x( _end - _start ); }
//    int w ( void ) const { return timeline.ts_to_x( (_end - _start) - ( timeline.xoffset - _offset) ); }

    Fl_Color color ( void ) { return _color; }
    Fl_Color box_color ( void ) { return _box_color; }

    Track * track ( void ) const { return _track; }
    void track ( Track *t ) {  _track = t; }

    nframes_t offset ( void ) const { return _offset; }
    void offset ( nframes_t o ) { _offset = o; }

    void end ( nframes_t v ) { _end = v; }
    nframes_t end ( void ) const { return _end; }
    void start ( nframes_t v ) { _start = v; }
    nframes_t start ( void ) const { return _start; }

    virtual nframes_t length ( void ) const { return _end - _start; }

    /* just draw a simple box */
    virtual void
    draw ( int X, int Y, int W, int H )
        {
            fl_draw_box( FL_FLAT_BOX, x(), y(), w(), y(), _box_color );
        }

    /* base hanlde just does basic dragging */
    virtual int
    handle ( int m )
        {
            static int ox, oy;

            int X = Fl::event_x();
            int Y = Fl::event_y();

            switch ( m )
            {
                case FL_PUSH:
                {
                    ox = x() - X;
                    oy = y() - Y;

                    return 1;
                }
                case FL_RELEASE:
                    fl_cursor( FL_CURSOR_DEFAULT );
                    return 1;
                case FL_DRAG:
                {
                    if ( ox + X >= _track->x() )
                    {
                        int nx = ox + X;

                        _offset = timeline.x_to_ts( nx ) + timeline.xoffset;

                        _track->snap( this );
                    }

                    _track->redraw();
                    fl_cursor( FL_CURSOR_MOVE );

                    return 1;
                }
                default:
                    return 0;
            }
        }

    bool
    operator< ( const Track_Widget & rhs )
        {
            return _offset < rhs._offset;
        }
};


    class Tempo_Point : public Track_Widget
    {
        float _tempo;

        static Fl_Boxtype box ( void ) { return FL_UP_BOX; }

    public:

        Tempo_Point ( )
            {
                _tempo = 120;
                _offset = 0;
                _start = 0;
                _end = 300;
            }

        Tempo_Point ( nframes_t when, float bpm )
            {
                _tempo = bpm;
                _offset = when;
                _start = 0;
                _end = 300;
            }

        int w ( void ) const { return 10; }
        nframes_t length ( void ) const { return timeline.x_to_ts( w() ); }

        void
        draw ( int X, int Y, int W, int H )
            {
                if ( x() < 0 )
                    return;

                fl_draw_box( box(), x(), Y, w(), H, _box_color );

                char pat[40];

                snprintf( pat, 40, "%.1f", _tempo );

                fl_font( FL_HELVETICA, 14 );
                fl_color( FL_BLACK );
                fl_draw( pat, x() + w() + 1, Y + 1, w(), h(), FL_ALIGN_LEFT );
                fl_color( FL_WHITE );
                fl_draw( pat, x() +  w(), Y, w(), h(), FL_ALIGN_LEFT );

            }

    };

    class Region : public Track_Widget
    {

        Clip *_clip;                                                /* clip this region represents */

        float _scale;                                               /* amplitude adjustment */

        static Fl_Boxtype _box;
        static Fl_Color _selection_color;
        static Fl_Color selection_color ( void ) { return _selection_color; }
        static void selection_color ( Fl_Color v ) { _selection_color = v; }
        static Fl_Boxtype box ( void ) { return _box; }

        enum trim_e { NO, LEFT, RIGHT };
        void trim ( enum trim_e t, int X );
        void init ( void );

    public:

        Region ( const Region & rhs );
        Region ( Clip *c );

        int handle ( int m );
        void draw ( int X, int Y, int W, int H );
        void resize ( void );

    };
