
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

/* Base class for all audio file library interfaces */

#include <stdlib.h>
#include <string>
#include <map>
#include <list>

#include "types.h"
#include "util/Mutex.H"
#include "Peaks.H"

class Peak_Writer;

class Audio_File : protected Mutex
{
    static std::map <std::string, Audio_File*> _open_files;

    /* not permitted */
    Audio_File ( const Audio_File &rhs );
    const Audio_File & operator= ( const Audio_File &rhs );

protected:

    struct format_desc
    {
        const char *name;
        const char *extension;
        unsigned long id;
    };

    char *_filename;
    volatile nframes_t _length;                 /* length of file in samples */
    nframes_t _samplerate;                   /* sample rate */
    int _channels;

    Peaks _peaks;

    static const format_desc *
    find_format ( const format_desc *fd, const char *name )
        {
            for ( ; fd->name; ++fd )
                if ( ! strcmp( fd->name, name ) )
                    return fd;

            return NULL;
        }


public:

    Audio_File ( ) : _peaks( this )
        {
            _filename = NULL;
            _samplerate = 0;
            _length = _channels = 0;
        }

    virtual ~Audio_File ( );

    virtual bool dummy ( void ) const { return false; }

    static void all_supported_formats ( std::list <const char *> &formats );

    static Audio_File *from_file ( const char *filename );

    Peaks const * peaks ( ) { return &_peaks; }
    const char *name ( void ) const { return _filename; }
    nframes_t length ( void ) const  { return _length; }
    int channels ( void ) const { return _channels; }
    nframes_t samplerate ( void ) const { return _samplerate; }
//    Peaks const * peaks ( void ) { return &_peaks; }

    virtual bool open ( void ) = 0;
    virtual void close ( void ) = 0;
    virtual void seek ( nframes_t offset ) = 0;
    virtual nframes_t read ( sample_t *buf, int channel, nframes_t len ) = 0;
    virtual nframes_t read ( sample_t *buf, int channel, nframes_t start, nframes_t len ) = 0;
    virtual nframes_t write ( sample_t *buf, nframes_t len ) = 0;

    virtual void finalize ( void ) { _peaks.finish_writing(); }

    bool read_peaks( float fpp, nframes_t start, nframes_t end, int *peaks, Peak **pbuf, int *channels );

};
