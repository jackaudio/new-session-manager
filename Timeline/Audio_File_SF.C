
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

#include "Audio_File_SF.H"
// #include "Timeline.H"

#include <sndfile.h>

#include <stdlib.h>
#include <string.h>

#include <assert.h>

#include "Peaks.H"

// #define HAS_SF_FORMAT_VORBIS

#include "debug.h"

const Audio_File::format_desc Audio_File_SF::supported_formats[] =
{
    {      "Wav 24",       "wav",   SF_FORMAT_WAV    | SF_FORMAT_PCM_24    | SF_ENDIAN_FILE },
    {      "Wav 16",       "wav",   SF_FORMAT_WAV    | SF_FORMAT_PCM_16    | SF_ENDIAN_FILE },
    {      "Wav f32",      "wav",   SF_FORMAT_WAV    | SF_FORMAT_FLOAT     | SF_ENDIAN_FILE },
    {      "Au 24",       "au",     SF_FORMAT_AU     | SF_FORMAT_PCM_24    | SF_ENDIAN_FILE },
    {      "Au 16",       "au",     SF_FORMAT_AU     | SF_FORMAT_PCM_16    | SF_ENDIAN_FILE },
    {      "FLAC",       "flac",    SF_FORMAT_FLAC   | SF_FORMAT_PCM_24 },
#ifdef HAS_SF_FORMAT_VORBIS
    {      "Ogg Vorbis", "ogg",     SF_FORMAT_OGG    | SF_FORMAT_VORBIS | SF_FORMAT_PCM_16 },
#endif
    {      0,            0          }
};

Audio_File_SF *
Audio_File_SF::from_file ( const char *filename )
{
    SNDFILE *in;
    SF_INFO si;

    Audio_File_SF *c = NULL;

    memset( &si, 0, sizeof( si ) );

    if ( ! ( in = sf_open( filename, SFM_READ, &si ) ) )
        return NULL;

/*     if ( si.samplerate != timeline->sample_rate() ) */
/*     { */
/*         printf( "error: samplerate mismatch!\n" ); */
/*         goto invalid; */
/*     } */

    c = new Audio_File_SF;

//    c->_peak_writer  = NULL;
    c->_current_read = 0;
    c->_filename     = strdup( filename );
    c->_length       = si.frames;
    c->_samplerate   = si.samplerate;
    c->_channels     = si.channels;

    c->_in = in;
//    sf_close( in );

    return c;

invalid:

    sf_close( in );
    return NULL;
}

Audio_File_SF *
Audio_File_SF::create ( const char *filename, nframes_t samplerate, int channels, const char *format )
{
    SF_INFO si;
    SNDFILE *out;

    memset( &si, 0, sizeof( si ) );

    const Audio_File::format_desc *fd = Audio_File::find_format( Audio_File_SF::supported_formats, format );

    if ( ! fd )
        return (Audio_File_SF *)1;

    si.samplerate =  samplerate;
    si.channels   =  channels;
    si.format = fd->id;

    char *name;
    asprintf( &name, "%s.%s", filename, fd->extension );

    if ( ! ( out = sf_open( name, SFM_WRITE, &si ) ) )
    {
        printf( "couldn't create soundfile.\n" );
        free( name );
        return NULL;
    }

    Audio_File_SF *c = new Audio_File_SF;

    c->_filename   = name;
    c->_length     = 0;
    c->_samplerate = samplerate;
    c->_channels   = channels;

    c->_in         = out;

    c->_peaks.prepare_for_writing();

    return c;
}

bool
Audio_File_SF::open ( void )
{
    SF_INFO si;

    assert( _in == NULL );

    memset( &si, 0, sizeof( si ) );

    if ( ! ( _in = sf_open( _filename, SFM_READ, &si ) ) )
        return false;

    _current_read = 0;
    _length       = si.frames;
    _samplerate   = si.samplerate;
    _channels     = si.channels;

//    seek( 0 );
    return true;
}

void
Audio_File_SF::close ( void )
{
    if ( _in )
        sf_close( _in );

    _in = NULL;
}

void
Audio_File_SF::seek ( nframes_t offset )
{
    lock();

    if ( offset != _current_read )
        sf_seek( _in, _current_read = offset, SEEK_SET | SFM_READ );

    unlock();
}

/* if channels is -1, then all channels are read into buffer
 (interleaved).  buf should be big enough to hold them all */
nframes_t
Audio_File_SF::read ( sample_t *buf, int channel, nframes_t len )
{
    if ( len > 256 * 100 )
        WARNING( "warning: attempt to read an insane number of frames (%lu) from soundfile\n", (unsigned long)len );

//    printf( "len = %lu, channels = %d\n", len, _channels );

    lock();

    nframes_t rlen;

    if ( _channels == 1 || channel == -1 )
        rlen = sf_readf_float( _in, buf, len );
    else
    {
        sample_t *tmp = new sample_t[ len * _channels ];

        rlen = sf_readf_float( _in, tmp, len );

        /* extract the requested channel */
        for ( unsigned int i = channel; i < rlen * _channels; i += _channels )
            *(buf++) = tmp[ i ];

        delete[] tmp;
    }

    _current_read += rlen;

    unlock();

    return rlen;
}

/** read samples from /start/ to /end/ into /buf/ */
nframes_t
Audio_File_SF::read ( sample_t *buf, int channel, nframes_t start, nframes_t end )
{
    assert( end > start );

    lock();
//    open();

    seek( start );

    nframes_t len = read( buf, channel, end - start );

    unlock();

//    close();

    return len;
}

/** write /nframes/ from /buf/ to soundfile. Should be interleaved for
 * the appropriate number of channels */
nframes_t
Audio_File_SF::write ( sample_t *buf, nframes_t nframes )
{
    _peaks.write( buf, nframes );

//    lock();

    nframes_t l = sf_writef_float( _in, buf, nframes );

    _length += l;

//    unlock();

    return l;
}
