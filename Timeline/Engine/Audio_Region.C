
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

/** Apply a (portion of) fade from /start/ to /end/ assuming a
 * buffer size of /nframes/. /start/ and /end/ are relative to the
 * given buffer, and /start/ may be negative. */
void
Audio_Region::Fade::apply ( sample_t *buf, Audio_Region::Fade::fade_dir_e dir, long start, nframes_t end, nframes_t nframes ) const
{
//    printf( "apply fade %s: start=%ld end=%lu\n", dir == Fade::Out ? "out" : "in", start, end );

    if ( ! nframes )
        return;

    const nframes_t i = start > 0 ? start : 0;
    const nframes_t e = end > nframes ? nframes : end;

    assert( i < nframes );

    const float inc = increment();
    float fi = ( i - start ) / (float)length;

//    buf += i;
    buf = &buf[ i ];

    nframes_t n = e - i;

    assert( i + n <= nframes );

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
Audio_Region::read ( sample_t *buf, nframes_t pos, nframes_t nframes, int channel ) const
{
    const Range r = _range;

    /* do nothing if we aren't covered by this frame range */
    if ( pos > r.start + r.length || pos + nframes < r.start )
        return 0;

    /* calculate offsets into file and sample buffer */

    nframes_t sofs, ofs, cnt;

    cnt = nframes;

    if ( pos < r.start )
    {
        sofs = 0;
        ofs = r.start - pos;
        cnt -= ofs;
    }
    else
    {
        ofs = 0;
        sofs = pos - r.start;
    }

    if ( ofs >= nframes )
        return 0;

//    const nframes_t start = ofs + r.start + sofs;
    const nframes_t start =  r.offset + sofs;
    const nframes_t len = min( cnt, nframes - ofs );
    const nframes_t end = start + len;

    if ( len == 0 )
        return 0;

    /* now that we know how much and where to read, get on with it */

    //    printf( "reading region ofs = %lu, sofs = %lu, %lu-%lu\n", ofs, sofs, start, end  );

    cnt = _clip->read( buf + ofs, channel, start, end );

    /* apply gain */

    buffer_apply_gain( buf + ofs, cnt, _scale );

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

            assert( cnt <= nframes );

            fade.apply( buf + ofs, Fade::In, d, d + fade.length, cnt );
        }

        fade = declick < _fade_out ? _fade_out : declick;

        /* do fade out if necessary */
//        if ( start + cnt + fade.length > r.end )
        if ( start + fade.length > ( r.offset + r.length ) )
        {
            const nframes_t d = ( r.offset + r.length )  - start;

            assert( cnt <= nframes );

            fade.apply( buf, Fade::Out, cnt + (long)d - fade.length, cnt + d, cnt );
        }
    }
//    printf( "read %lu frames\n", cnt );

    return cnt;
}


/** prepare for capturing */
void
Audio_Region::prepare ( void )
{
    log_start();
}

/* THREAD: IO */
/** write /nframes/ from /buf/ to source. /buf/ is interleaved and
    must match the channel layout of the write source!  */
nframes_t
Audio_Region::write ( nframes_t nframes )
{
    _range.length += nframes;

    /* FIXME: too much? */
//    _track->damage( FL_DAMAGE_EXPOSE, x() + w(), y(), 10/* FIXME: guess */, h() );

    if ( 0 == ( timeline->ts_to_x( _range.length ) % 20 ) )
    {
        nframes_t oldl = _clip->length();

        /* get the new size. Remember, this is a read-only handle on the source--not the same
         one being written to */
        _clip->close();
        _clip->open();

        int W = timeline->ts_to_x( _clip->length() - oldl );

        /* why - 1? */

        if ( W )
        {
            ++W;
            Fl::lock();
            sequence()->damage( FL_DAMAGE_ALL, x() + w() - W, y(), W, h() );
            Fl::awake();
            Fl::unlock();
        }
    }

    return nframes;
}

/** finalize region capture. Assumes that this *is* a captured region
 and that no other regions refer to the same source */
bool
Audio_Region::finalize ( nframes_t frame )
{
    _range.length = frame - _range.start;

    log_end();

    _clip->close();
    _clip->open();

    /* FIXME: should we attempt to truncate the file? */

    redraw();

    return true;
}
