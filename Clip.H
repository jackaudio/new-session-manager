
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

typedef unsigned long nframes_t;

#include "Peaks.H"

class Clip
{
    const char *_filename;

    Peaks _peaks;

    nframes_t _length;                                              /* length of clip in samples */

public:

    Clip ( const char *filename ) : _peaks( this )
        {
            _filename = filename;

            // FIXME: open file

            _peaks.open( filename );
        }

    Peaks const * peaks ( void ) { return &_peaks; }

    const char *name ( void ) { return _filename; }

    nframes_t length ( void ) { return _length; }
};
