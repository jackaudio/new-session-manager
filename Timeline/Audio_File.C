
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
#include "Audio_File_SF.H"

map <string, Audio_File*> Audio_File::_open_files;

Audio_File::~Audio_File ( )
{
    _open_files[ string( _filename ) ] = NULL;
}

/** attmpet to open any supported filetype */
Audio_File *
Audio_File::from_file ( const char * filename )
{

    Audio_File *a;

    if ( ( a = _open_files[ string( filename ) ] ) )
        return a;

    if ( ( a = Audio_File_SF::from_file( filename ) ) )
        goto done;

// TODO: other formats

    return NULL;

done:

/*     a->_peaks = new Peaks; */

/*     a->_peaks->clip( a ); */
/*     a->_peaks->open(); */

    _open_files[ string( filename ) ] = a;

    return a;
}


bool
Audio_File::read_peaks( float fpp, nframes_t start, nframes_t end, int *peaks, Peak **pbuf, int *channels )
{
    Peaks pk;

    *peaks    = 0;
    *channels = 0;
    *pbuf     = NULL;

    pk.clip( this );

    if ( ! pk.open() )
        return false;

    *peaks = pk.fill_buffer( fpp, start, end );

    *channels = this->channels();

    *pbuf = new Peak[ *peaks * *channels ];

    /* deintereave peaks */
    int k = 0;
    for ( int i = 0; i < *channels; i++ )
        for ( int j = i; j < *peaks * *channels; j += *channels )
            (*pbuf)[ k++ ] = pk.peakbuf()[ j ];

    return true;
}
