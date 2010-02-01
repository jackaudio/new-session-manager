
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

#include "../Audio_Sequence.H"

#include "dsp.h"

#include "const.h"
#include "debug.h"
#include "Thread.H"

using namespace std;



/**********/
/* Engine */
/**********/

/** determine region coverage and fill /buf/ with interleaved samples
 * from /frame/ to /nframes/ for exactly /channels/ channels. */
nframes_t
Audio_Sequence::play ( sample_t *buf, nframes_t frame, nframes_t nframes, int channels )
{
    THREAD_ASSERT( Playback );

    sample_t *cbuf = new sample_t[ nframes ];

    memset( cbuf, 0, nframes * sizeof( sample_t ) );

    /* quick and dirty--let the regions figure out coverage for themselves */
    for ( list <Sequence_Widget *>::const_iterator i = _widgets.begin();
          i != _widgets.end(); ++i )
    {
        const Audio_Region *r = (Audio_Region*)(*i);

        for ( int i = channels; i--;  )
        {
            int nfr;

            if ( ! ( nfr = r->read( cbuf, frame, nframes, i ) ) )
                /* error ? */
                continue;

            if ( channels == 1 )
                buffer_mix( buf, cbuf, nframes );
            else
                buffer_interleave_one_channel_and_mix( buf, cbuf, i, channels, nframes );
        }
    }

    delete[] cbuf;

    /* FIXME: bogus */
    return nframes;
}
