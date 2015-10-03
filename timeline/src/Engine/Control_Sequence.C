
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

#include "../Control_Sequence.H"

#include "../Transport.H" // for ->frame

#include "const.h"
#include "debug.h"
#include "Thread.H"

#include <list>
using std::list;



/**********/
/* Engine */
/**********/

static inline float
linear_interpolate ( float y1, float y2, float mu )
{
//    return y1 + mu * ( y2 - y1 );
    return y1 * ( 1.0f - mu ) + y2 * mu;
}

static inline float
sigmoid_interpolate ( float y1, float y2, float mu )
{
    return linear_interpolate( y1, y2, ( 1 - cos( mu * M_PI ) ) / 2 );
}



/** fill buf with /nframes/ of interpolated control curve values
 * starting at /frame/  */
nframes_t
Control_Sequence::play ( sample_t *buf, nframes_t frame, nframes_t nframes )
{
    //  THREAD_ASSERT( RT );

    Control_Point *p2, *p1 = (Control_Point*)&_widgets.front();

    nframes_t n = nframes;

    for ( list <Sequence_Widget *>::const_iterator i = _widgets.begin();
          i != _widgets.end(); ++i, p1 = p2 )
    {
        p2 = (Control_Point*)(*i);

        if ( p2->when() < frame )
            continue;

        /* do incremental linear interpolation */

        const nframes_t len = p2->when() - p1->when();
        
        const float y1 = 1.0f - p1->control();
        const float y2 = 1.0f - p2->control();
        
        const nframes_t start = frame - p1->when();
        
        float incr;
        
        if ( interpolation() != None )
            incr = ( y2 - y1 ) / (float)len;
        else
            incr = 0.0f;
        
        float v = y1 + start * incr;

        if ( ! n )
            /* buffer's full, no point in continuing */
            break;
        
        for ( nframes_t i = start; i < len && n && n--; ++i, v += incr )
            *(buf++) = v;
    }

    return nframes - n;
}

nframes_t
Control_Sequence::process ( nframes_t nframes )
{
    THREAD_ASSERT( RT );
    
    if ( ! _output )
        return nframes;
    
    if ( _output->connected() ) /* don't waste CPU on disconnected ports */
    {
        void *buf = _output->buffer( nframes );

        return play( (sample_t*)buf, transport->frame, nframes );
    }
    else
        return nframes;
}
