
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


/* Regions are "virtual" FLTK widgets; this is necessary because the
 * dimensions of real FLTK widgets are limited to 16-bits, which is
 * far too little for our purposes */


/* Base class for virtual widget on a track */
class TrackWidget
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

    Fl_Group * parent ( void ) const { return _track; }

    int y ( void ) const { return _track->y(); }
    int h ( void ) const { return _track->h(); }
    int x ( void ) const { return _track->x() + timeline.ts_to_x( _offset - timeline.xoffset ); }
    int w ( void ) const { return timeline.ts_to_x( _end - _start ); }

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

    nframes_t length ( void ) const { return _end - _start; }
};

class Region : public TrackWidget
{

    Clip *_clip;                                                /* clip this region represents */

    float _scale;                                               /* amplitude adjustment */

    static Fl_Color _selection_color;
    static Fl_Color selection_color ( void ) { return _selection_color; }
    static void selection_color ( Fl_Color v ) { _selection_color = v; }

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
