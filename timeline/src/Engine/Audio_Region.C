
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

/**********/
/* Engine */
/**********/

#include "../Audio_Region.H"

#include "Audio_File.H"
#include "dsp.h"

#include "const.h"
#include "const.h"
#include "debug.h"
#include "Thread.H"



/** Apply a (portion of) fade from /start/ to a buffer up to size /nframes/. */
void
Audio_Region::Fade::apply ( sample_t *buf, Audio_Region::Fade::fade_dir_e dir, nframes_t start, nframes_t nframes ) const
{
//    printf( "apply fade %s: start=%ld end=%lu\n", dir == Fade::Out ? "out" : "in", start, end );
    if ( ! nframes )
        return;

    nframes_t n = nframes;

    const double inc = increment();
    double fi = start / (double)length;

    if ( dir == Fade::Out )
    {
        fi = 1.0f - fi;
        for ( ; n--; fi -= inc  )
            *(buf++) *= gain( fi );
    }
    else
        for ( ; n--; fi += inc )
            *(buf++) *= gain( fi );
}

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
Audio_Region::read ( sample_t *buf, nframes_t pos, nframes_t nframes, int channel ) const
{
    THREAD_ASSERT( Playback );

    const Range r = _range;

    /* do nothing if we aren't covered by this frame range */
    if ( pos > r.start + r.length || pos + nframes < r.start )
        return 0;

    /* calculate offsets into file and sample buffer */

    nframes_t sofs,                                              /* offset into source */
        ofs,                                                    /* offset into buffer */
        cnt;                                                    /* number of frames to read  */

    cnt = nframes;

    if ( pos < r.start )
    {
        /* region starts somewhere after the beginning of this buffer */
        sofs = 0;
        ofs = r.start - pos;
        cnt -= ofs;
    }
    else
    {
        /* region started before this buffer */
        ofs = 0;
        sofs = pos - r.start;
    }

    if ( ofs >= nframes )
        return 0;

//    const nframes_t start = ofs + r.start + sofs;
    const nframes_t start = r.offset + sofs;
    const nframes_t len = cnt;

    if ( len == 0 )
        return 0;

    /* now that we know how much and where to read, get on with it */

    //    printf( "reading region ofs = %lu, sofs = %lu, %lu-%lu\n", ofs, sofs, start, end  );

    /* FIXME: keep the declick defults someplace else */
    Fade declick;

    declick.length = 256;
    declick.type   = Fade::Sigmoid;

    if ( _loop )
    {
        nframes_t lofs = sofs % _loop;
        nframes_t lstart = r.offset + lofs;


        if ( lofs + len > _loop )
        {
            /* this buffer covers a loop binary */

            /* read the first part */
            cnt = _clip->read( buf + ofs, channel, lstart, len - ( ( lofs + len ) - _loop ) );
            /* read the second part */
            cnt += _clip->read( buf + ofs + cnt, channel, lstart + cnt, len - cnt );

            /* TODO: declick/crossfade transition? */

            assert( cnt == len );
        }
        else
            cnt = _clip->read( buf + ofs, channel, lstart, len );

        /* this buffer is inside declicking proximity to the loop boundary */

        if ( lofs + cnt + declick.length > _loop /* buffer ends within declick length of the end of loop */
             &&
             sofs + declick.length < r.length /* not the last loop */
            )
        {
            /* */
            /* fixme: what if loop is shorter than declick? */
            const nframes_t declick_start = _loop - declick.length;

            /* when the buffer covers the beginning of the
             * declick, how many frames between the beginning of
             * the buffer and the beginning of the declick */
            const nframes_t declick_onset_offset = declick_start > lofs ? declick_start - lofs : 0;

            /* how far into the declick we are */
            const nframes_t declick_offset = lofs > declick_start ? lofs - declick_start : 0;

            /* this is the end side of the loop boundary */

            const nframes_t fl = cnt - declick_onset_offset;

            declick.apply( buf + ofs + declick_onset_offset,
                           Fade::Out,
                           declick_offset, fl );
        }
            
        if ( lofs < declick.length /* buffer begins within declick length of beginning of loop */
             &&
             sofs > _loop )               /* not the first loop */
        {
                
            const nframes_t declick_end = declick.length;
                
            const nframes_t click_len = lofs + cnt > declick_end ? declick_end - lofs : cnt;

            /* this is the beginning of the loop next boundary */
            declick.apply( buf + ofs, Fade::In, lofs, click_len );
        }
    }
    else
        cnt = _clip->read( buf + ofs, channel, start, len );

    if ( ! cnt )
        return 0;

    /* apply gain */

    buffer_apply_gain( buf + ofs, cnt, _scale );

    /* perform declicking if necessary */


    {
        assert( cnt <= nframes );
            
        Fade fade;

        fade = declick < _fade_in ? _fade_in : declick;

        /* do fade in if necessary */
        if ( sofs < fade.length )
            fade.apply( buf + ofs, Fade::In, sofs, cnt );

        fade = declick < _fade_out ? _fade_out : declick;

        /* do fade out if necessary */
        if ( start + fade.length > r.offset + r.length )
            fade.apply( buf, Fade::Out, ( start + fade.length ) - ( r.offset + r.length ), cnt );                
    }

    return cnt;
}

/** prepare for capturing */
void
Audio_Region::prepare ( void )
{
    THREAD_ASSERT( Capture );

    DMESSAGE( "Preparing capture region" );

    log_start();
}

/** write /nframes/ from /buf/ to source. /buf/ is interleaved and
    must match the channel layout of the write source!  */
nframes_t
Audio_Region::write ( nframes_t nframes )
{
    THREAD_ASSERT( Capture );

    if ( 0 == ( timeline->ts_to_x( _range.length ) % 20 ) )
    {
        int W = 20;

        if ( W )
        {
            Fl::lock();
            sequence()->damage(FL_DAMAGE_USER1, x(), y(), w(), h());
            Fl::unlock();
        }
    }

    _range.length += nframes;

    return nframes;
}

/** finalize region capture. Assumes that this *is* a captured region
    and that no other regions refer to the same source */
bool
Audio_Region::finalize ( nframes_t frame )
{
    THREAD_ASSERT( Capture );

    DMESSAGE( "finalizing capture region" );

    _range.length = frame - _range.start;

    _clip->close();
    _clip->open();

    log_end();

    return true;
}
