
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

#include "Audio_File.H"

class Audio_File_Dummy : public Audio_File
{
    Audio_File_Dummy ( )
        {
        }

public:

    static Audio_File_Dummy *from_file ( const char *filename );

    bool dummy ( void ) const { return true; }

    ~Audio_File_Dummy ( )
        {
        }

    bool open ( void ) { return true; }
    void close ( void ) { }
    void seek ( nframes_t ) { }
    nframes_t read ( sample_t *, int, nframes_t len ) { return len; }
    nframes_t read ( sample_t *, int,  nframes_t start, nframes_t end ) { return end - start; }
    nframes_t write ( sample_t *, nframes_t nframes ) { return nframes; }
};
