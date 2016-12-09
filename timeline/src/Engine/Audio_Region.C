
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

    if ( n > length - start )
        /* don't try to apply fade to more samples than specified by the fade length... */
        n = length - start;
    
    /* ASSERT( nframes < length - start, "Attempt to apply fade to more samples than its length" ); */
            
    if ( dir == Fade::Out )
    {
        fi = 1.0 - fi;
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

static
void
apply_fade ( sample_t *buf, const int channels, Audio_Region::Fade fade, const nframes_t bS, const nframes_t bE, const nframes_t edge, Audio_Region::Fade::fade_dir_e dir )
{
    const nframes_t bSS = dir == Audio_Region::Fade::Out ? bS + fade.length : bS;    
    const nframes_t fade_start = bSS > edge ? bSS - edge : 0;
    const nframes_t fade_offset =  bSS > edge ? 0 : edge - bSS;
            
    fade.apply_interleaved( buf + ( channels * fade_offset ), dir, fade_start, (bE - bS) - fade_offset, channels );
};

/** read the overlapping at /pos/ for /nframes/ of this region into
    /buf/, where /pos/ is in timeline frames. /buf/ is an interleaved
    buffer of /channels/ channels */
/* this runs in the diskstream thread. */
nframes_t
Audio_Region::read ( sample_t *buf, bool buf_is_empty, nframes_t pos, nframes_t nframes, int channels ) const
{
    THREAD_ASSERT( Playback );

    const Range r = _range;

    const nframes_t rS = r.start;
    const nframes_t rE = r.start + r.length;
    const nframes_t bS = pos;
    const nframes_t bE = pos + nframes;
    
    /* do nothing if region isn't inside buffer */
    if ( bS > rE || bE < rS )
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

    const nframes_t sO = bS < rS ? 0 : bS - rS;  /* offset into source */
    const nframes_t bO = bS < rS ? rS - bS : 0; /* offset into buffer (when region start is after beginning of buffer) */
    
    nframes_t cnt = nframes;                                                    /* number of frames to read  */

    /* if region ends within this buffer, don't read beyond end */
    if ( bE > rE )
        cnt = nframes - ( bE - rE );

    cnt -= bO;
    
    const nframes_t len = cnt;

    /* FIXME: keep the declick defults someplace else */
    Fade declick;

    declick.length = (float)timeline->sample_rate() * 0.01f;
    declick.type   = Fade::Sigmoid;

    /* FIXME: what was this for? */
    if ( bO >= nframes )
    {
        cnt = 0;
        goto done;
    }

    /* FIXME: what was this for? */
    if ( len == 0 )
    {
        cnt = 0;
        goto done;
    }

    /* now that we know how much and where to read, get on with it */

    //    printf( "reading region ofs = %lu, sofs = %lu, %lu-%lu\n", ofs, sofs, start, end  );

    if ( _loop )
    {
        if ( _loop < nframes )
        {
            /* very small loop or very large buffer... */
            WARNING("Loop size (%lu) is smaller than buffer size (%lu). Behavior undefined.", _loop, nframes );            
        }
        
        const nframes_t lO = sO % _loop; /* how far we are into the loop */
        const nframes_t nthloop = sO / _loop; /* which loop iteration */
        const nframes_t seam_L = rS + ( nthloop * _loop ); /* receding seam */
        const nframes_t seam_R = rS + ( (nthloop + 1 ) * _loop ); /* upcoming seam */
        
        /* read interleaved channels */
        if ( seam_R > bS && seam_R < bE  )
        {
            /* this buffer covers a loop boundary */

            /* read the first part */
            cnt = _clip->read( cbuf + ( _clip->channels() * bO ), -1, r.offset + lO, ( seam_R - bS ) - bO ); 
            /* read the second part */
            cnt += _clip->read( cbuf + ( _clip->channels() * ( bO + cnt ) ), -1, r.offset + 0, ( len - cnt ) - bO );

            /* assert( cnt == len ); */
        }
        else
            /* buffer contains no loop seam, perform straight read. */
            cnt = _clip->read( cbuf + ( _clip->channels() * bO ), -1, r.offset + lO, cnt );

        for ( int i = 0; i < 2; i++ )
        {
            nframes_t seam = i ? seam_R : seam_L;

            if ( seam != rS && seam != rE ) /* not either end of the region */
            {
                if ( seam >= bS && seam <= bE + declick.length )
                    /* fade out previous loop segment */
                    apply_fade( cbuf, _clip->channels(), declick, bS, bE, seam, Fade::Out );                
               
                if ( seam <= bE && seam + declick.length >= bS )
                    /* fade in next loop segment */
                    apply_fade( cbuf, _clip->channels(), declick, bS, bE, seam, Fade::In );            
            }
        }
    }
    else
    {
//    DMESSAGE("Clip read, rL=%lu, b0=%lu, sO=%lu, r.offset=%lu, len=%lu",r.length,bO,sO,r.offset,len);
        cnt = _clip->read( cbuf + ( _clip->channels() * bO ), -1, sO + r.offset, len );
    }

    if ( ! cnt )
        goto done;

    /* apply gain */

    /* just do the whole buffer so we can use the alignment optimized
     * version when we're in the middle of a region, this will be full
     * anyway */
    buffer_apply_gain( cbuf, nframes * _clip->channels(), _scale );

    /* perform fade/declicking if necessary */
    {
        assert( cnt <= nframes );
            
        Fade fade;

        fade = declick < _fade_in ? _fade_in : declick;
        
        /* do fade in if necessary */
        if ( sO < fade.length )
            apply_fade( cbuf, _clip->channels(), fade, bS, bE, rS, Fade::In );                

        fade = declick < _fade_out ? _fade_out : declick;

        /* do fade out if necessary */
        if ( sO + cnt + fade.length > r.length )
            apply_fade( cbuf, _clip->channels(), fade, bS, bE, rE, Fade::Out );                
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
