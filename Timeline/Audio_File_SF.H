
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

#include "Audio_File.H"

#include <sndfile.h>

class Audio_File_SF : public Audio_File
{
//    Audio_File_SF ( const char *filename )

    SNDFILE *_in;

    /* used to avoid unnecessary seeking--libsndfile isn't smart
     * enough to do this for us */
    nframes_t _current_read;

public:

    static Audio_File_SF *from_file ( const char *filename );
    static Audio_File_SF *create ( const char *filename, nframes_t samplerate, int channels, const char *format );

    ~Audio_File_SF ( )
        {
            /* stupid C++ */
            close();
        }

    bool open ( void );
    void close ( void );
    void seek ( nframes_t offset );
    nframes_t read ( sample_t *buf, int channel, nframes_t len );
    nframes_t read ( sample_t *buf, int channel,  nframes_t start, nframes_t end );
    nframes_t write ( sample_t *buf, nframes_t nframes );

};
