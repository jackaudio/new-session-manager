
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

void
Audio_Region::Fade::apply_interleaved ( sample_t *buf, Audio_Region::Fade::fade_dir_e dir, nframes_t start, nframes_t nframes, int channels ) const
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
        {
            const float g = gain(fi);

            for ( int i = channels; i--; )
                *(buf++) *= g;
        }
    }
    else
        for ( ; n--; fi += inc )
        {
            const float g = gain(fi);

            for ( int i = channels; i--; )
                *(buf++) *= g;
        }
}


/** read the overlapping at /pos/ for /nframes/ of this region into
    /buf/, where /pos/ is in timeline frames. /buf/ is an interleaved
    buffer of /channels/ channels */
/* this runs in the diskstream thread. */
nframes_t
Audio_Region::read ( sample_t *buf, bool buf_is_empty, nframes_t pos, nframes_t nframes, int channels ) const
{
    THREAD_ASSERT( Playback );

    const Range r = _range;

    /* do nothing if we aren't covered by this frame range */
    if ( pos > r.start + r.length || pos + nframes < r.start )
        return 0;

    sample_t *cbuf = NULL;

    if ( buf_is_empty && channels == _clip->channels() )
    {
        /* in this case we don't need a temp buffer */
        cbuf = buf;
    }
    else
    {
        /* temporary buffer to hold interleaved samples from the clip */
        cbuf = buffer_alloc( _clip->channels() * nframes );
        memset(cbuf, 0, _clip->channels() * sizeof(sample_t) * nframes );
    }

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


//    const nframes_t start = ofs + r.start + sofs;
    const nframes_t start = r.offset + sofs;
    const nframes_t len = cnt;

    /* FIXME: keep the declick defults someplace else */
    Fade declick;

    declick.length = (float)timeline->sample_rate() * 0.01f;
    declick.type   = Fade::Sigmoid;

    if ( ofs >= nframes )
    {
        cnt = 0;
        goto done;
    }

    if ( len == 0 )
    {
        cnt = 0;
        goto done;
    }

    /* now that we know how much and where to read, get on with it */

    //    printf( "reading region ofs = %lu, sofs = %lu, %lu-%lu\n", ofs, sofs, start, end  );


    if ( _loop )
    {
        nframes_t lofs = sofs % _loop;
        nframes_t lstart = r.offset + lofs;

        /* read interleaved channels */
        if ( lofs + len > _loop )
        {
            /* this buffer covers a loop boundary */

            /* read the first part */
            cnt = _clip->read( cbuf + ( _clip->channels() * ofs ), -1, lstart, len - ( ( lofs + len ) - _loop ) );
            /* read the second part */
            cnt += _clip->read( cbuf + ( _clip->channels() * ( ofs + cnt ) ), -1, lstart + cnt, len - cnt );

            assert( cnt == len );
        }
        else
            cnt = _clip->read( cbuf + ( channels * ofs ), -1, lstart, len );

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

            declick.apply_interleaved( cbuf + ( _clip->channels() * ( ofs + declick_onset_offset  ) ),
                                       Fade::Out,
                                       declick_offset, fl, _clip->channels() );
        }
            
        if ( lofs < declick.length /* buffer begins within declick length of beginning of loop */
             &&
             sofs > _loop )               /* not the first loop */
        {
                
            const nframes_t declick_end = declick.length;
                
            const nframes_t click_len = lofs + cnt > declick_end ? declick_end - lofs : cnt;

            /* this is the beginning of the loop next boundary */
            declick.apply_interleaved( cbuf + ( _clip->channels() * ofs ), Fade::In, lofs, click_len, _clip->channels() );
        }
    }
    else
        cnt = _clip->read( cbuf + ( _clip->channels() * ofs ), -1, start, len );

    if ( ! cnt )
        goto done;

    /* apply gain */

    /* just do the whole buffer so we can use the alignment optimized
     * version when we're in the middle of a region, this will be full
     * anyway */
    buffer_apply_gain( cbuf, nframes * _clip->channels(), _scale );

    /* perform declicking if necessary */
    {
        assert( cnt <= nframes );
            
        Fade fade;

        fade = declick < _fade_in ? _fade_in : declick;
        
        /* do fade in if necessary */
        if ( sofs < fade.length )
            fade.apply_interleaved( cbuf + ( _clip->channels() * ofs ), Fade::In, sofs, cnt, _clip->channels() );

        fade = declick < _fade_out ? _fade_out : declick;

        /* do fade out if necessary */
        if ( start + fade.length > r.offset + r.length )
            fade.apply_interleaved( cbuf, Fade::Out, ( start + fade.length ) - ( r.offset + r.length ), cnt, _clip->channels() );
    }

    if ( buf != cbuf )
    {
        /* now interleave the clip channels into the playback buffer */
        for ( int i = 0; i < channels && i < _clip->channels(); i++ )
        {
            if ( buf_is_empty )
                buffer_interleaved_copy( buf, cbuf, i, i, channels, _clip->channels(), nframes );
            else
                buffer_interleaved_mix( buf, cbuf, i, i, channels, _clip->channels(), nframes );
            
        }
    }

done:

    if ( buf != cbuf )
    {
        free( cbuf );
    }

    return cnt;
}

/** prepare for capturing */
void
Audio_Region::prepare ( void )
{
    THREAD_ASSERT( Capture );

    DMESSAGE( "Preparing capture region" );

//    log_start();
}


class SequenceRedrawRequest {
public:
    nframes_t start;
    nframes_t length;
    Sequence *sequence;
};

static 
void
sequence_redraw_request_handle ( void *v )
{
    THREAD_ASSERT(UI);

    SequenceRedrawRequest *o = (SequenceRedrawRequest*)v;
    
    o->sequence->damage( FL_DAMAGE_USER1, timeline->offset_to_x( o->start ), o->sequence->y(), timeline->ts_to_x( o->length ), o->sequence->h() );

    delete o;
};

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
            SequenceRedrawRequest *o = new SequenceRedrawRequest();
            o->sequence = sequence();
            o->start = _range.start + ( _range.length - timeline->x_to_ts( 20 ) );
            o->length = timeline->x_to_ts( 20 );

            Fl::awake(sequence_redraw_request_handle, o);
        }
    }

    timeline->sequence_lock.wrlock();

    _range.length += nframes;

    timeline->sequence_lock.unlock();

    return nframes;
}

/** finalize region capture. Assumes that this *is* a captured region
    and that no other regions refer to the same source */
bool
Audio_Region::finalize ( nframes_t frame )
{
    THREAD_ASSERT( Capture );

    DMESSAGE( "finalizing capture region" );

    timeline->sequence_lock.wrlock();

    _range.length = frame - _range.start;

    timeline->sequence_lock.unlock();

    _clip->close();
    _clip->open();

    log_create();
//    log_end();

    return true;
}
