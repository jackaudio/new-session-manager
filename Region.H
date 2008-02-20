
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

class Region
{

    Track *_track;                                              /* track this region belongs to */

    Clip *_clip;                                                /* clip this region represents */

    nframes_t _offset;                                          /* where on the timeline */
    nframes_t _start;                                           /* first sample from clip */
    nframes_t _end;                                             /* last sample from clip */

    float _scale;                                               /* amplitude adjustment */

    enum trim_e { NO, LEFT, RIGHT };
    void trim ( enum trim_e t, int X );
    void init ( void );

public:

    Region ( const Region & rhs );
    Region ( Clip *c );

    int y ( void ) { return _track->y(); }
    int h ( void ) { return _track->h(); }
    int x ( void ) { return timeline.ts_to_x( _offset ) - timeline.xoffset; }

    int handle ( int m );
    void draw ( int X, int Y, int W, int H );
    void resize ( void );

    Track * track ( void ) const { return _track; }
    void track ( Track *t ) {  _track = t; }

    nframes_t offset ( void ) const { return _offset; }
    void offset ( nframes_t o ) { _offset = o; }

    nframes_t length ( void ) const { return _end - _start; }

    void end ( nframes_t v ) { _end = v; }
};
