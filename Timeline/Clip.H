
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

/* a clip is just a stupid wrapper object for communicating with the peak server */

#include "Peak_Client.H"
extern Peak_Client peak_client;

class Clip
{
    const char *_filename;
    nframes_t _length;                                              /* length of file in samples */
    int _channels;

public:

    Clip ( )
        {
            _filename = NULL;
            _length = _channels = 0;
        }

    const char *name ( void ) const { return _filename; }
    nframes_t length ( void ) const  { return _length; }
    int channels ( void ) const { return _channels; }

    static Clip *
    from_file ( const char * filename )
        {

            Clip *c = new Clip;

            c->_filename = filename;

            if ( ! peak_client.get_info( filename, &c->_length, &c->_channels ) )
            {
                delete c;
                return false;
            }

            return c;
        }

    bool
    read_peaks ( float fpp, nframes_t start, nframes_t end,
                 int *peaks, Peak **pbuf, int *channels )
        {
            return peak_client.read_peaks( _filename, fpp, start, end, peaks, pbuf, channels );
        }
};
