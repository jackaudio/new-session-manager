
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

#include <FL/Fl_Widget.H>
#include <FL/fl_draw.H>

#include "Timeline.H"

#include "Peaks.H"

class Waveform {

public:

    static bool fill;
    static bool outline;
    static bool vary_color;
    static bool logarithmic;

    static void scale ( Peak *pbuf, int npeaks, float gain );
    static void draw ( int X, int Y, int W, int H,
                       const Peak *pbuf, int peaks, int skip,
                       Fl_Color color );

};
