
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

#include "Disk_Stream.H"

#include "Audio_File_SF.H"
class Audio_File;
class Peak_Writer;

class Record_DS : public Disk_Stream
{

    volatile bool _recording;

    Audio_File_SF *_af;                             /* capture file */

    void write_block ( sample_t *buf, nframes_t nframes );
    void disk_thread ( void );

public:

    Record_DS ( Track_Header *th, float frame_rate, nframes_t nframes, int channels ) :
        Disk_Stream( th, frame_rate, nframes, channels )
        {
            sem_destroy( &_blocks );
            sem_init( &_blocks, 0, 0 );

            _recording = false;
        }

/*     bool seek_pending ( void ); */
/*     void seek ( nframes_t frame ); */

    void start ( nframes_t frame );
    void stop ( nframes_t frame );
    nframes_t process ( nframes_t nframes );

};
