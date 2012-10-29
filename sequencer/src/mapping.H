
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

#include "scale.H"
#include "instrument.H"
#include <MIDI/midievent.H>

/* C++'s inheritance system falls down dead for this application, so we
   have to do it backwards, using the base class (Mapping) as an interface
   to Instrument and Scale */


class Mapping
{

public:

    enum map_type { NONE, INSTRUMENT, SCALE };

private:

    union {
        Instrument *_instrument;
        Scale *_scale;
    };

    int _key;                                                   /* used for scales, 0 == A */

    int _type;

public:

    Mapping (  );
    Mapping ( const Mapping &rhs );

    bool open ( map_type type, const char *name );

    const char * name ( void ) const;
    int height ( void ) const;

    Instrument * instrument ( void ) const;

    /* mutation */
    void note ( int from, int to );
    void velocity ( int n, int v );
    void note_name ( int n, char *s );
    void key ( int n );

    /* inspection */
    bool translate ( MIDI::midievent *e ) const;
    const char * note_name ( int n ) const;
    int velocity ( int n ) const;
    int key ( void ) const;
    bool editable ( void ) const;

    const char * type ( void ) const;

};
